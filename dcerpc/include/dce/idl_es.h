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
**  NAME:
**
**      idl_es.h
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      User routine protoypes for IDL encoding services
**
*/

#ifndef IDL_ES_H
#define IDL_ES_H	1

#include <dce/dce.h>
#include <dce/rpc.h>

/*****************************************************************************/
/*                                                                           */
/*  IDL encoding services                                                    */
/*                                                                           */
/*****************************************************************************/

typedef struct idl_es_handle_s_t *idl_es_handle_t;

/*
** Data types for read/write/alloc routines that manage
** incremental processing of the encoding.
*/

#ifdef __cplusplus 
extern "C"  {
#endif /* __cplusplus */


typedef void (*idl_es_allocate_fn_t) (
    idl_void_p_t    ,  /* [in,out] user state */
    idl_byte	    **,   /* [out] Address of buffer */
    idl_ulong_int   *   /* [in,out] Requested Size/Size of buffer */
  );/* Routine to allocate a buffer */

typedef void (*idl_es_write_fn_t)  (
    idl_void_p_t    ,  /* [in,out] user state */ 
    idl_byte	    *,   /* [in] Encoded data */
    idl_ulong_int       /* [in] Size of encoded data */
  );   /* Routine to write encoded data */

typedef void (*idl_es_read_fn_t)  (
    idl_void_p_t ,	    /* [in,out] user state */ 
    idl_byte **,          /* [out] Data to be decoded */
    idl_ulong_int *     /* [out] Size of buf */
  );    /* Routine to read encoded data */

/*
** Provides an idl_es_handle_t which allows an encoding operation to be
** performed in an incremental fashion similar to pipe processing.
** Buffers areas are requested by the stubs via the allocate routine as
** necessary and when filled, are provided to the application via a
** call to the write routine.  This routine is suitable for encodings
** that can be incrementally processed by the applications such as when
** they are written to a file.  The application may invoke multiple
** operations utilizing the same handle.
*/

void idl_es_encode_incremental  (
    idl_void_p_t	    ,  /* [in] user state */
    idl_es_allocate_fn_t    ,  /* [in] alloc routine */
    idl_es_write_fn_t	    ,  /* [in] write routine */
    idl_es_handle_t	    *,	    /* [out] encoding handle */
    error_status_t	    *	    /* [out] status */
  );

/*
** Provides an idl_es_handle_t which allows an encoding operation to be
** performed into a buffer provided by the application.  If the buffer
** is not of sufficient size to receive the encoding the error
** rpc_s_no_memory is reported.  This mechanism provides a simple, but
** high performance encoding mechanism provided that the upper limit of
** the encoding size is known.  The application may invoke multiple
** operation utilizing the same handle.
*/
void idl_es_encode_fixed_buffer  (
    idl_byte		    *,    /* [in] pointer to buffer to    */
				    /* receive the encoding (must   */
				    /* be 8-byte aligned).	    */
    idl_ulong_int	    ,  /* [in] size of buffer provided */
    idl_ulong_int	    *, /* [out] size of the resulting  */
				    /* encoding	(set after	    */
				    /* execution of the stub)	    */
    idl_es_handle_t	    *,	    /* [out] encoding handle */
    error_status_t	    *	    /* [out] status */
  );

/*
** Provides and idl_es_handle_t which provides an encoding in a
** stub-allocated buffer.  Although this mechanism provides the
** simplest application interface, large encodings may incur a
** performance penalty for this convenience.  The return buffer is
** allocated via the client memory allocation mechanism currently in
** effect at the time of the encoding call.
*/
void idl_es_encode_dyn_buffer  (
    idl_byte		    **,   /* [out] pointer to recieve the */
				    /* dynamically allocated buffer */
				    /* which contains the encoding  */
    idl_ulong_int	    *, /* [out] size of the resulting  */
				    /* encoding (set after	    */
				    /* execution of the stub)	    */
    idl_es_handle_t	    *,	    /* [out] decoding handle */
    error_status_t	    *	    /* [out] status */
  );

/*
** Provides an idl_es_handle_t which allows an decoding operation to
** be performed in an incremental fashion similar to pipe processing.
** Buffers containing portions of the encoding are provided by the
** application via calls to the read routine.  This routine is
** suitable for encodings that can be incremental decoded by the
** stubs such as when they are read from a file.
*/
void idl_es_decode_incremental  (
    idl_void_p_t	    ,  /* [in] user state */
    idl_es_read_fn_t	    ,   /* [in] routine to supply buffers */
    idl_es_handle_t	    *,	    /* [out] decoding handle */
    error_status_t	    *	    /* [out] status */
  );

/*
** Provides an idl_es_handle_t which allows an decoding operation to
** be performed from a buffer containing the encoded data.  There may
** be a performance penalty if the buffer provided is not 8-byte
** aligned.
*/
void idl_es_decode_buffer  (
    idl_byte		    *,    /* [in] pointer to buffer	    */
				    /* containing the encoding	    */
    idl_ulong_int	    ,   /* [in] size of buffer provided */
    idl_es_handle_t	    *,	    /* [out] decoding handle */
    error_status_t	    *	    /* [out] status */
  );

/*
** Returns the rpc_if_id_t and operation number contained within the
** encoding associated with an idl_es_handle_t.  This information is
** available during a decode operation, and after the operation has
** been invoked for an encode operations.  If this information is not
** available, the status rpc_s_unknown_if is returned.
*/
void idl_es_inq_encoding_id  (
    idl_es_handle_t	    ,	    /* [in] decoding handle */
    rpc_if_id_t		    *, /* [out] RPC interface	    */
				    /* identifier (including	    */
				    /* version information)	    */
    idl_ulong_int	    *,    /* [out] operation number */
    error_status_t	    *	    /* [out] status */
  );

/*
** Frees a idl_es_handle_t and its associated resources
*/
void idl_es_handle_free (
    idl_es_handle_t	*,	    /* [in,out] handle to free */
    error_status_t	*	    /* [out] status */
  );

/*
 *  Machinery for stubs which support encodings in more than one transfer
 *  syntax
 */
typedef enum {
   idl_es_transfer_syntax_ndr
}idl_es_transfer_syntax_t;

void idl_es_set_transfer_syntax  (
    idl_es_handle_t ,
    idl_es_transfer_syntax_t ,
    error_status_t *
  );

/*
 * Routines for getting and setting the attribute flag.
 * 
 * Flags:
 * IDL_ES_NO_ENCODING_CHECK     This tells the encoding services to not
 *                              check the interface id so an interface that
 * 				did not encode data can decode parts of it
 *				(such as a common header).
 */

#define IDL_ES_NO_ENCODING_CHECK	0x1

/*
 * Flags:
 * IDL_ES_MIDL_COMPAT           Compat with Microsoft Encoding Services
 */
#define IDL_ES_MIDL_COMPAT              0x2

/*
 * Flags:
 * IDL_ES_NO_HEADER             No header (e.g. embedded in LDAP values)
 */
#define IDL_ES_NO_HEADER		0x4

void idl_es_set_attrs(
    idl_es_handle_t,	/* [in] handle */
    unsigned32,		/* [in] flags */
    error_status_t *);	/* [out] status */

void idl_es_inq_attrs(
    idl_es_handle_t,	/* [in] handle */
    unsigned32 *,	/* [out] flags */
    error_status_t *);	/* [out] status */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* IDL_ES_H */

