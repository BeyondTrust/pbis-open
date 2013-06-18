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
**  NAME
**
**      server.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Server main code for performance and system exerciser.
**
**
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <perf_c.h>
#include <perf_p.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <stdlib.h>
#include <unistd.h>

#ifdef AUTH_KRB
#include <dce/id_base.h>
#endif /* AUTH_KRB */


idl_boolean auth = false;       /* should we check authentication? */

unsigned16 server_loops = 1;    /* # of times to loop on calls to rpc_server_listen */
#define SERVER_LOOP_SLEEP_TIME 15   /* # of secs to sleep after return from " " " */

idl_boolean use_reserved_threads = false;
idl_boolean pool_active = false;
static rpc_thread_pool_handle_t thread_poolh;
static rpc_if_id_t perfg_if_id;
static rpc_if_id_t perf_if_id;

int verbose = 1;

unsigned32 socket_buf_size = 0;  /* os default */

rpc_binding_vector_p_t bv;

static struct 
{
    unsigned32      count;
    uuid_p_t        uuid[2];
} object_vec = 
{
    2,
    {
        &NilObj,
        &NilTypeObj,
    }
};


/*
 * M G M T _ A U T H _ F N
 *
 * Procedure called to determine whether server will be allowed to be shutdown
 * remotely.
 */

idl_boolean shut_ok = true;

static boolean32 mgmt_auth_fn (h, op, st)

handle_t            h __attribute__((unused));
unsigned32          op;
unsigned32          *st;

{
    boolean32 b;

    b = (op == rpc_c_mgmt_stop_server_listen) ? shut_ok : true;

    VRprintf (1, ("+ In management authorization function: op=%lu, returning \"%s\"\n",
                op, b ? "true" : "false"));

    *st = b ? rpc_s_ok : rpc_s_mgmt_op_disallowed;

    return (b);
}

/*
 * I F _ I D _ I S _ E Q U A L
 *
 * return true iff the rpc_if_id_t's are identical.
 */
static idl_boolean if_id_is_equal(ifid1, ifid2)
rpc_if_id_p_t ifid1, ifid2;
{
    unsigned32 st;

    return (ifid1->vers_major == ifid2->vers_major
        && ifid1->vers_minor == ifid2->vers_minor
        && dce_uuid_equal(&ifid1->uuid, &ifid2->uuid, &st));
}

/*
 * T H R E A D _ P O O L _ F N
 *
 * Thread pool lookup function.  Manage the thread pool(s)
 * based on a simple object uuid scheme:
 *      for perfg.idl/perfg_op2 for non-nil object id use reserved pool threads
 *      for perf.idl/perf_null_slow use reserved pool threads
 *      otherwise use a default pool thread
 */
static void thread_pool_fn(obj_uuid, if_id, opnum, phandle, status)
uuid_p_t                obj_uuid;
rpc_if_id_p_t           if_id;
unsigned32              opnum;
rpc_thread_pool_handle_t *phandle;
unsigned32              *status;
{
    unsigned32 st;
    static int once = 1;

    if (once == 1)
    {
        VRprintf (1, ("+ Thread pool selection function active\n"));
        once = 0;
    }

    if (! use_reserved_threads)
    {
        fprintf(stderr, "Thread Pool FN called without being enabled\n");
        fprintf(stderr, "Ignoring RPC\n");
        *status = -1;
    }

    *phandle = NULL;
    if ((opnum == 1 && if_id_is_equal(if_id, &perfg_if_id)
            && obj_uuid != NULL && ! dce_uuid_is_nil(obj_uuid, &st))
      || (opnum == 15 && if_id_is_equal(if_id, &perf_if_id)))
    {
        *phandle = thread_poolh;
    }
    *status = 0;
}

/*
 * S E T U P _ T H R E A D _ P O O L S
 *
 * Create a threads pool and register a pool lookup function.
 * Create a thread pool for the specified interface.
 * return 0 - ok
 *       -1 - couldn't create pool
 *        1 - couldn't set thread pool fn
 */
static int setup_thread_pools()
{
    unsigned32 st;

    if (! use_reserved_threads)
        return 0;

    rpc_server_create_thread_pool(2, &thread_poolh, &st);
    if (st != error_status_ok) 
    {
        fprintf(stderr, "Cannot create thread pool - %s\n", error_text(st));
        return -1;
    }
    pool_active = true;

    rpc_server_set_thread_pool_fn(thread_pool_fn, &st);
    if (st != error_status_ok) 
    {
        fprintf(stderr, "Cannot set thread pool fn - %s\n", error_text(st));
        return 1;
    }

    /*
     * A little more setup for the thread_pool_fn().
     */
    rpc_if_inq_id(perfg_v1_0_s_ifspec, &perfg_if_id, &st);
    rpc_if_inq_id(perf_v2_0_s_ifspec, &perf_if_id, &st);

    return 0;
}

/*
 * T E A R D O W N _ T H R E A D _ P O O L S
 *
 * Free our thread pools and unregister our lookup fn.
 * return 0 - ok
 *       -1 - couldn't free pool
 *        1 - couldn't set thread pool fn NULL
 */
int teardown_thread_pools(wait_flg)
idl_boolean wait_flg;
{
    unsigned32 st;

    if (! use_reserved_threads)
        return 0;

    rpc_server_set_thread_pool_fn(NULL, &st);
    if (st != error_status_ok) 
    {
        fprintf(stderr, "Cannot thread pool fn NULL - %s\n", error_text(st));
        return 1;
    }

    if (pool_active)
    {
        pool_active = false;
        rpc_server_free_thread_pool(&thread_poolh, wait_flg, &st);
        if (st != error_status_ok) 
        {
            fprintf(stderr, "Cannot free thread pool - %s\n", error_text(st));
            return -1;
        }
    }

    return 0;
}

/*
 * R E G I S T E R _ I F S
 *
 * Register the interfaces we export.
 */

static void register_ifs ()

{
    unsigned32          st;

    rpc_server_register_if (perf_v2_0_s_ifspec, NULL,
        (rpc_mgr_epv_t) &perf_epv, &st);

    if (st != 0)
    {
        fprintf(stderr, "*** Can't register - %s\n", error_text(st));
        exit(1);
    }

    rpc_server_register_if (perfg_v1_0_s_ifspec, (uuid_p_t) &FooType, 
        (rpc_mgr_epv_t) &foo_perfg_epv, &st);

    if (st != 0)
    {
        fprintf(stderr, "*** Can't register Foo mgr - %s\n", error_text(st));
        exit(1);
    }

    rpc_server_register_if (perfg_v1_0_s_ifspec, (uuid_p_t) &BarType, 
        (rpc_mgr_epv_t) &bar_perfg_epv, &st);

    if (st != 0)
    {
        fprintf(stderr, "*** Can't register Bar mgr - %s\n", error_text(st));
        exit(1);
    }
}

/*
 * R E G I S T E R _ O B J S
 *
 * Register the bogus objects we handle.
 */

static void register_objs ()

{
    unsigned32          st;

    rpc_object_set_type ((uuid_p_t) &FooObj1, (uuid_p_t) &FooType, &st);

    if (st != 0)
    {
        fprintf(stderr, "*** Can't register Foo1 object - %s\n", error_text(st));
        exit(1);
    }

    rpc_object_set_type ((uuid_p_t) &BarObj1, (uuid_p_t) &BarType, &st);

    if (st != 0)
    {
        fprintf(stderr, "*** Can't register Bar1 object - %s\n", error_text(st));
        exit(1);
    }

    rpc_object_set_type ((uuid_p_t) &FooObj2, (uuid_p_t) &FooType, &st);

    if (st != 0)
    {
        fprintf(stderr, "*** Can't register Foo1 object - %s\n", error_text(st));
        exit(1);
    }

    rpc_object_set_type ((uuid_p_t) &BarObj2, (uuid_p_t) &BarType, &st);

    if (st != 0)
    {
        fprintf(stderr, "*** Can't register Bar2 object - %s\n", error_text(st));
        exit(1);
    }

    rpc_object_set_type ((uuid_p_t) &ZotObj, (uuid_p_t) &ZotType, &st);

    if (st != 0)
    {
        fprintf(stderr, "*** Can't register Zot object - %s\n", error_text(st));
        exit(1);
    }
}

/*
 * G E T _ S O C K E T S
 *
 * Create sockets to listen on.
 */

static void get_sockets (argc, argv, max_calls)

int                 argc;
char                *argv[];
unsigned32          max_calls;

{
    unsigned32          st;
    unsigned long       i;

    i = 2;

    while (i < (unsigned)argc)
    {
        if (strcmp(argv[i], "all") == 0)
        {
            rpc_server_use_all_protseqs (max_calls, &st);
        }
        else if (strcmp(argv[i], "allif") == 0)
        {
            rpc_server_use_all_protseqs_if (max_calls, perf_v2_0_s_ifspec, &st);
        }
        else if (strcmp(argv[i], "notif") == 0)
        {
            rpc_server_use_protseq 
                ((unsigned_char_p_t) argv[i + 1], max_calls, &st);
            i += 1;
        }
        else if (strcmp(argv[i], "ep") == 0)
        {
            rpc_server_use_protseq_ep 
                ((unsigned_char_p_t) argv[i + 1], max_calls,
                    (unsigned_char_p_t) argv[i + 2], &st);
            i += 2;
        }
        else
        {
            rpc_server_use_protseq_if 
                ((unsigned_char_p_t) argv[i], max_calls, perf_v2_0_s_ifspec, &st);
        }

        if (st != rpc_s_ok)
        {
            fprintf(stderr, "*** Can't use_protseq - %s\n", error_text(st));
            exit(1);
        }

        i += 1;
    }

    rpc_server_inq_bindings(&bv, &st);

    if (st != rpc_s_ok)
    {
        fprintf(stderr, "*** Can't inq_bindings - %lx\n", st);
        exit(1);
    }

    for (i = 0; i < bv->count; i++)
    {
        unsigned_char_p_t sb;

        rpc_binding_to_string_binding (bv->binding_h[i], &sb, &st);

        if (st != rpc_s_ok)
        {
            fprintf(stderr, "*** Can't get string binding - %lx\n", st);
            exit(1);
        }

        VRprintf (1, ("Got binding: %s\n", sb));

        rpc_string_free(&sb, &st);
    }
}

/*
 * U S A G E
 */

void usage(void)

{
    fprintf(stderr, "usage: server [-sD] [-S <server loops>] [-d <debug switches>]\n");
    fprintf(stderr, "              [-p <authn proto>,<principal>[,<keytab file>]] [-v <verbose level>]\n");
    fprintf(stderr, "              [-B <bufsize>\n");
    fprintf(stderr, "              <max calls> <protseq spec> [<protseq spec> ...]\n");
    fprintf(stderr, "  -d: Turns on NCK runtime debug output\n");
    fprintf(stderr, "  -D: Turns on default NCK runtime debug output\n");
    fprintf(stderr, "  -s: Disable remote shutdown\n");    
    fprintf(stderr, "  -S: Number of times to run listen loop (default = 1)\n");    
    fprintf(stderr, "  -p: Accept authentication using <authn proto> to <principal>\n");
    fprintf(stderr, "  -r: Use reserved threads\n");
    fprintf(stderr, "  -e: Register with endpoint map at startup\n");
    fprintf(stderr, "  -B: Set CN TCP socket buffer size (bytes)\n");
    fprintf(stderr, "  -b: Register B interface with endpoint map at startup\n");
    fprintf(stderr, "  <protseq spec> is one of:\n");
    fprintf(stderr, "     <protseq>         (rpc_server_use_protseq_if)\n");
    fprintf(stderr, "     all               (rpc_server_use_all_protseqs)\n");
    fprintf(stderr, "     allif             (rpc_server_use_all_protseqs_if)\n");
    fprintf(stderr, "     ep <endpoint>     (rpc_server_use_protseq_ep)\n");
    fprintf(stderr, "     notif <protseq>   (rpc_server_use_protseq)\n");

    exit(1);
}

/*
 * P R I N T _ B I N D I N G _ I N F O
 *
 * Print out binding info (location and auth info) for a server binding.  This
 * routine is called (selectively) from manager routines.
 */ 

void print_binding_info(text, h)

char        *text;
handle_t    h;

{
    unsigned32          st;
    unsigned32          authn_level, authn_protocol, authz_protocol;
    unsigned_char_p_t   server_princ, client_princ;
    unsigned_char_p_t   name;


    if (h == NULL)
    {
        return;
    }

    rpc_binding_to_string_binding (h, &name, &st);
    if (st != rpc_s_ok)
    {
        fprintf (stderr, "*** Can't convert binding to string binding - %s\n",
            error_text (st));
        exit(1);
    }

    VRprintf (1, ("+ %s: called from %s ", text, name));

    rpc_string_free (&name, &st);

    if (verbose <= 1)
    {
        VRprintf (1, ("\n"));
        return;
    }

    rpc_binding_inq_auth_client 
        (h, (rpc_authz_handle_t *) &client_princ, &server_princ, 
        &authn_level, &authn_protocol, &authz_protocol, &st);

    if (st == rpc_s_binding_has_no_auth)
    {
        printf ("(unauthenticated)\n");
    }
    else if (st != rpc_s_ok)
    {
        printf ("(can't get auth info - %s)\n", error_text (st));
    }
    else
    {
        printf("(authentication info follows)\n");
    
        printf("    authn protocol: %s\n    authz protocol: %s\n    level: %s\n    server princ: \"%s\"\n",
            authn_names[authn_protocol], 
            authz_names[authz_protocol],
            authn_level_names[authn_level], 
            server_princ == NULL ? "(NULL)" : (char *) server_princ);
    
        if (server_princ != NULL)
        {
            rpc_string_free (&server_princ, &st);
        }
    
        switch (authz_protocol) 
        {
        case rpc_c_authz_name:
            printf("    client princ: \"%s\"\n", 
                client_princ == NULL ? "(NULL)" : (char *) client_princ);
            break;
    
#ifdef AUTH_KRB
        case rpc_c_authz_dce:
        {
            sec_id_pac_t    *pac = (sec_id_pac_t *) client_princ;
            unsigned16      i;
        
            printf("    PAC: uid %d, gid %d, ngroups %d:",
                pac->principal.uuid.time_low,
                pac->group.uuid.time_low,
                pac->num_groups);
            for (i = 0; i < pac->num_groups; i++) 
            {
                printf (" %d", pac->groups[i].uuid.time_low);
            }
            printf ("\n");
            break;
        }    
#endif /* AUTH_KRB */
        default:
            printf("    unknown authorization protocol\n");
            break;
        }
    }
}

/*
 * M A I N
 *
 * Main program.
 */
extern void dump_stg_info(void);
extern void rpc__dbg_set_switches    (
        char            * /*s*/,
        unsigned32      * /*st*/
    );
void rpc__cn_set_sock_buffsize (
        unsigned32	  /* rsize */,
        unsigned32	  /* ssize */,
        error_status_t	* /* st */);
void rpc__cn_inq_sock_buffsize (
        unsigned32	* /* rsize */,
        unsigned32	* /* ssize */,
        error_status_t  * /* st */);


extern int lookup_name(char *table[], char *s);

int main (argc, argv)

int                 argc;
char                *argv[];

{
    unsigned32      st;
    idl_boolean     debug = false;
    idl_char        *auth_principal, *auth_principal_2;
    char            *s;
    int             c;
    unsigned32      authn_protocol;
    unsigned16      i;
    extern int      optind;
    extern char     *optarg;
    unsigned32      max_calls;
    idl_boolean     ep_reg = false;     /* should we rpc_ep_register() at startup? */
    idl_boolean     b_reg = false;      /* ditto for perfb i/f */
    idl_char        *keytab;
    unsigned32      ssize,rsize;

	 DO_NOT_CLOBBER(i);
	 DO_NOT_CLOBBER(ep_reg);

    while ((c = getopt(argc, argv, "beslrDB:d:p:S:v:")) != EOF)
    {
        switch (c)
        {
        case 'd':
        case 'D':
            rpc__dbg_set_switches(c == 'd' ? optarg : DEBUG_LEVEL, &st);
            if (st != rpc_s_ok)
            {
                fprintf(stderr, "*** Error setting debug level - %s\n",
                    error_text(st));
                usage();
            }

            debug = true;
            break;

        case 'p':
            if ((s = (char *)strtok(optarg, ",")) == NULL)
                usage();
            authn_protocol = strcmp(s, "default") == 0 ? 
                                rpc_c_authn_default : lookup_name(authn_names, s);

            if ((auth_principal = (idl_char *) strtok(NULL, ",")) == NULL)
                usage();

            keytab = (idl_char *) strtok(NULL, " ");

            VRprintf(2, ("+ Authentication params; authn_protocol: %s, auth_principal: %s, keytab: %s\n",
                authn_protocol == (unsigned32)rpc_c_authn_default ? 
                    "default" : authn_names[authn_protocol], 
                auth_principal,
                keytab == NULL ?
                   (idl_char *) "<not specified>" : keytab
                ));

            rpc_server_register_auth_info 
                ((unsigned_char_p_t) auth_principal, authn_protocol, NULL, keytab, &st);
            if (st != rpc_s_ok)
            {
                fprintf(stderr, "*** Error setting principal - %s\n", error_text(st));
                exit(1);
            }

            rpc_mgmt_inq_server_princ_name
                (NULL, authn_protocol, &auth_principal_2, &st);
            if (st != rpc_s_ok)
            {
                fprintf(stderr, "*** Can't get my principal name %s\n", error_text(st));
                exit(1);
            }
            VRprintf(2, ("+ Server principal name set to \"%s\"\n", auth_principal_2));

            auth = true;
            break;

        case 's':
            shut_ok = false;
            break;

        case 'S':
            server_loops = atoi(optarg);
            break;

        case 'v':
            verbose = atoi(optarg);
            break;

        case 'r':
            use_reserved_threads = true;
            if (setup_thread_pools() != 0)
                exit(1);
            break;

        case 'e':
            ep_reg = true;
            break;

        case 'b':
            b_reg = true;
            break;

        case 'B':
            socket_buf_size = atoi(optarg);
            break;

        default:
            usage();
        }
    }

    argc -= optind - 1;
    argv = &argv[optind - 1];

    if (argc < 3)
    {
        usage();
    }

    if (debug)
    {
        dump_stg_info();
    }

    rpc__cn_set_sock_buffsize(socket_buf_size, socket_buf_size, &st);
    if (st != rpc_s_ok)
    {
	fprintf(stderr,"*** rpc__cn_set_sock_buffsize failed (0x%lx)\n", st);
	exit(1);
    }

    rpc__cn_inq_sock_buffsize(&rsize, &ssize, &st);
    if (st != rpc_s_ok)
    {
	fprintf(stderr,"*** rpc__cn_inq_sock_buffsize failed (0x%lx)\n", st);
	exit(1);
    }
    if (socket_buf_size != rsize || socket_buf_size != ssize)
    {
        fprintf(stderr, "*** CN socket buffer sizes dont match:\n");
        fprintf(stderr, "*** READ desired: %lu   actual: %lu\n", socket_buf_size, rsize);
        fprintf(stderr, "*** WRITE desired: %lu  actual: %lu\n", socket_buf_size, ssize);
	exit(1);
    }

    rpc_mgmt_set_authorization_fn(mgmt_auth_fn, &st);

    max_calls = atoi(argv[1]);

    get_sockets(argc, argv, max_calls);
    register_ifs();
    register_objs();

    if (ep_reg)
    {
        rpc_ep_register(perf_v2_0_s_ifspec, bv, (uuid_vector_p_t) &object_vec,
                (unsigned_char_p_t) "perf test", &st);
        if (st != rpc_s_ok)
        {
            fprintf(stderr, "*** Can't register with endpoint map - %s\n",
                    error_text(st));
            exit(1);
        }
    }

    if (b_reg)
    {
        perf_register_b(NULL, false, &st);
        if (st != rpc_s_ok)
        {
            fprintf(stderr, "*** Can't register B interface with endpoint map - %s\n",
                    error_text(st));
            exit(1);
        }
    }

    i = 0;

    TRY
    {
        while (true)
        {
            VRprintf(1, ("+ Listening...\n"));
    
            rpc_server_listen(max_calls, &st);
            if (st != rpc_s_ok)
            {
                fprintf(stderr, "*** Listen returns - %s\n", error_text(st));
                exit(1);
            }
    
            if (++i >= server_loops)
            {
                break;
            }
    
            /*
             * Unreserve threads on the last server_loop iteration.
             */
            if (use_reserved_threads && i == (server_loops - 1))
            {
                VRprintf (1, ("+ Returned from listen; unreserving threads\n"));
                teardown_thread_pools(true /* block */);
            }
    
            VRprintf (2, ("+ Returned from listen; sleeping for %d secs\n", 
                        SERVER_LOOP_SLEEP_TIME));
            SLEEP (SERVER_LOOP_SLEEP_TIME);
        }
    }
    FINALLY
    {
        if (ep_reg)
        {
            rpc_ep_unregister(perf_v2_0_s_ifspec, bv, (uuid_vector_p_t) &object_vec,
                    &st);
            if (st != rpc_s_ok)
            {
                fprintf(stderr, "*** Can't unregister from endpoint map - %s\n",
                        error_text(st));
            }
        }
    }
    ENDTRY

    VRprintf(1, ("Exiting\n"));
	 return 0;
}
