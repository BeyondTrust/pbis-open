/*
 * 
 * (c) Copyright 1991 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1991 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1991 DIGITAL EQUIPMENT CORPORATION
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
**      cs_s_eval.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) evaluation setup function
**
**  ABSTRACT:
**
**      PUBLIC RPC 
**      by 
**
*/

#include <commonp.h>         /* RPC common definitions                    */
#include <com.h>             /* RPC communication definitons              */
#include <comp.h>            /* Private Communication Services            */
#include <ns.h>              /* Private NS defs for other RPC components  */
#include <nsp.h>             /* Private defs for Naming Service component */
#include <nsentry.h>         /* Externals for NS Entry sub-component      */
#include <dce/rpc.h>

#include <cs_s.h>	     /* Private defs for code set interoperability */
#include <dce/dce_cf.h>	     /* Access to the backing store library */


/*
**++
**  ROUTINE NAME:           rpc_ns_import_ctx_add_eval
**
**  SCOPE:                  PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  Add an evaluation routine (function pointer) to a list of an import context.
**  If the list does not exit, allocate the list first.
**
**  INPUTS:
**
**      function type	    Evaluation function type.  Currently, only types
**			    supported are 'RPC_EVAL_TYPE_CODESETS' and
**			    'RPC_CUSTOM_EVAL_TYPE_CODESETS'.
**
**      args		    Arguments to the custom evaluation routine.
**                          For OSF supplied evaluation routine, this is NULL.
**
**	evaluation function 
**			    Function pointer to an evaluation routine.
**			    Currently, only evaluation expected is code set
**			    compatibility evaluations.
**
**	free function 
**			    Function pointer to a free routine.  This is
**			    a user supplied routine, which is called when 
**			    'function type' is 'rpc_custom_eval_type_codesets'.
**
**      
**      
**  INPUT/OUPUTS:
**
**	import context	    Import context which is used for finding a server.
**			    This is allocated by the previous 
**			    rpc_ns_binding_import_begin() call. 'eval_routines'
**			    (which is type rpc_ns_handle_t) will hold the
**			    information about evaluation routines.
**
**  OUTPUTS:
**
**      status              The result of the operation. One of:
**                              rpc_s_ok
**				rpc_s_invalid_ns_handle
**                              rpc_s_no_memory
**                          Status from rpc_rgy_get_codesets()
**
**  IMPLICIT INPUTS:        none
**
**  IMPLICIT OUTPUTS:       none
**
**  FUNCTION VALUE:         void
**
**  SIDE EFFECTS:           none
**
**--
*/

PUBLIC 
void rpc_ns_import_ctx_add_eval
(
	rpc_ns_handle_t		*import_ctx,
	unsigned32		func_type,
	void			*args,
	void			(*eval_func)(handle_t binding_h, void *args, void **cntx),
	void			(*cs_free_func)(void *cntx),
	error_status_t		*status
)
{
	rpc_cs_eval_func_p_t		eval_func_rep;
	rpc_cs_eval_list_p_t		eval_list_p;
	rpc_import_rep_p_t		import_p;
	rpc_lkup_rep_p_t		lookup_p;
	unsigned_char_p_t		client_codesets_file_p;
	rpc_codeset_mgmt_p_t		client_codeset_p;


	CODING_ERROR (status);
	RPC_NS_VERIFY_INIT();

	import_p = (rpc_import_rep_p_t)*import_ctx;
	lookup_p = (rpc_lkup_rep_p_t)import_p->lookup_context;

	if (lookup_p == NULL)
	{
		*status = rpc_s_invalid_ns_handle;
		return;
	}

	switch (func_type)
	{
	case RPC_EVAL_TYPE_CODESETS:
	case RPC_CUSTOM_EVAL_TYPE_CODESETS:

		/*
		 * Check if import func context is already allocated
		 */

		if (lookup_p->eval_routines == NULL)	/* no list exists yet */
		{
			/*
			 * Allocate the new import func context.
			 */
			RPC_MEM_ALLOC (
			eval_func_rep,
			rpc_cs_eval_func_p_t,
			sizeof (rpc_cs_eval_func_t),
			RPC_C_MEM_FUNC,
			RPC_C_MEM_WAITOK);
	

			/*
			 * Allocate a list
			 */
			RPC_MEM_ALLOC (
				eval_list_p,
				rpc_cs_eval_list_p_t,
				sizeof (rpc_cs_eval_list_t),
				RPC_C_MEM_LIST,
				RPC_C_MEM_WAITOK);
	
			/*
			 * set up the contents of the stack
			 */
			eval_list_p->type = func_type;
			eval_list_p->eval_func = eval_func;
			eval_list_p->cs_free_func = cs_free_func;
			eval_list_p->cntx = NULL;
			eval_list_p->next = NULL;
	
			/* Get client's supported code sets */
			rpc_rgy_get_codesets (  &client_codeset_p,
						status );
	
			if (*status != rpc_s_ok)
			{
				RPC_MEM_FREE (eval_func_rep, RPC_C_MEM_FUNC);
				RPC_MEM_FREE (eval_list_p, RPC_C_MEM_LIST);
				return;
			}
			eval_list_p->args = (void *)client_codeset_p;
	
	
			/*
			 * set the list to import func context
			 */
			eval_func_rep->list = eval_list_p;
	
			eval_func_rep->num = 1;
	
			/*
			 * set the list into import context
			 */
			lookup_p->eval_routines = (rpc_ns_handle_t)eval_func_rep;
	
		}
		else
		{
	
			/*
			 * Allocate a list
			 */
			RPC_MEM_ALLOC (
				eval_list_p,
				rpc_cs_eval_list_p_t,
				sizeof (rpc_cs_eval_list_t),
				RPC_C_MEM_LIST,
				RPC_C_MEM_WAITOK);
	
			/*
			 * set up the contents of the stack
			 */
			eval_list_p->type = func_type;
			eval_list_p->eval_func = eval_func;
			eval_list_p->cs_free_func = cs_free_func;
			eval_list_p->cntx = NULL;
			eval_list_p->next = NULL;
	
			/* Get client's supported code sets */
			rpc_rgy_get_codesets (	&client_codeset_p,
						status );
	
			if (*status != rpc_s_ok)
			{
				RPC_MEM_FREE (eval_func_rep, RPC_C_MEM_FUNC);
				RPC_MEM_FREE (eval_list_p, RPC_C_MEM_LIST);
				return;
			}
			eval_list_p->args = (rpc_ns_handle_t *)client_codeset_p;
	
			/*
			 * set the stack pointer to newly allocated stack
			 */
			eval_func_rep = (rpc_cs_eval_func_p_t)lookup_p->eval_routines; 
			eval_func_rep->list->next = eval_list_p;
	
			eval_func_rep->num += 1;
		}
		*status = rpc_s_ok;
		return;

	default:
		;

	}
}



/*
**++
**  ROUTINE NAME:           rpc_cs_eval_with_universal
**
**  SCOPE:                  PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  An evaluation routine to evaluate client's and server's supported
**  code sets.  If no code sets match, Universal code set will be used
**  for the wire communication.
**
**  INPUTS:
**
**	binding_h
**			RPC binding_handle.
**
**	args 		Actually points to 'rpc_cs_codeset_i14y_data_p'
**			data type.
**      
**  INPUT/OUPUTS:
**
**	cntx 		Points to 'rpc_cs_codeset_i14y_data_p' data type,
**			and keep track of function execution.
**
**  OUTPUTS: none
**
**  IMPLICIT INPUTS:        none
**
**  IMPLICIT OUTPUTS:       none
**
**  FUNCTION VALUE:         void
**
**  SIDE EFFECTS:           none
**
**--
*/

PUBLIC 
void rpc_cs_eval_with_universal
(
	handle_t		binding_h,
	void			*args,
	void			**cntx
)
{
	rpc_cs_codeset_i14y_data_p	i14y_data_p;	
	rpc_cs_codeset_i14y_data_p	cntx_i14y_data_p;	
	rpc_cs_method_eval_p_t		method_p;
	rpc_ns_handle_t			inq_context;
	unsigned_char_p_t		client_codesets_file;
	unsigned_char_p_t		ns_name_p;
	int				i,j;
	int				model_found;
	int				smir_true;
	int				cmir_true;
	long				i_code;
	int				i_max_bytes;
	error_status_t			temp_status;



	i14y_data_p = (rpc_cs_codeset_i14y_data_p)args;

	if (i14y_data_p->cleanup)
	{
		cntx_i14y_data_p = (rpc_cs_codeset_i14y_data_p)*cntx;
		cntx_i14y_data_p->status = rpc_s_ok;
		return;
	}

	method_p = i14y_data_p->method_p;
	if (cntx == NULL)
	{
			return;
	}
	else
		cntx_i14y_data_p = (rpc_cs_codeset_i14y_data_p)*cntx;

	/* 
	 * Get the client's supported code sets.
	 */
	if (method_p->client == NULL)
	{
		rpc_rgy_get_codesets ( 
			&method_p->client,
			&cntx_i14y_data_p->status);
		
		if (cntx_i14y_data_p->status != rpc_s_ok)
			return;
	}

	/* 
	** Get the server's supported code sets from NSI.
	*/

	ns_name_p = i14y_data_p->ns_name;

	rpc_ns_mgmt_read_codesets (
		rpc_c_ns_syntax_default,
		ns_name_p,
		&method_p->server,
		&cntx_i14y_data_p->status);

	if (cntx_i14y_data_p->status != rpc_s_ok)
	{
		rpc_ns_mgmt_free_codesets( &method_p->client, &temp_status);
		return;
	}

	/* 
	 * Start evaluation
	 */
	if (method_p->client->codesets[0].c_set 
			== method_p->server->codesets[0].c_set)
	{
		/* 
		 * Both client and server are using the same code set 
		 */
		method_p->method = RPC_EVAL_NO_CONVERSION;
		method_p->tags.stag = method_p->client->codesets[0].c_set;
		method_p->tags.drtag = method_p->server->codesets[0].c_set;
	}
	else
	{
		/*
		 * We check character set compatibility first.
		 */
		rpc_cs_char_set_compat_check (
			method_p->client->codesets[0].c_set,
			method_p->server->codesets[0].c_set,
			&cntx_i14y_data_p->status);
		
		if (cntx_i14y_data_p->status != rpc_s_ok)
		{
			/* 
			 * Character set for client and server didn't match.
			 * Mass of data loss could result, so we quit the 
			 * evaluation here. 
			 */
			rpc_ns_mgmt_free_codesets( &method_p->server, &temp_status);
			return;
		}
		else
		{
			smir_true = cmir_true = model_found = 0;

			for (i = 1; i <= method_p->server->count; i++)
			{
			   if (model_found)
				break;
	
			   if (method_p->client->codesets[0].c_set 
				== method_p->server->codesets[i].c_set)
			   {
				smir_true = 1;
				model_found = 1;
			   }

			   if (method_p->server->codesets[0].c_set 
				== method_p->client->codesets[i].c_set)
			   {
				cmir_true = 1;
				model_found = 1;
			   }
			}
	
			if (model_found)
			{
			   if (smir_true && cmir_true)
			   {
				/* RMIR model works */
				method_p->method = RPC_EVAL_RMIR_MODEL;
				method_p->tags.stag 
					= method_p->client->codesets[0].c_set;
				method_p->tags.drtag
				    	= method_p->server->codesets[0].c_set;
				method_p->tags.stag_max_bytes 
				    = method_p->client->codesets[0].c_max_bytes;
				method_p->tags.client_tag 
				    = method_p->client->codesets[0].c_set;
				method_p->tags.client_max_bytes
				    = method_p->client->codesets[0].c_max_bytes;
			   }
			   else if (smir_true)
			   {
				/* SMIR model */
				method_p->method = RPC_EVAL_SMIR_MODEL;
				method_p->tags.stag
				    	= method_p->client->codesets[0].c_set;
				method_p->tags.drtag
				     	= method_p->client->codesets[0].c_set;
				method_p->tags.stag_max_bytes 
				    = method_p->client->codesets[0].c_max_bytes;
				method_p->tags.client_tag 
				    = method_p->client->codesets[0].c_set;
				method_p->tags.client_max_bytes
				    = method_p->client->codesets[0].c_max_bytes;
			   }
			   else
			   {
				/* CMIR model */
				method_p->method = RPC_EVAL_CMIR_MODEL;
				method_p->tags.stag
					= method_p->server->codesets[0].c_set;
				method_p->tags.drtag
					= method_p->server->codesets[0].c_set;
				method_p->tags.stag_max_bytes 
				    = method_p->server->codesets[0].c_max_bytes;
				method_p->tags.client_tag 
				    = method_p->client->codesets[0].c_set;
				method_p->tags.client_max_bytes
				    = method_p->client->codesets[0].c_max_bytes;
			   }
			}
			else
			{	
				/*
				 * We try to find the intermediate code set
				 */
				method_p->tags.client_tag 
				    = method_p->client->codesets[0].c_set;
				method_p->tags.client_max_bytes
				    = method_p->client->codesets[0].c_max_bytes;

				for (i = 1; i <= method_p->client->count; i++)
				{
				   if (model_found)
					break;
			  	   for (j = 1; j <= method_p->server->count; j++)
					{
					   if (method_p->client->codesets[i].c_set
						== method_p->server->codesets[j].c_set)
					   {
						i_code = method_p->client->codesets[i].c_set;
						i_max_bytes = method_p->client->codesets[i].c_max_bytes;
						method_p->tags.stag_max_bytes 
						   = method_p->client->codesets[i].c_max_bytes;
						model_found = 1;
						   break;
				    	    }
					}
				}
				if (model_found)
				{
					method_p->method = RPC_EVAL_INTERMEDIATE_MODEL;
					method_p->tags.stag = i_code;
					method_p->tags.drtag = i_code;
			   	}
				else
				{
					/*
					 * We use UNIVERSAL code set
					 */
					method_p->method = RPC_EVAL_UNIVERSAL_MODEL;
					method_p->tags.stag = UCS2_L2;
					method_p->tags.drtag = UCS2_L2;
					method_p->tags.client_tag 
				    	   = method_p->client->codesets[0].c_set;
					method_p->tags.client_max_bytes
					    = method_p->client->codesets[0].c_max_bytes;
			    		rpc_rgy_get_max_bytes (
						UCS2_L2,
						&method_p->tags.stag_max_bytes,
						&cntx_i14y_data_p->status
					);
					if (cntx_i14y_data_p->status != rpc_s_ok)
					{
						rpc_ns_mgmt_free_codesets( &method_p->server, &temp_status);
						return;
					}
			   	}
			}
		}
	}

	method_p->fixed = ndr_true;
	cntx_i14y_data_p->status = rpc_s_ok;
	rpc_ns_mgmt_free_codesets( &method_p->server, &temp_status);
	return;
}



/*
**++
**  ROUTINE NAME:           rpc_cs_eval_without_universal
**
**  SCOPE:                  PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  An evaluation routine to evaluate client's and server's supported
**  code sets.  If no code sets match it fails.  Universal code set will 
**  not be used for the wire communication.
**
**  INPUTS:
**
**	binding_h
**			RPC binding_handle.
**
**	args 		Actually points to 'rpc_cs_codeset_i14y_data_p'
**			data type.
**      
**  INPUT/OUPUTS:
**
**	cntx 		Points to 'rpc_cs_codeset_i14y_data_p' data type,
**			and keep track of function execution.
**      
**  OUTPUTS: none
**
**  IMPLICIT INPUTS:        none
**
**  IMPLICIT OUTPUTS:       none
**
**  FUNCTION VALUE:         void
**
**  SIDE EFFECTS:           none
**
**--
*/

PUBLIC 
void rpc_cs_eval_without_universal
(
	handle_t		binding_h,
	void			*args,
	void			**cntx
)
{
	rpc_cs_codeset_i14y_data_p	i14y_data_p;	
	rpc_cs_codeset_i14y_data_p	cntx_i14y_data_p;	
	rpc_cs_method_eval_p_t		method_p;
	rpc_ns_handle_t			inq_context;
	unsigned_char_p_t		ns_name_p;
	unsigned_char_p_t		client_codesets_file;
	int				i,j;
	int				model_found;
	int				smir_true;
	int				cmir_true;
	long				i_code;
	int				i_max_bytes;
	error_status_t			temp_status;



	i14y_data_p = (rpc_cs_codeset_i14y_data_p)args;

	if (i14y_data_p->cleanup)
	{
		cntx_i14y_data_p = (rpc_cs_codeset_i14y_data_p)*cntx;
		cntx_i14y_data_p->status = rpc_s_ok;
		return;
	}

	method_p = i14y_data_p->method_p;
	if (cntx == NULL)
	{
		return;
	}
	else
		cntx_i14y_data_p = (rpc_cs_codeset_i14y_data_p)*cntx;


	/* 
	 * Get the client's supported code sets if it is not set.
	method_p->client = (rpc_codeset_mgmt_p_t)i14y_data_p->args;
	 */
	if (method_p->client == NULL)
	{
		rpc_rgy_get_codesets ( 
			&method_p->client,
			&cntx_i14y_data_p->status);
		
		if (cntx_i14y_data_p->status != rpc_s_ok)
			return;
	}

	/* 
	** Get the server's supported code sets from NSI.
	*/

	ns_name_p = i14y_data_p->ns_name;

	rpc_ns_mgmt_read_codesets (
		rpc_c_ns_syntax_default,
		ns_name_p,
		&method_p->server,
		&cntx_i14y_data_p->status);

	if (cntx_i14y_data_p->status != rpc_s_ok)
	{
		rpc_ns_mgmt_free_codesets( &method_p->client, &temp_status);
		return;
	}

	/* 
	 * Start evaluation
	 */
	if (method_p->client->codesets[0].c_set 
			== method_p->server->codesets[0].c_set)
	{
		/* 
		 * Both client and server are using the same code set 
		 */
		method_p->method = RPC_EVAL_NO_CONVERSION;
		method_p->tags.stag = method_p->client->codesets[0].c_set;
		method_p->tags.drtag = method_p->server->codesets[0].c_set;
	}
	else
	{
		/*
		 * We check character set compatibility first.
		 */
		rpc_cs_char_set_compat_check (
			method_p->client->codesets[0].c_set,
			method_p->server->codesets[0].c_set,
			&cntx_i14y_data_p->status);
		
		if (cntx_i14y_data_p->status != rpc_s_ok)
		{
			/* 
			 * Character set for client and server didn't match.
			 * Mass of data loss could result, so we quit the 
			 * evaluation here. 
			 */
			rpc_ns_mgmt_free_codesets( &method_p->server, &temp_status);
			return;
		}
		else
		{
			smir_true = cmir_true = model_found = 0;

			for (i = 1; i <= method_p->server->count; i++)
			{
			   if (model_found)
				break;

			   if (method_p->client->codesets[0].c_set 
				== method_p->server->codesets[i].c_set)
			   {
				smir_true = 1;
				model_found = 1;
			   }

			   if (method_p->server->codesets[0].c_set 
				== method_p->client->codesets[i].c_set)
			   {
				cmir_true = 1;
				model_found = 1;
			   }
			}

			if (model_found)
			{
			   if (smir_true && cmir_true)
			   {
				/* RMIR model works */
				method_p->method = RPC_EVAL_RMIR_MODEL;
				method_p->tags.stag = method_p->client->codesets[0].c_set;
				method_p->tags.drtag = method_p->server->codesets[0].c_set;
				method_p->tags.stag_max_bytes
				    = method_p->client->codesets[0].c_max_bytes;
				method_p->tags.client_tag 
				    = method_p->client->codesets[0].c_set;
				method_p->tags.client_max_bytes
				    = method_p->client->codesets[0].c_max_bytes;
			   }
			   else if (smir_true)
			   {
				/* SMIR model */
				method_p->method = RPC_EVAL_SMIR_MODEL;
				method_p->tags.stag = method_p->client->codesets[0].c_set;
				method_p->tags.drtag = method_p->client->codesets[0].c_set;
				method_p->tags.stag_max_bytes
				    = method_p->client->codesets[0].c_max_bytes;
				method_p->tags.client_tag 
				    = method_p->client->codesets[0].c_set;
				method_p->tags.client_max_bytes
				    = method_p->client->codesets[0].c_max_bytes;
			   }
			   else
			   {
				/* CMIR model */
				method_p->method = RPC_EVAL_CMIR_MODEL;
				method_p->tags.stag = method_p->server->codesets[0].c_set;
				method_p->tags.drtag = method_p->server->codesets[0].c_set;
				method_p->tags.stag_max_bytes
				    = method_p->server->codesets[0].c_max_bytes;
				method_p->tags.client_tag 
				    = method_p->client->codesets[0].c_set;
				method_p->tags.client_max_bytes
				    = method_p->client->codesets[0].c_max_bytes;
			   }
			}
			else
			{	
			   /*
			    * We try to find intermediate code set
			    */
			   method_p->tags.client_tag
				= method_p->client->codesets[0].c_set;
			   method_p->tags.client_max_bytes
				= method_p->client->codesets[0].c_max_bytes;

			   for (i = 1; i <= method_p->client->count; i++)
			   {
			   	if (model_found)
					break;
				for (j = 1; j <= method_p->server->count; j++)
			 	{
				   if (method_p->client->codesets[i].c_set
				   	== method_p->server->codesets[j].c_set)
				   {
				   	i_code = method_p->client->codesets[i].c_set;
					i_max_bytes = method_p->client->codesets[i].c_max_bytes;

					method_p->tags.stag_max_bytes
					    = method_p->client->codesets[i].c_max_bytes;
				 	model_found = 1;
				 	break;
				   }
				}
			   }
			   if (model_found)
			   {
				method_p->method 
					= RPC_EVAL_INTERMEDIATE_MODEL;
				method_p->tags.stag = i_code;
				method_p->tags.drtag = i_code;
			   }
			   else
			   {
				/*
				 * We do not use UNIVERSAL code set
				 */
				cntx_i14y_data_p->status 
						= rpc_s_ss_no_compat_codeset;
				rpc_ns_mgmt_free_codesets( &method_p->server, &temp_status);
				return;
			   }
			}
		}
	}
	
	method_p->fixed = ndr_true;
	cntx_i14y_data_p->status = rpc_s_ok;
	rpc_ns_mgmt_free_codesets( &method_p->server, &temp_status);
	return;
}



/*
**++
**  ROUTINE NAME:           rpc_cs_char_set_compat_check
**
**  SCOPE:                  PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  Evaluate the character set compatibility between client and server.
**  Character set matching logic prevents the massive data loss when client
**  and server are connected.
**
**  INPUTS:
**      
**	client_codeset 	    OSF code set registry value for client's current
**			    code set.
**
**	server_codeset 	    OSF code set registry value for server's current
**			    code set.
**
**  INPUT/OUPUTS:	    none
**
**  OUTPUTS:
**
**      status              The result of the operation. One of:
**                              rpc_s_ok
**                              rpc_s_ss_no_compat_charsets
**                          Status from dce_cs_loc_to_rgy()
**
**  IMPLICIT INPUTS:        none
**
**  IMPLICIT OUTPUTS:       none
**
**  FUNCTION VALUE:         void
**
**  SIDE EFFECTS:           none
**
**--
*/

PUBLIC 
void rpc_cs_char_set_compat_check
(
	unsigned32		client_codeset,
	unsigned32		server_codeset,
	error_status_t		*status
)
{
	unsigned_char_t	*client_code_set_name;
	unsigned16	client_char_sets_number;
	unsigned16	*client_char_sets_value;
	unsigned_char_t	*server_code_set_name;
	unsigned16	server_char_sets_number;
	unsigned16	*server_char_sets_value;

	dce_cs_rgy_to_loc (
		client_codeset,
		&client_code_set_name,
		&client_char_sets_number,
		&client_char_sets_value,
		status );

	if (*status != dce_cs_c_ok)
		return;
		
	dce_cs_rgy_to_loc (
		server_codeset,
		&server_code_set_name,
		&server_char_sets_number,
		&server_char_sets_value,
		status );

	if (*status != dce_cs_c_ok)
		return;

	if (client_char_sets_number == 1 && server_char_sets_number == 1)
	{
		if (*client_char_sets_value == *server_char_sets_value)
			*status = rpc_s_ok;
		else
			*status = rpc_s_ss_no_compat_charsets;
	}
	else
	{
		int		match = 0;
		unsigned16	server_number_save;
		unsigned16	*server_value_save;

		server_number_save = server_char_sets_number;
		server_value_save  = server_char_sets_value;

		while (client_char_sets_number--) 
		{
			while (server_char_sets_number--)
			{
				if (*client_char_sets_value 
						== *server_char_sets_value++)
					match++;
			}
			server_char_sets_number = server_number_save;
			server_char_sets_value  = server_value_save;
			client_char_sets_value++;
		}
		if (match >= 2)
			*status = rpc_s_ok;
		else
			*status = rpc_s_ss_no_compat_charsets;
	}

	return;
}


/*
**++
**  ROUTINE NAME:           rpc_cs_binding_set_method
**
**  SCOPE:                  PRIVATE - declared in cs_s.h
**
**  DESCRIPTION:
**
**  An evaluation routine to evaluate client's and server's supported
**  code sets.  If no code sets match, Universal code set will be used
**  for the wire communication.
**
**  INPUTS:
**	method_p	   pointer to the method structure.
**	
**      
**  INPUT/OUPUTS:
**
**	h 		    rpc binding handle.  rpc_binding_rep_t will be
**			    modified to rpc_binding_eval_t data structure.
**
**  OUTPUTS:
**
**      status              The result of the operation. One of:
**                              rpc_s_ok
**                              rpc_s_no_memory
**
**  IMPLICIT INPUTS:        none
**
**  IMPLICIT OUTPUTS:       none
**
**  FUNCTION VALUE:         void
**
**  SIDE EFFECTS:           none
**
**--
*/
PRIVATE
void rpc_cs_binding_set_method
(
	rpc_binding_handle_t		*h,
	rpc_cs_method_eval_p_t		method_p,
	error_status_t			*status
)
{
	rpc_cs_method_eval_p_t  bind_method;
	rpc_binding_rep_p_t	bind_eval_p;
	
	bind_eval_p = (rpc_binding_rep_p_t)*h;
	bind_method = &bind_eval_p->cs_eval.tagged_union.method_key;

	bind_eval_p->cs_eval.key = RPC_CS_EVAL_METHOD;
	bind_method->method = method_p->method;
	bind_method->tags.stag = method_p->tags.stag;
	bind_method->tags.drtag = method_p->tags.drtag;
	bind_method->tags.stag_max_bytes = method_p->tags.stag_max_bytes;
	bind_method->tags.client_tag = method_p->tags.client_tag;
	bind_method->tags.client_max_bytes = method_p->tags.client_max_bytes;
	bind_method->fixed = method_p->fixed;
	bind_method->cs_stub_eval_func = NULL;
	bind_method->tags.type_handle = NULL;
	bind_method->server = method_p->server;
	bind_method->client = method_p->client;

	bind_eval_p->extended_bind_flag = RPC_C_BH_EXTENDED_CODESETS;
        *status = rpc_s_ok;
        return;
}


/*
**++
**  ROUTINE NAME:           rpc_cs_binding_set_tags
**
**  SCOPE:                  PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  An evaluation routine to evaluate client's and server's supported
**  code sets.  If no code sets match, Universal code set will be used
**  for the wire communication.
**
**  INPUTS:
**	stag		   sending tag
**	
**	drtag		   desired receving tag
**      
**  INPUT/OUPUTS:
**
**	h 		    rpc binding handle.  rpc_binding_rep_t will be
**			    modified to rpc_binding_eval_t data structure.
**
**  OUTPUTS:
**
**      status              The result of the operation. One of:
**                              rpc_s_ok
**                          Status from rpc_rgy_get_codesets()
**
**  IMPLICIT INPUTS:        none
**
**  IMPLICIT OUTPUTS:       none
**
**  FUNCTION VALUE:         void
**
**  SIDE EFFECTS:           none
**
**--
*/

PUBLIC 
void rpc_cs_binding_set_tags
(
	rpc_binding_handle_t		*h,
	unsigned32			stag,
	unsigned32			drtag,
	unsigned16			stag_max_bytes,
	error_status_t			*status
)
{
	rpc_cs_tags_eval_p_t	bind_tags;
	rpc_binding_rep_p_t	bind_eval_p;
	rpc_codeset_mgmt_p_t	client;
	
	bind_eval_p = (rpc_binding_rep_p_t)*h;
	bind_tags = &bind_eval_p->cs_eval.tagged_union.tags_key;

	bind_eval_p->cs_eval.key = RPC_CS_EVAL_TAGS;
	bind_tags->stag = stag;
	bind_tags->drtag = drtag;
	if (stag_max_bytes != 0)
		bind_tags->stag_max_bytes = stag_max_bytes;
	else
		bind_tags->stag_max_bytes = 0;
	/*
	 * Get the client's supported code sets
	 */
	rpc_rgy_get_codesets (
		&client,
		status );

	if (*status != rpc_s_ok)
		return;

	bind_tags->client_tag = client->codesets[0].c_set;
	bind_tags->client_max_bytes = client->codesets[0].c_max_bytes;

	bind_tags->type_handle = NULL;

	bind_eval_p->extended_bind_flag = RPC_C_BH_EXTENDED_CODESETS;

        *status = rpc_s_ok;
        return;
}


/*
**++
**  ROUTINE NAME:           rpc_cs_binding_set_eval
**
**  SCOPE:                  PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**
**  Add in-stub evaluation routine to a binding handle.
**  When this method is used, every single RPC call will perform code set
**  compatibility evaluation, which can be a big impact to RPC call performance.
**
**  INPUTS:
**	cs_stub_eval_func   pointer to an in-stub evaluation routine.
**	
**  INPUT/OUPUTS:
**
**	h 		    rpc binding handle.  rpc_binding_rep_t will be
**			    modified to rpc_binding_eval_t data structure.
**
**  OUTPUTS:
**
**      status              The result of the operation. One of:
**                              rpc_s_ok
**
**  IMPLICIT INPUTS:        none
**
**  IMPLICIT OUTPUTS:       none
**
**  FUNCTION VALUE:         void
**
**  SIDE EFFECTS:           none
**
**--
*/

PUBLIC 
void rpc_cs_binding_set_eval
(
	rpc_binding_handle_t		*h,
	void				(*cs_stub_eval_func)(unsigned32 *p_stag, unsigned32 *p_drtag, error_status_t *status),
	error_status_t			*status
)
{
	rpc_cs_method_eval_p_t  bind_method;
	rpc_binding_rep_p_t	bind_eval_p;
	
	bind_eval_p = (rpc_binding_rep_p_t)*h;
	bind_method = &bind_eval_p->cs_eval.tagged_union.method_key;

	bind_eval_p->cs_eval.key = RPC_CS_EVAL_METHOD;
	bind_method->fixed = ndr_false;
	bind_method->cs_stub_eval_func = cs_stub_eval_func;
	bind_method->tags.type_handle = NULL;

	bind_eval_p->extended_bind_flag = RPC_C_BH_IN_STUB_EVALUATION;
        *status = rpc_s_ok;
        return;
}
