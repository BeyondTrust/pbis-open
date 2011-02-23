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

/* FIXME: Hack to get prototypes we need on AIX */
#if defined(_AIX) && defined(_BSD)
extern pid_t setpgrp(void);
int ioctl(int d, int request, ...);
#ifdef SETPGRP_ARGS
#    undef SETPGRP_ARGS
#endif
#define SETPGRP_ARGS 0
#endif

INTERNAL void usage
    (
        void
    );

#if 0
INTERNAL boolean32 match_command
    (
        char           *key,
        char           *str,
        long           min_len
    );
#endif

INTERNAL void process_args
    (
        int             argc,
        char            *argv[]
    );

INTERNAL void register_ifs
    (
        error_status_t  *status
    );

#if 0
INTERNAL void use_protseqs
    (
        error_status_t  *status
    );
#endif

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

INTERNAL boolean32      foreground = false;


INTERNAL char           rpcd_version_str[] =
#ifdef ENABLE_DCOM
	"rpcd/orpcd version freedce 1.1";
#else
	"rpcd version freedce 1.1";
#endif

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

INTERNAL void usage()
{
#ifdef DEBUG
    fprintf(stderr, "usage: rpcd [-vDuf] [-d<debug switches>] [<protseq> ...]\n");
#else
    fprintf(stderr, "usage: rpcd [-vuf] [<protseq> ...]\n");
#endif
    fprintf(stderr, "  -v: Print rpcd version and exit\n");
#ifdef DEBUG
    fprintf(stderr, "  -D: Turns on default RPC runtime debug output\n");
#endif
    fprintf(stderr, "  -u: Print this message and exit\n");
    fprintf(stderr, "  -f: Run in foreground (default is to fork and parent exit)\n");
#ifdef DEBUG
    fprintf(stderr, "  -d: Turns on specified RPC runtime debug output\n");
#endif
    fprintf(stderr, "  If any <protseq>s are specified, the rpcd listens only on those; otherwise\n");
    fprintf(stderr, "  all protseqs are listened on.\n");
}

/* 
 * match_command 
 * takes a key and string as input and returns whether the
 * string matches the key where at least min_len characters
 * of the key are required to be specified.
 */
#if 0
INTERNAL boolean32 match_command(key,str,min_len)
char *key;
char *str;
long min_len;
{
    int i = 0;

    if (*key) while (*key == *str) {
        i++;
        key++;
        str++;
        if (*str == '\0' || *key == '\0')
            break;
    }
    if (*str == '\0' && i >= min_len)
        return true;
    return false;
}
#endif


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

    while ((c = getopt(argc, argv, "vufDd:")) != EOF)
    {
        switch (c)
        {
        case 'u':
            usage();
            exit(0);

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
            usage();
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

#if 0
/*
 * Arrange to handle calls on the protocol sequences of interest.
 * Note that while both interfaces specify well know endpoints,
 * the ept_ endpoints are a superset of the llb_ endpoints (which 
 * precludes us from doing a "use_protseq_if" on both).
 */
INTERNAL void use_protseqs(status)
error_status_t  *status;
{
    unsigned32 i;

    if (use_all_protseqs)
    {
        rpc_server_use_all_protseqs_if(0, ept_v3_0_s_ifspec, status);
        if (! STATUS_OK(status)) 
        {
            if (*status == rpc_s_cant_bind_sock)
                show_st("Verify that no other rpcd/llbd is running", status);
            else
                show_st("Unable to rpc_server_use_all_protseqs for ept", status);
        }
    }
    else
    {
        for (i = 0; i < num_protseq; i++)
        {
            rpc_server_use_protseq_if(protseq[i], 0, ept_v3_0_s_ifspec, status);
            if (! STATUS_OK(status)) 
            {
                if (*status == rpc_s_cant_bind_sock)
                    show_st("Verify that no other rpcd/llbd is running", status);
                else
                    show_st("Unable to rpc_server_use_all_protseqs for ept", status);
            }
        }
    }


    /*
     * If folks are interested, tell em what we're listening on...
     */

    if (dflag)
    {
        rpc_binding_vector_p_t  bv;
        unsigned_char_p_t       bstr;
        error_status_t          st;

        printf("(rpcd) got bindings:\n");

        rpc_server_inq_bindings(&bv, status);
        if (check_st_bad("Unable to rpc_server_inq_bindings", status))
            return;

        for (i = 0; i < bv->count; i++)
        {   
            rpc_binding_to_string_binding(bv->binding_h[i], &bstr, &st);
            printf("    %s\n", bstr);
            rpc_string_free(&bstr, &st);
        }

        rpc_binding_vector_free(&bv, status);
        if (check_st_bad("Unable to rpc_binding_vector_free", status))
            return;
    }
}
#endif

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


//#if defined(UNIX) || defined(unix)

#define RPCD_PID_FILE "/var/run/dcerpcd.pid" 
#define PID_FILE_CONTENTS_SIZE ((9 * 2) + 2)
#define RPCD_DAEMON_NAME "dcerpcd"

static
void
StripLeadingWhitespace(
    char* str
    )
{
    char* pszNew = str;
    char* pszTmp = str;

    if (!str || !*str || !isspace((int)*str)) {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0' && isspace((int)*pszTmp)) {
        pszTmp++;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        *pszNew++ = *pszTmp++;
    }
    *pszNew = '\0';
}

static
void
StripTrailingWhitespace(
    char* str
    )
{
    char* pszLastSpace = NULL;
    char* pszTmp = str;

    if (!str || !*str) {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        pszLastSpace = (isspace((int)*pszTmp) ? (pszLastSpace ? pszLastSpace : pszTmp) : NULL);
        pszTmp++;
    }

    if (pszLastSpace != NULL) {
        *pszLastSpace = '\0';
    }
}

static
void
StripWhitespace(
    char* str
    )
{
    if (!str || !*str)
       return;
    StripLeadingWhitespace(str);
    StripTrailingWhitespace(str);
}

static
int
MatchProgramToPID(
    const char* pszProgramName,
    pid_t pid
    )
{
    int ceError = 0;
    char szBuf[PATH_MAX+1];
    FILE* pFile = NULL;

#if defined(__MACH__) && defined(__APPLE__)
    sprintf(szBuf, "ps -p %d -o command= | grep %s", pid, pszProgramName);
#else
    sprintf(szBuf, "UNIX95=1 ps -p %ld -o comm= | grep %s", (long)pid, pszProgramName);
#endif

    pFile = popen(szBuf, "r");
    if (pFile == NULL) {
        ceError = errno;
        goto error;
    }

    while (TRUE) {

        if (NULL == fgets(szBuf, PATH_MAX, pFile)) {
            if (feof(pFile))
                break;
            else {
                ceError = errno;
                goto error;
            }
        }

        StripWhitespace(szBuf);
        if (*szBuf) {
            ceError = 0;
            break;
        }

    }

error:

    if (pFile)
        fclose(pFile);

    return ceError;
}

static
pid_t
pid_from_pid_file()
{
    pid_t pid = 0;
    int fd = -1;
    int result;
    char contents[PID_FILE_CONTENTS_SIZE];

    fd = open(RPCD_PID_FILE, O_RDONLY, 0644);
    if (fd < 0) {
        goto error;
    }

    result = dcethread_read(fd, contents, sizeof(contents)-1);
    if (result < 0) {
        goto error;
    } else if (result == 0) {
        unlink(RPCD_PID_FILE);
        goto error;
    }
    contents[result-1] = 0;

    result = atoi(contents);
    if (result < 0) {
        result = -1;
        goto error;
    } else if (result == 0) {
        unlink(RPCD_PID_FILE);
        goto error;
    }

    pid = (pid_t) result;
    result = kill(pid, 0);
    if (result != 0 || errno == ESRCH) {
        unlink(RPCD_PID_FILE);
        pid = 0;
    } else {
        // Verify that the peer process is a rpc daemon
        if (MatchProgramToPID(RPCD_DAEMON_NAME, pid) != 0) {
            unlink(RPCD_PID_FILE);
            pid = 0;
        }
    }

error:
    if (fd != -1) {
        close(fd);
    }

    return pid;
}

static
void
delete_pid_file()
{
    pid_t pid;

    pid = pid_from_pid_file();
    if (pid == getpid()) {
        unlink(RPCD_PID_FILE);
    }
}

static
void
create_pid_file()
{
    int result = -1;
    pid_t pid;
    char contents[PID_FILE_CONTENTS_SIZE];
    size_t len;
    int fd = -1;

    pid = pid_from_pid_file();
    if (pid > 0) {
        fprintf(stderr, "Daemon already running as %d", (int) pid);
        result = -1;
        goto error;
    }

    fd = open(RPCD_PID_FILE, O_CREAT | O_WRONLY | O_EXCL, 0644);
    if (fd < 0) {
        fprintf(stderr, "Could not create pid file: %s", strerror(errno));
        result = 1;
        goto error;
    }

    pid = getpid();
    snprintf(contents, sizeof(contents)-1, "%d\n", (int) pid);
    contents[sizeof(contents)-1] = 0;
    len = strlen(contents);

    result = (int) dcethread_write(fd, contents, len);
    if ( result != (int) len ) {
        fprintf(stderr, "Could not write to pid file: %s", strerror(errno));
        result = -1;
        goto error;
    }

    result = 0;

error:
    if (fd != -1) {
        close(fd);
    }

    if (result < 0) {
        exit(1);
    }
}


static
void
attach_log_file(
    void
    )
{
    int fd;
    char *fname;
    char *p;

    if ((fname = malloc(strlen(rpcd_c_database_name_prefix1) + 
                        strlen(rpcd_c_database_name_prefix2) + 
                        strlen(rpcd_c_logfile_name) + 1)) != NULL)
    {
        sprintf((char *) fname, "%s%s%s", rpcd_c_database_name_prefix1,
                rpcd_c_database_name_prefix2, rpcd_c_logfile_name);
        if ((fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC,
                       S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) != -1)
        {
            (void)dup2(fd,2);
        }
        (void)close(fd);
        /*
         * We don't care if open() or dup2() failed.
         */

        if ((p = strrchr(fname, (int)'/')) != NULL)
        {
            *p = '\0';
            if (chdir(fname))
            {
                /*
                 * Again, we don't care if chdir() failed.
                 */
            }
        }
        free(fname);
    }
}

static
void
start_as_daemon(
    void
    )
{
    int pid;
    int fd;

    /* Use dcethread_fork() rather than fork() because we link with DCE/RPC */
    if ((pid = dcethread_fork()) != 0)
    {
        // Parent terminates
        exit(0);
    }

    // Let the first child be a session leader
    setsid();

    // Ignore SIGHUP, because when the first child terminates
    // it would be a session leader, and thus all processes in
    // its session would receive the SIGHUP signal. By ignoring
    // this signal, we are ensuring that our second child will
    // ignore this signal and will continue execution.
    if (signal(SIGHUP, SIG_IGN) < 0)
    {
        exit(1);
    }

    // Spawn a second child
    if ((pid = fork()) != 0) {
        // Let the first child terminate
        // This will ensure that the second child cannot be a session leader
        // Therefore, the second child cannot hold a controlling terminal
        exit(0);
    }

    for (fd = 0; fd < 3; fd++)
    {
        close(fd);
    }

    for (fd = 0; fd < 3; fd++)
    {
        int null_fd = open("/dev/null", O_RDWR, 0);
        if (null_fd < 0)
        {
            null_fd = open("/dev/null", O_WRONLY, 0);
        }
        if (null_fd < 0)
        {
            exit(1);
        }
        if (null_fd != fd)
        {
            exit(1);
        }
    }

    atexit(delete_pid_file);
    create_pid_file();
    attach_log_file();
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

        if (!STATUS_OK(&status))
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

static
void
rpcd_handle_sigint(
    int sig
    )
{
    raise(SIGTERM);
}

static
void
rpcd_configure_signals(
    void
    )
{
    sigset_t set;
    int i = 0;

    static const int block_signals[] =
    {
        SIGTERM,
        SIGHUP,
        -1
    };
    static const struct sigaction act =
    {
        .sa_handler = rpcd_handle_sigint,
        .sa_flags = 0
    };

    if (sigaction(SIGINT, &act, NULL) < 0)
    {
        printf("(rpcd) System call failed\n");
        exit(1);
    }

    if (sigemptyset(&set) < 0)
    {
        printf("(rpcd) System call failed\n");
        exit(1);
    }

    for (i = 0; block_signals[i] != -1; i++)
    {
        if (sigaddset(&set, block_signals[i]) < 0)
        {
            printf("(rpcd) System call failed\n");
            exit(1);
        }
    }

    if (pthread_sigmask(SIG_SETMASK, &set, NULL) != 0)
    {
        printf("(rpcd) pthread_sigmask(): %s\n", strerror(errno));
        exit(1);
    }
}

static
void
rpcd_wait_signals(
    error_status_t* status
    )
{
    sigset_t set;
    static int wait_signals[] =
    {
        SIGTERM,
        SIGHUP,
        -1
    };
    int sig = -1;
    int i = 0;

    if (sigemptyset(&set) < 0)
    {
        *status = rpc_s_unknown_error;
        goto error;
    }

    for (i = 0; wait_signals[i] != -1; i++)
    {
        if (sigaddset(&set, wait_signals[i]) < 0)
        {
            *status = rpc_s_unknown_error;
            goto error;
        }
    }

    for (;;)
    {
        if (sigwait(&set, &sig) < 0)
        {
            *status = rpc_s_unknown_error;
            goto error;
        }

        printf("(rpcd) Received signal %i\n", sig);

        switch (sig)
        {
        case SIGTERM:
            goto cleanup;
        default:
            break;
        }
    }

cleanup:

    return;

error:

    goto cleanup;
}

int main(int argc, char *argv[])
{
    error_status_t  status, listen_status, network_status;
    int uid ;
    int ret;
    const char* sm_notify = NULL;
    int notify_fd = -1;
    int notify_code = 0;
    dcethread* listen_thread = NULL;
    dcethread* network_thread = NULL;
    boolean32 is_listening = false;
    const static mode_t np_dir_mode = 0755;

    /* begin */

    setlocale(LC_ALL, "");

    process_args(argc, argv);

    /*
     * Must be root (pid=0) to be able to start llbd
     */
    uid = getuid();
    if (uid != 0) {  
        fprintf(stderr, "(rpcd) Must be root to start rpcd, your uid = %d \n", uid);
        exit(2);
    }

    /* Set up signals */
    rpcd_configure_signals();

    /*
     * If not debugging, fork off a process to be the rpcd.  The parent exits.
     */

    if (!dflag && !foreground) 
    {
        start_as_daemon();
    }

    /*
     * Initialize the runtime by calling this public routine
     */
    rpc_network_is_protseq_valid ((unsigned_char_p_t) "ncadg_ip_udp", &status);

    /*
     * Initialize the database and other misc stuff.
     */
    init(&status);
    if (! STATUS_OK(&status)) exit(1);

    register_ifs(&status);
    if (! STATUS_OK(&status)) exit(1);

    /*
     * Ensure existence/permissions on named pipe socket directory
     */
    if (chmod(RPC_C_NP_DIR, np_dir_mode) != 0)
    {
        if (errno != ENOENT ||
            mkdir(RPC_C_NP_DIR, np_dir_mode) != 0)
        {

            printf("(rpcd) could not change permissions on " RPC_C_NP_DIR " directory...\n");
            exit(1);
        }
    }

    /*
     * Register lcalrpc endpoint as a baseline to ensure local services can talk to us
     */
    rpc_server_use_protseq_if((unsigned char*) "ncalrpc", 0, ept_v3_0_s_ifspec, &status);

    if (!STATUS_OK(&status))
    {
        printf("(rpcd) could not listen on ncalrpc\n");
        exit(1);
    }

    rpc__server_register_fwd_map(fwd_map, &status);
    if (check_st_bad("Unable to rpc_server_register_fwd_map", &status))
        exit(1);

    if (dflag)
        printf("(rpcd) listening...\n");

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

        if (!STATUS_OK(&status))
        {
            printf("(rpcd) could not determine if listener is running\n");
            exit(1);
        }
    }

    /*
     * If we were started by the service manager, let it know we are ready
     */
    if ((sm_notify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        notify_fd = atoi(sm_notify);
        
        do
        {
            ret = dcethread_write(notify_fd, &notify_code, sizeof(notify_code));
        } while(ret != sizeof(notify_code) && errno == EINTR);

        if (ret < 0)
        {
            printf("(rpcd) Could not notify service manager: %s (%i)", strerror(errno), errno);
            exit(1);
        }

        close(notify_fd);
    }

    /*
     * Fire up network detection thread
     */

    dcethread_create_throw(&network_thread, NULL, rpcd_network_thread, (void*) &network_status);

    /*
     * Wait for signals
     */

    rpcd_wait_signals(&status);

    if (check_st_bad("Error waiting for signals", &status))
    {
        exit(1);
    }

    /*
     * Tell listener thread to stop
     */
    
    rpc_mgmt_stop_server_listening(NULL, &status);

    if (check_st_bad("Error stopping server", &status))
    {
        exit(1);
    }
    
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

    /*
     * We're done
     */
    exit(0);
}
