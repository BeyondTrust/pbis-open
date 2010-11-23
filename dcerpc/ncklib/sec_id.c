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
 */
/*
**
**  NAME
**      
**      sec_id.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**  
**  ABSTRACT:
**
**      Routines for PAC pickling.
*/

/*
 * Functions
 */
#include <commonp.h>
#include <com.h> 
#include <dce/idl_es.h>
#include <sec_id_pickle.h>

#include <unistd.h>
#include <ctype.h>

#include "pickle.h"

struct pickle_handle_s {
	idl_es_handle_t idl_h;
};

/* s e c _ p i c k l e _ c r e a t e
 *
 * Create a pickling context.  This must be called to obtain a pickling
 * context before any pickling calls can be performed.
 */
PUBLIC pickle_handle_t sec_pickle_create(void)
{
	pickle_handle_t handle;

	RPC_MEM_ALLOC(handle, pickle_handle_t,
		sizeof(*handle), RPC_C_MEM_UTIL, RPC_C_MEM_WAITOK);
	handle->idl_h = NULL;

	return handle;
}


/* s e c _ p i c k l e _ r e l e a s e
 *
 * Terminate a pickling context.  This function will release any storage
 * associated with the pickling context.
 */
PUBLIC void sec_pickle_release(pickle_handle_t *p)
{
	pickle_handle_t handle = *p;
	unsigned32 st;

	if (handle->idl_h != NULL) {
		idl_es_handle_free(&handle->idl_h, &st);
	}
	RPC_MEM_FREE(handle, RPC_C_MEM_UTIL);
	*p = NULL;
}

PRIVATE void sec_id_free(sec_id_t *identity)
{
	unsigned32 st;

	if (identity != NULL) {
		rpc_string_free(&identity->name, &st);
	}
}

/* s e c _ i d _ p a c _ f r e e
 * 
 * Release dynamic storage associated with a PAC.
 */

PUBLIC void sec_id_pac_free (sec_id_pac_t *pac)
{
	unsigned32 i;

	if (pac == NULL) {
		return;
	}

	switch (pac->pac_type) {
		case sec_id_pac_format_v1: {
			sec_id_pac_format_v1_t *v1;

			v1 = &pac->pac.v1_pac;

			sec_id_free(&v1->realm);
			sec_id_free(&v1->principal);
			sec_id_free(&v1->group);
			if (v1->groups != NULL) {
				for (i = 0; i < v1->num_groups; i++) {
					sec_id_free(&v1->groups[i]);
				}
			}
			if (v1->foreign_groups != NULL) {
				for (i = 0; i < v1->num_foreign_groups; i++) {
					sec_id_free(&v1->foreign_groups[i]);
				}
			}
			break;
		}
		case sec_id_pac_format_raw: {
			sec_id_pac_format_raw_t *raw;

			raw = &pac->pac.raw_pac;

			if (raw->value != NULL) {
				RPC_MEM_FREE(raw->value, RPC_C_MEM_UTIL);
				raw->value = NULL;
			}

			raw->type = 0;
			raw->length = 0;
			break;
		}
		default:
			break;
	}
}

/* s e c _ i d _ p a c _ p i c k l e
 *
 * Pickle a pac.
 */
PUBLIC void sec_id_pac_pickle(pickle_handle_t  handle,
	sec_id_pac_t            *pac,
	sec_id_pickled_pac_t    **pickled_pac)
{
	unsigned32 st;
	byte *data;
	unsigned32 len;

	idl_es_encode_dyn_buffer(&data, &len, &handle->idl_h, &st);
	if (st != error_status_ok) {
		*pickled_pac = NULL;
		return;
	}

	sec__id_pac_pickle(handle->idl_h, pac, &st);
	if (st != error_status_ok) {
		*pickled_pac = NULL;
		return;
	}

	*pickled_pac = (sec_id_pickled_pac_t *)data;
}

/* s e c _ i d _ p a c _ u n p i c k l e
 *
 * unpickle a pac 
 */
PUBLIC void sec_id_pac_unpickle(sec_id_pickled_pac_t *pickled_pac,
	sec_id_pac_t *pac)
{
	idl_es_handle_t h;
	unsigned32 st;
	idl_ulong_int size;

	memset(pac, 0, sizeof(*pac));

	size = sizeof(*pickled_pac) - 1;
	size += pickled_pac->num_bytes;

	idl_es_decode_buffer((idl_void_p_t)pickled_pac, size,
		&h, &st);

	sec__id_pac_unpickle(h, pac, &st);
	idl_es_handle_free(&h, &st);
}

