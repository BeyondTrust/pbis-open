/*
 * SAMR test
 *
 * Jim Doyle, jrd@bu.edu  09-05-1998
 * Stefan Metzmacher, metze@samba.org, 2008
 *
 *
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <compat/dcerpc.h>
#include "samrt.h"
#include "misc.h"

#ifndef _WIN32
static void wait_for_signals();
#endif

/*
 *
 * A template DCE RPC server
 *
 * main() contains the basic calls needed to register an interface,
 * get communications endpoints, and register the endpoints
 * with the endpoint mapper.
 *
 * ReverseIt() implements the interface specified in echo.idl
 *
 */


int main(int ac ATTRIBUTE_UNUSED, char *av[] ATTRIBUTE_UNUSED)
{
  unsigned32 status;
  rpc_binding_vector_p_t     server_binding;
  char * string_binding;
  unsigned32 i;

  /*
   * Register the Interface with the local endpoint mapper (rpcd)
   */

  printf ("Registering server.... \n");
  rpc_server_register_if(samrt_v1_0_s_ifspec, 
			 NULL,
			 NULL,
			 &status);
      chk_dce_err(status, "rpc_server_register_if()", "", 1);

      printf("registered.\nPreparing binding handle...\n");
      
      rpc_server_use_protseq("ncacn_ip_tcp", rpc_c_protseq_max_calls_default, &status);
	
      chk_dce_err(status, "rpc_server_use_all_protseqs()", "", 1);
      rpc_server_inq_bindings(&server_binding, &status);
      chk_dce_err(status, "rpc_server_inq_bindings()", "", 1);

      /*
       * Register bindings with the endpoint mapper
       */

	printf("registering bindings with endpoint mapper\n");
		
  rpc_ep_register(samrt_v1_0_s_ifspec,
		  server_binding,
		  NULL,
		  (unsigned char *)"QDA application server",
		  &status);
      chk_dce_err(status, "rpc_ep_register()", "", 1);

	printf("registered.\n");

      /*
       * Print out the servers endpoints (TCP and UDP port numbers)
       */

  printf ("Server's communications endpoints are:\n");
 
    for (i=0; i<RPC_FIELD_COUNT(server_binding); i++)
    {
        rpc_binding_to_string_binding(RPC_FIELD_BINDING_H(server_binding)[i],
				    (unsigned char **)&string_binding,
				    &status
				    );
      if (string_binding)
		printf("\t%s\n",string_binding);
    }


#ifndef _WIN32
  /*
   * Start the signal waiting thread in background. This thread will
   * Catch SIGINT and gracefully shutdown the server.
   */

  wait_for_signals();
#endif

  /*
   * Begin listening for calls
   */

  printf ("listening for calls.... \n");

  DCETHREAD_TRY
    {
      rpc_server_listen(rpc_c_listen_max_calls_default, &status);
    }
  DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
      printf ("Server stoppped listening\n");
    }
  DCETHREAD_ENDTRY

    /*
     * If we reached this point, then the server was stopped, most likely
     * by the signal handler thread called rpc_mgmt_stop_server().
     * gracefully cleanup and unregister the bindings from the 
     * endpoint mapper. 
     */

#ifndef _WIN32
    /*
     * Kill the signal handling thread
     */

#endif

  printf ("Unregistering server from the endpoint mapper.... \n");
  rpc_ep_unregister(samrt_v1_0_s_ifspec,
		    server_binding,
		    NULL,
		    &status);
  chk_dce_err(status, "rpc_ep_unregister()", "", 0);

  /*
   * retire the binding information
   */

  printf("Cleaning up communications endpoints... \n");
  rpc_server_unregister_if(samrt_v1_0_s_ifspec,
			   NULL,
			   &status);
  chk_dce_err(status, "rpc_server_unregister_if()", "", 0);

  exit(0);

}


/*=========================================================================
 *
 * Server implementation of ReverseIt()
 *
 *=========================================================================*/

unsigned32
samrt_Connect(h, system_name, access_mask, handle)
     handle_t h;
     unsigned16 *system_name;
     unsigned32 access_mask;
     void **handle;
{

  char * binding_info;
  error_status_t e;
  unsigned32 authn_protocol;
  void *mech_ctx;

  /*
   * Get some info about the client binding
   */

  rpc_binding_to_string_binding(h, (unsigned char **)&binding_info, &e);
  if (e == rpc_s_ok)
    {
      printf ("samrt_Connect() called by client: %s\n", binding_info);
    }

  mech_ctx = NULL;
  rpc_binding_inq_security_context(h, &authn_protocol, &mech_ctx, &e);

  printf("\n\nFunction samrt_Connect() -- input argments\n");
  
  printf("\taccess_mask = %ld\n", access_mask);

  printf ("\n=========================================\n");

  *handle = NULL;
  return 0;

}


#ifndef _WIN32
/*=========================================================================
 *
 * wait_for_signals()
 *
 *
 * Set up the process environment to properly deal with signals.
 * By default, we isolate all threads from receiving asynchronous
 * signals. We create a thread that handles all async signals. 
 * The signal handling actions are handled in the handler thread.
 *
 * For AIX, we cant use a thread that sigwaits() on a specific signal,
 * we use a plain old, lame old Unix signal handler.
 *
 *=========================================================================*/

void
wait_for_signals()
{
    sigset_t signals;

    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);

    dcethread_signal_to_interrupt(&signals, dcethread_self());
}

#endif
