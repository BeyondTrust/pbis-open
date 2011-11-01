/*
 * 
 * (c) Copyright 1990 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1990 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1990 DIGITAL EQUIPMENT CORPORATION
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
**      comauth.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC) 
**
**  ABSTRACT:
**
**  Generic interface to authentication services
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comp.h>       /* Private communications services */
#include <comauth.h>    /* Common Authentication services */


/*
 * Internal variables to maintain the auth info cache.
 */
INTERNAL rpc_list_t     auth_info_cache;
INTERNAL rpc_mutex_t    auth_info_cache_mutex;    

/*
 * R P C _ _ A U T H _ I N F O _ C A C H E _ L K U P
 */
INTERNAL rpc_auth_info_t *rpc__auth_info_cache_lkup (
        unsigned_char_p_t                    /*server_princ_name*/,
        rpc_authn_level_t                    /*authn_level*/,
        rpc_auth_identity_handle_t           /*auth_identity*/,
        rpc_authz_protocol_id_t              /*authz_protocol*/,
        rpc_authn_protocol_id_t             /* authn_protocol*/,
        unsigned32                          /* authn_flags*/
    );

/*
 * R P C _ _ A U T H _ I N F O _ C A C H E _ A D D
 */

INTERNAL void rpc__auth_info_cache_add (
        rpc_auth_info_p_t                   /*auth_info*/
    );

/*
 * R P C _ _ A U T H _ I N F O _ C A C H E _ R E M O V E
 */
INTERNAL void rpc__auth_info_cache_remove (
        rpc_auth_info_p_t                   /*auth_info*/
    );



/*
 * Macro to assign through a pointer iff the pointer is non-NULL.  Note
 * that we depend on the fact that "val" is not evaluated if "ptr" is
 * NULL (i.e., this must be a macro, not a function).
 */
#define ASSIGN(ptr, val) \
{ \
    if ((ptr) != NULL) *(ptr) = (val); \
}

#define ASSIGN_COPY(buffer, length, val) do { \
        char* _val = (char*) (val);                             \
        unsigned32 _vallength = _val ? strlen(_val) : 0;        \
        if ((buffer) == NULL || (length) < _vallength) { \
            *st = rpc_s_ss_bad_buffer; \
            return; \
        } else { \
            if (_val != NULL)                                 \
                memcpy((buffer), _val, _vallength + 1);       \
            else \
                (buffer)[0] = '\0'; \
            (length) = _vallength; \
        } \
    } while (0)


/*
**++
**
**  ROUTINE NAME:       rpc__auth_inq_supported
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:   
**
**  Return a boolean indicating whether the authentication protocol
**  is supported by the runtime.
**      
**  INPUTS:
**
**      authn_prot_id           Authentication protocol ID
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     boolean32
**
**  true if supported
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE boolean32 rpc__auth_inq_supported 
(
  rpc_authn_protocol_id_t         authn_prot_id
)
{
    return (RPC_AUTHN_INQ_SUPPORTED(authn_prot_id));
}


/*
**++
**
**  ROUTINE NAME:       rpc__auth_cvt_id_api_to_wire
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:   
**
**  Return the wire value of an authentication protocol ID given its
**  API counterpart.
**      
**  INPUTS:
**
**      api_authn_prot_id       API Authentication protocol ID
**
**  INPUTS/OUTPUTS:
**
**  OUTPUTS:            none
**
**      status          A value indicating the return status of the routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     unsigned32
**
**  The wire Authentication protocol ID.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32 rpc__auth_cvt_id_api_to_wire 
(
  rpc_authn_protocol_id_t api_authn_prot_id,
  unsigned32              *status
)
{
    if (! RPC_AUTHN_IN_RANGE(api_authn_prot_id) || 
        ! RPC_AUTHN_INQ_SUPPORTED(api_authn_prot_id))
    {
        *status = rpc_s_unknown_auth_protocol;
        return (0xeffaced);
    }

    *status = rpc_s_ok;
    return (rpc_g_authn_protocol_id[api_authn_prot_id].dce_rpc_authn_protocol_id);
}


/*
**++
**
**  ROUTINE NAME:       rpc__auth_cvt_id_wire_to_api
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:   
**
**  Return the API value of an authentication protocol ID given its
**  wire counterpart.
**      
**  INPUTS:
**
**      wire_authn_prot_id      Wire Authentication protocol ID
**
**  INPUTS/OUTPUTS:
**
**  OUTPUTS:            
**
**      status          A value indicating the return status of the routine
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     unsigned32
**
**  The API Authentication protocol ID.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE rpc_authn_protocol_id_t rpc__auth_cvt_id_wire_to_api 
(
  unsigned32      wire_authn_prot_id,
  unsigned32      *status
)
{
    rpc_authn_protocol_id_t authn_protocol;

    for (authn_protocol = 0; 
         authn_protocol < RPC_C_AUTHN_PROTOCOL_ID_MAX; 
         authn_protocol++)
    {
        rpc_authn_protocol_id_elt_p_t aprot = &rpc_g_authn_protocol_id[authn_protocol];

        if (aprot->epv != NULL &&
            aprot->dce_rpc_authn_protocol_id == wire_authn_prot_id)
        {
            break;
        }
    }

    if (authn_protocol >= RPC_C_AUTHN_PROTOCOL_ID_MAX)
    {
        *status = rpc_s_unknown_auth_protocol;
        return ((rpc_authn_protocol_id_t)0xdeaddeadUL);
    }

    *status = rpc_s_ok;
    return (authn_protocol);
}


/*
**++
**
**  ROUTINE NAME:       rpc__auth_inq_rpc_prot_epv
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:   
**
**  Return the RPC protocol specific entry point vector for a given
**  Authentication protocol.
**      
**  INPUTS:
**
**      authn_prot_id   Authentication protocol ID
**      rpc_prot_id     RPC protocol ID
**
**  INPUTS/OUTPUTS:
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_prot_epv_tbl
**
**  The address of the RPC protocol specific, authentication
**  protocol specific entry point vector.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE rpc_auth_rpc_prot_epv_t *rpc__auth_rpc_prot_epv
(
  rpc_authn_protocol_id_t authn_prot_id,
  rpc_protocol_id_t       rpc_prot_id
)
{
    return (RPC_AUTHN_INQ_RPC_PROT_EPV(authn_prot_id,rpc_prot_id));
}


/*
**++
**
**  ROUTINE NAME:       rpc__auth_info_reference
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:   
**
**  Establish a reference to authentication info.
**      
**  INPUTS:
**
**      auth_info       Authentication information   
**
**  INPUTS/OUTPUTS:
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

PRIVATE void rpc__auth_info_reference
(
  rpc_auth_info_p_t   auth_info
)
{
#ifdef DEBUG
    char *info_type = auth_info->is_server?"server":"client";
#endif

    RPC_DBG_PRINTF(rpc_e_dbg_auth, 3, ("(rpc__auth_info_reference) %x: bumping %s refcount (was %d, now %d)\n",
        auth_info,
        info_type, auth_info->refcount,
        auth_info->refcount + 1));

/*    assert (auth_info->refcount >= 0);  XXX unsigned values always >= 0*/
    auth_info->refcount++;
}


/*
**++
**
**  ROUTINE NAME:       rpc__auth_info_binding_release  
**
**  SCOPE:              PRIVATE - declared in comauth.h
**
**  DESCRIPTION:   
**
**  Release reference to authentication info (stored in passed binding
**  handle) previously returned by set_server or inq_caller.  If we don't
**  have any auth info, do nothing.
**      
**  INPUTS:
**
**      binding_rep     RPC binding handle
**
**  INPUTS/OUTPUTS:
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

PRIVATE void rpc__auth_info_binding_release
(
  rpc_binding_rep_p_t     binding_rep
)
{
    rpc__auth_info_release (&binding_rep->auth_info);
}



/*
**++
**
**  ROUTINE NAME:       rpc__auth_info_release
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:   
**
**  Release reference to authentication info previously returned by
**  set_server or inq_caller.
**      
**  INPUTS:
**
**      info            Authentication info
**
**  INPUTS/OUTPUTS:
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

PRIVATE void rpc__auth_info_release
(
 rpc_auth_info_p_t       *info
)
{
    rpc_auth_info_p_t auth_info = *info;
    
    if (auth_info == NULL)
    {
        return;
    }

    RPC_DBG_PRINTF(rpc_e_dbg_auth, 3, ("(rpc__auth_info_release) %x: dropping %s refcount (was %d, now %d)\n",
        auth_info,
        auth_info->is_server ? "server" : "client",
        auth_info->refcount,
        auth_info->refcount-1 ));
    assert(auth_info->refcount >= 1);
    
    /*
     * Remove the reference.
     */
    auth_info->refcount--;

    /*
     * Some special logic is required for cache maintenance on the
     * client side.
     */
    if (!(auth_info->is_server))
    {
        if (auth_info->refcount == 1)
        {
            /*
             * The auth info can be removed from the cache if there is only
             * one reference left to it. That single reference is the cache's
             * reference. 
             */
            rpc__auth_info_cache_remove (auth_info);
        }
    }

    /*
     * Free the auth info when nobody holds a reference to it.
     */
    if (auth_info->refcount == 0)
    {
        (*rpc_g_authn_protocol_id[auth_info->authn_protocol].epv->free_info) 
            (&auth_info);
    }

    /*
     * NULL out the caller's reference to the auth info.
     */
    *info = NULL;
}



/*
**++
**
**  ROUTINE NAME:       rpc__key_info_reference
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:   
**
**  Establish a reference to keyentication info.
**      
**  INPUTS:
**
**      key_info       Authentication information   
**
**  INPUTS/OUTPUTS:
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

PRIVATE void rpc__key_info_reference
(
  rpc_key_info_p_t   key_info
)
{
    RPC_DBG_PRINTF(rpc_e_dbg_auth, 3, ("(rpc__key_info_reference) %x: bumping %s refcnt (was %d, now %d)\n",
        key_info,
        (key_info->is_server?"server":"client"),
        key_info->refcnt,
        key_info->refcnt + 1));

    assert (key_info->refcnt >= 1);
    key_info->refcnt++;
}

/*
**++
**
**  ROUTINE NAME:       rpc__key_info_release
**
**  SCOPE:              PRIVATE - declared in com.h
**
**  DESCRIPTION:   
**
**  Release reference to keyentication info previously returned by
**  set_server or inq_caller.
**      
**  INPUTS:
**
**      info            Authentication info
**
**  INPUTS/OUTPUTS:
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

PRIVATE void rpc__key_info_release
(
  rpc_key_info_p_t       *info
)
{
    rpc_key_info_p_t key_info = *info;
    
    if (key_info == NULL)
    {
        return;
    }
    *info = NULL;
    
    RPC_DBG_PRINTF(rpc_e_dbg_auth, 3,
        ("(rpc__key_info_release) %x: dropping %s refcnt (was %d, now %d)\n",
            key_info,
            key_info->is_server?"server":"client",
            key_info->refcnt,
            key_info->refcnt-1 ));
    assert(key_info->refcnt >= 1);
    
    /*
     * Remove the reference.
     */
    key_info->refcnt--;

    /*
     * Free the auth info when nobody holds a reference to it.
     */
    if (key_info->refcnt == 0)
    {
        (*rpc_g_authn_protocol_id[key_info->auth_info->authn_protocol].epv->free_key) 
            (&key_info);
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc__auth_inq_my_princ_name
**
**  SCOPE:              PRIVATE - declared in comauth.h
**
**  DESCRIPTION:   
**
**  
**  
**  
**      
**  INPUTS:
**
**      h               RPC binding handle
**
**  INPUTS/OUTPUTS:
**
**  OUTPUTS:
**
**      status          A value indicating the return status of the routine
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

PRIVATE void rpc__auth_inq_my_princ_name
(
  unsigned32              dce_rpc_authn_protocol,
  unsigned32              princ_name_size,
  unsigned_char_p_t       princ_name,
  unsigned32              *st
)
{
    rpc_authn_protocol_id_t authn_protocol;

    authn_protocol = rpc__auth_cvt_id_wire_to_api(dce_rpc_authn_protocol, st);
    if (*st != rpc_s_ok)
    {
        return;
    }

    (*rpc_g_authn_protocol_id[authn_protocol]
        .epv->inq_my_princ_name)
            (princ_name_size, princ_name, st);
}

 
/*
**++
**
**  ROUTINE NAME:       rpc_binding_set_auth_info
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Set up client handle for authentication.
**  
**  INPUTS:
**
**      h               RPC binding handle
**
**      server_princ_name     
**                      Name of server to authenticate to
**
**      authn_level     Authentication level
**  
**      authn_protocol  Desired authentication protocol to use
**
**      auth_identity   Credentials to use on calls
**
**      authz_protocol  Authorization protocol to use
**
**  INPUTS/OUTPUTS:
**
**  OUTPUTS:
**
**      status          A value indicating the return status of the routine
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
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

PUBLIC void rpc_binding_set_auth_info
(
  rpc_binding_handle_t    binding_h,
  unsigned_char_p_t       server_princ_name,
  unsigned32              authn_level,
  unsigned32              authn_protocol,
  rpc_auth_identity_handle_t auth_identity,
  unsigned32              authz_protocol,
  unsigned32              *st
)
{
    unsigned32 authn_flags = 0;

    switch (authn_protocol)
    {
    case rpc_c_authn_none:
    case rpc_c_authn_dce_secret:
    case rpc_c_authn_dce_public:
    case rpc_c_authn_dce_dummy:
    case rpc_c_authn_dssa_public:
    case rpc_c_authn_gss_tls:
    case rpc_c_authn_netlogon:
    case rpc_c_authn_default:
        authn_flags = 0;
        break;

    case rpc_c_authn_winnt:
        authn_flags |= rpc_c_protect_flags_header_sign;
	break;

    default:
        authn_flags = 0;
        break;
    }

    rpc_binding_set_auth_info_2(
                       binding_h,
		       server_princ_name,
		       authn_level,
		       authn_protocol,
		       authn_flags,
		       auth_identity,
		       authz_protocol,
		       st);
}
 
/*
**++
**
**  ROUTINE NAME:       rpc_binding_set_auth_info_2
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Set up client handle for authentication.
**  
**  INPUTS:
**
**      h               RPC binding handle
**
**      server_princ_name     
**                      Name of server to authenticate to
**
**      authn_level     Authentication level
**  
**      authn_protocol  Desired authentication protocol to use
**
**      auth_identity   Credentials to use on calls
**
**      authz_protocol  Authorization protocol to use
**
**      authn_flags     Authentication and protection level flags
**
**  INPUTS/OUTPUTS:
**
**  OUTPUTS:
**
**      status          A value indicating the return status of the routine
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
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

PUBLIC void rpc_binding_set_auth_info_2
(
  rpc_binding_handle_t    binding_h,
  unsigned_char_p_t       server_princ_name,
  unsigned32              authn_level,
  unsigned32              authn_protocol,
  unsigned32              authn_flags,
  rpc_auth_identity_handle_t auth_identity,
  unsigned32              authz_protocol,
  unsigned32              *st
)
{
    rpc_auth_identity_handle_t ref_auth_identity;
    rpc_auth_info_p_t       auth_info;
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_h;
    rpc_auth_epv_p_t        auth_epv;
    boolean                 need_to_free_server_name = false;
    
    CODING_ERROR (st);  
    RPC_VERIFY_INIT ();

    RPC_BINDING_VALIDATE_CLIENT(binding_rep, st);
    if (*st != rpc_s_ok)
        return;

    /*
     * If asking to set to authentication type "none", just free any auth info
     * we have and return now.
     */
    
    if (authn_protocol == rpc_c_authn_none)
    {
        rpc__auth_info_binding_release(binding_rep);
        return;
    }

    RPC_AUTHN_CHECK_SUPPORTED_RPC_PROT(authn_protocol, binding_rep->protocol_id, st);

    /*
     * If asking for default authn level, get the actual level now (i.e., 
     * spare each auth service the effort of coding this logic).
     */

    if (authn_level == rpc_c_authn_level_default)
    {
        rpc_mgmt_inq_dflt_authn_level (authn_protocol, &authn_level, st);
        if (*st != rpc_s_ok)
            return;
    }

    auth_epv = rpc_g_authn_protocol_id[authn_protocol].epv;
    /*
     * Resolve the auth_identity into a real reference to the identity
     * prior to the cache lookup.
     */
    
    *st = (*auth_epv->resolve_id)
        (auth_identity, &ref_auth_identity);

    if (*st != rpc_s_ok)
        return;

    
    /*
     * If no server principal name was specified, go ask for it.
     *
     * Not all authentication protocols require a server principal
     * name to do authentication.  Hence, only inquire the server
     * principal name if we know the authentication protocol is
     * secret key based.
     *
     * We did not move the inq_princ_name function to an
     * authentication service specific module because we need
     * a server principal name for the auth_info cache lookup.
     *
     * Note that we want to be avoid bypassing the auth_info
     * cache because certain protocol services cache credentials.
     * Allocating a new auth_info structure on every call can
     * cause these credentials to accumulate until the heap is
     * exhausted.
     */
     if (server_princ_name == NULL) {
          switch (authn_protocol) {
          case rpc_c_authn_dce_secret:
          case rpc_c_authn_winnt:
          case rpc_c_authn_gss_negotiate:
          case rpc_c_authn_gss_mskrb: 
          rpc_mgmt_inq_server_princ_name
              (binding_h,
               authn_protocol,
               &server_princ_name,
               st);

           if (*st != rpc_s_ok)
              return;

           need_to_free_server_name = true;
              break;

           default:
              break;
         }
    }

    /*
     * Consult the cache before creating a new auth info structure.
     * We may be able to add a reference to an already existing one.
     */
    if ((auth_info = rpc__auth_info_cache_lkup (server_princ_name,
                                                authn_level,
                                                ref_auth_identity,
                                                authz_protocol, 
                                                authn_protocol,
                                                authn_flags)) == NULL)
    {
        
        /*
         * A new auth info will have to be created.
         * Call authentication service to do generic (not specific
         * to a single RPC protocol) "set server" function.
         */
        (*auth_epv->binding_set_auth_info)
            (server_princ_name, authn_level, authn_flags, auth_identity, 
             authz_protocol, binding_h, &auth_info, st);
        
        if (*st != rpc_s_ok)
        {
            if (need_to_free_server_name)
                RPC_MEM_FREE (server_princ_name, RPC_C_MEM_STRING);
            return;
        }
        
        /*
         * Add this new auth info to a cache of auth infos. This cache
         * will be consulted on a subsequent call to this routine.
         */
        rpc__auth_info_cache_add (auth_info);

    }
        
    /*
     * Release our reference to the identity.  If a new auth_info was
     * created, then it added a reference also.
     */

    (*auth_epv->release_id) (&ref_auth_identity);

    /*
     * If we inquired the server principal name, free it now.
     */
    if (need_to_free_server_name)
        RPC_MEM_FREE (server_princ_name, RPC_C_MEM_STRING);

    /*
     * If we have any auth info for this binding already, lose it.
     */

    if (binding_rep->auth_info != NULL)
    {
        rpc__auth_info_binding_release(binding_rep);
    }

    binding_rep->auth_info = auth_info;

    /*
     * Notify the protocol service that the binding has changed.
     */

    (*rpc_g_protocol_id[binding_rep->protocol_id].binding_epv
        ->binding_changed) (binding_rep, st);
}

/*
**++
**
**  ROUTINE NAME:       rpc_binding_inq_auth_info_2
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Return authentication and authorization information from a binding
**  handle.
**  
**  INPUTS:
**
**      h               RPC binding handle
**
**  INPUTS/OUTPUTS:
**
**  OUTPUTS:
**
**      server_princ_name     
**                      Name of server to authenticate to
**
**      authn_level     Authentication level
**  
**      authn_protocol  Desired authentication protocol to use
**
**      auth_identity   Credentials to use on calls
**
**      authz_protocol  Authorization protocol to use
**
**      authn_flags     Authentication flags
**
**      status          A value indicating the return status of the routine
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
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

PUBLIC void rpc_binding_inq_auth_info_2
(
    rpc_binding_handle_t    binding_h,
    unsigned_char_p_t       *server_princ_name,
    unsigned32              *authn_level,
    unsigned32              *authn_protocol,  
    unsigned32              *authn_flags,
    rpc_auth_identity_handle_t *auth_identity,
    unsigned32              *authz_protocol,
    unsigned32              *st
    )
{
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_h;
    rpc_auth_info_p_t       auth_info;

    CODING_ERROR (st);  
    RPC_VERIFY_INIT (); 

    RPC_BINDING_VALIDATE_CLIENT(binding_rep, st);
    if (*st != rpc_s_ok)
        return;

    auth_info = ((rpc_binding_rep_p_t)binding_h)->auth_info;

    if (auth_info == NULL)
    {
        *st = rpc_s_binding_has_no_auth;
        return;
    }

    assert(! auth_info->is_server);

    if (auth_info->server_princ_name == NULL) 
    {
        ASSIGN(server_princ_name, NULL);
    } else 
    {
        ASSIGN(server_princ_name, rpc_stralloc(auth_info->server_princ_name));
    }
    ASSIGN(authn_level,         auth_info->authn_level);
    ASSIGN(authn_protocol,      auth_info->authn_protocol);  
    ASSIGN(auth_identity,       auth_info->u.auth_identity);
    ASSIGN(authz_protocol,      auth_info->authz_protocol);
    ASSIGN(authn_flags,         auth_info->authn_flags);

    *st = rpc_s_ok;
}


/*
**++
**
**  ROUTINE NAME:       rpc_binding_inq_auth_info
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Return authentication and authorization information from a binding
**  handle.
**  
**  INPUTS:
**
**      h               RPC binding handle
**
**  INPUTS/OUTPUTS:
**
**  OUTPUTS:
**
**      server_princ_name     
**                      Name of server to authenticate to
**
**      authn_level     Authentication level
**  
**      authn_protocol  Desired authentication protocol to use
**
**      auth_identity   Credentials to use on calls
**
**      authz_protocol  Authorization protocol to use
**
**      status          A value indicating the return status of the routine
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
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

PUBLIC void rpc_binding_inq_auth_info
(
    rpc_binding_handle_t    binding_h,
    unsigned_char_p_t       *server_princ_name,
    unsigned32              *authn_level,
    unsigned32              *authn_protocol,  
    rpc_auth_identity_handle_t *auth_identity,
    unsigned32              *authz_protocol,
    unsigned32              *st
    )
{
    unsigned32 authn_flags = 0;

    rpc_binding_inq_auth_info_2(
                      binding_h,
                      server_princ_name,
                      authn_level,
                      authn_protocol,
                      &authn_flags,
                      auth_identity,
                      authz_protocol,
                      st);
}


/*
**++
**
**  ROUTINE NAME:       rpc_server_register_auth_info
**
**  SCOPE:              PUBLIC - declared in rpcauth.idl
**
**  DESCRIPTION:
**      
**  Register authentication information with the RPC runtime.
**
**  INPUTS:
**
**      authn_protocol  Desired authentication protocol to use
**
**      server_princ_name     
**                      Name server should use
**
**      get_key_func    Function ptr to call to get keys
**
**      arg             Opaque params for key func.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the return status of the routine
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

PUBLIC void rpc_server_register_auth_info 
(
    unsigned_char_p_t       server_princ_name,
    unsigned32              authn_protocol,
    rpc_auth_key_retrieval_fn_t get_key_func,
    ndr_void_p_t            arg,
    unsigned32              *st
)
{
    CODING_ERROR (st);
    RPC_VERIFY_INIT ();
    
    if (authn_protocol == rpc_c_authn_none)
    {
        *st = rpc_s_ok;
        return;
    }

    if (authn_protocol == (typeof(authn_protocol))(rpc_c_authn_default) && get_key_func != NULL)
    {
        *st = rpc_s_key_func_not_allowed;
        return;
    }

    RPC_AUTHN_CHECK_SUPPORTED (authn_protocol, st);
    
    (*rpc_g_authn_protocol_id[authn_protocol]
        .epv->server_register_auth_info)
            (server_princ_name, get_key_func, (pointer_t) arg, st);
}

/*
**++
**
**  ROUTINE NAME:       rpc_binding_inq_auth_client
**
**  SCOPE:              PUBLIC - declared in rpcauth.idl
**
**  DESCRIPTION:
**
**  Return authentication and authorization information from a binding
**  handle to an authenticated client.
**  be freed.
**
**  INPUTS:
**
**      binding_h       Server-side binding handle to remote caller whose
**                      identity is being requested.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      privs           PAC for remote caller.
**
**      server_princ_name         
**                      Server name that caller authenticated to.
**
**      authn_level     Authentication level used by remote caller.
**
**      authn_protocol  Authentication protocol used by remote caller.
**
**      authz_protocol  Authorization protocol used by remote caller.
**
**      status          A value indicating the return status of the routine
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
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

PUBLIC void rpc_binding_inq_auth_client
    
(
    rpc_binding_handle_t    binding_h,
    rpc_authz_handle_t      *privs,
    unsigned_char_p_t       *server_princ_name,
    unsigned32              *authn_level,
    unsigned32              *authn_protocol,
    unsigned32              *authz_protocol,
    unsigned32              *st
)
{
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_h;
    rpc_auth_info_p_t       auth_info;

    CODING_ERROR (st);
    RPC_VERIFY_INIT ();
    
    RPC_BINDING_VALIDATE_SERVER(binding_rep, st);
    if (*st != rpc_s_ok)
        return;
    
    auth_info = ((rpc_binding_rep_p_t)binding_h)->auth_info;
    
    if (auth_info == NULL)
    {
        *st = rpc_s_binding_has_no_auth;
        return;
    }
    
    assert(auth_info->is_server);
    
    ASSIGN(privs,               auth_info->u.s.privs);
    
    if (server_princ_name != NULL)
    {
        if (auth_info->server_princ_name == NULL) 
        {
            ASSIGN(server_princ_name,   NULL);
        } 
        else 
        {
            ASSIGN(server_princ_name, rpc_stralloc(auth_info->server_princ_name));
        }
    }

    ASSIGN(authn_level,         auth_info->authn_level);
    ASSIGN(authn_protocol,      auth_info->authn_protocol);
    ASSIGN(authz_protocol,      auth_info->authz_protocol);

    *st = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       rpc_binding_inq_auth_caller
**
**  SCOPE:              PUBLIC - declared in rpcauth.idl
**
**  DESCRIPTION:
**
**  Return authentication and 1.1+ authorization information from a binding
**  handle to an authenticated client.
**
**  INPUTS:
**
**      binding_h       Server-side binding handle to remote caller whose
**                      identity is being requested.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      creds           Opaque handle on caller's credentials to
**                      be used in making sec_cred_ calls
**
**      server_princ_name         
**                      Server name that caller authenticated to.
**
**      authn_level     Authentication level used by remote caller.
**
**      authn_protocol  Authentication protocol used by remote caller.
**
**      authz_protocol  Authorization protocol used by remote caller.
**
**      status          A value indicating the return status of the routine
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
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

PUBLIC void rpc_binding_inq_auth_caller

(
    rpc_binding_handle_t    binding_h,
    rpc_authz_cred_handle_t *creds,
    unsigned_char_p_t       *server_princ_name,
    unsigned32              *authn_level,
    unsigned32              *authn_protocol,
    unsigned32              *authz_protocol,
    unsigned32              *st
)
{
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_h;
    rpc_auth_info_p_t       auth_info;

    CODING_ERROR (st);
    RPC_VERIFY_INIT ();
    
    RPC_BINDING_VALIDATE_SERVER(binding_rep, st);
    if (*st != rpc_s_ok)
        return;

    auth_info = ((rpc_binding_rep_p_t)binding_h)->auth_info;

    if (auth_info == NULL)
    {
        *st = rpc_s_binding_has_no_auth;
        return;
    }

    assert(auth_info->is_server);

    if (auth_info->u.s.creds != NULL) {
	*creds = *auth_info->u.s.creds;
    }

    if (server_princ_name != NULL)
    {
        if (auth_info->server_princ_name == NULL) 
        {
            ASSIGN(server_princ_name,   NULL);
        } 
        else 
        {
            ASSIGN(server_princ_name, rpc_stralloc(auth_info->server_princ_name));
        }
    }

    ASSIGN(authn_level,         auth_info->authn_level);
    ASSIGN(authn_protocol,      auth_info->authn_protocol);
    ASSIGN(authz_protocol,      auth_info->authz_protocol);

    *st = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       rpc_mgmt_inq_dflt_protect_level
**
**  SCOPE:              PUBLIC - declared in rpcauth.idl
**
**  DESCRIPTION:
**      
**  Returns the default authentication level for an authentication service.
**
**  INPUTS:
**
**      authn_protocol  Desired authentication protocol.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      authn_level     Authentication level used by remote caller.
**
**      status          A value indicating the return status of the routine
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

PUBLIC void rpc_mgmt_inq_dflt_protect_level 
(
    unsigned32              authn_protocol,
    unsigned32              *authn_level,
    unsigned32              *st
)
{
    CODING_ERROR (st);
    RPC_VERIFY_INIT ();
    
    if (authn_protocol == rpc_c_authn_none)
    {
        *authn_level = rpc_c_authn_level_none;
        *st = rpc_s_ok;
        return;
    }

    RPC_AUTHN_CHECK_SUPPORTED (authn_protocol, st);
    
    (*rpc_g_authn_protocol_id[authn_protocol]
        .epv->mgmt_inq_dflt_auth_level)
            (authn_level, st);
}

/*
 * Retain entry point with old name for compatibility.
 */

PUBLIC void rpc_mgmt_inq_dflt_authn_level 
(
  unsigned32              authn_protocol,
  unsigned32              *authn_level,
  unsigned32              *st
)
{
    rpc_mgmt_inq_dflt_protect_level (authn_protocol, authn_level, st);
}



/*
**++
**
**  ROUTINE NAME:       rpc__auth_info_cache_init
**
**  SCOPE:              PRIVATE - declared in comauth.h
**
**  DESCRIPTION:
**      
**  Initialize the auth info cache including mutexes and list head.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:           
**
**      status          A value indicating the return status of the routine
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

PRIVATE void rpc__auth_info_cache_init 
(
  unsigned32      *status
)
{
    CODING_ERROR (status);

    RPC_MUTEX_INIT (auth_info_cache_mutex);
    RPC_LIST_INIT (auth_info_cache);
    *status = rpc_s_ok;
}


/*
**++
**
**  ROUTINE NAME:       rpc__auth_info_cache_lkup
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Scan a linked list of auth info structures looking for one which
**  contains fields which match the input parameters. If and when a
**  match is found the reference count of the auth info structure will
**  be incremented before being returned.
**
**  Note that it is possible for a null server principal name to
**  match an auth info structure if that structure also has a 
**  null server principal name.
**
**  INPUTS:
**
**      server_princ_name Server principal name.
**      authn_level     Authentication level.
**      authn_identity  Authentication identity handle.
**      authz_protocol  Authorization protocol.
**      authn_protocol  Authentication protocol.
**      authn_flags     Authentication flags
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     rpc_auth_info_t *
**
**      A pointer to a matching auth info, NULL if none found.
**
**  SIDE EFFECTS:      
**
**      If a matching auth info is found the reference count of it
**      will have already been incremented.
**
**--
**/

INTERNAL rpc_auth_info_t *rpc__auth_info_cache_lkup 
(
    unsigned_char_p_t                   server_princ_name,
    rpc_authn_level_t                   authn_level,
    rpc_auth_identity_handle_t          auth_identity,
    rpc_authz_protocol_id_t             authz_protocol,
    rpc_authn_protocol_id_t             authn_protocol,
    unsigned32                          authn_flags
)
{
    rpc_auth_info_t     *auth_info;

    RPC_MUTEX_LOCK (auth_info_cache_mutex);

    /*
     * Scan the linked list of auth info structure looking for one
     * whose fields match the input args.
     */
    RPC_LIST_FIRST (auth_info_cache,
                    auth_info,
                    rpc_auth_info_p_t);
    while (auth_info != NULL)
    {
        if ((
             /*
              * DCE secret key authentication requires a
              * non-null server principal name for authentication.
              * We allow a null server principal name here so
              * that this will work in the future when an 
              * authentication service without this requirement
              * is used.
              */
             ((server_princ_name == NULL)
              && (auth_info->server_princ_name == NULL))
             ||
             (server_princ_name 
	      && auth_info->server_princ_name
	      && (strcmp ((char *) server_princ_name, 
			  (char *) auth_info->server_princ_name) == 0))
	    )
            &&
            (authn_level == auth_info->authn_level)
            &&
            (authn_protocol == auth_info->authn_protocol)
            &&
            (authz_protocol == auth_info->authz_protocol)
            &&
            (auth_identity == auth_info->u.auth_identity)
            &&
            (authn_flags == auth_info->authn_flags))
        {
            /*
             * A matching auth info was found. 
             */
            rpc__auth_info_reference (auth_info);
            break;
        }
        RPC_LIST_NEXT (auth_info, auth_info, rpc_auth_info_p_t);
    }
    RPC_MUTEX_UNLOCK (auth_info_cache_mutex);
    return (auth_info);
}


/*
**++
**
**  ROUTINE NAME:       rpc__auth_info_cache_add
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Add an auth info structure to a linked list of them. The
**  reference count of the added auth info structure will be incremented
**  by the caller to account for its presence in the cache.
**
**  INPUTS:
**
**      auth_info       The auth info to be added.
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
**      The reference count of the added auth info structure will be
**      incremented. 
**
**--
**/

INTERNAL void rpc__auth_info_cache_add 
(
  rpc_auth_info_p_t auth_info
)
{
    assert (!auth_info->is_server);

    RPC_MUTEX_LOCK (auth_info_cache_mutex);
    RPC_LIST_ADD_HEAD (auth_info_cache,
                       auth_info,
                       rpc_auth_info_p_t);
    rpc__auth_info_reference (auth_info);
    RPC_MUTEX_UNLOCK (auth_info_cache_mutex);
}


/*
**++
**
**  ROUTINE NAME:       rpc__auth_info_cache_remove
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**      
**  Remove an auth info structure from a linked list of them. The
**  cache's reference to the auth info structure will be released by
**  the caller.
**
**  It is assumed that the caller has already determined it is OK to
**  remove this auth info from the cache. It is expected that the
**  cache holds the last reference to this auth info structure at
**  this point. 
**
**  INPUTS:             
**
**      auth_info       The auth info to be removed.
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

INTERNAL void rpc__auth_info_cache_remove 
(
  rpc_auth_info_p_t       auth_info
)
{
    assert (!auth_info->is_server);

    RPC_MUTEX_LOCK (auth_info_cache_mutex);
    
    /*
     * Make sure, under the protection of the cache lock, that this
     * really should be removed from the cache.
     */
    if (auth_info->refcount == 1)
    {
        RPC_LIST_REMOVE (auth_info_cache, auth_info);
        RPC_DBG_PRINTF(rpc_e_dbg_auth, 3, ("(rpc__auth_info_release) %x: dropping %s refcount (was %d, now %d)\n",
                                           auth_info,
                                           auth_info->is_server ? "server" : "client",
                                           auth_info->refcount,
                                           auth_info->refcount-1 ));
        assert(auth_info->refcount >= 1);
        auth_info->refcount--;
    }
    RPC_MUTEX_UNLOCK (auth_info_cache_mutex);
}


/*
**++
**
**  ROUTINE NAME:       rpc_server_inq_call_attributes
**
**  SCOPE:              PUBLIC - declared in rpcauth.idl
**
**  DESCRIPTION:
**
**  Return RPC call attributes
**
**  INPUTS:
**
**      binding_h       Server-side binding handle to remote caller whose
**                      identity is being requested.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      attributes      RPC call attributes
**
**      status          Status code
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

PUBLIC void rpc_server_inq_call_attributes

(
    rpc_binding_handle_t    binding_h,
    rpc_call_attributes_t   *attributes,
    unsigned32              *st
)
{
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_h;
    rpc_auth_info_p_t       auth_info;
    rpc_call_attributes_v1_t *v1_attributes;

    CODING_ERROR (st);
    RPC_VERIFY_INIT ();
    
    RPC_BINDING_VALIDATE_SERVER(binding_rep, st);
    if (*st != rpc_s_ok)
        return;

    auth_info = ((rpc_binding_rep_p_t)binding_h)->auth_info;

    if (auth_info == NULL)
    {
        *st = rpc_s_binding_has_no_auth;
        return;
    }

    assert(auth_info->is_server);

    if (attributes->version != 1)
    {
        *st = rpc_s_auth_badversion;
        return;
    }

    v1_attributes = (rpc_call_attributes_v1_p_t)attributes;

    if (v1_attributes->flags & rpc_query_server_principal_name)
    {
        ASSIGN_COPY(v1_attributes->server_princ_name,
                    v1_attributes->server_princ_name_buff_len,
                    auth_info->server_princ_name);
    }

    if (v1_attributes->flags & rpc_query_client_principal_name)
    {
        if (auth_info->authz_protocol != rpc_c_authz_name)
        {
            *st = rpc_s_binding_has_no_auth;
            return;
        }

        ASSIGN_COPY(v1_attributes->client_princ_name,
                    v1_attributes->client_princ_name_buff_len,
                    (unsigned_char_p_t)auth_info->u.s.privs);
    }

    v1_attributes->authn_level = auth_info ? auth_info->authn_level : rpc_c_protect_level_none;
    v1_attributes->authn_protocol = auth_info ? auth_info->authn_protocol : rpc_c_authn_winnt;
    v1_attributes->null_session = /* FIXME: determine meaning of this flag */ 0;

    *st = rpc_s_ok;
}


/*
**++
**
**  ROUTINE NAME:       rpc_binding_inq_security_context
**
**  SCOPE:              PUBLIC - declared in rpc.idl
**
**  DESCRIPTION:
**      
**  Return mechanism-specific security context from a binding handle.
**  
**  INPUTS:
**
**      h               RPC binding handle
**
**  INPUTS/OUTPUTS:
**
**  OUTPUTS:
**
**      authn_protocol  Authentication level
**  
**      sec_context     Security context
**
**      status          A value indicating the return status of the routine
**          rpc_s_invalid_binding
**                          RPC Protocol ID in binding handle was invalid.
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

PUBLIC void rpc_binding_inq_security_context
(
    rpc_binding_handle_t    binding_h,
    unsigned32              *authn_protocol,  
    void                    **mech_context,
    unsigned32              *st
    )
{
    rpc_binding_rep_p_t     binding_rep = (rpc_binding_rep_p_t) binding_h;
    rpc_auth_info_p_t       auth_info;

    CODING_ERROR (st);  
    RPC_VERIFY_INIT (); 

    *authn_protocol = rpc_c_authn_none;
    *mech_context = NULL;

    auth_info = binding_rep->auth_info;
    if (auth_info == NULL)
    {
        *st = rpc_s_binding_has_no_auth;
        return;
    }

    *authn_protocol = auth_info->authn_protocol;
    if (*authn_protocol == rpc_c_authn_none)
    {
        *st = rpc_s_ok;
        return;
    }

    RPC_AUTHN_CHECK_SUPPORTED (*authn_protocol, st);

    if (rpc_g_authn_protocol_id[*authn_protocol]
        .epv->inq_sec_context == NULL)
    {
        *st = rpc_s_binding_has_no_auth;
        return;
    }

    (*rpc_g_authn_protocol_id[*authn_protocol]
        .epv->inq_sec_context) (auth_info, mech_context, st);
}

