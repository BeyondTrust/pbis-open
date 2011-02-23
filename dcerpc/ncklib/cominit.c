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
**      cominit.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Initialization Service routines.
**
**
*/

#define _DCE_PTHREAD_EXC_H

#include <commonp.h>    /* Common internals for RPC Runtime System      */
#include <com.h>        /* Externals for Common Services component      */
#include <comp.h>       /* Internals for Common Services component      */
#include <comcthd.h>    /* Externals for call thread services component */
#include <cominit.h>    /* Externals for Initialization sub-component   */
#include <cominitp.h>   /*                                              */
#include <comnaf.h>     /* Internals for NAF Extension services         */
#include <mgmtp.h>      /* Private management services                  */
#include <assert.h>


#define SUPPORTED true
#define UNSUPPORTED false

/*
 * 64K bytes should be sufficient as that is what DCE Security
 * uses for its own threads.
 */
#define DEFAULT_STACK_SIZE 64000

#ifdef RRPC
PRIVATE unsigned32 rpc__rrpc_init (void);
#endif

INTERNAL boolean supported_naf (
        rpc_naf_id_elt_p_t               /*naf*/
    );
    
INTERNAL boolean supported_interface (
        rpc_naf_id_t                    /*naf*/,
        rpc_network_if_id_t             /*network_if*/,
        rpc_network_protocol_id_t        /*network_protocol*/
    );

INTERNAL boolean protocol_is_compatible (
        rpc_protocol_id_elt_p_t          /*rpc_protocol*/
    );

INTERNAL void init_once (void);

INTERNAL void thread_context_destructor (
        rpc_thread_context_p_t   /*ctx_value*/
    );

/*
 * The structure that defines the one-time initialization code. This
 * structure will be associated with init_once.
 */
INTERNAL dcethread_oncectl init_once_block = DCETHREAD_ONCE_INIT;

/*
 * The value that indicates whether or not the RPC runtime is currently
 * being initialized.
 */
INTERNAL boolean        init_in_progress = false;

INTERNAL rpc_mutex_t    rpc_in_fork_mutex;

/*
 * The id of the thread that is executing (has executed) the RPC runtime 
 * initialization code.
 */
GLOBAL   dcethread*      init_thread;


/*
 * Don't do the getenv() stuff if not built with DEBUG #define'd
 */

#if !defined(DEBUG) && !defined(NO_GETENV)
#  define NO_GETENV
#endif

#ifndef NO_GETENV

INTERNAL void init_getenv_protseqs (void);

INTERNAL void init_getenv_debug (void);

INTERNAL void init_getenv_port_restriction (void);

#endif


/*
**++
**
**  ROUTINE NAME:       rpc__init
**
**  SCOPE:              PRIVATE - declared in cominit.h
**
**  DESCRIPTION:
**      
**  rpc__init() is called to intialize the runtime.  It can safely be
**  called at any time, though it is typically called only be
**  RPC_VERIFY_INIT() in order to minimize overhead.  Upon return from
**  this routine, the runtime will be initialized.
**
**  Prevent rpc__init() (actually dcethread_once() of init_once()) recursive
**  call deadlocks by the thread that is actually performing the
**  initialization (see init_once()).
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__init(void)

{
    dcethread*       current_thread;
    

    if (init_in_progress)
    {
        current_thread = dcethread_self();
        
        if (dcethread_equal(init_thread, current_thread))
        {
            /*
             * We're the thread in the middle of initialization (init_once).
             * Assume it knows what it's doing and prevent a deadlock.
             */
            return;
        }
    }

    dcethread_once_throw (&init_once_block, init_once);
}

/*
 * This is a debugging-only interface: it does not cal dcethread_once(0
 * so should be called from main().
 *
 * We use this because valgrind does not like nested calls to
 * dcethread_once(), and libdl on Linux (invoked whilst loading
 * modules from init_once()) calls dcethread_once().
 */
PRIVATE void rpc__static_init(void)
{
    dcethread*       current_thread;
    

    if (init_in_progress)
    {
        current_thread = dcethread_self();
        
        if (dcethread_equal(init_thread, current_thread))
        {
            /*
             * We're the thread in the middle of initialization (init_once).
             * Assume it knows what it's doing and prevent a deadlock.
             */
            return;
        }
    }
    init_once();
}




/*
**++
**
**  ROUTINE NAME:       init_once
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  init_once() is invoked only once, by either the RPC runtime or,
**  (in the case of shared images), the host operating system. It performs
**  the basic initialization functions for the all of the RPC Runtime
**  Services (by calling the initialization routines in other components).
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:
**
**  The random number generator is seeded with a psuedo-random value (the
**  system time).
**
**--
**/

INTERNAL void init_once(void)
{
	rpc_naf_id_elt_p_t      naf;
	rpc_protocol_id_elt_p_t rpc_protocol;
	rpc_protseq_id_elt_p_t  rpc_protseq;
	rpc_authn_protocol_id_elt_p_t auth_protocol;
	unsigned32              ctr;
	unsigned32              status;


	/*
	*  Assert that the size of unsigned long is the same as the size of
	*  a pointer.  This assumption may not be true for all machines.
	 *
	*  If you are getting this assertion, you will need to change the
	*  macros RPC_CN_ALIGN_PTR(ptr, boundary) in cnp.h and
	*  RPC_DG_ALIGN_8(p) in dg.h.  The pointer should be cast to
	*  the correct scalar data type inorder to make the bitwise-AND
	*  work.  The bitwise operators do not allow pointers for
	*  operands.  The following assert will also have to be changed
	*  to use the new scalar data type.
	*/
	assert(sizeof(unsigned long) == sizeof(unsigned8 *));

#ifdef APOLLO_GLOBAL_LIBRARY
	apollo_global_lib_init();
#endif

	/*
	 * Initialize the performance logging service.
	 */
	RPC_LOG_INITIALIZE;

#ifdef DCE_RPC_SVC
	/*
	 * Remove this ifdef when SVC is implemented
	 * in the kernel.
	 */
	rpc__svc_init();
#endif
#ifndef NO_GETENV
	init_getenv_debug ();
#endif

	/*
	 * These first two operations (and their order) are critical to
	 * creating a deadlock safe environment for the initialization
	 * processing.  This code (in conjunction with rpc__init()) allows
	 * a thread that is executing init_once() to call other runtime
	 * operations that would normally want to ensure that the runtime
	 * is initialized prior to executing (which would typically recursively
	 * call dcethread_once for this block and deadlock).
	 * 
	 * While this capability now allows the initializing thread to not
	 * deadlock, it also allows it to *potentially* attempt some operation
	 * that require initialization that hasn't yet been performed.
	 * Unfortunately, this can't be avoided; be warned, be careful, don't
	 * do anything crazy and all will be fine.
	 *
	 * One example of where this type of processing is occuring is in
	 * the nca_dg initialization processing - the dg init code calls
	 * rpc_server_register_if.
	 */
	init_thread = dcethread_self();
	init_in_progress = true;
        RPC_MUTEX_INIT (rpc_in_fork_mutex);

	/*
	 * Register our fork handler, if such a service is supported.
	 */

     /*
      * LIKEWISE -- we don't support arbitrary forking anyway,
      * so this has little benefit and is prone to error.  Disable it.
      */
#if 0
	ATFORK((void *)rpc__fork_handler);
#endif

	/*
	 * Initialize the global mutex variable.
	 */
	RPC_LOCK_INIT (0);

	/*
	 * Initialize the global lookaside list mutex.
	 */
	RPC_LIST_MUTEX_INIT (0);

	/*
	 * Initialize the global binding handle condition variable.
	 */
	RPC_BINDING_COND_INIT (0);

	/*
	 * create the per-thread context key
	 */
	dcethread_keycreate_throw (&rpc_g_thread_context_key, 
			(void (*) (pointer_t)) thread_context_destructor);

	/*
	 * Initialize the timer service.
	 */
	rpc__timer_init();

	/*
	 * Initialize the interface service.
	 */
	rpc__if_init (&status);    
	if (status != rpc_s_ok)
	{
		dce_error_string_t error_text;
		int temp_status;

		dce_error_inq_text(status, (unsigned char*) error_text, &temp_status);
                
		/*
		 * rpc_m_call_failed
		 * "%s failed: %s"
		 */
		RPC_DCE_SVC_PRINTF ((
					DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
					rpc_svc_general,
					svc_c_sev_fatal | svc_c_action_abort,
					rpc_m_call_failed,
					"rpc__if_init",
					error_text ));
	}

	/*
	 * Initialize the object service.
	 */
	rpc__obj_init (&status);    
	if (status != rpc_s_ok)
	{
		dce_error_string_t error_text;
		int temp_status;

		dce_error_inq_text(status, (unsigned char*) error_text, &temp_status);

		/*
		 * rpc_m_call_failed
		 * "%s failed: %s"
		 */
		RPC_DCE_SVC_PRINTF ((
					DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
					rpc_svc_general,
					svc_c_sev_fatal | svc_c_action_abort,
					rpc_m_call_failed,
					"rpc__obj_init",
					error_text ));
	}

	/*
	 * Initialize the cthread service (this doesn't do too much
	 * so it doesn't matter if this process never becomes a server).
	 */
	rpc__cthread_init (&status);    
	if (status != rpc_s_ok)
	{
		dce_error_string_t error_text;
		int temp_status;

		dce_error_inq_text(status, (unsigned char*) error_text, &temp_status);

		/*
		 * rpc_m_call_failed
		 * "%s failed: %s"
		 */
		RPC_DCE_SVC_PRINTF ((
					DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
					rpc_svc_general,
					svc_c_sev_fatal | svc_c_action_abort,
					rpc_m_call_failed,
					"rpc__cthread_init",
					error_text ));
	}



	/*
	 * go through the NAF id table and check each Network Address Family
	 * to see if it is supported on this system
	 */

	/* pre-load modules */
	rpc__load_modules();

	for (ctr = 0; ctr < RPC_C_NAF_ID_MAX; ctr++)
	{
		naf = (rpc_naf_id_elt_p_t) &(rpc_g_naf_id[ctr]);

		if (supported_naf (naf))
		{
			/*
			 * if there is no pointer to the init routine for this NAF
			 * it must be a shared image, so load it
			 */
			if (naf->naf_init == NULL)
			{
				naf->naf_id = ctr;
				naf->naf_init = rpc__load_naf (naf, &status);
			}


			/*
			 * check it again - shouldn't be NULL now, but if it is
			 * we'll leave it that way (unsupported)
			 */
			if (naf->naf_init)
			{
				(*naf->naf_init) (&(naf->epv), &status);
				if (status != rpc_s_ok)
				{
					/*
					 * If the NAF couldn't be intialized make it unsupported.
					 */
					naf->naf_init = NULL;
				}
			}
		}
		else
		{
			/*
			 * network family not supported
			 */
			naf->naf_init = NULL;
		}
	}

	/*
	 * go through the RPC protocol id table and check each protocol to
	 * see if it is supported on this system
	 */
	for (ctr = 0; ctr < RPC_C_PROTOCOL_ID_MAX; ctr++)
	{
		if (protocol_is_compatible (rpc_protocol =
					(rpc_protocol_id_elt_p_t) &(rpc_g_protocol_id[ctr])))
		{
			/*
			 * if there is no pointer to the init routine for this protocol
			 * it must be a shared image, so load it
			 */
			if ((rpc_protocol->prot_init) == NULL)
			{
				rpc_protocol->prot_init =
					rpc__load_prot (rpc_protocol, &status);
			}

			/*
			 * check it again - shouldn't be NULL now, but if it is
			 * we'll leave it that way (unsupported)
			 */
			if (rpc_protocol->prot_init)
			{
				(*rpc_protocol->prot_init)
					(&(rpc_protocol->call_epv), &(rpc_protocol->mgmt_epv),
					 &(rpc_protocol->binding_epv), &(rpc_protocol->network_epv),
					 &(rpc_protocol->prot_fork_handler), &status);
				if (status != rpc_s_ok)
				{
					dce_error_string_t error_text;
					int temp_status;

					dce_error_inq_text(status, (unsigned char*) error_text, &temp_status);

					/*
					 * rpc_m_call_failed
					 * "%s failed: %s"
					 */
					RPC_DCE_SVC_PRINTF ((
								DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
								rpc_svc_general,
								svc_c_sev_fatal | svc_c_action_abort,
								rpc_m_call_failed,
								"prot_init",
								error_text ));
				}
			}
		}
		else
		{
			/*
			 * protocol family not supported
			 */
			rpc_protocol->prot_init = NULL;
		}
	}

	/*
	 * go through the RPC protocol sequence table and check each protocol
	 * sequence to see if it is supported on this system
	 */
	for (ctr = 0; ctr < RPC_C_PROTSEQ_ID_MAX; ctr++)
	{
		rpc_protseq = (rpc_protseq_id_elt_p_t) &(rpc_g_protseq_id[ctr]);

		rpc_protocol = (rpc_protocol_id_elt_p_t)
			&(rpc_g_protocol_id[RPC_PROTSEQ_INQ_PROT_ID(ctr)]);

		naf = (rpc_naf_id_elt_p_t)
			&(rpc_g_naf_id[RPC_PROTSEQ_INQ_NAF_ID(ctr)]);

		if (rpc_protocol->prot_init != NULL 
				&& naf->naf_init != NULL
				&& supported_interface (rpc_protseq->naf_id,
					rpc_protseq->network_if_id, rpc_protseq->network_protocol_id))
		{
			rpc_protseq->supported = SUPPORTED;
		}
		else
		{
			rpc_protseq->supported = UNSUPPORTED;
		}
	}

#ifndef NO_GETENV

	/*
	 * See if a list of protocol sequences to use was specified in the
	 * RPC_SUPPORTED_PROTSEQS enviroment variable and if one was, process
	 * them appropriately.  Note well that this call must follow the above
	 * loop.  (See comments in init_getenv_protseqs.)
	 */

	init_getenv_protseqs ();

#endif                                  /* NO_GETENV */

	/*
	 * Initialize the auth info cache.
	 */
	rpc__auth_info_cache_init (&status);    
	if (status != rpc_s_ok)
	{
		dce_error_string_t error_text;
		int temp_status;

		dce_error_inq_text(status, (unsigned char*) error_text, &temp_status);

		/*
		 * rpc_m_call_failed
		 * "%s failed: %s"
		 */
		RPC_DCE_SVC_PRINTF ((
					DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
					rpc_svc_general,
					svc_c_sev_fatal | svc_c_action_abort,
					rpc_m_call_failed,
					"rpc__auth_info_cache_init",
					error_text ));
	}

	/*
	 * go through the authentication protocol id table and check each protocol to
	 * see if it is supported on this system
	 */
	for (ctr = 0; ctr < RPC_C_AUTHN_PROTOCOL_ID_MAX; ctr++)
	{
		auth_protocol = &rpc_g_authn_protocol_id[ctr];

		/*
		 * if there is no pointer to the init routine for this protocol
		 * it must be a shared image, so load it
		 */
		if ((auth_protocol->auth_init) == NULL)
		{
			auth_protocol->auth_init =
				rpc__load_auth (auth_protocol, &status);
		}

		/*
		 * check it again - shouldn't be NULL now, but if it is
		 * we'll leave it that way (unsupported)
		 */
		if (auth_protocol->auth_init)
		{
			(*auth_protocol->auth_init)
				(&(auth_protocol->epv),
				 (&(auth_protocol->rpc_prot_epv_tbl)),
				 &status);
			if (status != rpc_s_ok) return;
		}
	}

	/*
	 * make calls to initialize other parts of the RPC runtime
	 */

#ifndef _DCE_PTHREAD_EXC_H
	if (dcethread_attr_create(&rpc_g_server_dcethread_attr) == -1)
	{
		/*
		 * rpc_m_call_failed_errno
		 * "%s failed, errno = %d"
		 */
		RPC_DCE_SVC_PRINTF ((
					DCE_SVC(RPC__SVC_HANDLE, "%s%d"),
					rpc_svc_general,
					svc_c_sev_fatal | svc_c_action_abort,
					rpc_m_call_failed_errno,
					"dcethread_attr_create",
					errno ));
	}
#else
	DCETHREAD_TRY 
		dcethread_attr_create_throw(&rpc_g_server_dcethread_attr);
	DCETHREAD_CATCH_ALL(THIS_CATCH)
		/*
		 * rpc_m_call_failed_no_status
		 * "%s failed"
		 */
		RPC_DCE_SVC_PRINTF ((
					DCE_SVC(RPC__SVC_HANDLE, "%s"),
					rpc_svc_general,
					svc_c_sev_fatal | svc_c_action_abort,
					rpc_m_call_failed_no_status,
					"dcethread_attr_create" ));
	DCETHREAD_ENDTRY
#endif



		/*
		 * We explicitly set the stack size for the threads that
		 * the runtime uses internally.  This size needs to accomodate
		 * deeply nested calls to support authenticated RPC.
		 */
#ifndef _DCE_PTHREAD_EXC_H
		if (dcethread_attr_create(&rpc_g_default_dcethread_attr) == -1)
		{
			/*
			 * rpc_m_call_failed_errno
			 * "%s failed, errno = %d"
			 */
			RPC_DCE_SVC_PRINTF ((
						DCE_SVC(RPC__SVC_HANDLE, "%s%d"),
						rpc_svc_general,
						svc_c_sev_fatal | svc_c_action_abort,
						rpc_m_call_failed_errno,
						"dcethread_attr_create",
						errno ));
		}
	if (dcethread_attr_setstacksize(&rpc_g_default_dcethread_attr,
				DEFAULT_STACK_SIZE) == -1) 
	{
		/*
		 * rpc_m_call_failed_errno
		 * "%s failed, errno = %d"
		 */
		RPC_DCE_SVC_PRINTF ((
					DCE_SVC(RPC__SVC_HANDLE, "%s%d"),
					rpc_svc_general,
					svc_c_sev_fatal | svc_c_action_abort,
					rpc_m_call_failed_errno,
					"dcethread_attr_setstacksize",
					errno ));
	}

#else  /* _DCE_PTHREAD_EXC_H */
	DCETHREAD_TRY 
		dcethread_attr_create_throw(&rpc_g_default_dcethread_attr);
	DCETHREAD_CATCH_ALL(THIS_CATCH)
		/*
		 * rpc_m_call_failed_no_status
		 * "%s failed"
		 */
		RPC_DCE_SVC_PRINTF ((
					DCE_SVC(RPC__SVC_HANDLE, "%s%d"),
					rpc_svc_general,
					svc_c_sev_fatal | svc_c_action_abort,
					rpc_m_call_failed_no_status,
					"dcethread_attr_create" ));
	DCETHREAD_ENDTRY

		DCETHREAD_TRY 
		dcethread_attr_setstacksize_throw(&rpc_g_default_dcethread_attr,
				DEFAULT_STACK_SIZE);
	DCETHREAD_CATCH_ALL(THIS_CATCH)
		/*
		 * rpc_m_call_failed_no_status
		 * "%s failed"
		 */
		RPC_DCE_SVC_PRINTF ((
					DCE_SVC(RPC__SVC_HANDLE, "%s%d"),
					rpc_svc_general,
					svc_c_sev_fatal | svc_c_action_abort,
					rpc_m_call_failed_no_status,
					"dcethread_attr_setstacksize" ));
	DCETHREAD_ENDTRY
#endif    /* not _DCE_PTHREAD_EXC_H */


		rpc__network_init (&status);    
	if (status != rpc_s_ok)
	{
		dce_error_string_t error_text;
		int temp_status;

		dce_error_inq_text(status, (unsigned char*) error_text, &temp_status);

		/*
		 * rpc_m_call_failed
		 * "%s failed: %s"
		 */
		RPC_DCE_SVC_PRINTF ((
					DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
					rpc_svc_general,
					svc_c_sev_fatal | svc_c_action_abort,
					rpc_m_call_failed,
					"rpc__network_init",
					error_text ));
	}

	status = rpc__mgmt_init();
	if (status != rpc_s_ok)
	{
		dce_error_string_t error_text;
		int temp_status;

		dce_error_inq_text(status, (unsigned char*) error_text, &temp_status);

		/*
		 * rpc_m_call_failed
		 * "%s failed: %s"
		 */
		RPC_DCE_SVC_PRINTF ((
					DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
					rpc_svc_general,
					svc_c_sev_fatal | svc_c_action_abort,
					rpc_m_call_failed,
					"rpc__mgmt_init",
					error_text ));
	}

#ifdef RRPC
	status = rpc__rrpc_init();
	if (status != rpc_s_ok)
	{
		dce_error_string_t error_text;
		int temp_status;

		dce_error_inq_text(status, error_text, &temp_status);

		/*
		 * rpc_m_call_failed
		 * "%s failed: %s"
		 */
		RPC_DCE_SVC_PRINTF ((
					DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
					rpc_svc_general,
					svc_c_sev_fatal | svc_c_action_abort,
					rpc_m_call_failed,
					"rpc__rrpc_init",
					error_text ));
	}
#endif

	/*
	 * initialize (seed) the random number generator using the current
	 * system time
	 */
	RPC_RANDOM_INIT(time (NULL));

#ifndef NO_GETENV

	/* 
	 * See if there are any protocol sequences which should only bind to 
	 * certain ranges of network endpoints.
	 */

	init_getenv_port_restriction ();

#endif                                  /* ! NO_GETENV */

	init_in_progress = false;
	rpc_g_initialized = true;
}    

/*
**++
**
**  ROUTINE NAME:       supported_naf
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Called by rpc__init to determine whether a specified Network Family 
**  is supported on the local host operating system.  It makes this
**  determination by trying to create a "socket" under the specified
**  network family.
**
**  INPUTS:
**
**      naf             The network family whose existence is to be tested.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      Returns true if the network family is supported.
**      Otherwise returns false.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL boolean supported_naf 
(
    rpc_naf_id_elt_p_t      naf
)
{
    rpc_socket_basic_t            socket;
    rpc_socket_error_t      socket_error;
    
    if (naf->naf_id == 0)
        return (false);
    else if (naf->naf_id >= RPC_C_NAF_ID_VIRTUAL)
        /* NAF id is not known by OS, so assume it is supported */
        return (true);
    
    socket_error = rpc__socket_open_basic
        (naf->naf_id, naf->network_if_id, 0, &socket);

    if (! RPC_SOCKET_IS_ERR (socket_error))
    {
        rpc__socket_close_basic (socket);
        return (true);
    }

    return (false);
}

/*
**++
**
**  ROUTINE NAME:       supported_interface
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Called by rpc__init to determine whether the Network Protocol ID/
**  Network Interface Type ID combination will work on the local host 
**  operating system (for the specified Network Family).
**
**  INPUTS:
**
**      naf             The network family to be tested.
**
**      network_if      The network interface to be tested.
**
**      network_protocol The network protocol to be tested.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      Returns true if the combination is valid.
**      Otherwise returns false.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL boolean supported_interface 
(
    rpc_naf_id_t            naf,
    rpc_network_if_id_t     network_if,
    rpc_network_protocol_id_t network_protocol
)
{
    rpc_socket_basic_t            socket;
    rpc_socket_error_t      socket_error;

    if (naf >= RPC_C_NAF_ID_VIRTUAL)
    {
        return (true);
    }

    socket_error = rpc__socket_open_basic
        (naf, network_if, network_protocol, &socket);

    if (! RPC_SOCKET_IS_ERR (socket_error))
    {
        (void) rpc__socket_close_basic (socket);
        return (true);
    }

    return (false);
}

/*
**++
**
**  ROUTINE NAME:       protocol_is_compatible
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Called by rpc__init to determine whether the specified RPC protocol
**  is compatible with at least one of the Network Families previously
**  found to be supported under the local operating system.
**
**  INPUTS:
**
**      rpc_protocol        The RPC protocol to be tested.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      Returns true if the protocol is compatible.
**      Otherwise returns false.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL boolean protocol_is_compatible 
(
    rpc_protocol_id_elt_p_t rpc_protocol
)
{
    unsigned32              i;
    rpc_protseq_id_elt_p_t  rpc_protseq;
    
    
    for (i = 0; i < RPC_C_PROTSEQ_ID_MAX; i++)
    {
        rpc_protseq = (rpc_protseq_id_elt_p_t) &(rpc_g_protseq_id[i]);

        if ((rpc_protseq->rpc_protocol_id == rpc_protocol->rpc_protocol_id) &&
            (RPC_NAF_INQ_SUPPORTED (RPC_PROTSEQ_INQ_NAF_ID (i))))
        {
            return (true);
        }
    }

    return (false);
}


/*
**++
**
**  ROUTINE NAME:       thread_context_destructor
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Called by the threads mechanism when a context has been
**  associated with a particular key. For RPC this routine will be
**  used to free the context associated with the
**  rpc_g_thread_context_key key. The context being freed is a
**  rpc_thread_context_t structure.
**
**  INPUTS:
**
**      ctx_value       pointer to an rpc_thread_context_t
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void thread_context_destructor 
(
    rpc_thread_context_p_t      ctx_value
)
{
    RPC_MEM_FREE (ctx_value, RPC_C_MEM_THREAD_CONTEXT);
}

#ifndef NO_GETENV

/*
**++
**
**  ROUTINE NAME:       init_getenv_debug
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Read the RPC_DEBUG enviroment variable to set some debug switches.
**  
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       
**
**      Sets RPC debugging switches.
**
**--
**/

INTERNAL void init_getenv_debug (void)
{
    unsigned32  status;
    char        *env_name;

    env_name = (char *) getenv ("RPC_DEBUG");
    if (env_name == NULL)
    {
        return;
    }

    rpc__dbg_set_switches (env_name, &status);
    if (status != rpc_s_ok)
    {
        dce_error_string_t error_text;
        int temp_status;

        dce_error_inq_text(status, (unsigned char*) error_text, &temp_status);

	/*
	 * rpc_m_call_failed
	 * "%s failed: %s"
	 */
	RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s%x"),
	    rpc_svc_general,
	    svc_c_sev_error,
	    rpc_m_call_failed,
	    "init_getenv_debug",
	    error_text ));
    }
}


/*
**++
**
**  ROUTINE NAME:       init_getenv_protseqs
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Read the RPC_SUPPORTED_PROTSEQS enviroment variable to determine
**  whether we're restricting the protocol sequence we'll use.  The value
**  of the environment variable is a ":"-separated list of protocol
**  sequence ID strings.  Update the protseq table to eliminate any not
**  specified in the environment variable.
**
**  Depends on protocol sequence table having been set up so that we
**  can convert protocol sequence strings to ID via rpc__network_pseq_id_from_pseq().
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:   
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void init_getenv_protseqs (void)

{
    unsigned16          i, j;
    unsigned32          status;
    char                *env_name, *s;
    unsigned_char_t     protseq_string[RPC_C_PROTSEQ_MAX], *sp;
    rpc_protseq_id_t    protseq_id;
    rpc_protseq_id_t    protseq_id_vec[RPC_C_PROTSEQ_ID_MAX];
    unsigned16          protseq_count;


    /*
     * See if environment variable exists.  If it doesn't, we're done now.
     */

    env_name = (char *) getenv ("RPC_SUPPORTED_PROTSEQS");
    if (env_name == NULL)
    {
        return;
    }

    /*
     * Parse the value of the environment variable into a list of protocol
     * sequence IDs.
     */

    protseq_count = 0;
    s = env_name;
    sp = protseq_string;

    do
    {
        if (*s == '\0' || *s == ':')
        {
            *sp = '\0';
            protseq_id = rpc__network_pseq_id_from_pseq (protseq_string, &status);
            if (status == rpc_s_ok)
            {
                protseq_id_vec[protseq_count++] = protseq_id;
            }
            sp = protseq_string;
        }
        else
        {
            *sp++ = *s;
        }
    } while (*s++ != '\0' &&
             protseq_count < RPC_C_PROTSEQ_ID_MAX &&
             (sp - protseq_string) < RPC_C_PROTSEQ_MAX);

    /*
     * If no valid protocol sequence was specified, we're done now.
     */

    if (protseq_count == 0)
    {   
        return;
    }

    /*
     * Loop through all the supported protocol sequences.  Make any of them
     * that aren't in our list become UNsupported.
     */

    for (j = 0; j < RPC_C_PROTSEQ_ID_MAX; j++)
    {
        rpc_protseq_id_elt_p_t  protseq = (rpc_protseq_id_elt_p_t) &(rpc_g_protseq_id[j]);

        if (protseq->supported)
        {
            for (i = 0; 
                 i < protseq_count && protseq_id_vec[i] != protseq->rpc_protseq_id; 
                 i++)
                ;
    
            if (i >= protseq_count)
            {
                protseq->supported = UNSUPPORTED;
            }
        }
    }
}


/*
**++
**
**  ROUTINE NAME:       init_getenv_port_restriction
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**      
**  Read the RPC_RESTRICTED_PORTS environment variable to determine whether
**  we're restricting the ports for a (set of) protocol sequence.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:   
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void init_getenv_port_restriction (void)
{

    unsigned_char_p_t   env_name;
    unsigned32          status;

    /*
     * See if environment variable exists.  If it doesn't, we're done now.
     */

    env_name = (unsigned_char_p_t) getenv ("RPC_RESTRICTED_PORTS");
    if (env_name != NULL)
    {
        rpc__set_port_restriction_from_string (env_name, &status);
    }
}                                       /* init_getenv_protseqs */

#endif  /* ifndef NO_GETENV */

/*
**++
**
**  ROUTINE NAME:       rpc__set_port_restriction_from_string
**
**  SCOPE:              PRIVATE
**
**  DESCRIPTION:
**      
**  Parses an input string containing a port restriction list for one or
**  more protocol sequences.
**
**  Input grammar:
** 
**     <entry> [COLON <entry>]*
**     
**     <entry> : <protseq_name> LEFT-BRACKET <ranges> RIGHT-BRACKET
**    
**     <ranges>: <range> [COMMA <range>]*
**     
**     <range> : <endpoint-low> HYPHEN <endpoint-high>
**     
**     Example:
**     
**          ncacn_ip_tcp[5000-5110,5500-5521]:ncadg_ip_udp[6500-7000]
**     
**
**  INPUTS:             
**
**      input_string    String as described above.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**      status          rpc_s_ok
**                      rpc_s_invalid_rpc_protseq
**
**  IMPLICIT INPUTS:   
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/


PRIVATE void rpc__set_port_restriction_from_string
(
 unsigned_char_p_t  input_string,
 unsigned32         *status
)
{
    unsigned_char_p_t       buf = NULL;
    unsigned_char_p_t       p;
    unsigned32              max_ranges;
    unsigned_char_p_t       * from_vec = NULL;
    unsigned_char_p_t       * to_vec = NULL;
    unsigned_char_p_t       protseq_name;
    rpc_protseq_id_t        protseq_id;
    unsigned32              range_index;
    boolean                 found;
    boolean                 more_protseqs;
    boolean                 more_ranges;

    CODING_ERROR (status);

    /* 
     * We're going to do some serious gnawing on the input string, so 
     * make a copy.
     */
    
    buf = rpc__stralloc (input_string);

    /* 
     * rpc__naf_set_port_restriction takes two arrays of pointers to 
     * strings (low port & high port).  Make a pass through the input text 
     * to find how large these vectors must be.  Allocate the vectors.
     */

    max_ranges = 0;
    p = buf;

    while (*p)
    {
        if (*p == '-')
            max_ranges ++;
        p++;
    }                                   /* while *p */

    if (max_ranges == 0)
    {
        *status = rpc_s_invalid_rpc_protseq;
        goto cleanup_and_return;
    }

    RPC_MEM_ALLOC 
        (from_vec,
         unsigned_char_p_t *,
         max_ranges * sizeof (* from_vec),
         RPC_C_MEM_STRING,
         RPC_C_MEM_WAITOK);

    RPC_MEM_ALLOC 
        (to_vec,
         unsigned_char_p_t *,
         max_ranges * sizeof (* to_vec),
         RPC_C_MEM_STRING,
         RPC_C_MEM_WAITOK);

    /* 
     * Loop over protseqs in input string.
     */
    
    p = buf;

    protseq_name = p;
    more_protseqs = true;

    while (more_protseqs)
    {
        
        /* 
         * Loop to find end of protseq.
         */

        found = false;

        while (*p && !found)
        {
            if (*p == '[')
            {
                *p = '\0';
                found = true;
            }
            p++;
        }                               /* while *p */
        
        if (!found)
        {
            *status = rpc_s_invalid_rpc_protseq;
            goto cleanup_and_return;
        }

        protseq_id = 
            rpc__network_pseq_id_from_pseq (protseq_name, status);

        if (*status != rpc_s_ok)
            goto cleanup_and_return;

        /* 
         * Loop over ranges for this protseq.
         */

        more_ranges = true;
        range_index = 0;

        while (more_ranges)
        {
            /* 
             * Grab low port and null terminate.
             */

            from_vec [range_index] = p;

            /* 
             * Scan for end of low range.
             */

            found = false;
            while (*p && !found)
            {
                if (*p == '-')
                {
                    *p = '\0';
                    found = true;
                }
                p++;
            }                           /* while *p */

            if (!found)
            {
                *status = rpc_s_invalid_rpc_protseq;
                goto cleanup_and_return;
            }

            to_vec [range_index] = p;

            /* 
             * Scan for end of high range.
             */
            
            found = false;
            while (*p && !found)
            {
                if (*p == ']')
                {
                    *p = '\0';
                    more_ranges = false;
                    found = true;
                }

                else if (*p == ',')
                {
                    *p = '\0';
                    found = true;
                }
                p++;
            }                           /* while *p */
            
            if (!found)
            {
                *status = rpc_s_invalid_rpc_protseq;
                goto cleanup_and_return;
            }

            range_index ++;

        }                               /* while more ranges */

        /* 
         * Have everything for this protocol sequence.  
         */

        rpc__naf_set_port_restriction
            (protseq_id,
             range_index,
             from_vec,      
             to_vec,
             status);

        if (*status != rpc_s_ok)
            goto cleanup_and_return;
                 
        /* 
         * At this point we're at the physical end of the string, or at the 
         * beginning of the next protseq.
         */
        
        if (*p == ':')
        {
            more_protseqs = true;       
            p++;
            protseq_name = p;
        }
        else if (*p == '\0')
            more_protseqs = false;
        else
        {
            *status = rpc_s_invalid_rpc_protseq;
            goto cleanup_and_return;
        }
    }                                   /* while more_protseqs */
                   
    *status = rpc_s_ok;
    
cleanup_and_return:

    if (buf != NULL)
        rpc_string_free (&buf, status);

    if (from_vec != NULL)
        RPC_MEM_FREE (from_vec, RPC_C_MEM_STRING);

    if (to_vec != NULL)
        RPC_MEM_FREE (to_vec, RPC_C_MEM_STRING);
}


#ifdef ATFORK_SUPPORTED

/*
**++
**
**  ROUTINE NAME:       rpc__fork_handler
**
**  SCOPE:              PRIVATE - declared in cominit.h
**
**  DESCRIPTION:
**      
**  This routine is called prior to, and immediately after, forking
**  the process's address space.  The input argument specifies which
**  stage of the fork we're currently in.
**
**  INPUTS:             
**
**        stage         indicates the stage in the fork operation
**                      (prefork | postfork_parent | postfork_child)
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/       
PRIVATE void rpc__fork_handler
(
  rpc_fork_stage_id_t stage
)
{   
    unsigned32 ctr;
    rpc_protocol_id_elt_p_t rpc_protocol;

    /*
     * Pre-fork handlers are called in reverse order of init_once().
     * Post-fork handlers are called in same order.
     *
     * First, take care of stage-independent operations, such as 
     * calling into handlers that differentiate among the stages
     * themselves.
     *
     * Next, take care of any stage-specific activities for this
     * module..
     */

    switch ((int)stage)
    {
    case RPC_C_PREFORK:
        RPC_MUTEX_LOCK(rpc_in_fork_mutex);
        rpc__network_fork_handler(stage);
        /* each auth protocol */
        /* auth_info_cache */
        /*
         * go through the RPC protocol id table and call each protocol's
         * fork handler.
         */
        for (ctr = 0; ctr < RPC_C_PROTOCOL_ID_MAX; ctr++)
        {   
            rpc_protocol = (rpc_protocol_id_elt_p_t) &(rpc_g_protocol_id[ctr]);
            if (rpc_protocol->prot_fork_handler != NULL)
            {
                (*rpc_protocol->prot_fork_handler)(stage);
            }
        }
        /* each NAF */
        /* cthread */
        rpc__obj_fork_handler(stage);
        rpc__if_fork_handler(stage);
        rpc__timer_fork_handler(stage);
        rpc__list_fork_handler(stage);
        RPC_MUTEX_UNLOCK(rpc_in_fork_mutex);
        break;
    case RPC_C_POSTFORK_CHILD:  
        /*
         * Reset any debug switches.
         */       
#ifdef DEBUG
        if (!RPC_DBG(rpc_es_dbg_inherit, 1))
        {
            for (ctr = 0; ctr < RPC_C_DBG_SWITCHES; ctr++)
                rpc_g_dbg_switches[ctr] = 0; 
        }
#endif            
        /*
         * We want the rpc__init code to run in the new invokation.
         */
        rpc_g_initialized = false;
        memset((char *)&init_once_block, 0, sizeof(dcethread_oncectl));
        
        /*
         * Increment the global fork count.  For more info on the use
         * of this variable, see comp.c.
         */
        rpc_g_fork_count++;
        
        /* fall through */
    case RPC_C_POSTFORK_PARENT:
        rpc__list_fork_handler(stage);
        rpc__timer_fork_handler(stage);
        rpc__if_fork_handler(stage);
        rpc__obj_fork_handler(stage);
        /* cthread */
        /* each NAF */
        /*
         * go through the RPC protocol id table and call each protocol's
         * fork handler.
         */
        for (ctr = 0; ctr < RPC_C_PROTOCOL_ID_MAX; ctr++)
        {   
            rpc_protocol = (rpc_protocol_id_elt_p_t) &(rpc_g_protocol_id[ctr]);
            if (rpc_protocol->prot_fork_handler != NULL)
            {
                (*rpc_protocol->prot_fork_handler)(stage);
            }
        }
        /* auth_info_cache */
        /* each auth protocol */
        rpc__network_fork_handler(stage);
        break;
    }  
}
#endif


