/*
 * Copyright (c) 2007, Novell, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Novell, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
**
**  NAME:
**
**      nsldap.c
**
**  FACILITY:
**
**      LDAP RPC locator
**
**  ABSTRACT:
**
**  This module supports locating RPC servers in LDAP directories.
**  Note: only a portion of the RPC NS API is implemented, and
**  this support is presently experimental. 
**
*/

#if HAVE_CONFIG_H
#include <config.h>
#endif

/* #define BUILD_RPC_NS_LDAP 1  */

#ifdef BUILD_RPC_NS_LDAP

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comp.h>       /* Private communications services */

#include <lber.h>
#include <ldap.h>

typedef struct rpc_ns_handle_s_t
{
	unsigned32 cursor;
	unsigned32 count;
	unsigned_char_p_t *bindings;
} rpc_ns_handle_rep_t, *rpc_ns_handle_rep_p_t;


/*
 * Turn a DCE name into an LDAP distinguished name.
 * This needs to be fixed to support DCE CDS style
 * names.
 */
static void rpc_ns__ldap_crack_name(unsigned32 entry_name_syntax,
	unsigned_char_p_t entry_name,
	unsigned_char_p_t *dn,
	unsigned32 *status)
{
	switch (entry_name_syntax) {
		/* /DC=com/DC=padl/DC=nt/CN=RpcServices/CN=foo */
		case rpc_c_ns_syntax_x500: {
			char *tmp;

			tmp = ldap_dcedn2dn(entry_name);
			if (tmp == NULL) {
				*status = rpc_s_invalid_name_syntax;
			} else {
				*status = rpc_s_ok;
				*dn = rpc_stralloc(tmp);
			}
		}
		/* CN=foo,CN=RpcServices,DC=nt,DC=padl,DC=com */
		case rpc_c_ns_syntax_ldap:
			*dn = rpc_stralloc(entry_name);
			break;
		/*
		 * /.:/foo or
		 * /.../nt.padl.com/foo
		 */
		case rpc_c_ns_syntax_default:
		case rpc_c_ns_syntax_dce:
			/* need to do something about cell-relative names */
		default:
			*status = rpc_s_unsupported_name_syntax;
			break;
	}
}

/*
 * Create a new connection to the LDAP server.
 */
static void rpc_ns__ldap_connect_to_server(LDAP **ld,
	unsigned32 *status)
{
	if (ldap_initialize(ld, "ldapi://") != LDAP_SUCCESS) {
		*status = rpc_s_name_service_unavailable;
	} else {
		*status = rpc_s_ok;
	}

	if (*status == rpc_s_ok) {
		if (ldap_sasl_bind_s(*ld, NULL, "EXTERNAL",
			NULL, NULL, NULL, NULL) != LDAP_SUCCESS)
			*status = rpc_s_name_service_unavailable;
	}
}

/*
 * Create an LDAP server container entry.
 */
static void rpc_ns__ldap_export_server(LDAP *ld,
	char *dn,
	rpc_if_handle_t if_spec,
	unsigned32 *status
	)	
{
	unsigned_char_p_t uuid = NULL;
	rpc_if_id_t if_id;
	LDAPMod *modV[4];
	LDAPMod modRpcNsObjectID, modObjectClass;
	char *valueRpcNsObjectID[2], *valueObjectClass[3];

	rpc_if_inq_id(if_spec, &if_id, status);
	if (*status != rpc_s_ok) {
		return;
	}

	dce_uuid_to_string(&if_id.uuid, &uuid, status);
	if (*status != rpc_s_ok) {
		return;
	}

	valueRpcNsObjectID[0] = uuid;
	valueRpcNsObjectID[1] = NULL;

	modRpcNsObjectID.mod_op = LDAP_MOD_ADD;
	modRpcNsObjectID.mod_type = "rpcNsObjectID";
	modRpcNsObjectID.mod_values = valueRpcNsObjectID;

	valueObjectClass[0] = "rpcServer";
	valueObjectClass[1] = "rpcEntry";
	valueObjectClass[2] = "top";

	modObjectClass.mod_op = LDAP_MOD_ADD;
	modObjectClass.mod_type = "objectClass";
	modObjectClass.mod_values = valueObjectClass;

	modV[0] = &modRpcNsObjectID;
	modV[1] = &modObjectClass;
	modV[2] = NULL;

	if (ldap_add_s(ld, dn, modV) != LDAP_SUCCESS) {
		*status = rpc_s_update_failed;
	} else {
		*status = rpc_s_ok;
	}

	rpc_string_free(&uuid, status);
}

/*
 * Create or update an LDAP server binding entry.
 */
static void rpc_ns__ldap_export_server_element_ext(LDAP *ld,
	char *dn,
	rpc_if_handle_t if_spec,
	rpc_binding_vector_p_t vec,
	int modop,
	unsigned32 *status
	)
{
	unsigned_char_p_t uuid = NULL;
	unsigned_char_p_t interfaceID = NULL;
	rpc_if_id_t if_id;
	LDAPMod *modV[4];
	LDAPMod modRpcNsInterfaceID, modRpcNsBindings, modObjectClass;
	char **valueRpcNsBindings = NULL;
	char *valueRpcNsInterfaceID[2], *valueObjectClass[3];
	int rc;
	unsigned i;

	rpc_if_inq_id(if_spec, &if_id, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	/* Get the interface ID */
	dce_uuid_to_string(&if_id.uuid, &uuid, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	RPC_MEM_ALLOC(interfaceID, unsigned_char_p_t,
		strlen(uuid) + sizeof(",65535.65535"),
		RPC_C_MEM_NSRESOLUTION, RPC_C_MEM_WAITOK);

	sprintf(interfaceID, "%s,%hu.%hu", uuid,
		if_id.vers_major, if_id.vers_minor);

	valueRpcNsInterfaceID[0] = interfaceID;
	valueRpcNsInterfaceID[1] = NULL;

	modRpcNsInterfaceID.mod_op = LDAP_MOD_ADD;
	modRpcNsInterfaceID.mod_type = "rpcNsInterfaceID";
	modRpcNsInterfaceID.mod_values = valueRpcNsInterfaceID;

	RPC_MEM_ALLOC(valueRpcNsBindings, char **,
		(vec->count * sizeof(char *)),
		RPC_C_MEM_NSRESOLUTION, RPC_C_MEM_WAITOK);
	memset(valueRpcNsBindings, 0, (vec->count * sizeof(unsigned_char_p_t)));

	for (i = 0; i < vec->count; i++) {
		rpc_binding_to_string_binding(vec->binding_h[i],
			(unsigned_char_p_t *)&valueRpcNsBindings[i], status);
		if (*status != rpc_s_ok) {
			goto out;
		}
	}
	valueRpcNsBindings[vec->count] = NULL;

	modRpcNsBindings.mod_op = modop;
	modRpcNsBindings.mod_type = "rpcNsBindings";
	modRpcNsBindings.mod_values = valueRpcNsBindings;

	valueObjectClass[0] = "rpcServerElement";
	valueObjectClass[1] = "rpcEntry";
	valueObjectClass[2] = "top";

	modObjectClass.mod_op = modop;
	modObjectClass.mod_type = "objectClass";
	modObjectClass.mod_values = valueObjectClass;

	modV[0] = &modRpcNsInterfaceID;
	modV[1] = &modRpcNsBindings;
	modV[2] = &modObjectClass;
	modV[3] = NULL;

	if (modop == LDAP_MOD_ADD) {
		rc = ldap_add_s(ld, dn, modV);
	} else {
		rc = ldap_modify_s(ld, dn, modV);
	}
	*status = (rc == LDAP_SUCCESS) ? rpc_s_ok : rpc_s_update_failed;

out:
	if (uuid != NULL)
		free(uuid);
	if (interfaceID != NULL)
		free(interfaceID);
	if (valueRpcNsBindings != NULL) {
		char **p;

		for (p = valueRpcNsBindings; *valueRpcNsBindings != NULL; p++) {
			unsigned_char_p_t tmp = (unsigned_char_p_t)*p;

			rpc_string_free(&tmp, status);
		}
		RPC_MEM_FREE(valueRpcNsBindings, RPC_C_MEM_NSRESOLUTION);
	}
}

/*
 * Create a new named server binding entry.
 */
static void rpc_ns__ldap_export_server_element(LDAP *ld,
	char *serverDN,
	rpc_if_handle_t if_spec,
	rpc_binding_vector_p_t vec,
	unsigned32 *status
	)
{
	unsigned_char_p_t dn = NULL, rdn = NULL;
	uuid_t rdnUuid;

	/* Just create an arbitary UUID to name this entry. */
	dce_uuid_create(&rdnUuid, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	dce_uuid_to_string(&rdnUuid, &rdn, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	RPC_MEM_ALLOC(dn, unsigned_char_p_t,
		strlen(serverDN) + strlen(rdn) + sizeof("CN=,"),
		RPC_C_MEM_NSRESOLUTION, RPC_C_MEM_WAITOK);

	sprintf(dn, "CN=%s,%s", rdn, serverDN);

	rpc_ns__ldap_export_server_element_ext(ld, dn, if_spec,
		vec, LDAP_MOD_ADD, status);

out:
	if (dn != NULL) {
		RPC_MEM_FREE(dn, RPC_C_MEM_NSRESOLUTION);
	}

	if (rdn != NULL) {
		rpc_string_free(&rdn, status);
	}
}

static void rpc_ns__ldap_lookup_server_element(LDAP *ld,
	unsigned_char_p_t serverDN,
	rpc_if_handle_t if_spec,
	unsigned_char_p_t *dn,
	unsigned32 *status)
{
	unsigned_char_p_t filter = NULL;
	unsigned_char_p_t uuid = NULL;
	rpc_if_id_t if_id;
	LDAPMessage *msg = NULL, *e;
	char *_dn;
	size_t len;

	rpc_if_inq_id(if_spec, &if_id, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	/* Get the interface ID */
	dce_uuid_to_string(&if_id.uuid, &uuid, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	len = strlen(uuid);
	len += sizeof("(&(objectClass=rpcServerElement)(rpcNsInterfaceID=,65535.65535))");

	RPC_MEM_ALLOC(filter, unsigned_char_p_t, len,
		RPC_C_MEM_NSRESOLUTION, RPC_C_MEM_WAITOK);

	sprintf(filter, "(&(objectClass=rpcServerElement)(rpcNsInterfaceID=%s,%hu.%hu))",
		uuid, if_id.vers_major, if_id.vers_minor);

	if (ldap_search_s(ld, serverDN, LDAP_SCOPE_ONELEVEL,
		filter, NULL, 0, &msg) != LDAP_SUCCESS) {
		*status = rpc_s_not_found;
		goto out;
	}

	e = ldap_first_entry(ld, msg);
	if (e == NULL) {
		*status = rpc_s_not_found;
		goto out;
	}

	_dn = ldap_get_dn(ld, e);
	if (dn == NULL) {
		*status = rpc_s_not_found;
		goto out;
	}

	*dn = rpc_stralloc(_dn);
	ldap_memfree(_dn);

out:
	if (filter != NULL) {
		RPC_MEM_FREE(filter, RPC_C_MEM_NSRESOLUTION);
	}

	if (msg != NULL) {
		ldap_msgfree(msg);
	}

	if (uuid != NULL) {
		rpc_string_free(&uuid, status);
	}
}

void rpc_ns_binding_export(
	/* [in] */ unsigned32 entry_name_syntax,
	/* [in] */ unsigned_char_p_t entry_name,
	/* [in] */ rpc_if_handle_t if_spec,
	/* [in] */ rpc_binding_vector_p_t binding_vector,
	/* [in] */ uuid_vector_p_t object_uuid_vector ATTRIBUTE_UNUSED,
	/* [out] */ unsigned32 *status
)
{
	LDAP *ld = NULL;
	unsigned_char_p_t elementDN = NULL;
	unsigned_char_p_t serverDN = NULL;

	rpc_ns__ldap_connect_to_server(&ld, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	rpc_ns__ldap_crack_name(entry_name_syntax, entry_name, &serverDN, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	rpc_ns__ldap_lookup_server_element(ld, serverDN, if_spec, &elementDN, status);
	if (*status != rpc_s_ok) {
		rpc_ns__ldap_export_server(ld, serverDN, if_spec, status);
		if (*status == rpc_s_ok) {
			rpc_ns__ldap_export_server_element(ld, serverDN,
				if_spec, binding_vector, status);
			}
	} else {
		rpc_ns__ldap_export_server_element_ext(ld, elementDN, if_spec,
			binding_vector, LDAP_MOD_REPLACE, status);
	}

out:
	if (ld != NULL) {
		ldap_unbind(ld);
	}
	if (elementDN != NULL) {
		RPC_MEM_FREE(elementDN, RPC_C_MEM_NSRESOLUTION);
	}
	if (serverDN != NULL) {
		RPC_MEM_FREE(serverDN, RPC_C_MEM_NSRESOLUTION);
	}
}

static void rpc_ns__ldap_import_server_element(LDAP *ld,
	unsigned_char_p_t serverDN,
	rpc_if_handle_t if_spec,
	rpc_ns_handle_t *ctx,
	unsigned32 *status
	)
{
	unsigned_char_p_t filter = NULL;
	unsigned_char_p_t uuid = NULL;
	rpc_if_id_t if_id;
	LDAPMessage *msg = NULL, *e;
	rpc_ns_handle_rep_t *rep;
	unsigned_char_p_t *bindings;
	size_t len;

	rpc_if_inq_id(if_spec, &if_id, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	/* Get the interface ID */
	dce_uuid_to_string(&if_id.uuid, &uuid, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	len = strlen(uuid);
	len += sizeof("(&(objectClass=rpcServerElement)(rpcNsInterfaceID=,65535.65535))");
	RPC_MEM_ALLOC(filter, unsigned_char_p_t, len,
		RPC_C_MEM_NSRESOLUTION, RPC_C_MEM_WAITOK);

	sprintf(filter, "(&(objectClass=rpcServerElement)(rpcNsInterfaceID=%s,%hu.%hu))",
		uuid, if_id.vers_major, if_id.vers_minor);

	if (ldap_search_s(ld, serverDN, LDAP_SCOPE_ONELEVEL,
		filter, NULL, 0, &msg) != LDAP_SUCCESS) {
		*status = rpc_s_not_found;
		goto out;
	}

	e = ldap_first_entry(ld, msg);
	if (e == NULL) {
		*status = rpc_s_not_found;
		goto out;
	}

	bindings = (unsigned_char_p_t *)ldap_get_values(ld, e, "rpcNsBindings");
	if (bindings == NULL) {
		*status = rpc_s_not_found;
		goto out;
	}

	RPC_MEM_ALLOC(rep, rpc_ns_handle_rep_p_t,
		sizeof(*rep),
		RPC_C_MEM_NSRESOLUTION, RPC_C_MEM_WAITOK);

	rep->count = ldap_count_values((char **)bindings);
	rep->bindings = bindings;
	rep->cursor = 0;

	*ctx = (rpc_ns_handle_t)rep;
	*status = rpc_s_ok;

out:
	if (filter != NULL) {
		RPC_MEM_FREE(filter, RPC_C_MEM_NSRESOLUTION);
	}

	if (msg != NULL) {
		ldap_msgfree(msg);
	}
}

void
rpc_ns_binding_import_begin(
    /* [in] */ unsigned32 entry_name_syntax,
    /* [in] */ unsigned_char_p_t entry_name,
    /* [in] */ rpc_if_handle_t if_spec,
    /* [in] */ dce_uuid_p_t object_uuid ATTRIBUTE_UNUSED,
    /* [out] */ rpc_ns_handle_t *import_context,
    /* [out] */ unsigned32 *status
)
{
	unsigned_char_p_t serverDN = NULL;
	LDAP *ld;

	rpc_ns__ldap_connect_to_server(&ld, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	rpc_ns__ldap_crack_name(entry_name_syntax, entry_name, &serverDN, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	rpc_ns__ldap_import_server_element(ld, serverDN, if_spec, import_context, status);

out:
	if (serverDN != NULL) {
		RPC_MEM_FREE(serverDN, RPC_C_MEM_NSRESOLUTION);
	}
	if (ld != NULL) {
		ldap_unbind(ld);
	}
}

void
rpc_ns_binding_import_done(
    /* [in, out] */ rpc_ns_handle_t *import_context,
    /* [out] */ unsigned32 *status    
)
{
	rpc_ns_handle_rep_t *rep = (rpc_ns_handle_rep_t *)import_context;

	if (rep != NULL) {
		unsigned_char_p_t *p;

		for (p = rep->bindings; *p != NULL; p++) {
			RPC_MEM_FREE(*p, RPC_C_MEM_NSRESOLUTION);
		}
		RPC_MEM_FREE(rep, RPC_C_MEM_NSRESOLUTION);
	}
	*status = rpc_s_ok;
}

void
rpc_ns_mgmt_handle_set_exp_age(
    /* [in] */ rpc_ns_handle_t ns_handle ATTRIBUTE_UNUSED,
    /* [in] */ unsigned32 expiration_age ATTRIBUTE_UNUSED,
    /* [out] */ unsigned32 *status

)
{
	*status = rpc_s_ok;
}

void
rpc_ns_binding_import_next(
    /* [in] */ rpc_ns_handle_t import_context,
    /* [out] */ rpc_binding_handle_t *binding,
    /* [out] */ unsigned32 *status
	       )
{
	rpc_ns_handle_rep_t *rep = (rpc_ns_handle_rep_t *)import_context;

	if (rep->cursor < rep->count) {
		rpc_binding_from_string_binding(rep->bindings[rep->cursor],
			binding, status);
		rep->cursor++;
	} else {
		*status = rpc_s_no_more_bindings;
	}
}

void rpc_ns_binding_lookup_begin
(
	unsigned32              entry_name_syntax,
	unsigned_char_p_t       entry_name,
	rpc_if_handle_t         if_spec,
	uuid_p_t                object_uuid,
	unsigned32              binding_max_count ATTRIBUTE_UNUSED,
	rpc_ns_handle_t         *lookup_context,
	unsigned32              *status
)
{
	rpc_ns_binding_import_begin(entry_name_syntax,
		entry_name,
		if_spec,
		object_uuid,
		lookup_context,
		status);
}

void rpc_ns_binding_lookup_done
(
	rpc_ns_handle_t         *lookup_context,
	unsigned32              *status
)
{
	rpc_ns_binding_import_done(lookup_context, status);
}

void rpc_ns_binding_lookup_next
(
	rpc_ns_handle_t         lookup_context,
	rpc_binding_vector_p_t  *binding_vector,
	unsigned32              *status
)
{
	rpc_ns_handle_rep_t *rep = (rpc_ns_handle_rep_t *) lookup_context;

	if (rep->cursor == 0) {
		unsigned32 size, i;

		size = sizeof(rpc_binding_vector_t);
		size += (rep->count - 1) * sizeof(rpc_binding_handle_t);
		RPC_MEM_ALLOC(*binding_vector, rpc_binding_vector_p_t,
			size, RPC_C_MEM_BINDING_VEC, RPC_C_MEM_WAITOK);

		for (i = 0; i < rep->count; i++) {
			rpc_binding_from_string_binding(rep->bindings[i],
				&((*binding_vector)->binding_h[i]), status);
		}
	} else {
		*status = rpc_s_no_more_bindings;
	}
}

void rpc_ns_binding_unexport
(
	unsigned32              entry_name_syntax,
	unsigned_char_p_t       entry_name,
	rpc_if_handle_t         if_spec,
	uuid_vector_p_t         object_uuid_vector ATTRIBUTE_UNUSED,
	unsigned32              *status
)
{
	LDAP *ld = NULL;
	unsigned_char_p_t elementDN = NULL;
	unsigned_char_p_t serverDN = NULL;

	rpc_ns__ldap_connect_to_server(&ld, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	rpc_ns__ldap_crack_name(entry_name_syntax, entry_name, &serverDN, status);
	if (*status != rpc_s_ok) {
		goto out;
	}

	rpc_ns__ldap_lookup_server_element(ld, serverDN, if_spec, &elementDN, status);
	if (*status == rpc_s_ok) {
		if (ldap_delete_s(ld, elementDN) != LDAP_SUCCESS) {
			*status = rpc_s_update_failed;
			goto out;
		}
	}
	if (ldap_delete_s(ld, serverDN) != LDAP_SUCCESS) {
		*status = rpc_s_update_failed;
		goto out;
	}

	*status = rpc_s_ok;

out:
	if (ld != NULL) {
		ldap_unbind(ld);
	}
	if (elementDN != NULL) {
		RPC_MEM_FREE(elementDN, RPC_C_MEM_NSRESOLUTION);
	}
	if (serverDN != NULL) {
		RPC_MEM_FREE(serverDN, RPC_C_MEM_NSRESOLUTION);
	}
}

#endif /* BUILD_RPC_NS_LDAP */

