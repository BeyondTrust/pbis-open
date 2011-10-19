/*
 * echo_client  : demo DCE RPC application
 *
 * Jim Doyle, jrd@bu.edu, 09-05-1998
 *
 *
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#define getopt getopt_system

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <compat/dcerpc.h>
#include "echon.h"
#include <misc.h>

#undef getopt

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#define MAX_USER_INPUT 128
#define MAX_LINE 128

/*
 * Forward declarations
 */

static int
get_client_rpc_binding(rpc_binding_handle_t *, char *, 
		       rpc_if_handle_t, char *);

/*
 * usage()
 */

static void usage()
{
  printf("usage:   echon_client [-h hostname] [-u] [-t] [-i number]\n");
  printf("         -u:  use UDP protocol \n");
  printf("         -t:  use TCP protocol (default) \n");
  printf("         -v:  more verbosity\n");
  printf("         -h:  specify host where RPC server lives \n");
  printf("         -i:  unsigned int to be sent (5 - if not specified)\n\n");
  exit(0);
}

int
main(argc, argv)
 int argc;
 char *argv[];
{

  /* 
   * command line processing and options stuff
   */

  extern char *optarg;
  extern int optind, opterr, optopt;
  int c;

  int verbose = 1;
  int use_udp = 0;
  int use_tcp = 0;
  char rpc_host[128] = "localhost";
  char * protocol;

  /*
   * stuff needed to make RPC calls
   */

  unsigned32 status;
  rpc_binding_handle_t     echo_server;
  int ok;
  unsigned32 in_num = 5;
  unsigned32 out_num;
  
  /*
   * Process the cmd line args
   */
  
    while ((c = getopt(argc, argv, "h:utvi:")) != EOF)
    {
      switch (c)
	{
	 case 'u':
	   use_udp = 1;
	   break;
	 case 't':
	   use_tcp = 1;
	   break;
	 case 'v':
	   verbose = 0;
	   break;
   	 case 'h':
	   strncpy(rpc_host, optarg, sizeof(rpc_host)-1);
	   break;
   	 case 'i':
	   in_num = (unsigned int) atoi(optarg);
	   break;
	 default:
	   usage();
	}
    }

  if (!use_tcp && !use_udp) use_tcp=1;

  if (use_udp) 
    protocol = "udp";
  else
    protocol = "tcp";

  /*
   * Get a binding handle to the server using the following params:
   *
   *  1. the hostname where the server lives
   *  2. the interface description structure of the IDL interface
   *  3. the desired transport protocol (UDP or TCP)
   */

  if (get_client_rpc_binding(&echo_server, 
		      rpc_host, 
		      echon_v1_0_c_ifspec, 
		      protocol) == 0)
    {
      printf ("Couldnt obtain RPC server binding. exiting.\n");
      exit(1);
    }


  /*
   * Do the RPC call
   */

  printf ("calling server\n");
  ok = ReplyBack(echo_server, in_num, &out_num, &status);

  /*
   * Print the results
   */

  if (ok && status == error_status_ok)
    {
      printf ("got response from server. results: \n");
      printf("\tout_num = %u\n", (unsigned int)(out_num));
      printf("\n===================================\n");

    }

  if (status != error_status_ok)
      chk_dce_err(status, "ReverseIt()", "main()", 1);

  /*
   * Done. Now gracefully teardown the RPC binding to the server
   */

  rpc_binding_free(&echo_server, &status);
  exit(0);
  
}

/*==========================================================================
 *
 * get_client_rpc_binding()
 *
 *==========================================================================
 *
 * Gets a binding handle to an RPC interface.
 *
 * parameters:
 *
 *    [in/out]  binding_handle
 *    [in]      hostname       <- Internet hostname where server lives
 *    [in]      interface_uuid <- DCE Interface UUID for service
 *    [in]      protocol       <- "udp", "tcp" or "any"
 *
 *==========================================================================*/

static int
get_client_rpc_binding(binding_handle, hostname, interface_spec, protocol)
     rpc_binding_handle_t * binding_handle;
     char * hostname;
     rpc_if_handle_t interface_spec;
     char * protocol;
{
  char * resolved_binding;
  char * printable_uuid ATTRIBUTE_UNUSED;
  char * protocol_family;
  char partial_string_binding[128];
  rpc_if_id_t nonkeyword_interface ATTRIBUTE_UNUSED;
  dce_uuid_t ifc_uuid ATTRIBUTE_UNUSED;
  error_status_t status;

  /*
   * create a string binding given the command line parameters and
   * resolve it into a full binding handle using the endpoint mapper.
   *  The binding handle resolution is handled by the runtime library
   */


  if (strcmp(protocol, "udp")==0)
    protocol_family = "ncadg_ip_udp";
  else
    protocol_family = "ncacn_ip_tcp";


  sprintf(partial_string_binding, "%s:%s[]", 
	  protocol_family,
	  hostname);

  rpc_binding_from_string_binding((unsigned char *)partial_string_binding,
				  binding_handle,
				  &status);
      chk_dce_err(status, "string2binding()", "get_client_rpc_binding", 1);
  
  /*
   * Resolve the partial binding handle using the endpoint mapper
   */

  rpc_ep_resolve_binding(*binding_handle,
			 interface_spec,
			 &status);
  chk_dce_err(status, "rpc_ep_resolve_binding()", "get_client_rpc_binding", 1);

/*
 * Get a printable rendition of the binding handle and echo to
 * the user.
 */

  rpc_binding_to_string_binding(*binding_handle,
				(unsigned char **)&resolved_binding,
				&status);
        chk_dce_err(status, "binding2string()", "get_client_rpc_binding", 1);

  printf("fully resolving binding for server is: %s\n", resolved_binding);


  return 1;
}
