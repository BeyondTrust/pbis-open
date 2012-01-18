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
**  NAME:
**
**      idlbase.h
**
**  FACILITY:
**
**      IDL Stub Support Include File
**
**  ABSTRACT:
**
**  This file is #include'd by all ".h" files emitted by the IDL compiler.
**  This file defines various primitives that are missing from C but
**  present in IDL (e.g. booleans, handles).
**
*/

#ifndef IDLBASE_H
#define IDLBASE_H 	1

#if defined(__GNUC__) && (__GNUC__ >= 3)
#    define __IDL_UNUSED__ __attribute__((unused))
#    define __IDL_UNUSED_LABEL__ __IDL_UNUSED__
#else
#    define __IDL_UNUSED__
#    define __IDL_UNUSED_LABEL__
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif  /* TRUE */

#ifdef __cplusplus
    extern "C" {
#endif

/************************** Preprocessor variables *************************/

/*
 * The following variables are defined by the environment somehow:
 *
 *   MSDOS
 *       Means that the system is MS/DOS compatible.
 *   M_I86
 *       Means that the system uses an Intel 8086 cpu.
 *   cray
 *       Means that the system is CRAY/Unicos compatible.
 *   vax (__VAX for ANSI C)
 *       Means that the system uses the VAX architecture.
 *   vaxc
 *       Means that the system uses the VAXC C compiler.
 *   MIPSEL (__MIPSEL for ANSI C)
 *       Means a MIPS processor with little-endian integers
 *   apollo
 *      Means that the system is an Apollo.
 *   __STDC__
 *      Means that ANSI C prototypes are enabled.
 *
 * The following variables are defined (and undefined) within this file
 * to control the definition of macros which are emitted into header
 * files by the IDL compiler.  For each variable there is a set of default
 * definitions which is used unless a target system specific section
 * #undef-s it and supplies an alternate set of definitions.  Exactly
 * which macro definitions are governed by each variable is listed below.
 *
 *   USE_DEFAULT_NDR_REPS
 *      Controls the definition of the macros which assign a particular
 *      target system type to each NDR scalar type.  The following macros
 *      need to be defined if USE_DEFAULT_NDR_REPS is #undef-ed:
 *          ndr_boolean
 *          ndr_false
 *          ndr_true
 *          ndr_byte
 *          ndr_char
 *          ndr_small_int
 *          ndr_short_int
 *          ndr_long_int
 *          ndr_hyper_int
 *          ndr_usmall_int
 *          ndr_ushort_int
 *          ndr_ulong_int
 *          ndr_uhyper_int
 *          ndr_short_float
 *          ndr_long_float
 *
 */

/***************************************************************************/

/*
 * Work around C's flawed model for global variable definitions.
 * this definition now depends on the preprocessor variable 
 * HAS_GLOBALDEF 
 * which should be defined in platform specific dce.h file 
 */

#ifndef HAS_GLOBALDEF
#  define globaldef
#  define globalref extern
#endif /* HAS_GLOBALDEF */

/***************************************************************************/

/*
 * Unless otherwise stated, don't innocously redefine "volatile"
 * (redefining it for compilers that really support it will cause nasty
 * program bugs).  There are several compilers (it's wrong to think in
 * terms of hw platforms) that support volatile yet they don't define
 * "__STDC__", so we can't just use that.
 *
 * So, unless your compiler is explicitly listed below we don't mess
 * with "volatile".  Expressing things in this fashion errs on the cautious
 * side... at worst your compiler will complain and you can enhance the
 * list and/or add "-Dvolatile" to the cc command line.
 *
 * this definition now depends on the preprocessor variable 
 * VOLATILE_NOT_SUPPORTED 
 * which should be defined in platform specific dce.h file 
 *
 */

#ifdef VOLATILE_NOT_SUPPORTED 
#  define volatile
#endif

/*
 * define true and false
 */

#ifndef true

#ifdef NIDL_bug_boolean_def
#   define true        0xFF
#else
#   define true        TRUE
#endif /* NIDL_bug_boolean_def */

#endif /* true */

#ifndef false
#   define false       FALSE 
#endif /* false */

/***************************************************************************/

/*
 * The definition of the primitive "handle_t" IDL type.
 */
typedef struct rpc_handle_s_t *handle_t;

/***************************************************************************/

/*
 * Use the default definitions for representations for NDR scalar types
 * (unless some target specific section below #undef's the symbol) of
 * these symbols.
 *
 * for DCE 1.1, we include the platform specific file ndrtypes.h 
 */

#include <dce/ndrtypes.h>

/***************************************************************************/

typedef ndr_boolean		idl_boolean ;

#define idl_false ndr_false 
#define idl_true  ndr_true 

typedef ndr_byte		idl_byte ;

/*
 * when compiling DCE programs and/or libraries, we want the base type
 * of idl_char to be "unsigned char" (IDL doesn't support signed chars).
 * However, we compiling external programs, we want idl_char to have 
 * the char type native to the platform on which the program is being
 * compiled. So ... use a macro that should only be defined if we 
 * are building the RPC runtime of the IDL compiler.
 */

#ifndef IDL_CHAR_IS_CHAR 
typedef unsigned char idl_char ;
#else 
typedef char idl_char ;
#endif /* idl_char */

typedef ndr_small_int		idl_small_int ;

typedef ndr_usmall_int		idl_usmall_int ;

typedef ndr_short_int		idl_short_int ;

typedef ndr_ushort_int		idl_ushort_int ;

typedef ndr_long_int		idl_long_int ;

typedef ndr_ulong_int		idl_ulong_int ;

typedef ndr_hyper_int		idl_hyper_int ;

typedef ndr_uhyper_int		idl_uhyper_int ;

typedef ndr_short_float		idl_short_float ;

typedef ndr_long_float		idl_long_float ;


typedef ndr_ulong_int      idl_size_t;

typedef void * idl_void_p_t ;

/*
 *  Opaque data types
 */

typedef idl_void_p_t rpc_ss_context_t;
typedef idl_void_p_t rpc_ss_pipe_state_t;
typedef idl_void_p_t ndr_void_p_t;

/*
 *  Allocate and free node storage
 */

idl_void_p_t rpc_ss_allocate (idl_size_t);

void rpc_ss_free (idl_void_p_t);

void rpc_ss_client_free (idl_void_p_t);

/*
 *  Helper thread support
 */

typedef idl_void_p_t rpc_ss_thread_handle_t;

rpc_ss_thread_handle_t rpc_ss_get_thread_handle (void);

void rpc_ss_set_thread_handle (rpc_ss_thread_handle_t);

void rpc_ss_set_client_alloc_free (
     idl_void_p_t (*)(idl_size_t),
    void (*)(idl_void_p_t)
);
 
void rpc_ss_swap_client_alloc_free (
    idl_void_p_t (*)(idl_size_t),
    void (*)(idl_void_p_t),
    idl_void_p_t (**)(idl_size_t),
    void (**)( idl_void_p_t)
);

void rpc_ss_enable_allocate (void);

void rpc_ss_disable_allocate (void);

/*
 * Destroy an unusable client context handle
 */
void rpc_ss_destroy_client_context (rpc_ss_context_t *);



/*
 *  Prototypes for rpc_sm_... routines
 */

idl_void_p_t rpc_sm_allocate (idl_size_t, idl_ulong_int *);

void rpc_sm_client_free (idl_void_p_t, idl_ulong_int *);

void rpc_sm_destroy_client_context  (
    rpc_ss_context_t *,
    idl_ulong_int * 
);

void rpc_sm_disable_allocate (idl_ulong_int * );

void rpc_sm_enable_allocate (  idl_ulong_int * );

void rpc_sm_free (idl_void_p_t, idl_ulong_int * );

rpc_ss_thread_handle_t rpc_sm_get_thread_handle (idl_ulong_int * );

void rpc_sm_set_client_alloc_free  (
    idl_void_p_t (*)(idl_size_t),
    void (*)(idl_void_p_t ),
    idl_ulong_int *
);

void rpc_sm_set_thread_handle ( rpc_ss_thread_handle_t , idl_ulong_int * );

void rpc_sm_swap_client_alloc_free (
     idl_void_p_t (*)(idl_size_t),
    void (*)(idl_void_p_t),
    idl_void_p_t (**)(idl_size_t),
    void (**)(idl_void_p_t),
    idl_ulong_int *
);

/* International character machinery */

typedef enum {
    idl_cs_no_convert,          /* No codeset conversion required */
    idl_cs_in_place_convert,    /* Codeset conversion can be done in a single
                                    storage area */
    idl_cs_new_buffer_convert   /* The converted data must be written to a
                                    new storage area */
} idl_cs_convert_t;

#ifdef __cplusplus
    }
#endif

#ifdef DCEPrototypesDefinedLocally
#undef DCEPrototypesDefinedLocally
#endif

#endif /* IDLBASE_H */

/***************************************************************************/

/*
 *  Cause a syntax error if compiling on VAX and G_float not specified when any
 *  doubles are used.  Note that on VAX G_float *must* be used by the
 *  application, or incorrect double floating point values will be returned. 
 *  This test should remain outside the IDLBASE_H conditional, because it
 *  needs to be checked on every include.
 *
 *  This need not be done for DEC C or DEC C++
 */

#if defined(IDL_DOUBLE_USED) && ( defined(VAX) || defined(__VAX) || defined(vax) || defined(__vax) )

#if defined(vaxc) && !defined(__cplusplus) && !defined(__DECC)
#  if CC$gfloat != 1
#    if defined(vms) || defined(__VMS)
 #error "RPC requires VAX G_float data format: build with /G_FLOAT qualifier"
#    else
 #error "RPC requires VAX G_float data format: build with -Mg option"
#    endif /* vsm OR __VMS  */
#  endif   /* CC$gloat */
#endif /* vaxc && ! c++ && ! __DECC */

#if defined(__cplusplus) || defined(__DECC)
#  if __G_FLOAT != 1
 #error "RPC requires VAX G_float data format: build with /FLOAT=G_FLOAT qualifier"
#  endif/* __G_FLOAT */
#endif /* c++ or __DECC */

#if defined(ultrix) && defined(vax) && !defined(vaxc)
#  if GFLOAT != 1
 #error \
"RPC requires GFLOAT:RPC requires VAX G_float data format: build with -Mg option"
#  endif/* GFLOAT */
#endif /* ultrix && vax && ! vaxc */

#endif /* IDL_DOUBLE_USED */

