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
**
**  NAME
**
**      perf.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Server manager routines for the perf interface of the performance and
**  system exerciser.
**
**
*/
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <perf_c.h>
#include <perf_p.h>

void print_binding_info(char *text, handle_t h);


#ifdef vms
#define unlink delete
#endif

static int n_calls = 0,
    n_brd = 0,
    n_maybe = 0,
    n_brd_maybe = 0;

static struct 
{
    unsigned32      count;
    uuid_p_t        uuid[3];
} object_vec = 
{
    3,
    {
        &NilObj,
        &NilTypeObj,
        (uuid_p_t) &FooObj1
    }
};



/***************************************************************************/

static void common()
{
    n_calls++;
}

/***************************************************************************/

void perf_init
(
    handle_t                h
)
{
    print_binding_info ("perf_init", h);
    n_calls = 0;
}

/***************************************************************************/

void perf_info 
(
    handle_t                h __attribute__((unused)),
    unsigned32           *n, 
    unsigned32           *nm,
    unsigned32           *nb,
    unsigned32           *nbm
)
{
    *n = n_calls;
    *nm = n_maybe;
    *nb = n_brd;
    *nbm = n_brd_maybe;
}

/***************************************************************************/

void perf_null 
(
    handle_t                h __attribute__((unused))
)
{
    common();
}

/***************************************************************************/

void perf_null_idem 
(
    handle_t                h __attribute__((unused))
)
{
    common();
}

/***************************************************************************/

void perf_in 
(
    handle_t                h __attribute__((unused)),
    perf_data_t             d,
    unsigned32           l,
    idl_boolean             verify,
    unsigned32           *sum
)
{
    unsigned long           i, rsum;
    
    common();

    if (! verify)
    {
        return;
    }

    for (i = 0, rsum = 0; i < l; i++)
    {
        rsum += d[i];
    }

    *sum = rsum;
}

/***************************************************************************/

void perf_in_idem 
(
    handle_t                h __attribute__((unused)),
    perf_data_t             d,
    unsigned32           l,
    idl_boolean             verify,
    unsigned32           *sum
)
{
    perf_in(h, d, l, verify, sum);
}

/***************************************************************************/

void perf_out 
(
    handle_t                h __attribute__((unused)),
    perf_data_t             d,
    unsigned32           *l,
    unsigned32           m,
    unsigned32           pass,
    idl_boolean             verify
)
{
    unsigned long           i;


    common();

    if (! verify)
    {
        return;
    }

    for (i = 0; i < *l; i++)
    {
        d[i] = i * perf_magic * pass;
    }

    *l = m;
}

/***************************************************************************/

void perf_out_idem 
(
    handle_t                h,
    perf_data_t             d,
    unsigned32           *l,
    unsigned32           m,
    unsigned32           pass,
    idl_boolean             verify
)
{
    perf_out(h, d, l, m, pass, verify);
}

/***************************************************************************/

void perf_brd 
(
    handle_t                h,
    idl_char                *name
)
{
    print_binding_info ("perf_brd", h);
    common();
    n_brd++; 
    gethostname(name, 256);
}

/***************************************************************************/

void perf_maybe 
(
    handle_t                h
)
{
    print_binding_info ("perf_maybe", h);
    common();
    n_maybe++;
}

/***************************************************************************/

void perf_brd_maybe 
(
    handle_t                h
)
{
    print_binding_info ("perf_brd_maybe", h);
    common();
    n_brd_maybe++;
}

/***************************************************************************/

void perf_fp_test 
(
    handle_t                h __attribute__((unused)),
    float                   *f1,
    float                   *f2,
    double                  d1,
    double                  d2,
    float                   *o1,
    double                  *o2
)
{
    common();

    UNMARSHALL_DOUBLE(d1);
    UNMARSHALL_DOUBLE(d2);

    *o1 = (*f1 / *f2) * (d1 / d2);
    *o2 = (*f2 / *f1) * (d2 / d1);

    MARSHALL_DOUBLE(*o2);
}

/***************************************************************************/

static boolean32            got_fwd_bindings = false;
static rpc_binding_vector_p_t fwd_bv;

void perf_register_b 
(
    handle_t                h,
    idl_boolean             global __attribute__((unused)),
    unsigned32              *st
)
{
    unsigned_char_p_t       bstr;
    unsigned_char_p_t       protseq;
    unsigned int                     i;
    unsigned32              xst;
    extern rpc_if_handle_t    perfb_v1_0_s_ifspec;
    extern perfb_v1_0_epv_t   perfb_mgr_epv; 
    extern rpc_binding_vector_p_t bv;


    print_binding_info ("perf_register_b", h);

    if (! got_fwd_bindings)
    {
#ifndef REUSE_SERVER_BINDINGS
        /*
         * Create a new endpoint to receive the request on.  Just use
         * a single protseq; the first one in the server's binding list
         * will do.
         */

        rpc_binding_to_string_binding(bv->binding_h[0], &bstr, st);
        rpc_string_binding_parse(bstr, NULL, &protseq, NULL, NULL, NULL, st);
        rpc_server_use_protseq(protseq, 1, st);
        if (*st != 0)
        {
            fprintf(stderr, "*** Can't use_protseq - %s\n",
                    error_text(*st));
            return;
        }
        rpc_string_free(&protseq, st);
        rpc_string_free(&bstr, st);

        /*
         * Need to come up with a vector of handles to the newly created
         * endpoint.  This is a real hack (the ordering of the handles
         * is presumptious), but it should work for the purposes of this
         * test.  The correct thing to do would be to convert all the bindings 
         * to binding-strings and filter out all duplicates.
         */

        rpc_server_inq_bindings(&fwd_bv, st);
        if (fwd_bv->count <= bv->count)
        {
            fprintf(stderr, "*** No additional bindings created for forwarding?\n");
            *st = -1;   /* !!! */
            return;
        }
                   
        /*
         * Free all the pre-existing handles and shuffle the new ones to the
         * beginning of the vector (adjust the count appropriately).
         */

        for (i = 0; i < bv->count; i++)
            rpc_binding_free(&fwd_bv->binding_h[i], st);

        for (i = bv->count; i < fwd_bv->count; i++)
        {
            rpc_binding_copy(fwd_bv->binding_h[i], 
                    &fwd_bv->binding_h[i-bv->count], st);
            rpc_binding_free(&fwd_bv->binding_h[i], st);
        }
        fwd_bv->count = i - bv->count;
#else
        /*
         * Just use the original bindings... this is slightly unorthodox
         * (not going fully through the effort to duplicate the binding
         * references) but it should work since neither the original nor
         * the duplicate bindings are freed.
         */
        fwd_bv = bv;
#endif
        printf("+ Got bindings:\n");
        for (i = 0; i < fwd_bv->count; i++)
        {
            rpc_binding_to_string_binding(fwd_bv->binding_h[i], &bstr, st);
            printf("    %s\n", (char *)bstr);
            rpc_string_free(&bstr, st);
        }
        got_fwd_bindings = true;
    }

    rpc_server_register_if(perfb_v1_0_s_ifspec, 
                    (uuid_p_t) NULL, (rpc_mgr_epv_t) &perfb_mgr_epv, st);
    if (*st != 0)
    {
        fprintf(stderr, "*** Can't rpc_server_register_if - %s\n", 
                error_text (*st));
        return;
    }

    rpc_ep_register(perfb_v1_0_s_ifspec, fwd_bv, (uuid_vector_p_t) &object_vec,
            (unsigned_char_p_t)"perfb forwarding test manager", st);

    if (*st != 0)
    {
        fprintf(stderr, "*** Can't rpc_ep_register - %s\n", 
                error_text (*st));
        rpc_server_unregister_if(perfb_v1_0_s_ifspec, (uuid_p_t) NULL, &xst);
        if (xst != 0)
        {
            fprintf(stderr, "*** Can't rpc_server_unregister_if - %s\n",
                   error_text(xst));
        }
        return;
    }
}

/***************************************************************************/

void perf_unregister_b
(
    handle_t                h,
    unsigned32              *st
)
{
    unsigned32              st1, st2;
    extern rpc_if_handle_t  perfb_v1_0_s_ifspec;

    print_binding_info ("perf_unregister_b", h);

    if (!got_fwd_bindings)
    {
        fprintf(stderr, "*** perf_unregister_b - no fwd bindings\n");
        *st = -1;   /* !!! */
    }

    rpc_ep_unregister(perfb_v1_0_s_ifspec, fwd_bv, 
                            (uuid_vector_p_t) &object_vec, &st1);
    if (st1 != 0)
    {
        fprintf(stderr, "*** Can't rpc_ep_unregister - %s\n",
                error_text (st1));
    }

    rpc_server_unregister_if(perfb_v1_0_s_ifspec, (uuid_p_t) NULL, &st2);
    if (st2 != 0)
    {
        fprintf(stderr, "*** Can't rpc_server_unregister_if - %s\n",
               error_text(st2));
    }

    *st = (st1 != 0) ? st1 : st2;
}


/***************************************************************************/

void perf_exception 
(
    handle_t                h
)
{

    print_binding_info ("perf_exception", h);

    /*
     * Here we raise an exception which the server stub will
     * catch, and hopefully, bounce back to the client.
     *
     * At various times, this code did a floating point divide by
     * zero, and a kill (self, SIGFPE).  In the current DCE 1.0.2
     * implementation, the threads package terminates the process
     * upon synchronous terminating signals, and we cannot easily
     * catch them. Therefore do a simple RAISE here.
     */

    RAISE (exc_e_intdiv);
    
}

/***************************************************************************/

static void slow
(
    handle_t                h __attribute__((unused)),
    perf_slow_mode_t        mode,
    unsigned32           secs
)
{
    long                    start_time;

    common();

    TRY
    {
        start_time = time(0l);

        switch ((int) mode)
        {
            case perf_slow_sleep:
                printf("+ Sleeping for %lu seconds...\n", secs);
                SLEEP(secs);
                printf("    ...awake!\n");
                break;

            case perf_slow_cpu: 
                printf("+ CPU looping for %lu seconds...\n", secs);
                while ((unsigned32)(time(0) - start_time) < secs)
                {
                    ;
                }
                printf("    ...done!\n");
                break;

            case perf_slow_io: 
            {
                char *heap = (char *) malloc(secs);
                int f, n;
                unsigned long i;
                static char buf[] = "0123456789ABCDE\n";
                char *t;
                char TempFileName [] = "/tmp/perfXXXXXX";

                t = (char *) mktemp(TempFileName);
                printf("+ Writing file \"%s\" (size=%ld bytes)\n", t, secs);
                f = open(t, (O_TRUNC | O_RDWR | O_CREAT), 0777);
                if (f < 0)
                {
                    perror("Can't create temp file");
                    goto DONE;
                }

                for (i = 0; i < secs; i++)
                {
                    n = write(f, buf, sizeof buf);
                    if (n != sizeof buf)
                    {
                        perror("Write failed");
                        goto DONE;
                    }
                }
                for (i = 0; i < 10; i++)
                {
                    lseek(f, 0L, 0);
                    n = read(f, heap, (int) secs);
                    if (n < 0)
                    {
                        perror("Read failed");
                        goto DONE;
                    }
                    printf("    ...read %d bytes (%ld)\n", n, i);
                }


DONE:
                printf("    ...done!\n");
                free(heap);
                if (f >= 0)
                    close(f);
                unlink(t);
                break;
            }
            case perf_slow_fork_sleep:
            {
                pid_t   cpid;
                pid_t   pid;
                char    buf[16];
                printf("+ Forking sleep for %lu seconds...\n", secs);
                sprintf(buf, "%lu", secs);
                cpid = fork();
                if (cpid == 0)
                {
                    /* Child */
                    execlp("sleep", "sleep", buf, 0);
                }
                else
                {
                    /* Parent */
                    /*
                     * CMA doesn't wrap waitpid()!
                     */
                    pid = waitpid(cpid, NULL, WNOHANG);
                    if (pid != -1)
                    {
                        SLEEP(secs-1);
                        waitpid(cpid, NULL, 0);
                    }
                }
                printf("    ...awake!\n");
                break;
            }
        }
    }
    CATCH(cma_e_alerted)
    {
        printf("    ...'cancel' exception caught\n");
        RERAISE;
    }
    CATCH_ALL
    {
        printf("    ...unknown exception caught\n");
        RERAISE;
    }
    ENDTRY
}

/***************************************************************************/

void perf_null_slow 
(
    handle_t                h,
    perf_slow_mode_t        mode,
    unsigned32           secs
)
{
  int oc = 0;
  
  if (mode == perf_slow_cpu)
    oc = pthread_setcancel(CANCEL_OFF);

  print_binding_info ("perf_null_slow", h);
  slow (h, mode, secs);

  if (mode == perf_slow_cpu)
    pthread_setcancel(oc);

}

/***************************************************************************/

void perf_null_slow_idem 
(
    handle_t                h,
    perf_slow_mode_t        mode,
    unsigned32           secs
)
{
  int oc =0;
  
  if (mode == perf_slow_cpu)
    oc = pthread_setcancel(CANCEL_OFF);

  print_binding_info ("perf_null_slow_idem", h);
  slow(h, mode, secs);

  if (mode == perf_slow_cpu)
    pthread_setcancel(oc);

}

/***************************************************************************/

void perf_shutdown 
(
    handle_t                h
)
{
    unsigned32          st;

    print_binding_info ("perf_shutdown", h);
    common();
    rpc_mgmt_stop_server_listening (NULL, &st);
}

/***************************************************************************/

struct shutdown_info
{
    unsigned32      secs;
};


static void *shutdown_thread
(
    void *p_
)
{
    struct shutdown_info *p = (struct shutdown_info *) p_;
    unsigned32          st;

    printf ("+ Shutdown thread...\n");

    printf ("  sleeping for %lu seconds\n", p->secs);
    SLEEP (p->secs);

    if (use_reserved_threads)
    {
        printf ("  unreserving threads (non blocking)...\n");
        teardown_thread_pools(false /* don't block */);
    }

    printf ("  calling \"rpc_mgmt_stop_server_listening\"...\n");
    rpc_mgmt_stop_server_listening (NULL, &st);

    free (p);
    printf ("  exiting thread\n");

    return NULL;
}


void perf_shutdown2
(
    handle_t                h __attribute__((unused)),
    unsigned32              secs
)
{
    struct shutdown_info *p;
    pthread_t           thread;

    common();

    printf ("+ Creating shutdown thread\n");

    p = (struct shutdown_info *) malloc (sizeof *p);
    p->secs = secs;

    pthread_create (&thread, pthread_attr_default, 
	shutdown_thread, (void *) p);
    pthread_detach (&thread);
}



/***************************************************************************/

void perf_call_callback 
(
    handle_t                h,
    unsigned32           idem
)
{
    unsigned                     i;
    unsigned32           c, passes;
    unsigned32          st;


    print_binding_info ("perf_call_callback", h);
    common();

    perfc_init(h, &passes);

    for (i = 1; i <= passes; i++)
    {
        if (idem)
        {
            perfc_cb_idem(h, &c);
        }
        else
        {
            perfc_cb(h, &c);
        }
    }

    if ((! idem) && c != passes)
    {
        printf("    ...count mismatch [%lu, %lu]\n", c, passes);
        st = 1;
#ifdef NOTDEF
        pfm_$signal(st);
#endif
    }
}

/***************************************************************************/

struct context 
{
    unsigned long   magic;
    unsigned long   data;
};

#define CONTEXT_MAGIC 0xfeedf00d

void perf_context_t_rundown
(
    rpc_ss_context_t        context
)
{
    struct context *p = (struct context *) context;

    printf("+ In context rundown function\n");

    if (p->magic != CONTEXT_MAGIC)
    {
        fprintf(stderr, "*** context mismatch; %08lx != %08x\n", p->magic, CONTEXT_MAGIC);
        return;
    }

    free(context);
}

/***************************************************************************/

void perf_get_context
(
    handle_t                h,
    unsigned32           data,
    perf_context_t          *context
)
{
    struct context *p;

    print_binding_info ("perf_get_context", h);
    p = (struct context *) malloc(sizeof(struct context));

    p->magic = CONTEXT_MAGIC;
    p->data  = data;

    *context = (perf_context_t) p;
}

/***************************************************************************/

idl_boolean perf_test_context
(
    perf_context_t          context,
    unsigned32           *data
)
{
    struct context *p = (struct context *) context;

    if (p->magic != CONTEXT_MAGIC)
    {
        fprintf(stderr, "*** context mismatch; %08lx != %08x\n", p->magic, CONTEXT_MAGIC);
        return (false);
    }

    *data = p->data;

    return (true);
}

/***************************************************************************/

idl_boolean perf_free_context
(
    perf_context_t          *context,
    unsigned32           *data
)
{
    struct context *p = (struct context *) *context;

    *context = NULL;

    if (p->magic != CONTEXT_MAGIC)
    {
        fprintf(stderr, "*** context mismatch; %08lx != %08x\n", p->magic, CONTEXT_MAGIC);
        return (false);
    }

    *data = p->data;

    free(p);

    return (true);
}

/***************************************************************************/

void perf_brd_fault
(
    handle_t                h
)
{
    common();
    n_brd++; 
    print_binding_info ("perf_brd_fault", h);
    RAISE (rpc_x_unknown_remote_fault);
}



/***************************************************************************/

perf_v2_0_epv_t perf_epv =
{
    perf_init,
    perf_info,
    perf_null,
    perf_null_idem,
    perf_in,
    perf_in_idem,
    perf_out,
    perf_out_idem,
    perf_brd,
    perf_maybe,
    perf_brd_maybe,
    perf_fp_test,
    perf_register_b,
    perf_unregister_b,
    perf_exception,
    perf_null_slow,
    perf_null_slow_idem,
    perf_shutdown,
    perf_call_callback,
    perf_get_context,
    perf_test_context,
    perf_free_context,
    perf_shutdown2,
    perf_brd_fault
};
