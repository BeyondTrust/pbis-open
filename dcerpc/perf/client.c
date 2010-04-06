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
 */
/*
 */
/*
**  NAME
**
**      client.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  Client application code for performance and system exerciser.
**
**
*/

#include <perf_c.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#ifndef NO_TIMES
#include <sys/times.h>
#include <unistd.h>
#endif  /* NO_TIMES */
#include <stdlib.h>
#include <sys/wait.h>


typedef struct
{
    unsigned short              ifspec_vers;
    unsigned short              opcnt;
    unsigned long               vers;
    dce_uuid_t                      id;
    unsigned short              stub_rtl_if_vers;
} rpc_if_rep_t, *rpc_if_rep_p_t;

static void looping_test();
static void brd_test();
static void unreg_test();
static void forwarding_test();
static void exception_test();
static void shutdown_test();
static void callback_test();
static void generic_test();
static void context_test();
static void static_cancel_test();
static void stats_test();
static void inq_if_ids_test();
static void one_shot_test();

#define SHUTMODE_MGR    1
#define SHUTMODE_NO_MGR 2
#define SHUTMODE_MGMT   3

struct tinfo_t
{
    void (*proc)();
    char *name;
    char *usage;
} tinfo[] =
    {
        /*  0 */     {looping_test,    "Null call",
                "host passes calls/pass verify? idempotent?"},
        /*  1 */     {looping_test,    "Variable length input arg",
                "host passes calls/pass verify? idempotent? nbytes"},
        /*  2 */     {looping_test,    "Variable length output arg",
                "host passes calls/pass verify? idempotent? nbytes"},
        /*  3 */     {brd_test,        "Broadcast",
                "family"},
        /*  4 */     {looping_test,    "Maybe",
                "host passes calls/pass"},
        /*  5 */     {brd_test,        "Broadcast/maybe",
                "family"},
        /*  6 */     {looping_test,    "Floating point",
                "host passes calls/pass verify? idempotent?"},
        /*  7 */     {unreg_test,      "Unregistered interface",
                "host"},
        /*  8 */     {forwarding_test, "Forwarding",
                "host global?"},
        /*  9 */     {exception_test,  "Exception",
                "host"},
        /* 10 */     {looping_test,    "Slow call",
                "host passes calls/pass verify? idempotent? seconds [mode]"},
        /* 11 */     {shutdown_test,   "Shutdown",
                "host[+ep] [mode [secs]] (1=manager, 2=no manager, 3=management)"},
        /* 12 */     {callback_test,   "Callback (UNSUPPORTED!)",
                "host passes callbacks/pass idempotent?"},
        /* 13 */     {generic_test,    "Generic interface",
                "host"},
        /* 14 */     {context_test,    "Context test",
                "host passes die? seconds"},
        /* 15 */     {static_cancel_test,    "Static Cancel",
                "host passes idempotent? [seconds [cancel-timeout-seconds]]"},
        /* 16 */     {stats_test,      "Statistics",
                "[host+ep]"},
        /* 17 */     {inq_if_ids_test, "Interface identifiers",
                "[host+ep]"},
        /* 18 */     {one_shot_test, "One shot",
                "host[+ep] forward? idempotent?"},
    };

#define N_TESTS (int)(sizeof tinfo / sizeof (struct tinfo_t))


#ifdef _POSIX_THREADS

#define MAX_TASKS 40

idl_boolean multithread = false;
idl_boolean use_shared_handle = false;
int n_tasks;
pthread_mutex_t global_mutex;

#endif

idl_boolean authenticate;
unsigned32  authn_level;
idl_char    *auth_principal;
unsigned32  authn_protocol;
unsigned32  authz_protocol;

idl_boolean debug = false;
idl_boolean stats = false;
idl_boolean use_obj = false;
idl_boolean compat_mode = false;
signed32 timeout = -1;
signed32 cancel_timeout = -1;

int recreate_binding_freq = 0;  /* "infinity" */
int reset_binding_freq = 0;     /* "infinity" */

int wait_point = -1;
int wait_time;

unsigned32 socket_buf_size = 0;	/* os default */

int verbose = 1;

#ifdef NO_TIMES
struct msec_time
{
    unsigned long msec;
    unsigned short usec;
};
#else
struct msec_time
{
    struct tms ptime;
    struct timeval elapsed;
    unsigned long r_msec;
    unsigned long r_usec;
    unsigned long u_msec;
    unsigned long s_msec;
};

static long clock_ticks;
#endif  /* NO_TIMES */

#ifndef NO_GETTIMEOFDAY

#define GETTIMEOFDAY(t) \
{ \
    struct timezone tz; \
 \
    gettimeofday(t, &tz); \
}

#else

struct timeval
{
    unsigned long tv_sec;
    unsigned long tv_usec;
};

#define GETTIMEOFDAY(t) \
{ \
    (t)->tv_sec  = time(NULL); \
    (t)->tv_usec = 0; \
}

#endif











/*
 * Print how to use this program
 */
void usage (test)

int             test;

{
    int             i;


    if (test != -1)
    {
        fprintf (stderr, "usage: client <flags> %d %s\n",
            test, tinfo[test].usage);
    }
    else
    {
        fprintf (stderr, "usage: client [-Dis] [-d <debug switches>] [(-m|M) <nthreads>]\n");
        fprintf (stderr, "              [-t <timeout>] [-c <timeout>] [-w <wait point>,<wait secs>]\n");
        fprintf (stderr, "              [-p <authn proto>,<authz proto>[,<level>[,<principal>]]]\n");
        fprintf (stderr, "              [-r <frequency>] [-R <frequency>] [-v <verbose level>]\n");
        fprintf (stderr, "              [-f <opt>] [-B <bufsize>]\n");
        fprintf (stderr, "              test# ...\n");
        fprintf (stderr, "  -d: Turns on NCK runtime debug output\n");
        fprintf (stderr, "  -D: Turns on default NCK runtime debug output\n");
        fprintf (stderr, "  -t: Set communications timeout (0-10)\n");
        fprintf (stderr, "  -c: Set cancel timeout (seconds)\n");
        fprintf (stderr, "  -B: Set CN TCP socket buffer (bytes)\n");
        fprintf (stderr, "  -f: Repeat test after fork, <opt> can be:\n");
        fprintf (stderr, "      1: Repeat test in the original and child processes\n");
        fprintf (stderr, "      2: Repeat test in the original process only\n");
        fprintf (stderr, "      3: Repeat test in the child process only\n");
        fprintf (stderr, "      4: Repeat test in the child and grandchild processes\n");
        fprintf (stderr, "      5: Repeat test in the grandchild process only\n");
        fprintf (stderr, "      6: Run test in the child process only\n");
        fprintf (stderr, "  -s: Prints statistics at end of test\n");
        fprintf (stderr, "  -w: Causes client to wait at specified point for specified time\n");
        fprintf (stderr, "  -o: Use perf object in bindings (default is no object)\n");
        fprintf (stderr, "  -1: Avoid doing certain things that NCS 1.5 can't do\n");
        fprintf (stderr, "  -r: Reset bindings every <frequency> calls in a single pass\n");
        fprintf (stderr, "  -R: Recreate bindings every <frequency> calls in a single pass\n");
#ifdef _POSIX_THREADS
        fprintf (stderr,
            "  -m: Causes nthreads tasks to run at same time (tasking systems only)\n");
        fprintf (stderr,
            "  -M: Same as -m but use a shared binding handle (tasking systems only)\n");
#endif
        fprintf (stderr,
            "  -i: Causes statistics to be dumped at end of run\n");
        fprintf (stderr,
            "  -p: Authenticate using <authn proto>/<authz proto> at <level> to <principal>\n");
        fprintf (stderr,
            "      <level> and <principal> may be omitted\n");
#ifndef NO_TIMES
        fprintf (stderr, "CLK_TCK: %ld ticks/sec\n", clock_ticks);
#endif  /* NO_TIMES */
        fprintf (stderr, "\n");

        for (i = 0; i < N_TESTS; i++)
        {
            fprintf(stderr, "  test #%d: %s test\n    usage: client %d %s\n",
                    i, tinfo[i].name, i, tinfo[i].usage);
        }
    }

    exit(1);
}

/*
 * If the specified point is a wait point, then sleep for the specified amount
 * of time.
 */

static void check_wait_point(point)

int point;

{
    if (wait_point == point)
    {
        printf("...wait point %d: sleeping for %d seconds\n", point, wait_time);
        SLEEP(wait_time);
    }
}

#ifdef NO_TIMES
/*
 * Take a starting time and an iteration count, and produce an average
 * time per iteration based on the current time.
 */

static void end_timing(start_time, iterations, avg_time)

struct timeval *start_time;
unsigned long iterations;
struct msec_time *avg_time;

{
    struct timeval elapsed_time;
    unsigned long elapsed_usec;

    GETTIMEOFDAY(&elapsed_time);

    if (elapsed_time.tv_usec < start_time->tv_usec)
    {
        elapsed_time.tv_sec--;
        elapsed_time.tv_usec += 1000000;
    }

    elapsed_time.tv_usec -= start_time->tv_usec;
    elapsed_time.tv_sec  -= start_time->tv_sec;

    elapsed_usec = (elapsed_time.tv_sec * 1000000) + elapsed_time.tv_usec;

    avg_time->msec = (elapsed_usec / iterations) / 1000;
    avg_time->usec = (elapsed_usec / iterations) % 1000;
}
#else
static void start_timing(start_time)
struct msec_time *start_time;
{
    GETTIMEOFDAY(&start_time->elapsed);

    if (times(&start_time->ptime) == -1)
        memset(&start_time->ptime, 0, sizeof(start_time->ptime));
}

/*
 * Take a starting time and an iteration count, and produce an average
 * time per iteration based on the current time.
 */

static void end_timing(start_time, iterations, avg_time)

struct msec_time *start_time;
unsigned long iterations;
struct msec_time *avg_time;

{
    struct tms ptime;
    unsigned long elapsed_usec;

    GETTIMEOFDAY(&avg_time->elapsed);

    if (times(&ptime) != -1)
    {
        avg_time->ptime.tms_utime = ptime.tms_utime
            - start_time->ptime.tms_utime;
        avg_time->ptime.tms_stime = ptime.tms_stime
            - start_time->ptime.tms_stime;

        avg_time->u_msec = (avg_time->ptime.tms_utime / iterations)
            * 1000 / clock_ticks;
        avg_time->s_msec = (avg_time->ptime.tms_stime / iterations)
            * 1000 / clock_ticks;
    }
    else
    {
        avg_time->u_msec = 0;
        avg_time->s_msec = 0;
    }

    if (avg_time->elapsed.tv_usec < start_time->elapsed.tv_usec)
    {
        avg_time->elapsed.tv_sec--;
        avg_time->elapsed.tv_usec += 1000000;
    }

    avg_time->elapsed.tv_usec -= start_time->elapsed.tv_usec;
    avg_time->elapsed.tv_sec  -= start_time->elapsed.tv_sec;

    elapsed_usec = (avg_time->elapsed.tv_sec * 1000000)
        + avg_time->elapsed.tv_usec;

    avg_time->r_msec = (elapsed_usec / iterations) / 1000;
    avg_time->r_usec = (elapsed_usec / iterations) % 1000;
}
#endif  /* NO_TIMES */

/*
 * Get an RPC handle for a name
 */

static handle_t binding_from_string_binding (object, name)

dce_uuid_t              *object;
char                *name;

{
    unsigned32          st;
    handle_t            h;


    rpc_binding_from_string_binding
        ((idl_char *) name, (rpc_binding_handle_t *) &h, &st);

    if (st != rpc_s_ok)
    {
        fprintf (stderr, "*** Can't convert name \"%s\" to binding - %s\n",
            name, error_text (st));
        exit(1);
    }

    rpc_binding_set_object (h,
                            object != NULL ?
                                object :
                                use_obj ? &NilTypeObj : NULL,
                            &st);

    if (st != rpc_s_ok)
    {
        fprintf (stderr, "*** Can't set binding's object - %s\n",
            error_text (st));
        exit(1);
    }

    if (timeout != -1)
    {
        rpc_mgmt_set_com_timeout (h, (unsigned32) timeout, &st);

        if (st != rpc_s_ok)
        {
            fprintf (stderr, "*** Can't set timeout for binding - %s\n",
                 error_text (st));
            exit(1);
        }
    }

    if (authenticate)
    {
        unsigned32 authn_level_local, authn_protocol_local, authz_protocol_local;
        unsigned_char_p_t auth_principal_local;

        /*
         * If no server principal name was supplied, we must go get one from
         * the server.
         */
        if (auth_principal == NULL)
        {
            rpc_mgmt_inq_server_princ_name (h, authn_protocol, &auth_principal, &st);
            if (st != rpc_s_ok)
            {
                fprintf (stderr, "*** Can't get server principal name - %s\n",
                     error_text (st));
                exit(1);
            }
            VRprintf(2, ("  Server principal name found to be: \"%s\"\n",
                    auth_principal));
        }

        /*
         * Leave a backdoor for testing the unsupported "null server
         * principal name" feature of rpc_binding_set_auth_info().
         */
        if (strcmp ((char *) auth_principal, "NULL") == 0)
        {
            auth_principal = NULL;
        }

        rpc_binding_set_auth_info
            (h, auth_principal, authn_level, authn_protocol, NULL, authz_protocol, &st);
        if (st != rpc_s_ok)
        {
            fprintf (stderr, "*** Can't set authentication for binding - %s\n",
                 error_text (st));
            exit(1);
        }
        rpc_binding_inq_auth_info
            (h, &auth_principal_local, &authn_level_local, &authn_protocol_local, NULL,
                &authz_protocol_local, &st);
        if (st != rpc_s_ok)
        {
            fprintf (stderr, "*** Can't get authentication back from binding - %s\n",
                error_text (st));
            exit(1);
        }
        VRprintf(2, ("  Authentication info from binding:\n    authn protocol: %s\n    authz protocol: %s\n    level: %s\n    server princ: \"%s\"\n  -----------------\n",
                authn_names[authn_protocol_local],
                authz_names[authz_protocol_local],
                authn_level_names[authn_level_local],
                auth_principal_local));
        if (auth_principal_local != NULL)
        {
            rpc_string_free (&auth_principal_local, &st);
        }
    }

    return h;
}

/*
 * Convert a binding to a string binding.
 */

static void binding_to_string_binding (rh, name)

handle_t            rh;
unsigned_char_p_t   *name;

{
    unsigned32      st;

    rpc_binding_to_string_binding (rh, name, &st);
    if (st != rpc_s_ok)
    {
        fprintf (stderr, "*** Can't convert binding to string binding - %s\n",
            error_text (st));
        exit(1);
    }
}

/*
 * Reset a binding.
 */

static void binding_reset (rh)

handle_t    rh;

{
    unsigned32      st;

    rpc_binding_reset((rpc_binding_handle_t) rh, &st);
    if (st != rpc_s_ok)
    {
        fprintf(stderr, "*** Can't reset binding - %s\n", error_text(st));
        exit(1);
    }
}



/*
 * Broadcast test
 */

static void brd_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];

{
    handle_t            rh;
    unsigned32          st;
    unsigned_char_p_t   name, name2;
    idl_char            result[256];

    if (argc < 3)
    {
        usage (test);
    }

    rpc_string_binding_compose (NULL, (unsigned_char_p_t) argv[2],
            NULL, NULL, NULL, &name, &st);
    if (st != rpc_s_ok)
    {
        fprintf(stderr, "*** Can't form string binding from \"%s\"\n", argv[2]);
        exit(1);
    }

    rh = binding_from_string_binding (NULL, name);

    /*
     * Broadcast/maybe test; just make the call and return.
     */

    if (test == 5)
    {
        perf_brd_maybe(rh);
        return;
    }

    /*
     * Normal broadcast.
     */

    printf ("  Broadcast call that should succeed...\n");

    perf_brd(rh, result);
    binding_to_string_binding (rh, &name2);
    printf("    bound to \"%s\", result=\"%s\"\n", name2, result);

    rpc_string_free (&name2, &st);
    rpc_binding_free (&rh, &st);

    /*
     * Call a broadcast routine that raises an exception; should see comm failure,
     * NOT exception.
     */

    printf ("  Broadcast call that should get communications failure (exception case)...\n");

    rh = binding_from_string_binding (NULL, name);

    TRY
    {
        perf_brd_fault(rh);
        binding_to_string_binding (rh, &name2);
        fprintf (stderr, "*** Call succeeded (shouldn't have); bound to \"%s\", result=\"%s\"\n", name2, result);
        rpc_string_free (&name2, &st);
        exit(1);
    }
    CATCH (rpc_x_comm_failure)
    {
        printf ("    call failed as expected\n");
    }
    CATCH_ALL
    {
        exc_report(THIS_CATCH);
        printf ("    call failed, but with something other than \"communications failure\"\n");
    }
    ENDTRY

    rpc_binding_free (&rh, &st);

    /*
     * Call a broadcast routine that should not be registered; should see comm
     * failure, NOT exception.
     */

    printf ("  Broadcast call that should get communications failure (unknown i/f case)...\n");

    rh = binding_from_string_binding (NULL, name);

    TRY
    {
        perfb_brd(rh, result);
        binding_to_string_binding (rh, &name2);
        fprintf (stderr, "*** Call succeeded (shouldn't have); bound to \"%s\", result=\"%s\"\n", name2, result);
        exit(1);
    }
    CATCH (rpc_x_comm_failure)
    {
        printf ("    call failed as expected\n");
    }
    CATCH_ALL
    {
        printf ("    call failed, but with something other than \"communications failure\"\n");
    }
    ENDTRY
}

/*
 * Shutdown test
 */

static void shutdown_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];

{
    handle_t            rh;
    int                 mode;
    unsigned32          secs;
    unsigned32          st;

    if (argc < 3)
    {
        usage(test);
    }

    rh = binding_from_string_binding(NULL, argv[2]);

    mode = (argc < 4) ? SHUTMODE_MGR : atoi(argv[3]);

    switch (mode)
    {
        case SHUTMODE_MGR:
            printf("  Shutdown via manager thread calling shutdown\n");
            perf_shutdown(rh);
            break;

        case SHUTMODE_NO_MGR:
            printf("  Shutdown via non-manager calling shutdown\n");
            secs = (argc < 5) ? 0 : atoi(argv[4]);
            perf_shutdown2(rh, secs);
            break;

        case SHUTMODE_MGMT:
            printf("  Shutdown via local call to management function\n");
            rpc_mgmt_stop_server_listening(rh, &st);
            if (st != rpc_s_ok)
            {
                fprintf(stderr, "*** Error from management function - %s\n",
                    error_text (st));
            }
            break;
    }

}

/*
 * Unregistered interface test.
 */

static void unreg_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];

{
    char                name[256];
    handle_t            rh;
    unsigned32          st;
    unsigned_char_t     *endpoint;

    rpc_if_rep_p_t      if_rep = (rpc_if_rep_p_t)perfb_v1_0_c_ifspec;
    dce_uuid_t              if_uuid;

    if (argc < 3)
    {
        usage(test);
    }

    memcpy(&if_uuid, &(if_rep->id), sizeof(dce_uuid_t));

    rh = binding_from_string_binding(NULL, argv[2]);

    rpc_string_binding_parse((unsigned_char_t *)argv[2], NULL, NULL, NULL,
                             &endpoint, NULL, &st);
    if (st != rpc_s_ok)
    {
        fprintf(stderr, "*** Can't parse string binding from \"%s\"\n", argv[2]);
        exit(1);
    }

    if (endpoint == NULL)
    {
        /*
         * Do a RPC to a supported interface so that we can get a complete
         * binding to the server.
         */
        perf_init(rh);
    }

    dce_uuid_create(&(if_rep->id), &st);
    if (st != rpc_s_ok)
    {
        fprintf(stderr, "*** Can't create uuid\n");
        exit(1);
    }

    /*
     * Now try a RPC to an interface that the server doesn't
     * have registered.
     */
    TRY
    {
        perfb_init(rh, (idl_char *) name);
        fprintf(stderr, "*** Call that should have failed succeeded!\n");
    }
    CATCH (rpc_x_unknown_if)
    {
        printf("  \"Unknown interface\" exception correctly raised\n");
    }
    ENDTRY

#ifdef _POSIX_THREADS
    if (! multithread)
#endif
    memcpy(&(if_rep->id), &if_uuid, sizeof(dce_uuid_t));
}

/*
 * Inquire Interface ids test
 */

static void do_inq_if_ids (name)
char *name;
{
    handle_t                rh;
    rpc_if_id_vector_p_t    if_ids;
    unsigned32              st;
    char                    *pstring;
    unsigned_char_p_t       uuid_string;
    unsigned32              i;
    unsigned32              temp_status;

    if (name == NULL)
    {
        pstring = "local process";
        rh = NULL;
    }
    else
    {
        pstring = name;
        rh = binding_from_string_binding(NULL, name);
    }

    rpc_mgmt_inq_if_ids (rh, &if_ids, &st);
    if (st != rpc_s_ok)
    {
        fprintf(stderr, "*** Can't inquire interface ids for %s - %s\n",
                pstring, error_text(st));
        exit(1);
    }

    printf ("%ld interface ids returned for %s.\n", if_ids->count, pstring);
    for (i = 0; i < if_ids->count; i++)
    {
        dce_uuid_to_string (&if_ids->if_id[i]->uuid, &uuid_string, &st);
        printf ("%ld:\tuuid:\t%s\n\tvers_major:\t%d\tvers_minor\t%d\n",
            (i+1), uuid_string, if_ids->if_id[i]->vers_major,
            if_ids->if_id[i]->vers_minor );
        rpc_string_free (&uuid_string, &temp_status);
    }

    rpc_if_id_vector_free (&if_ids, &st);
    if (st != rpc_s_ok)
    {
        fprintf(stderr, "*** Can't free interface ids - %s\n", error_text(st));
        exit(1);
    }
}

static void inq_if_ids_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];

{
    if (argc < 2)
    {
        usage(test);
    }

    do_inq_if_ids (argc == 2 ? NULL : argv[2]);
}


/*
 * Statistics test
 */

static void do_stats (name)
char *name;
{
    handle_t                rh;
    rpc_stats_vector_p_t    stats;
    unsigned32              st;
    unsigned char           *pstring;

    if (name == NULL)
    {
        pstring = (unsigned char *)"local process";
        rh = NULL;
    }
    else
    {
        pstring = (unsigned char *)name;
        rh = binding_from_string_binding(NULL, name);
    }

    rpc_mgmt_inq_stats (rh, &stats, &st);
    if (st != rpc_s_ok)
    {
        fprintf(stderr, "*** Can't inquire statistics for %s - %s\n",
                pstring, error_text(st));
        exit(1);
    }

    if (name != NULL)
    {
        binding_to_string_binding(rh, &pstring, &st);
    }

    printf("  Stats for %s\n", pstring);

    if (name != NULL)
        rpc_string_free(&pstring, &st);

    printf("    Calls sent:   %9lu\n", stats->stats[rpc_c_stats_calls_out]);
    printf("    Calls rcvd:   %9lu\n", stats->stats[rpc_c_stats_calls_in]);
    printf("    Packets sent: %9lu\n", stats->stats[rpc_c_stats_pkts_out]);
    printf("    Packets rcvd: %9lu\n", stats->stats[rpc_c_stats_pkts_in]);

    rpc_mgmt_stats_vector_free (&stats, &st);
    if (st != rpc_s_ok)
    {
        fprintf(stderr, "*** Can't free statistics - %s\n", error_text(st));
        exit(1);
    }
}


static void stats_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];

{
    if (argc < 2)
    {
        usage(test);
    }

    do_stats (argc == 2 ? NULL : argv[2]);
}


/*
 * Auxiliary routine for forwarding test.
 */

static void forwarding_mgmt_test (rh, obj, msg)

handle_t            rh;
uuid_p_t            obj;
char                *msg;

{
    unsigned int                 i;
    rpc_if_id_vector_p_t if_ids;
    rpc_if_id_t         perfb_if_id;
    unsigned32          st;

    /*
     * Clear binding again and make a management call to test that
     * management calls on partial bindings work.  We do a
     * mgmt_inq_if_ids and make sure that "perfb" is one of the i/f's
     * we get back.
     */
    binding_reset(rh);

    rpc_binding_set_object(rh, obj, &st);
    if (st != 0)
    {
        fprintf(stderr, "*** Can't set object in binding (%s) - %s\n",
            msg, error_text(st));
        exit(1);
    }

    rpc_if_inq_id(perfb_v1_0_c_ifspec, &perfb_if_id, &st);
    if (st != rpc_s_ok)
    {
        fprintf(stderr, "*** Can't get interface id for perfb interface (%s) - %s\n",
                msg, error_text(st));
        exit(1);
    }

    rpc_mgmt_inq_if_ids (rh, &if_ids, &st);
    if (st != 0)
    {
        fprintf(stderr, "*** Can't get interface ids through partial binding (%s) - %s\n",
            msg, error_text(st));
        exit(1);
    }

    for (i = 0; i < if_ids->count; i++)
    {
        if (dce_uuid_equal(&if_ids->if_id[i]->uuid, &perfb_if_id.uuid, &st))
            break;
    }

    if (i == if_ids->count)
    {
        fprintf(stderr, "*** Request for interface ids through partial binding completed OK,\n");
        fprintf(stderr, "***    but perfb i/f not found; probably forwarded to wrong server (%s).\n",
            msg);
        exit(1);
    }

    printf("  request for interface ids through partial binding completed OK (%s)\n",
                msg);

    rpc_if_id_vector_free (&if_ids, &st);
    if (st != rpc_s_ok)
    {
        fprintf(stderr, "*** Can't free interface ids (%s) - %s\n",
            msg, error_text(st));
        exit(1);
    }
}

/*
 * Forwarding test.
 */

static void forwarding_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];

{
    handle_t            rh;
    char                name[256];
    unsigned_char_p_t   name2;
    unsigned32          st;
    idl_boolean         global;
#define FSIZE 4000
    unsigned32       d[FSIZE];
    int                 i;
    unsigned32          sum, rsum;
    idl_char            result[256];
    unsigned_char_p_t   pstring;

	DO_NOT_CLOBBER(rh);
	DO_NOT_CLOBBER(rsum);
	 
    if (argc < 4)
    {
        usage(test);
    }

    global = argv[3][0] == 'y';
    rh = binding_from_string_binding(NULL, argv[2]);

    /*
     * Tell server to register the "perfb" interface.
     */
    perf_register_b(rh, global, &st);

    if (st != 0)
    {
        fprintf(stderr, "*** Can't register \"perfb\" interface - %s\n",
            error_text(st));
        exit(1);
    }

    TRY
    {
        if (! compat_mode)
        {
            /*
             * Clear the binding to the server and call resolve_binding.
             */
            binding_reset(rh);

            rpc_ep_resolve_binding(rh, perfb_v1_0_c_ifspec, &st);
            if (st != rpc_s_ok)
            {
                fprintf(stderr, "*** Can't resolve \"perfb\" interface - %s\n",
                        error_text(st));
                exit(1);
            }

            binding_to_string_binding(rh, &pstring, &st);
            printf("  binding resolved to: %s\n", pstring);
            rpc_string_free(&pstring, &st);
        }

        /*
         * Clear binding to server and make a simple call to the "perfb" interface.
         */
        binding_reset(rh);

        perfb_init(rh, (idl_char *) name);
        printf("  result of forwarded call = \"%s\"\n", name);

        /*
         * Clear binding again and make a call that's too big to fit in a
         * single pkt to make sure multi-pkt forwarding works.  Note that old
         * perf servers don't handle this call so catch that sort of error.
         */
        binding_reset(rh);

        for (i = 0, rsum = 0; i < FSIZE; i++)
        {
            d[i] = i;
            rsum += d[i];
        }

        TRY
        {
            perfb_in(rh, d, FSIZE, true, &sum);
            if (sum != rsum)
            {
                fprintf(stderr, "*** Sum mismatch in large forwarded call (%lu, %lu)\n",
                    sum, rsum);
            }
            else
            {
                printf("  large forwarded call completed OK\n");
            }
        }
        CATCH (rpc_x_op_rng_error)
        {
            printf ("  warning: server doesn't implement \"perfb_in\" procedure ");
            printf ("(probably an old server)\n");
            printf ("           large forwarded call test skipped\n");
        }
        ENDTRY

        if (! compat_mode)
        {
            /*
             * Test forwarded mgmt calls.
             */
            forwarding_mgmt_test (rh, &NilTypeObj, "nil type object");
            forwarding_mgmt_test (rh, &FooObj1, "non-nil type object");
        }

        /*
         * Clear binding again and make a broadcast call.
         */
        binding_reset(rh);

        rpc_binding_set_object(rh, NULL, &st);
        if (st != 0)
        {
            fprintf(stderr, "*** Can't reset object in binding to nil - %s\n",
                error_text(st));
            exit(1);
        }

        TRY
        {
            perfb_brd(rh, result);
            binding_to_string_binding (rh, &name2);
            printf("  broadcast forwarded call completed OK\n");
            printf("    bound to \"%s\", result=\"%s\"\n", name2, result);
        }
        CATCH (rpc_x_invalid_call_opt)
        {
            printf ("  warning: can't do broadcast\n");
        }
        CATCH (rpc_x_op_rng_error)
        {
            printf ("  warning: server doesn't implement \"perfb_brd\" procedure ");
            printf ("(probably an old server)\n");
            printf ("           broadcast call test skipped\n");
        }
        CATCH (rpc_x_comm_failure)
        {
            printf ("  warning: communications failure (may be an old server)\n");
            printf ("           broadcast call test skipped\n");
        }
        CATCH_ALL
        {
            exc_report(THIS_CATCH);
            fprintf (stderr, "*** Unknown exception raised in during broadcast forwarded call\n");
            RERAISE;
        }
        ENDTRY
    }
    FINALLY
    {
        /*
         * Tell server to unregister the "perfb" interface.
         */
        rh = binding_from_string_binding(NULL, argv[2]);

        perf_unregister_b(rh, &st);
        if (st != 0)
        {
            fprintf(stderr, "*** Can't unregister \"perfb\" interface - %s\n",
                error_text(st));
            exit(1);
        }
    }
    ENDTRY
}

/*
 * Exception test.
 */

static void exception_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];

{
    handle_t            rh;

    if (argc < 3)
    {
        usage(test);
    }

    rh = binding_from_string_binding(NULL, argv[2]);

    TRY
    {
        perf_exception(rh);
        fprintf(stderr, "*** NO exception raised\n");
        exit (1);
    }
#ifndef PD_BUILD
    CATCH (exc_intdiv_e)
#else
    CATCH (exc_e_intdiv)
#endif
    {
        printf("  Integer div-by-zero exception correctly raised\n");
    }
#ifndef PD_BUILD
    CATCH (exc_fltdiv_e)
#else
    CATCH (exc_e_fltdiv)
#endif
    {
        printf("  Floating div-by-zero exception correctly raised\n");
    }
    CATCH (exc_e_aritherr)
    {
        printf("  Arithmetic error exception correctly raised\n");
    }
    CATCH_ALL
    {
        exc_report(THIS_CATCH);
        fprintf(stderr, "*** WRONG exception raised\n");
        exit(1);
    }
    ENDTRY
}

/*
 * Callback routines for callback test.
 */

static unsigned long callback_passes, callback_count;

void perfc_init (h, p)

handle_t            h __attribute__((unused));
unsigned32       *p;

{
    callback_count = 0;
    *p = callback_passes;
}

void perfc_cb (h, c)

handle_t            h __attribute__((unused));
unsigned32       *c;

{
    *c = ++callback_count;
    printf("    ...in callback %lu\n", *c);
}

void perfc_cb_idem (h, c)

handle_t            h __attribute__((unused));
unsigned32       *c;

{
    *c = ++callback_count;
    printf("    ...in idempotent callback %lu\n", *c);
}

perfc_v2_0_epv_t perfc_v2_mgr_epv =
{
    perfc_init,
    perfc_cb,
    perfc_cb_idem
};


/*
 * Callback test
 */
static void callback_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];

{
    handle_t            rh;
    unsigned32          st;
    unsigned long       idem;
    unsigned short      passes, i;


    if (argc < 6)
    {
        usage(test);
    }

    passes = atoi(argv[3]);
    callback_passes = atoi(argv[4]);
    idem = argv[5][0] == 'y';

    rh = binding_from_string_binding(NULL, argv[2]);

    rpc_server_register_if (perfc_v2_0_s_ifspec, NULL,
        (rpc_mgr_epv_t) &perfc_v2_mgr_epv, &st);
    if (st != rpc_s_ok)
    {
        fprintf(stderr, "*** Can't register interface - %s\n", error_text(st));
        exit(1);
    }

    for (i = 1; i <= passes; i++)
    {
        perf_call_callback(rh, idem);
        printf("  pass %2d...OK\n", i);
    }

    rpc_server_unregister_if (perfc_v2_0_s_ifspec, NULL, &st);
    if (st != rpc_s_ok)
    {
        fprintf(stderr, "*** Can't unregister interface - %s\n", error_text(st));
        exit(1);
    }
}

/*
 * Returns T if two double floats are "pretty close" to each other.
 */

static idl_boolean approx_eq (d1, d2)

double              d1, d2;

{
    double ratio = d1 / d2;
    return (ratio > .9999 && ratio < 1.00001);
}

/*
 * Generic interface test
 */

static void generic_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];

{
    handle_t            rh;
    unsigned32       x;


    if (argc < 3)
    {
        usage(test);
    }

    rh = binding_from_string_binding((uuid_p_t) &FooObj1, argv[2]);
    perfg_op1(rh, 17l, &x);

    if (x != 34)
    {
        fprintf(stderr, "*** op1 on Foo1 returned %lu instead of 34\n", x);
    }

    rh = binding_from_string_binding((uuid_p_t) &BarObj2, argv[2]);
    perfg_op2(rh, 3l, &x);

    if (x != 15)
    {
        fprintf(stderr, "*** op2 on Bar2 returned %lu instead of 15\n", x);
    }

    rh = binding_from_string_binding((uuid_p_t) &FooObj2, argv[2]);
    perfg_op1(rh, 111l, &x);

    if (x != 222)
    {
        fprintf(stderr, "*** op1 on Foo2 returned %lu instead of 222\n", x);
    }

    rh = binding_from_string_binding((uuid_p_t) &BarObj1, argv[2]);
    perfg_op2(rh, 13l, &x);

    if (x != 65)
    {
        fprintf(stderr, "*** op2 on Bar1 returned %lu instead of 65\n", x);
    }

    TRY {
        dce_uuid_t  random_uuid;
        unsigned32 st;

        dce_uuid_create(&random_uuid, &st);
        rh = binding_from_string_binding(&random_uuid, argv[2]);
        perfg_op1(rh, 17, &x);
        fprintf(stderr,
            "*** op1 on random uuid: \"Unsupported type\" exception expected\n");
        exit(1);
    }
    CATCH (rpc_x_unsupported_type)
    {
        printf("    op1 on random uuid: \"Unsupported type\" exception correctly raised\n");
    }
    CATCH (rpc_x_comm_failure)
    {
        fprintf(stderr, "*** op1 on random uuid: comm failure\n");
        exit(1);
    }
    CATCH_ALL
    {
        exc_report(THIS_CATCH);
        fprintf(stderr, "*** op1 on random uuid: incorrect exception\n");
        exit(1);
    }
    ENDTRY

    TRY {
        rh = binding_from_string_binding(&ZotObj, argv[2]);
        perfg_op1(rh, 17, &x);
        fprintf(stderr,
            "*** op1 on unsupported type: \"Unsupported type\" exception expected\n");
        exit(1);
    }
    CATCH (rpc_x_unsupported_type)
    {
        printf("    op1 on unsupported uuid: \"Unsupported type\" exception correctly raised\n");
    }
    CATCH (rpc_x_comm_failure)
    {
        fprintf(stderr, "*** op1 on random uuid: comm failure\n");
        exit(1);
    }
    CATCH_ALL
    {
        exc_report(THIS_CATCH);
        fprintf(stderr, "*** op1 on random uuid: incorrect exception\n");
        exit(1);
    }
    ENDTRY

    printf("  ...OK\n");
}

/*
 * Static Cancel test
 *
 * (static in the sense that a cancel is posted to the thread before
 * the RPC is performed and we perform limited tests).
 *
 * Note: since this test is using existing perf RPCs, there is the
 * possibility that things may appear to work on the client side
 * without the server side actually having performed correctly
 * (because we don't actually check that the server side did or
 * did not detect the cancel as expected_; you should be able to
 * follow the test printout and verify this visually.
 */


/*
 * Determine if a cancel is pending (and clear it).
 */

static boolean32 cancel_was_pending()
{
    volatile boolean32 pending = false;

    TRY
    {
        pthread_testcancel();
    }
    CATCH (pthread_cancel_e)
    {
        printf("pending is set to TRUE\n"); fflush(stdout);
        pending = true;
    }
    CATCH_ALL
    {
        exc_report(THIS_CATCH);
        fprintf(stderr, "*** Unknown exception raised\n");
    }
    ENDTRY

    return pending;
}

/*
 * static_cancel_test1
 *
 * Perform a RPC that doesn't call a cancellable operation
 * and verify that a cancel is pending upon completion.
 */
void static_cancel_test1(rh, idem, slow_secs)

rpc_binding_handle_t    rh;
unsigned32              idem;
unsigned long           slow_secs;

{

  boolean32 pending;

  printf("    Static Cancel Test 1 (server should NOT detect cancel):\n");

  /* make sure general cancellability is on */
  pthread_setcancel(CANCEL_ON);

    TRY
    {

      /*
       * post a cancel to this thread and make a server RPC call
       */

        pthread_cancel(pthread_self());
        if (idem)
        {
            perf_null_slow_idem(rh, 2 /* CPU LOOP */, slow_secs);
        }
        else
        {
            perf_null_slow(rh, 2 /* CPU LOOP */, slow_secs);
        }
        pending =  cancel_was_pending();

	if (pending)
            printf("        Correct RPC cancel pending operation\n");
        else
            fprintf(stderr, "    *** lost cancel (not pending)!\n");
    }
    CATCH (pthread_cancel_e)
    {
        fprintf(stderr, "    *** unexpected cancel exception raised!\n");
        if (cancel_was_pending())
            fprintf(stderr, "    *** and cancel still pending!\n");
    }
    CATCH (rpc_x_cancel_timeout)
    {
        fprintf(stderr, "    *** unexpected cancel timeout exception raised!\n");
        if (! cancel_was_pending())
            fprintf(stderr, "    *** and lost cancel (not pending)!\n");
	    RERAISE;
    }
    CATCH_ALL
    {
        exc_report(THIS_CATCH);
        fprintf(stderr, "    *** unexpected exception raised!\n");
        fprintf(stderr, "    *** cancel %s pending\n",
                cancel_was_pending() ? "still" : "not");
	RERAISE;
    }
    ENDTRY
}

/*
 * static_cancel_test2
 *
 * Perform a RPC that does call a cancellable operation
 * and verify that a cancel is detected.
 */
void static_cancel_test2(rh, idem, slow_secs)

rpc_binding_handle_t    rh;
unsigned32              idem;
unsigned long           slow_secs;

{
    int oc;
    boolean32 pending;

    printf("    Static Cancel Test 2 (server SHOULD detect cancel):\n");

    TRY
    {
        pthread_cancel(pthread_self());
        if (idem)
        {
            perf_null_slow_idem(rh, 0 /* SLEEP LOOP */, slow_secs);
        }
        else
        {
            perf_null_slow(rh, 0 /* SLEEP LOOP */, slow_secs);
        }
        fprintf(stderr, "    *** cancel exception NOT raised!\n");

	pending = cancel_was_pending();
	oc = pthread_setcancel(CANCEL_OFF);

        if (pending)
            printf("    ... but cancel not lost (was pending)\n");
        else
            fprintf(stderr, "    *** and lost cancel (not pending)!\n");
    }
    CATCH (pthread_cancel_e)
    {
        fprintf(stderr, "        Correct cancel exception operation\n");
        if (cancel_was_pending())
            fprintf(stderr, "    *** but ERROR: cancel still pending!\n");
    }
    CATCH (rpc_x_cancel_timeout)
    {
        fprintf(stderr, "    *** unexpected cancel timeout exception raised!\n");
        if (! cancel_was_pending())
            fprintf(stderr, "    *** and lost cancel (not pending)!\n");
	RERAISE;
    }
    CATCH_ALL
    {
        exc_report(THIS_CATCH);
        fprintf(stderr, "    *** unexpected exception raised!\n");
        fprintf(stderr, "    *** cancel %s pending\n",
                cancel_was_pending() ? "still" : "not");
	RERAISE;
    }
    ENDTRY
}

/*
 * static_cancel_test3
 *
 * Perform a RPC that doesn't call a cancellable operation followed
 * by one that does and verify that a cancel is detected.
 */
void static_cancel_test3(rh, idem, slow_secs)

rpc_binding_handle_t    rh;
unsigned32              idem;
unsigned long           slow_secs;

{
    int oc;
    boolean32 pending;
    volatile idl_boolean first_done = false;

    printf("    Static Cancel Test 3 (server SHOULD detect cancel on 2nd RPC):\n");

    TRY
    {
        pthread_cancel(pthread_self());

        if (idem)
        {
            perf_null_slow_idem(rh, 2 /* CPU LOOP */, slow_secs);
            first_done = true;
            perf_null_slow_idem(rh, 0 /* SLEEP LOOP */, slow_secs);
        }
        else
        {
            perf_null_slow(rh, 2 /* CPU LOOP */, slow_secs);
            first_done = true;
            perf_null_slow(rh, 0 /* SLEEP LOOP */, slow_secs);
        }

	pending = cancel_was_pending();
	oc = pthread_setcancel(CANCEL_OFF);


        fprintf(stderr, "    *** cancel exception NOT raised!\n");
        if (pending)
        {
            printf("    ... but cancel not lost (was pending)\n");
        }
        else
        {
            fprintf(stderr, "    *** and lost cancel (not pending)!\n");
        }
    }
    CATCH (pthread_cancel_e)
    {
        if (! first_done)
        {
            fprintf(stderr, "    *** unexpected cancel raised (1st call)!\n");
            fprintf(stderr, "    *** cancel %s pending\n",
                cancel_was_pending() ? "still" : "not");
        }
        else
        {
            fprintf(stderr, "        Correct cancel exception operation\n");
            if (cancel_was_pending())
                fprintf(stderr, "    *** but ERROR: cancel still pending!\n");
        }
    }
    CATCH (rpc_x_cancel_timeout)
    {
        fprintf(stderr, "    *** unexpected cancel timeout raised (%s call)!\n",
                                    first_done ? "2nd" : "1st");
        if (! cancel_was_pending())
            fprintf(stderr, "    *** and lost cancel (not pending)!\n");
	RERAISE;
    }
    CATCH_ALL
    {
        exc_report(THIS_CATCH);
        fprintf(stderr, "    *** unexpected exception raised (%s call)!\n",
            first_done ? "2nd" : "1st");
        fprintf(stderr, "    *** cancel %s pending\n",
                cancel_was_pending() ? "still" : "not");
    }
    ENDTRY
}

/*
 * Static Cancel test
 */

static void static_cancel_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];

{
    handle_t            rh;
    int                 idem;
    int                 has_ctmo = false;
    signed32            ctmo = 0;
    unsigned short      i, passes;
    unsigned long       slow_secs = 5;
    unsigned32          st;

    if (argc < 5)
    {
        usage(test);
    }

    rh = binding_from_string_binding(NULL, argv[2]);
    passes = atoi(argv[3]);
    idem = argv[4][0] == 'y';

    if (argc > 5)
        slow_secs = atoi(argv[5]);

    if (argc > 6)
    {
        has_ctmo = true;
        ctmo = atoi(argv[6]);
    }

    printf("    passes: %d; idem: %s; slow secs: %ld; cancel timeout secs: %s\n",
            passes, idem ? "yes" : "no", slow_secs,
            has_ctmo ? argv[6] : "<default>");

    if (has_ctmo)
    {
        rpc_mgmt_set_cancel_timeout(ctmo, &st);
        if (st != rpc_s_ok)
        {
            fprintf(stderr, "*** Error setting cancel timeout - '%s'\n",
                error_text(st));
            exit(1);
        }
    }

    for (i = 0; i < passes; i++)
    {
        static_cancel_test1(rh, idem, slow_secs);
        static_cancel_test2(rh, idem, slow_secs);
        static_cancel_test3(rh, idem, slow_secs);

        printf("    pass %2d...OK\n", i);
    }
}


/*
 * Context test
 */

static void context_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];

{
    handle_t            rh;
    unsigned32          data, rdata;
    int                 die;
    struct timespec     delay;
    idl_void_p_t        context;
    unsigned short      i, passes;

    if (argc < 6)
    {
        usage(test);
    }

    rh = binding_from_string_binding(NULL, argv[2]);
    passes = atoi(argv[3]);
    die = argv[4][0] == 'y';

    delay.tv_sec = atoi(argv[5]);
    delay.tv_nsec = 0;

    printf("    passes: %d; die: %s; sleep secs: %ld\n",
            passes, die ? "yes" : "no", delay.tv_sec);

    for (i = 0; i < passes; i++)
    {
        data = time (NULL);

        perf_get_context (rh, data, &context);

        if (! perf_test_context (context, &rdata))
        {
            fprintf (stderr, "*** perf_get_context returned false\n");
        }

        if (rdata != data)
        {
            fprintf (stderr, "*** data mismatch on data returned by perf_get_context; %lu != %lu\n",
                rdata, data);
        }

        if (delay.tv_sec > 0)
        {
            printf ("        Sleeping for %ld seconds...\n", delay.tv_sec);
            pthread_delay_np(&delay);
            printf ("        ...awake\n");
        }

        if (! die)
        {
            if (! perf_free_context (&context, &rdata))
            {
                fprintf (stderr, "*** perf_free_context returned false\n");
            }

            if (rdata != data)
            {
                fprintf (stderr, "*** data mismatch on data returned by perf_free_context; %lu != %lu\n",
                    rdata, data);
            }
        }

        printf("    pass %2d...OK\n", i);
    }
}



/*
 * Looping test
 */

static void looping_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];

{
#define MSIZE (1024 * 1024)
    unsigned32          i;
    unsigned short      passes;
    handle_t            rh;
    unsigned32          st;
#ifdef NO_TIMES
    struct timeval      start_time;
#else
    struct msec_time    start_time;
#endif  /* NO_TIMES */
    struct msec_time    avg_time;
    unsigned short      cpp;
    unsigned32          *d;
    unsigned32          len;
    unsigned32          sum=0, rsum=0;
    unsigned32          n_calls, n_brd, n_maybe, n_brd_maybe;
    idl_boolean         idem;
    idl_boolean         verify;
    unsigned short      pass, calln;
    unsigned32          slow_secs=0;
    perf_slow_mode_t    slow_mode=0;
    static char         *slow_mode_names[4] = {"sleep", "I/O", "CPU", "Fork sleep"};
#ifdef _POSIX_THREADS
    static handle_t     first_handle = NULL;
#endif


    d = (unsigned32 *)malloc(MSIZE);

    switch (test)
    {
    case 4:
        if (argc < 5)
            usage(test);
        break;
    default:
        if (argc < 7)
            usage(test);
        break;
    }

    passes = atoi(argv[3]);
    cpp    = atoi(argv[4]);
    if (test == 4)
    {
        verify = false;
        idem   = true;
    }
    else
    {
        verify = argv[5][0] == 'y';
        idem   = argv[6][0] == 'y';
    }
    len    = 0;

    /*
     * Sanity check calls/pass.
     */

    if (cpp <= 0)
    {
        fprintf (stderr, "\nERROR: calls/pass must be > 0.\n\n");
        exit (1);
    }


#ifdef _POSIX_THREADS
    if (multithread)
    {
        pthread_mutex_lock(&global_mutex);
        if (first_handle == NULL)
        {
            rh = first_handle = binding_from_string_binding(NULL, argv[2]);
        }
        else
        {
            if (use_shared_handle)
            {
                rpc_binding_handle_copy(first_handle, &rh, &st);
            }
            else
            {
                rpc_binding_copy(first_handle, &rh, &st);
            }

            if (st != rpc_s_ok)
            {
                fprintf(stderr, "*** Can't copy binding - %s\n", error_text(st));
                exit(1);
            }
        }
        pthread_mutex_unlock(&global_mutex);
    }
    else
#endif
    {
        rh = binding_from_string_binding(NULL, argv[2]);
    }

    printf("    passes: %d; calls/pass: %d; verification: %s; idempotent: %s",
        passes, cpp, verify ? "on" : "off", idem ? "yes" : "no");

    switch (test)
    {
        case 1:
        case 2:
            if (argc < 8)
            {
                printf("\n");
                usage(test);
            }
            len = atoi(argv[7]) / 4;
            printf("; bytes/call: %ld", len * 4);
            break;
        case 10:
            if (argc < 8)
            {
                printf("\n");
                usage(test);
            }
            slow_secs = atoi(argv[7]);
            if (argc < 9)
            {
                slow_mode = perf_slow_sleep;
            }
            else
            {
                slow_mode = atoi(argv[8]);
                if (slow_mode > 3)
                {
                    printf("\n");
                    usage(test);
                }
            }
            printf("; sleep secs: %ld; mode: %s",
                   slow_secs, slow_mode_names[slow_mode]);
            break;
    }

    printf("\n");

    for (pass = 1; pass <= passes; pass++)
    {
        if (verify && ! idem)
        {
            perf_init(rh);
        }

#ifdef NO_TIMES
        GETTIMEOFDAY(&start_time);
#else
        start_timing(&start_time);
#endif  /* NO_TIMES */

        for (calln = 1; calln <= cpp; calln++)
        {
            if (verify)
            {
                for (i = 0, rsum = 0; i < len; i++)
                {
                    d[i] = i * perf_magic * pass * calln;
                    rsum += d[i];
                }
            }

            /*
             * for non-32 bit machines
             */
            rsum &= (unsigned long) 0xffffffff;

            switch (test)
            {
                case 0:
                    if (idem)
                    {
                        perf_null_idem(rh);
                    }
                    else
                    {
                        perf_null(rh);
                    }
                    break;

                case 4:
                    perf_maybe(rh);
                    break;

                case 10:
                    if (idem)
                    {
                        perf_null_slow_idem(rh, slow_mode, slow_secs);
                    }
                    else
                    {
                        perf_null_slow(rh, slow_mode, slow_secs);
                    }
                    break;

                case 1:
                    if (idem)
                    {
                        perf_in_idem(rh, d, len, verify, &sum);
                    }
                    else
                    {
                        perf_in(rh, d, len, verify, &sum);
                    }
                    if (verify)
                    {
                        if (sum != rsum)
                        {
                            fprintf(stderr, "*** Sum mismatch (%lu, %lu)\n",
                                sum, rsum);
                        }
                    }
                    break;

                case 2:
                {
                    unsigned32 tlen = len;

                    if (idem)
                    {
                        perf_out_idem (rh, d, &tlen, len,
				       pass * calln, verify);
                    }
                    else
                    {
                        perf_out (rh, d, &tlen, len,
				  pass * calln, verify);
                    }
                    if (len != tlen)
                    {
                        fprintf(stderr,
"*** Output length (%lu) is not equal to input length (%lu)\n", tlen, len);
                        exit(1);
                    }

                    if (verify)
                    {
                        for (i = 0, sum = 0; i < len; i++)
                        {
                            sum += d[i];
                        }

                        /*
                         * for non-32 bit machines
                         */
                        sum &= (unsigned long) 0xffffffff;
                        if (sum != rsum)
                        {
                            fprintf(stderr, "*** Sum mismatch (%lu, %lu)\n",
                                sum, rsum);
                        }
                    }
                    break;
                }

                case 6:
                {
                    float f1 = (float) (rand()+1);
                    float f2 = (calln & 1 ? 1.0 : -1.0) * (rand()+1);
                    double d1 = 1.0 / (rand()+1);
                    double d2;
                    float o1, o1_l;
                    double o2, o2_l;

                    d2 = sqrt(pow((double) (7 + (rand()+1) % 10),
                        (double) ((rand()+1) % 40)));
                    o1_l = (f1 / f2) * (d1 / d2);
                    o2_l = (f2 / f1) * (d2 / d1);

                    MARSHALL_DOUBLE(d1);
                    MARSHALL_DOUBLE(d2);

                    perf_fp_test(rh, &f1, &f2, d1, d2, &o1, &o2);

                    UNMARSHALL_DOUBLE(o2);

                    if (verify)
                    {
                        if (! approx_eq(o1, o1_l))
                        {
                            fprintf(stderr,
                "*** Floating error #1 (%g != %g -- (%g / %g) * (%g / %g))\n",
                                o1, o1_l, f1, f2, d1, d2);
                        }

                        if (! approx_eq(o2, o2_l))
                        {
                            fprintf(stderr,
                "*** Floating error #2 (%g != %g -- (%g / %g) * (%g / %g))\n",
                                o2, o2_l, f2, f1, d2, d1);
                        }
                    }
                    break;
                }
            }
#ifdef _POSIX_THREADS
            if (! multithread)
#endif
            {
                if (reset_binding_freq > 0 && calln % reset_binding_freq == 0)
                {
                    binding_reset(rh);
                    printf("          [binding has been reset]\n");
                }
                if (recreate_binding_freq > 0 && calln % recreate_binding_freq == 0)
                {
                    rpc_binding_free(&rh, &st);
                    rh = binding_from_string_binding(NULL, argv[2]);
                    printf("          [binding has been recreated]\n");
                }
            }
        }

        end_timing(&start_time, cpp, &avg_time);

        if (verify && (! idem)
#ifdef _POSIX_THREADS
            && (! multithread)
#endif
            )
        {
            perf_info(rh, &n_calls, &n_brd, &n_maybe, &n_brd_maybe);
            if (n_calls != cpp)
            {
                fprintf(stderr, "*** Call count mismatch (%lu, %d)\n",
                    n_calls, cpp);
            }
        }

#ifdef NO_TIMES
        printf("        pass %3d; ms/call: %lu.%03u",
                pass, avg_time.msec, avg_time.usec);

        if (len > 0 && avg_time.msec > 0)
        {
            printf("; kbytes/sec: %3lu", (len * 4) / avg_time.msec);
        }
#else
        if (avg_time.u_msec == 0 || avg_time.s_msec == 0)
            printf("        pass %3d; ms/call: %lu.%03lu (ms/pass: %lu/%lu)",
                   pass, avg_time.r_msec, avg_time.r_usec,
                   avg_time.ptime.tms_utime*(1000/clock_ticks),
                   avg_time.ptime.tms_stime*(1000/clock_ticks));
        else
            printf("        pass %3d; ms/call: %lu.%03lu (%lu/%lu)",
                   pass, avg_time.r_msec, avg_time.r_usec,
                   avg_time.u_msec, avg_time.s_msec);

        if (len > 0 && avg_time.r_msec > 0)
        {
            printf("; kbytes/sec: %3lu", (len * 4) / avg_time.r_msec);
        }
#endif  /* NO_TIMES */

        printf("\n");

    }
}

/*
 * One shot test
 */

static void one_shot_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];

{
    handle_t            rh;
#ifdef NO_TIMES
    struct timeval      start_time;
#else
    struct msec_time    start_time;
#endif  /* NO_TIMES */
    struct msec_time    avg_time;
    idl_boolean         fwd;
    idl_boolean         idem;

    if (argc < 5)
    {
        usage(test);
    }

    fwd  = argv[3][0] == 'y';
    idem = argv[4][0] == 'y';

    rh = binding_from_string_binding(NULL, argv[2]);

    printf("    forward: %s; idempotent: %s\n",
            fwd ? "yes" : "no", idem ? "yes" : "no");

#ifdef NO_TIMES
    GETTIMEOFDAY(&start_time);
#else
    start_timing(&start_time);
#endif  /* NO_TIMES */

    if (fwd)
        if (idem)
            perfb_null_idem(rh);
        else
            perfb_null(rh);
    else
        if (idem)
            perf_null_idem(rh);
        else
            perf_null(rh);

    end_timing(&start_time, 1, &avg_time);

#ifdef NO_TIMES
    printf("        ms: %lu.%03u\n", avg_time.msec, avg_time.usec);
#else
    printf("        ms: %lu.%03lu (%lu/%lu)\n",
           avg_time.r_msec, avg_time.r_usec,
           avg_time.u_msec, avg_time.s_msec);
#endif  /* NO_TIMES */
}

/*
 * Start test.  Catch and print any exceptions that are raised.
 */

void rpc__cn_set_sock_buffsize (
        unsigned32	  /* rsize */,
        unsigned32	  /* ssize */,
        error_status_t	* /* st */);
void rpc__cn_inq_sock_buffsize (
        unsigned32	* /* rsize */,
        unsigned32	* /* ssize */,
        error_status_t  * /* st */);

static void start_test(test, argc, argv)

int test;
int argc;
char *argv[];
{
    unsigned32 rsize, ssize;
    error_status_t status;

    rpc__cn_set_sock_buffsize(socket_buf_size, socket_buf_size, &status);
    if (status != rpc_s_ok)
    {
	fprintf(stderr,"*** rpc__cn_set_sock_buffsize failed (0x%lx)\n", status);
        exit(1);
    }

    rpc__cn_inq_sock_buffsize(&rsize, &ssize, &status);
    if (status != rpc_s_ok)
    {
	fprintf(stderr,"*** rpc__cn_inq_sock_buffsize failed (0x%lx)\n", status);
        exit(1);
    }
    if (socket_buf_size != rsize || socket_buf_size != ssize)
    {
        fprintf(stderr, "*** CN socket buffer sizes dont match:\n");
        fprintf(stderr, "*** READ desired: %lu   actual: %lu\n", socket_buf_size, rsize);
        fprintf(stderr, "*** WRITE desired: %lu  actual: %lu\n", socket_buf_size, ssize);
        exit(1);
    }

    TRY
    {
        (*tinfo[test].proc)(test, argc, argv);
    }
    CATCH (rpc_x_comm_failure)
    {
        fprintf(stderr, "*** \"Communications failure\" exception raised\n");
    }
    CATCH (rpc_x_op_rng_error)
    {
        fprintf(stderr, "*** \"Operation out of range\" exception raised\n");
    }
    CATCH (rpc_x_unknown_if)
    {
        fprintf(stderr, "*** \"Unknown interface\" exception raised\n");
    }
    CATCH (rpc_x_unknown_error)
    {
        fprintf(stderr, "*** \"Unknown error\" exception raised\n");
    }
    CATCH (rpc_x_unknown_remote_fault)
    {
        fprintf(stderr, "*** \"Unknown remote fault\" exception raised\n");
    }
    CATCH_ALL
    {
        exc_report(THIS_CATCH);
        fprintf(stderr, "*** Unknown exception raised\n");
    }
    FINALLY
    {
        check_wait_point(0);
    }
    ENDTRY
}



#ifdef _POSIX_THREADS

struct task_info_t
{
    int     thread;
    int     test;
    int     argc;
    char    **argv;
};


/*
 * Base procedure for multithreading test
 */

static void multi_task (info, len)

struct task_info_t  *info;
int                 len __attribute__((unused));

{
    unsigned32  st;

    if (cancel_timeout != -1)
    {
        rpc_mgmt_set_cancel_timeout(cancel_timeout, &st);
        if (st != rpc_s_ok)
        {
            fprintf(stderr,
                "*** Error setting cancel timeout (thread %d) - '%s'\n",
                info->thread, error_text(st));
            return;
        }
    }

    TRY
    {
        start_test(info->test, info->argc, info->argv);
    }
    CATCH_ALL
    {
        exc_report(THIS_CATCH);
        fprintf(stderr, "*** Multi-thread base: exception raised (thread %d)\n", info->thread);
        RERAISE;
    }
    ENDTRY
}

#endif


#ifdef _POSIX_THREADS

/*
 * Start up multiple tasks, each running a test.
 */

#define TASK_STACK_SIZE (64 * 1024)
#define TASK_PRIORITY 3

static void multi_test (test, argc, argv)

int                 test;
int                 argc;
char                *argv[];
{
    int                 i;
    volatile idl_boolean  done;
    pthread_t           tasks[MAX_TASKS];

	DO_NOT_CLOBBER(i);
	 
    if (n_tasks > MAX_TASKS)
    {
        fprintf(stderr, "%d is too many tasks (max is %d)\n", n_tasks, MAX_TASKS);
        exit(1);
    }

    setbuf(stdout, NULL);       /* Buffered output and tasking don't mix */

    printf("  Multithreading: # tasks = %d\n", n_tasks);

    for (i = 0; i < n_tasks; i++)
    {
        struct task_info_t *info = (struct task_info_t *) malloc(sizeof(struct task_info_t));

        info->thread = i;
        info->test   = test;
        info->argc   = argc;
        info->argv   = argv;

        TRY {
            pthread_create(&tasks[i], pthread_attr_default,
                (pthread_startroutine_t) multi_task, (pthread_addr_t) info);
        } CATCH_ALL {
            exc_report(THIS_CATCH);
            printf("*** pthread_create failed\n");
            exit(1);
        } ENDTRY
    }

    done = false;
    while (!done)
    {
        TRY {
            for (i = 0; i < n_tasks; i++)
            {
                void *junk;
                pthread_join(tasks[i], &junk);
            }
            done = true;
        } CATCH_ALL {
            exc_report(THIS_CATCH);
            printf("*** Cancelling threads\n");
            for (i = 0; i < n_tasks; i++)
            {
                pthread_cancel(tasks[i]);
            }
        } ENDTRY
    }

    if (stats)
    {
        do_stats (NULL);
    }
}

#endif


/*
 * Parse authentication (-p) option.
 */
extern int lookup_name(char *table[], char *s);

static void parse_auth_option()
{
    extern char         *optarg;
    char                *s;

    char                *tmp;

    if ((tmp = (char *)malloc(strlen(optarg)+1)) == NULL)
    {
        fprintf(stderr, "*** No more memery\n");
        exit(1);
    }

    strcpy(tmp, optarg);

    /*
     * We can't free tmp, so we will loose some memory after each fork.
     */

    if ((s = (char *) strtok(tmp, ",")) == NULL)
        usage(-1);
    authn_protocol = strcmp(s, "default") == 0 ?
                        rpc_c_authn_default : lookup_name(authn_names, s);

    if ((s = (char *) strtok(NULL, ",")) == NULL)
        usage(-1);
    authz_protocol = lookup_name(authz_names, s);

    if ((s = (char *) strtok(NULL, ",")) == NULL)
    {
        authn_level = rpc_c_authn_level_default;
    }
    else
    {
        authn_level = lookup_name(authn_level_names, s);

        if ((auth_principal = (idl_char *) strtok(NULL, " ")) == NULL)
        {
            auth_principal = NULL;
        }
    }
}


/*
 * Main program
 */
extern void rpc__dbg_set_switches    (
        char            * /*s*/,
        unsigned32      * /*st*/
    );
extern void dump_stg_info(void);

int main (argc, argv)

int                 argc;
char                *argv[];

{
    int                 test, save_argc=0;
    unsigned32          st;
    int                 c;
    idl_boolean         stats = false;
    extern int          optind;
    extern char         *optarg;
    char                *s, **save_argv=NULL;
    int                 do_fork = 0;
    int                 fork_count = 0;
    pid_t               cpid = 0;
    pid_t               opid = getpid();

    if ((clock_ticks = sysconf(_SC_CLK_TCK)) == -1)
        clock_ticks = 1;

fork_test_replay:

    while ((c = getopt(argc, argv, "oslDi1B:d:p:m:M:t:c:f:w:v:r:R:")) != EOF)
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
                usage(-1);
            }

            debug = true;
            break;

        case 'p':
            parse_auth_option();
            authenticate = true;
            break;

        case 'i':
            stats = true;
            break;

        case 't':
            timeout = atoi(optarg);
            break;

        case 'v':
            verbose = atoi(optarg);
            break;

        case 'c':
            cancel_timeout = atoi(optarg);
            if (cancel_timeout != -1)
            {
                rpc_mgmt_set_cancel_timeout(cancel_timeout, &st);
                if (st != rpc_s_ok)
                {
                    fprintf(stderr, "*** Error setting cancel timeout - '%s'\n",
                        error_text(st));
                    exit(1);
                }
            }
            break;

        case 's':
            stats = true;
            break;

        case 'f':
            /*
             * If we've already forked, don't do it again.
             */
            if (do_fork == 0)
            {
                do_fork = atoi(optarg);
                save_argc = argc;
                save_argv = argv;
            }
            break;

        case 'w':
            {
                char *tmp;

                if ((tmp = (char *)malloc(strlen(optarg)+1)) == NULL)
                {
                    fprintf(stderr, "*** No more memory\n");
                    exit(1);
                }

                strcpy(tmp, optarg);

                /*
                 * We can't free tmp, so we will loose some memory after each fork.
                 */

                if ((s = (char *) strtok(tmp, ",")) == NULL)
                    usage(-1);
                wait_point = atoi(s);

                if ((s = (char *) strtok(NULL, " ")) == NULL)
                    usage(-1);
                wait_time = atoi(s);
            }
            break;

        case 'o':
            use_obj = true;
            break;

        case '1':
            compat_mode = true;
            break;

        case 'r':
            reset_binding_freq = atoi(optarg);
            break;

        case 'R':
            recreate_binding_freq = atoi(optarg);
            break;

#ifdef _POSIX_THREADS
        case 'm':
        case 'M':
            multithread = true;
            n_tasks = atoi(optarg);
            use_shared_handle = (c == 'M');
            break;
#endif

	case 'B':
	    socket_buf_size = atoi(optarg);
	    break;
   
        default:
            usage(-1);
        }
    }

    argc -= optind - 1;
    argv = &argv[optind - 1];

    if (argc < 2)
    {
        usage(-1);
    }

    test = atoi(argv[1]);

    if (test < 0 || test > N_TESTS - 1)
    {
        usage(-1);
    }

    printf("%s test [%d]", tinfo[test].name, test);

    if (debug)
    {
        printf(" (debug on)");
    }

    printf("\n");

    if (authenticate)
    {
        VRprintf(2, ("  -----------------\n  Authentication params:\n    authn protocol: %s\n    authz protocol: %s\n    level: %s\n    server princ: \"%s\"\n  -----------------\n",
                authn_protocol == (unsigned32)rpc_c_authn_default ?
                    "default" : authn_names[authn_protocol],
                authz_names[authz_protocol],
                authn_level_names[authn_level],
                (auth_principal == NULL) ? "<none given>" : (char *) auth_principal));
    }

    if (debug)
    {
        dump_stg_info();
    }

    check_wait_point(1);

    if (fork_count != 0 || do_fork != 6)
    {
#ifdef _POSIX_THREADS
        pthread_mutex_init(&global_mutex, pthread_mutexattr_default);

        if (multithread)
        {
            multi_test(test, argc, argv);
            exit(0);
        }
#endif

        start_test(test, argc, argv);

        if (stats)
        {
            do_stats (NULL);
        }
    }

    switch (do_fork)
    {
    case 1:
        if (opid != getpid())        /* child */
            break;
        if (fork_count != 0)    /* original */
        {
            waitpid(cpid, NULL, 0);
            break;
        }
        /*
         * Do the fork.  Both parent and child need to jump back and
         * start over again.
         */
#ifndef VMS
        cpid = fork();
#else
        cpid = vfork();
#endif
        fork_count++;
        argc = save_argc;
        argv = save_argv;
        optind = 1;
        goto fork_test_replay;
        break;
    case 2:
        if (fork_count != 0)    /* original */
        {
            waitpid(cpid, NULL, 0);
            break;
        }
        /*
         * Do the fork.  Only parent need to jump back and
         * start over again.
         */
#ifndef VMS
        cpid = fork();
#else
        cpid = vfork();
#endif
        fork_count++;

        if (cpid == 0)  /* child */
            break;

        argc = save_argc;
        argv = save_argv;
        optind = 1;
        goto fork_test_replay;
        break;
    case 3:
        if (opid != getpid())        /* child */
            break;
        /*
         * Do the fork.  Only child need to jump back and
         * start over again.
         */
#ifndef VMS
        cpid = fork();
#else
        cpid = vfork();
#endif
        fork_count++;

        if (cpid != 0)    /* original */
        {
            waitpid(cpid, NULL, 0);
            break;
        }

        argc = save_argc;
        argv = save_argv;
        optind = 1;
        goto fork_test_replay;
        break;
    case 4:
        if (fork_count == 1)    /* child */
        {
            waitpid(cpid, NULL, 0);
            break;
        }
        if (opid != getpid())        /* grandchild */
            break;
        /*
         * Do the fork twice.  Both child and grandchild need to jump back and
         * start over again.
         */
#ifndef VMS
        cpid = fork();
#else
        cpid = vfork();
#endif
        fork_count++;

        if (cpid != 0)    /* original */
        {
            waitpid(cpid, NULL, 0);
            break;
        }

#ifndef VMS
        cpid = fork();
#else
        cpid = vfork();
#endif
        if (cpid == 0)  /* grandchild */
            fork_count++;

        argc = save_argc;
        argv = save_argv;
        optind = 1;
        goto fork_test_replay;
        break;
    case 5:
        if (opid != getpid())        /* grandchild */
            break;
        /*
         * Do the fork twice.  Only grandchild need to jump back and
         * start over again.
         */
#ifndef VMS
        cpid = fork();
#else
        cpid = vfork();
#endif
        fork_count++;

        if (cpid != 0)    /* original */
        {
            waitpid(cpid, NULL, 0);
            break;
        }

#ifndef VMS
        cpid = fork();
#else
        cpid = vfork();
#endif
        fork_count++;

        if (cpid != 0)    /* child */
        {
            waitpid(cpid, NULL, 0);
            break;
        }

        argc = save_argc;
        argv = save_argv;
        optind = 1;
        goto fork_test_replay;
        break;
    case 6:
        if (opid != getpid())        /* child */
            break;
        /*
         * Do the fork.  Only child need to jump back and
         * start over again.
         */
#ifndef VMS
        cpid = fork();
#else
        cpid = vfork();
#endif
        fork_count++;

        if (cpid != 0)    /* original */
        {
            waitpid(cpid, NULL, 0);
            break;
        }

        argc = save_argc;
        argv = save_argv;
        optind = 1;
        goto fork_test_replay;
        break;
    default:
        break;
    }

    exit(0);
}
