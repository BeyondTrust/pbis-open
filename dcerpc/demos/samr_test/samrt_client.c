/*
 * SAMR test
 *
 * Jim Doyle, jrd@bu.edu, 09-05-1998
 * Stefan Metzmacher, metze@samba.org, 2008
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
#include "samrt.h"
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
		       rpc_if_handle_t, char *, char *, char *);

/*
 * usage()
 */

static void usage()
{
  printf("usage:   samrt [-h hostname] [-u] [-t] [-k]\n");
  printf("         -u:  use UDP protocol \n");
  printf("         -t:  use TCP protocol (default) \n");
  printf("         -v:  more verbosity\n");
  printf("         -k:  use gss_mskrb\n");
  printf("         -h:  specify host where RPC server lives \n");
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
  char mech[128] = "spnego";
  char level[128] = "connect";
  char rpc_host[128] = "localhost";
  char * protocol;

  /*
   * stuff needed to make RPC calls
   */

  unsigned32 status;
  rpc_binding_handle_t     samr_server;
  long ntstatus;
  void *handle = NULL;
  unsigned32 authn_protocol;
  void *mech_ctx;
  
  /*
   * Process the cmd line args
   */
  
    while ((c = getopt(argc, argv, "h:l:m:utvv:")) != EOF)
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
	 case 'm':
	   strncpy(mech, optarg, sizeof(level)-1);
	   break;
	 case 'l':
	   strncpy(level, optarg, sizeof(level)-1);
	   break;
   	 case 'h':
	   strncpy(rpc_host, optarg, sizeof(rpc_host)-1);
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

  if (get_client_rpc_binding(&samr_server, 
		      rpc_host, 
		      samrt_v1_0_c_ifspec, 
		      protocol, mech, level) == 0)
    {
      printf ("Couldnt obtain RPC server binding. exiting.\n");
      exit(1);
    }


  /*
   * Do the RPC call
   */

  printf ("calling server\n");
  DCETHREAD_TRY
    {
      ntstatus = samrt_Connect(samr_server, NULL, 0, &handle);
    }
  DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
      printf ("Exception caught from samrt_Connect\n");
    }
  DCETHREAD_ENDTRY



  /*
   * Print the results
   */

  if (ntstatus == 0)
    {
      printf ("got response from server. results: \n");
      printf("\tntstatus = %u\n", (unsigned int)(ntstatus));
      printf("\n===================================\n");

    }

  if (ntstatus != 0)
      chk_dce_err(ntstatus, "samrt_Connet()", "main()", 1);

  mech_ctx = NULL;
  rpc_binding_inq_security_context(samr_server, &authn_protocol, &mech_ctx,
				   &status);

  /*
   * Done. Now gracefully teardown the RPC binding to the server
   */

  rpc_binding_free(&samr_server, &status);
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
get_client_rpc_binding(binding_handle, hostname, interface_spec, protocol, mech, level)
     rpc_binding_handle_t * binding_handle;
     char * hostname;
     rpc_if_handle_t interface_spec;
     char * protocol;
     char * mech;
     char * level;
{
  char * resolved_binding;
  char * printable_uuid ATTRIBUTE_UNUSED;
  char * protocol_family;
  char partial_string_binding[128];
  char server_principal[128];
  rpc_if_id_t nonkeyword_interface ATTRIBUTE_UNUSED;
  dce_uuid_t ifc_uuid ATTRIBUTE_UNUSED;
  error_status_t status;
  unsigned32 authn_protocol = rpc_c_authn_gss_negotiate;
  unsigned32 authn_level = rpc_c_authn_level_connect;

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

/*TODO*/  sprintf(server_principal, "host/%s",
	  hostname);

  if (strcmp(mech, "spnego") == 0)
      authn_protocol = rpc_c_authn_gss_negotiate;
  else if (strcmp(mech, "krb5") == 0)
      authn_protocol = rpc_c_authn_gss_mskrb;
  else {
	printf("invalid mech[%s]\n", mech);
	exit(1);
  }

  if (strcmp(level, "connect") == 0)
      authn_level = rpc_c_authn_level_connect;
  else if (strcmp(level, "sign") == 0)
      authn_level = rpc_c_protect_level_pkt_integ;
  else if (strcmp(level, "seal") == 0)
      authn_level = rpc_c_protect_level_pkt_privacy;
  else {
	printf("invalid level[%s]\n", level);
	exit(1);
  }

  rpc_binding_set_auth_info(*binding_handle,
			    server_principal,
			    authn_level,
			    authn_protocol,
			    NULL,
			    rpc_c_authz_name,
			    &status);
  chk_dce_err(status, "rpc_binding_set_auth_info()", "get_client_rpc_binding", 1);

/*
 * Get a printable rendition of the binding handle and echo to
 * the user.
 */

  rpc_binding_to_string_binding(*binding_handle,
				(unsigned char **)&resolved_binding,
				&status);
        chk_dce_err(status, "binding2string()", "get_client_rpc_binding", 1);

  printf("fully resolving binding for server is: %s (mech[%s],level[%s])\n",
	resolved_binding, mech, level);


  return 1;
}
