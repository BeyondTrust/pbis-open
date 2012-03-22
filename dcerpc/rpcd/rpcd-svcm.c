/*
 * 
 * (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 *                 permission to use, copy, modify, and distribute this
 * file for any purpose is hereby granted without fee, provided that
 * the above copyright notices and this notice appears in all source
 * code copies, and that none of the names of Open Software
 * Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
 * Corporation be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Neither Open Software Foundation, Inc., Hewlett-
 * Packard Company, nor Digital Equipment Corporation makes any
 * representations about the suitability of this software for any
 * purpose.
 * 
 */
/*
 */
/*
**
**  NAME:
**
**      rpcd.c
**
**  FACILITY:
**
**      RPC Daemon
**
**  ABSTRACT:
**
**  This daemon is a catch all for DCE RPC support functions.  This server
**  exports the DCE 1.0 endpoint map (ept_) network interfaces and, optionally,  
**  the NCS 1.5.1 llb_ network interfaces and .  Additionally, this server 
**  provides the RPC forwarding map function used by non-connection oriented
**  protocol services.
**
**
*/

#include <commonp.h>
#include <com.h>

#include <dce/ep.h>
EXTERNAL ept_v3_0_epv_t ept_v3_0_mgr_epv;

#ifdef RPC_LLB
#  include <dce/llb.h>
EXTERNAL llb__v4_0_epv_t llb_v4_0_mgr_epv;
#endif

#ifdef ENABLE_DCOM 
# include <objex.h>
EXTERNAL	IObjectExporter_v0_0_epv_t objex_mgr_epv;
#endif

#include <comfwd.h>

#include <dsm.h>

#include <rpcdp.h>
#include <rpcddb.h>
#include <rpcdepdb.h>

#ifdef RPC_LLB
#include <rpcdlbdb.h>
#endif
          
#include <dce/dce_error.h>

#ifndef VMS
#include <sys/ioctl.h>
#include <sys/stat.h>
#endif
#include <locale.h>
#include <syslog.h>
#include <ctype.h>

#include <errno.h>
#include <lw/ntstatus.h>
#include <lw/svcm.h>
#include <lw/rtllog.h>
#include <lw/rpcstatus.h>

#define GCOS(s) \
    do \
    { \
        if ((s)) goto cleanup; \
    } while (0)

#if 0
INTERNAL void process_args
    (
        int             argc,
        char            *argv[]
    );
#endif

INTERNAL void register_ifs
    (
        error_status_t  *status
    );

INTERNAL void init
    (
        error_status_t  *status
    );


INTERNAL void fwd_map
    (
        dce_uuid_p_t                object,
        rpc_if_id_p_t           interface,
        rpc_syntax_id_p_t       data_rep,
        rpc_protocol_id_t       rpc_protocol,
        unsigned32              rpc_protocol_vers_major,
        unsigned32              rpc_protocol_vers_minor,
        rpc_addr_p_t            addr,
        dce_uuid_p_t                actuuid,
        rpc_addr_p_t            *fwd_addr,
        rpc_fwd_action_t        *fwd_action,
        error_status_t          *status
    );

/*
 * These implementation constants can be redefined in the system specific 
 * config files if necessary (e.g. common/ultrix_mips.h).
 */

#ifdef DCELOCAL_PATH
#  define rpcd_c_database_name_prefix1 DCELOCAL_PATH
#  define rpcd_c_database_name_prefix2 "/var/rpc/" 
#else

#ifndef rpcd_c_database_name_prefix1 
#  define rpcd_c_database_name_prefix1 "/tmp/"
#endif

#ifndef rpcd_c_database_name_prefix2
#  define rpcd_c_database_name_prefix2 ""
#endif

#endif

#ifndef rpcd_c_ep_database_name
#  define rpcd_c_ep_database_name "rpcdep.dat"
#endif

#ifndef rpcd_c_llb_database_name
#   define rpcd_c_llb_database_name "rpcdllb.dat"
#endif

#ifndef rpcd_c_logfile_name
#   define rpcd_c_logfile_name "rpcd.log"
#endif

/* 
 *  Optional list of protocol sequences which rpcd will use.
 *  List is specified on the command line
 */
#define                 MAX_PROTSEQ_ARGS            8
INTERNAL unsigned_char_p_t protseq[MAX_PROTSEQ_ARGS];
INTERNAL unsigned32     num_protseq = 0;
INTERNAL boolean32      use_all_protseqs = true;

/*
 *  Debug flag controls
 */
GLOBAL   boolean32      dflag = false;
#define  DEBUG_LEVEL    "0.1"

GLOBAL   dce_uuid_t         nil_uuid;

PRIVATE boolean32 check_st_bad(str, st)
char            *str;
error_status_t  *st;
{
    if (STATUS_OK(st)) 
        return false;

    show_st(str, st);
    return true;
}

PRIVATE void show_st(str, st)
char            *str;
error_status_t  *st;
{
    dce_error_string_t estr;
    int             tmp_st;

    dce_error_inq_text(*st, (unsigned char*) estr, &tmp_st);
    fprintf(stderr, "(rpcd) %s: (0x%lx) %s\n", str, (unsigned long) *st, estr);
    syslog(LOG_ERR, "%s: (0x%lx) %s\n", str, (unsigned long) *st, estr);
}

#if 0
/*
 *  Process args
 */
INTERNAL void process_args(argc, argv)
int             argc;
char            *argv[];
{
    int             i, c;
    unsigned32      status;
    extern int      optind;
    extern char     *optarg;
    

    /*
     * Process args.
     */

    while ((c = getopt(argc, argv, "vfDd:")) != EOF)
    {
        switch (c)
        {
        case 'v':
            printf("\t%s\n", rpcd_version_str);
            exit(0);

        case 'f':
            foreground = true;
            break;

        case 'd':
        case 'D':
            rpc__dbg_set_switches(c == 'd' ? optarg : DEBUG_LEVEL, &status);
            if (check_st_bad("Error setting debug switches", &status))
                return;
            dflag = true;
            break;

        default:
            exit(1);
        }
    }

    argc -= optind - 1;
    argv = &argv[optind - 1];

    use_all_protseqs = (argc == 1);

    for (i = 1; i < argc; i++)
    {

        rpc_network_is_protseq_valid((unsigned_char_p_t)argv[i], &status);
        if (check_st_bad("Protseq is not valid", &status))
            return;

        if (num_protseq >= MAX_PROTSEQ_ARGS)
        {
            SET_STATUS(&status, ept_s_cant_perform_op); 
            show_st("Too many protseq args", &status);
            return;
        }

        protseq[num_protseq++] = (unsigned_char_p_t) argv[i];
        use_all_protseqs = false;
    }
}
#endif

/*
 *  Register the Endpoint Map and LLB interfaces.
 */
INTERNAL void register_ifs(status)
error_status_t  *status;
{
    ept_v3_0_epv_t* _epv = &ept_v3_0_mgr_epv;
    rpc_mgr_epv_t epv = (rpc_mgr_epv_t) _epv;

    rpc_server_register_if(ept_v3_0_s_ifspec, (dce_uuid_p_t)NULL, 
            epv, status);
    if (check_st_bad("Unable to rpc_server_register_if for ept", status))
        return;

#ifdef RPC_LLB
    rpc_server_register_if(llb__v4_0_s_ifspec, (uuid_p_t)NULL, 
            (rpc_mgr_epv_t) &llb_v4_0_mgr_epv, status);
    if (check_st_bad("Unable to rpc_server_register_if for llb", status))
        return;
#endif

#ifdef ENABLE_DCOM 
	 rpc_server_register_if(IObjectExporter_v0_0_s_ifspec, (uuid_p_t)NULL,
			 (rpc_mgr_epv_t)&objex_mgr_epv, status);
	 if (check_st_bad("Unable to rpc_server_register_if for orpc", status))
		 return;
#endif
	 
}

/*
 * Do some server database, ... initialization
 */
INTERNAL void init(status)
error_status_t  *status;
{
    epdb_handle_t       h;
    dce_uuid_t              epdb_obj;
    rpc_if_rep_p_t      ept_if_rep;
    unsigned_char_p_t   fname;
    unsigned_char_p_t   dname;
    struct stat         statbuf;

    dce_uuid_create_nil(&nil_uuid, status);
    if (check_st_bad("Can't create nil uuid", status)) {
        return;
    }

    if (dflag) {
        printf("(rpcd) initializing database\n");
    }

    fname = NULL;
    dname = NULL;

    fname = (unsigned_char_p_t) malloc(strlen(rpcd_c_database_name_prefix1) + 
                                       strlen(rpcd_c_database_name_prefix2) + 
                                       strlen(rpcd_c_ep_database_name) + 1);
    if (!fname) {
	*status = rpc_s_no_memory;
	check_st_bad("Error when allocating ept database filename", status);
	return;
    }

    sprintf((char *) fname, "%s%s%s", rpcd_c_database_name_prefix1,
            rpcd_c_database_name_prefix2, rpcd_c_ep_database_name);

    dname = (unsigned_char_p_t) malloc(strlen(rpcd_c_database_name_prefix1) + 
                                       strlen(rpcd_c_database_name_prefix2) + 1);
    if (!dname) {
	*status = rpc_s_no_memory;
	check_st_bad("Error when allocating ept database directory", status);
	return;
    }

    sprintf((char *) dname, "%s%s", rpcd_c_database_name_prefix1,
            rpcd_c_database_name_prefix2);

    if (stat((char *) dname, &statbuf) &&
	errno == ENOENT) {
	printf("(rpcd) ept database directory [%s] doesn't exist\n", dname);
    }

    h = epdb_init(fname, status);
    if (check_st_bad("Can't initialize ept database", status)) {
	free(fname);
	free(dname);

        return;
    }

    free(fname);
    free(dname);
    
#ifdef RPC_LLB
    fname = (unsigned_char_p_t) malloc(strlen(rpcd_c_database_name_prefix1) + 
                                       strlen(rpcd_c_database_name_prefix2) + 
                                       strlen(rpcd_c_llb_database_name) + 1);
    sprintf((char *) fname, "%s%s%s", rpcd_c_database_name_prefix1,
            rpcd_c_database_name_prefix2, rpcd_c_llb_database_name);

    lbdb_init(fname, status);
    if (check_st_bad("Can't initialize llb database", status))
        return;

    free(fname);
#endif

    epdb_inq_object(h, &epdb_obj, status);
    if (check_st_bad("Can't get ept object uuid", status))	{
		 /* do nothing */;
	 }
    ept_if_rep = (rpc_if_rep_p_t) ept_v3_0_s_ifspec;
    rpc_object_set_type(&epdb_obj, &ept_if_rep->id, status);
    if (check_st_bad("Can't set ept object type", status))	{
		 /* do nothing */;
	 }

    if (dflag)
    {
        unsigned_char_p_t   ustr;
        error_status_t      st;

        dce_uuid_to_string(&epdb_obj, &ustr, &st);
        printf("(rpcd) endpoint database object id: %s\n", ustr);
        rpc_string_free(&ustr, &st);
    }
}

/*
 * Perform the forwarding map algorithm to produce an rpc_addr to the
 * selected endpoint.
 *
 * Eventually, we probably want to get all packets from a single activity
 * to a single server (assuming that we can figure out how take advantage
 * of selecting different potential servers in the face of stale entries).
 * 
 */
INTERNAL void fwd_map
    (object, interface, data_rep, 
    rpc_protocol, rpc_protocol_vers_major, rpc_protocol_vers_minor, addr, 
    actuuid, fwd_addr, fwd_action, status)
dce_uuid_p_t                object;
rpc_if_id_p_t           interface;
rpc_syntax_id_p_t       data_rep;
rpc_protocol_id_t       rpc_protocol;
unsigned32              rpc_protocol_vers_major;
unsigned32              rpc_protocol_vers_minor;
rpc_addr_p_t            addr;
dce_uuid_p_t                actuuid ATTRIBUTE_UNUSED;
rpc_addr_p_t            *fwd_addr;
rpc_fwd_action_t        *fwd_action;
error_status_t          *status;
{
    unsigned32      num_ents;
    epdb_handle_t   h;

    /*
     * Forwarding algorithm:
     * Consult ep database (and possibly the llb database) to see if 
     * anybody has registered the matching interface/object uuids.  
     */

    num_ents = 0;

    h = epdb_inq_handle();
    epdb_fwd(h, object, interface, data_rep, 
             rpc_protocol, rpc_protocol_vers_major, rpc_protocol_vers_minor, 
             addr, NULL, 1L, &num_ents, fwd_addr, status);

#ifdef RPC_LLB
    if ((*status == ept_s_not_registered) ||
        ((*status == rpc_s_ok) && (num_ents == 0)) )
    {
        h = lbdb_inq_handle();
        lbdb_fwd(h, object, &interface->uuid, addr, 
                        NULL, 1L, &num_ents, fwd_addr, status);
    }
#endif

    if (*status != rpc_s_ok)
    {
        if (*status == ept_s_not_registered)
        {
            *fwd_action = rpc_e_fwd_drop;
            *status = rpc_s_ok;
        }
        return;
    }

    assert(num_ents <= 1);

    *fwd_action = num_ents == 0 ? rpc_e_fwd_drop : rpc_e_fwd_forward;
    return;
}

static
void*
rpcd_listen_thread(
    void* arg
    )
{
    error_status_t status = 0;

    rpc_server_listen(5, &status);
    *(error_status_t*) arg = status;

    return arg;
}

static
void*
rpcd_network_thread(
    void* arg
    )
{
    error_status_t status = 0;
    int index = 0;
    int jndex = 0;
    boolean32 use_protseq = false;
    static const struct timespec retry_interval = {5, 0};
    static const char* network_protseqs[] =
    {
        "ncacn_ip_tcp",
        "ncadg_ip_udp",
        NULL
    };

    while (network_protseqs[index])
    {
        use_protseq = false;
        if (!use_all_protseqs)
        {
            for (jndex = 0; jndex < num_protseq; jndex++)
            {
                if (!strcmp(network_protseqs[index], (char*) protseq[jndex]))
                {
                    use_protseq = true;
                    break;
                }
            }
        }
        else
        {
            use_protseq = true;
        }
        if (use_protseq)
        {
            rpc_server_use_protseq_if(
                (unsigned char*) network_protseqs[index],
                0,
                ept_v3_0_s_ifspec,
                &status);
        }

        if (status != rpc_s_ok)
        {
            printf("(rpcd) Could not listen on %s: %lx.  Retrying in %i seconds\n",
                   network_protseqs[index], (long) status, (int) retry_interval.tv_sec);
            dcethread_delay(&retry_interval);
        }
        else
        {
            index++;
        }
    }

    *(error_status_t*) arg = status;

    return arg;
}

NTSTATUS
RpcdSvcmInit(
    PCWSTR pServiceName,
    PLW_SVCM_INSTANCE pInstance
    )
{
    return STATUS_SUCCESS;
}

VOID
RpcdSvcmDestroy(
    PLW_SVCM_INSTANCE pInstance
    )
{
    return;
}

static dcethread* listen_thread = NULL;
static dcethread* network_thread = NULL;
static error_status_t listen_status, network_status;

NTSTATUS
RpcdSvcmStart(
    PLW_SVCM_INSTANCE pInstance,
    ULONG ArgCount,
    PWSTR* ppArgs,
    ULONG FdCount,
    int* pFds
    )
{
    error_status_t status;
    boolean32 is_listening = false;
    const static mode_t np_dir_mode = 0755;

    // process_args(argc, argv);

    /*
     * Initialize the runtime by calling this public routine
     */
    rpc_network_is_protseq_valid ((unsigned_char_p_t) "ncadg_ip_udp", &status);

    /*
     * Initialize the database and other misc stuff.
     */
    init(&status);
    GCOS(status);

    register_ifs(&status);
    GCOS(status);

    /*
     * Ensure existence/permissions on named pipe socket directory
     */
    if (chmod(RPC_C_NP_DIR, np_dir_mode) != 0)
    {
        if (errno != ENOENT ||
            mkdir(RPC_C_NP_DIR, np_dir_mode) != 0)
        {

            printf("(rpcd) could not change permissions on " RPC_C_NP_DIR " directory...\n");
            status = -1;
            GCOS(status);
        }
    }

    /*
     * Register lcalrpc endpoint as a baseline to ensure local services can talk to us
     */
    rpc_server_use_protseq_if((unsigned char*) "ncalrpc", 0, ept_v3_0_s_ifspec, &status);
    GCOS(status);

    rpc__server_register_fwd_map(fwd_map, &status);
    GCOS(status);

    /*
     * Fire up listener thread
     */
    dcethread_create_throw(&listen_thread, NULL, rpcd_listen_thread, (void*) &listen_status);

    /*
     * Busy wait until we are actually listening
     */
    while (!is_listening)
    {
        is_listening = rpc_mgmt_is_server_listening(NULL, &status);
        GCOS(status);
    }

    /*
     * Fire up network detection thread
     */

    dcethread_create_throw(&network_thread, NULL, rpcd_network_thread, (void*) &network_status);

cleanup:

    return LwRpcStatusToNtStatus(status);
}

NTSTATUS
RpcdSvcmStop(
    PLW_SVCM_INSTANCE pInstance
    )
{
    error_status_t status = rpc_s_ok;

    /*
     * Cancel listener thread
     */
    dcethread_interrupt_throw(listen_thread);

    /*
     * Wait for listener thread to exit
     */
    dcethread_join_throw(listen_thread, NULL);

    /*
     * Cancel network detection thread
     */
    dcethread_interrupt_throw(network_thread);

    /*
     * Join network detection thread
     */
    dcethread_join_throw(network_thread, NULL);

    return LwRpcStatusToNtStatus(status);
}

static LW_SVCM_MODULE gService =
{
    .Size = sizeof(gService),
    .Init = RpcdSvcmInit,
    .Destroy = RpcdSvcmDestroy,
    .Start = RpcdSvcmStart,
    .Stop = RpcdSvcmStop
};

#define SVCM_ENTRY_POINT LW_RTL_SVCM_ENTRY_POINT_NAME(dcerpc)

PLW_SVCM_MODULE
SVCM_ENTRY_POINT(
    VOID
    )
{
    return &gService;
}
