/*
 * 
 * (c) Copyright 1993 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1993 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1993 DIGITAL EQUIPMENT CORPORATION
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
**      stubbase.h
**
**  FACILITY:
**
**      IDL Stub Support Include File
**
**  ABSTRACT:
**
**  Definitions private to IDL-generated stubs.  This file is #include'd
**  by all stub files emitted by the IDL compiler.  The stub ".c" files
**  for interface "foo" do:
**
**      #include <foo.h>
**      #include <dce/stubbase.h>
**
**  <foo.h> does:
**
**      #include <dce/idlbase.h>
**      #include <dce/rpc.h>
**
**  Note that this file is also #include'd by the RPC runtime library
**  sources to allow the discreet sharing of definitions between the
**  stubs, stub-support library, and the RPC runtime.  The symbol NCK
**  is defined by the RPC runtime library sources so we can #ifdef out
**  the "unshared" parts of this file (which might contain stuff that
**  we don't want to deal with in the RPC runtime library compilation).
**
*/

#ifndef STUBBASE_H
#define STUBBASE_H	1

#ifdef __cplusplus
extern "C" {
#endif

#include <dce/dce.h>

/***************************************************************************/

/*
 * Data structures that define the internal representation of an interface
 * specifier (what the opaque pointer that "rpc.idl" defines points to).
 * These are the definitions used by the runtime and the runtime source
 * includes this file to get these definitions.
 */

typedef struct
{
    unsigned_char_p_t           rpc_protseq;
    unsigned_char_p_t           endpoint;
} rpc_endpoint_vector_elt_t, *rpc_endpoint_vector_elt_p_t;

typedef struct
{
    unsigned long               count;
    rpc_endpoint_vector_elt_t   *endpoint_vector_elt;
} rpc_endpoint_vector_t, *rpc_endpoint_vector_p_t;

typedef struct
{
    unsigned long               count;
    rpc_syntax_id_t             *syntax_id;
} rpc_syntax_vector_t, *rpc_syntax_vector_p_t;

typedef struct
{
    unsigned short              ifspec_vers;
    unsigned short              opcnt;
    unsigned long               vers;
    dce_uuid_t                  id;
    unsigned short              stub_rtl_if_vers;
    rpc_endpoint_vector_t       endpoint_vector;
    rpc_syntax_vector_t         syntax_vector;
    rpc_v2_server_stub_epv_t    server_epv;
    rpc_mgr_epv_t               manager_epv;
} rpc_if_rep_t, *rpc_if_rep_p_t;



/***************************************************************************/

/*
 * Define the structure of a translation table.
 */
typedef char rpc_trans_tab_t [256];
typedef rpc_trans_tab_t * rpc_trans_tab_p_t;


/***************************************************************************/

/*
 * Define the local scalar data representations used.
 */

#include <dce/ndr_rep.h>

/*
 * By default, the stub macros use the pthreads API, but there are hooks
 * to let the CMA API be used.
 */

#define STUBS_USE_PTHREADS	1

#if defined(VMS) || defined(__VMS)
#pragma nostandard
#define IDL_ENABLE_STATUS_MAPPING
#endif /* VMS */


/***************************************************************************/

/* 
 * Force stubs that are linked into the Apollo global library to use the 
 * shared version of malloc.  
 */  

#ifdef STUBS_NEED_SHARED_MALLOC
#  include <local/shlib.h>
#  include <stdlib.h>
#endif

/***************************************************************************/

/*
 * #include various files, but don't complicate the problems of compiling
 * the RPC runtime sources--don't do the #include's if NCK is defined.
 */

#ifndef NCK

#ifdef STUBS_USE_PTHREADS
# define DCETHREAD_USE_THROW
# define DCETHREAD_CHECKED
# include <dce/dcethread.h>
#else  
# include <cma.h>
#endif /* STUBS_USE_PTHREADS */

#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#if !defined(offsetof)
#define offsetof(_type, _member) \
    (((char *)&((_type *)0)->_member) - (char *)((_type *)0))
#endif /* defined(offsetof) */

#define IDL_offsetofarr(_type, _member) \
    (((char *)((_type *)0)->_member) - (char *)((_type *)0))

#if defined(__VMS) && ( defined(__DECC) || defined(__cplusplus) )
#pragma extern_model __save
#pragma extern_model __common_block __shr
#endif /* VMS or DECC or C++ */

#include <dce/rpcexc.h>


#else /* NCK defined */

#if defined(__VMS) && (defined(__DECC) || defined(__cplusplus))
#pragma extern_model __save
#pragma extern_model __common_block __shr
#endif /* VMS, DEC C, or C++ */

#endif  /* NCK */

/***************************************************************************/

/*
 * The rest of this file is typically not needed by RPC runtime library
 * source files.  However, some of that library's modules need the
 * marshalling macros; these modules #define NCK_NEED_MARSHALLING.  All
 * other RPC runtime library modules end up not seeing the rest of this
 * file.
 */

#if !defined(NCK) || defined(NCK_NEED_MARSHALLING)

/***************************************************************************/

/*
 * Untyped pointer for portability
 */

typedef pointer_t rpc_void_p_t ;


/***************************************************************************/

/*
 * The following variables are defined (and undefined) within this file
 * to control the definition of macros which are emitted into stub
 * files by the IDL compiler.  For each variable there is a set of default
 * definitions which is used unless a target system specific section
 * #undef-s it and supplies an alternate set of definitions.  Exactly
 * which macro definitions are governed by each variable is listed below.
 *
 *   USE_DEFAULT_MP_REP
 *      Controls the definition of a type and the macros which define
 *      the marshalling pointer scheme used on a particular target system.
 *      The following macros need to be defined if USE_DEFAULT_MP_REP
 *      is #undef-ed:
 *       rpc_init_mp
 *       rpc_init_op
 *       rpc_advance_mp
 *       rpc_advance_op
 *       rpc_advance_mop
 *       rpc_align_mp
 *       rpc_align_op
 *       rpc_align_mop
 *       rpc_synchronize_mp
 *
 *      and the following types need to be typedef-ed:
 *       rpc_mp_t
 *       rpc_op_t
 *
 *   USE_DEFAULT_MACROS
 *      Controls the definition of the macros which define how to marshall,
 *      unmarshall and convert values of each NDR scalar type as well
 *      as NDR string types.  The following macros need to be defined
 *      if USE_DEFAULT_MACROS is #undef-ed:
 *
 *       rpc_marshall_boolean, rpc_unmarshall_boolean, rpc_convert_boolean
 *       rpc_marshall_byte, rpc_unmarshall_byte, rpc_convert_byte
 *       rpc_marshall_char, rpc_unmarshall_char, rpc_convert_char
 *       rpc_marshall_enum, rpc_unmarshall_enum, rpc_convert_enum
 *       rpc_marshall_v1_enum, rpc_unmarshall_v1_enum, rpc_convert_v1_enum
 *       rpc_marshall_small_int, rpc_unmarshall_small_int, rpc_convert_small_int
 *       rpc_marshall_usmall_int, rpc_unmarshall_usmall_int, rpc_convert_usmall_int
 *       rpc_marshall_short_int, rpc_unmarshall_short_int, rpc_convert_short_int
 *       rpc_marshall_ushort_int, rpc_unmarshall_ushort_int, rpc_convert_ushort_int
 *       rpc_marshall_long_int, rpc_unmarshall_long_int, rpc_convert_long_int
 *       rpc_marshall_ulong_int, rpc_unmarshall_ulong_int, rpc_convert_ulong_int
 *       rpc_marshall_hyper_int, rpc_unmarshall_hyper_int, rpc_convert_hyper_int
 *       rpc_marshall_uhyper_int, rpc_unmarshall_uhyper_int, rpc_convert_uhyper_int
 *       rpc_marshall_short_float, rpc_unmarshall_short_float, rpc_convert_short_float
 *       rpc_marshall_long_float, rpc_unmarshall_long_float, rpc_convert_long_float
 */

#define USE_DEFAULT_MP_REP	1
#define USE_DEFAULT_MACROS	1
#define PACKED_SCALAR_ARRAYS	1
#define PACKED_CHAR_ARRAYS	1
#define PACKED_BYTE_ARRAYS	1

/***************************************************************************/

globalref ndr_format_t ndr_g_local_drep;

/*
 * Macro used by stubs to compare two rpc_$drep_t's.
 */

#define rpc_equal_drep(drep1, drep2)\
    (drep1.int_rep==drep2.int_rep)&&(drep1.char_rep==drep2.char_rep)&&(drep1.float_rep==drep2.float_rep)

/***************************************************************************/

/*
 * Define the versions of stubs that this instance of idl_base.h supports.
 */

#define IDL_BASE_SUPPORTS_V1	1

/***************************************************************************/

/*
 * Define some macros that stub code uses for brevity or clarity.
 */

#ifndef NULL
#define NULL 0
#endif /* NULL */

#define SIGNAL(code) {\
    error_status_t st;\
    st.all = code;\
    pfm_signal (st);\
    }


/***************************************************************************/

/*
 * include machine specific marshalling code 
 */

#include <dce/marshall.h>

/***************************************************************************/

/****
 **** Definition of the default marshalling pointer representation
 ****/

#ifdef USE_DEFAULT_MP_REP

typedef char *rpc_mp_t;
typedef ndr_ulong_int rpc_op_t;

#define rpc_init_mp(mp, bufp)\
    mp = (rpc_mp_t)bufp

#define rpc_align_mp(mp, alignment)\
    mp = (rpc_mp_t)(((mp - (rpc_mp_t)0) + (alignment-1)) & ~(alignment-1))

#define rpc_advance_mp(mp, delta)\
    mp += delta

#define rpc_init_op(op)\
    op = 0

#define rpc_advance_op(op, delta)\
    op += delta

#define rpc_align_op(op, alignment)\
    op = ((op + (alignment-1)) & ~(alignment-1))

#define rpc_advance_mop(mp, op, delta)\
    rpc_advance_mp(mp, delta);\
    rpc_advance_op(op, delta)

#define rpc_align_mop(mp, op, alignment)\
    rpc_align_mp(mp, alignment);\
    rpc_align_op(op, alignment)

#define rpc_synchronize_mp(mp, op, alignment)\
    mp += (((op & alignment-1)-(mp-(rpc_mp_t)0 & alignment-1)) &\
           (alignment-1))

#endif  /* USE_DEFAULT_MP_REP */

/***************************************************************************/
/*
 ****
 **** Definitions of the default marshall, unmarshall, and convert macros.
 ****/

#ifdef USE_DEFAULT_MACROS

#define rpc_marshall_boolean(mp, src)\
    *(ndr_boolean *)mp = src

#define rpc_unmarshall_boolean(mp, dst)\
    dst = *(ndr_boolean *)mp

#define rpc_convert_boolean(src_drep, dst_drep, mp, dst)\
    rpc_unmarshall_boolean(mp, dst)



#define rpc_marshall_byte(mp, src)\
    *(ndr_byte *)mp = src

#define rpc_unmarshall_byte(mp, dst)\
    dst = *(ndr_byte *)mp

#define rpc_convert_byte(src_drep, dst_drep, mp, dst)\
    rpc_unmarshall_byte(mp, dst)



#define rpc_marshall_char(mp, src)\
    *(ndr_char *)mp = src

#define rpc_unmarshall_char(mp, dst)\
    dst = *(ndr_char *)mp

#define rpc_convert_char(src_drep, dst_drep, mp, dst)\
    if (src_drep.char_rep == dst_drep.char_rep)\
        rpc_unmarshall_char(mp, dst);\
    else if (dst_drep.char_rep == ndr_c_char_ascii)\
        dst = (*ndr_g_ebcdic_to_ascii) [*(ndr_char *)mp];\
    else\
        dst = (*ndr_g_ascii_to_ebcdic) [*(ndr_char *)mp]


#define rpc_marshall_enum(mp, src)\
    *(ndr_short_int *)mp = (ndr_short_int)src

#define rpc_unmarshall_enum(mp, dst)\
    dst = *(ndr_short_int *)mp

#define rpc_convert_enum(src_drep, dst_drep, mp, dst)\
    if (src_drep.int_rep == dst_drep.int_rep)\
        rpc_unmarshall_enum(mp, dst);\
    else {\
        ndr_short_int _sh;\
        ndr_byte *_d = (ndr_byte *) &_sh;\
        ndr_byte *_s = (ndr_byte *) mp;\
        _d[0]=_s[1]; _d[1]=_s[0];\
        dst = _sh;\
        }



#ifdef TWO_BYTE_ENUMS
#define rpc_marshall_v1_enum(mp, src)\
    rpc_marshall_ushort(mp, 0);\
    rpc_marshall_enum((mp+2), src)

#define rpc_unmarshall_v1_enum(mp, dst)\
    rpc_unmarshall_enum((mp+2), src)

#define rpc_convert_v1_enum(src_drep, dst_drep, mp, dst)\
    if (src_drep.int_rep == dst_drep.int_rep)\
        rpc_unmarshall_v1_enum(mp, dst);\
    else {\
        ndr_ushort_int _sh;\
        ndr_byte *_d = (ndr_byte *) &_sh;\
        ndr_byte *_s = (ndr_byte *) mp;\
        _d[0]=_s[1]; _d[1]=_s[0];\
        dst = _sh;\
        }
#else
#define rpc_marshall_v1_enum(mp, src)\
    *(ndr_ulong_int *)mp = (ndr_ulong_int)src

#define rpc_unmarshall_v1_enum(mp, dst)\
    dst = *(ndr_ulong_int *)mp

#define rpc_convert_v1_enum(src_drep, dst_drep, mp, dst)\
    if (src_drep.int_rep == dst_drep.int_rep)\
        rpc_unmarshall_v1_enum(mp, dst);\
    else {\
        ndr_ulong_int _l;\
        ndr_byte *_d = (ndr_byte *) &_l;\
        ndr_byte *_s = (ndr_byte *) mp;\
        _d[0]=_s[3]; _d[1]=_s[2]; _d[2]=_s[1]; _d[3]=_s[0];\
        dst = _l;\
        }
#endif /* TWO_BYTE_ENUMS */


#define rpc_marshall_small_int(mp, src)\
    *(ndr_small_int *)mp = src

#define rpc_unmarshall_small_int(mp, dst)\
    dst = *(ndr_small_int *)mp

#define rpc_convert_small_int(src_drep, dst_drep, mp, dst)\
    rpc_unmarshall_small_int(mp, dst)



#define rpc_marshall_usmall_int(mp, src)\
    *(ndr_usmall_int *)mp = src

#define rpc_unmarshall_usmall_int(mp, dst)\
    dst = *(ndr_usmall_int *)mp

#define rpc_convert_usmall_int(src_drep, dst_drep, mp, dst)\
    rpc_unmarshall_usmall_int(mp, dst)



#define rpc_marshall_short_int(mp, src)\
    *(ndr_short_int *)mp = src

#define rpc_unmarshall_short_int(mp, dst)\
    dst = *(ndr_short_int *)mp

#define rpc_convert_short_int(src_drep, dst_drep, mp, dst)\
    if (src_drep.int_rep == dst_drep.int_rep)\
        rpc_unmarshall_short_int(mp, dst);\
    else {\
        ndr_byte *_d = (ndr_byte *) &dst;\
        ndr_byte *_s = (ndr_byte *) mp;\
        _d[0]=_s[1]; _d[1]=_s[0];\
        }



#define rpc_marshall_ushort_int(mp, src)\
    *(ndr_ushort_int *)mp = (ndr_ushort_int)src

#define rpc_unmarshall_ushort_int(mp, dst)\
    dst = *(ndr_ushort_int *)mp

#define rpc_convert_ushort_int(src_drep, dst_drep, mp, dst)\
    if (src_drep.int_rep == dst_drep.int_rep)\
        rpc_unmarshall_ushort_int(mp, dst);\
    else {\
        ndr_byte *_d = (ndr_byte *) &dst;\
        ndr_byte *_s = (ndr_byte *) mp;\
        _d[0]=_s[1]; _d[1]=_s[0];\
        }



#define rpc_marshall_long_int(mp, src)\
    *(ndr_long_int *)mp = src

#define rpc_unmarshall_long_int(mp, dst)\
    dst = *(ndr_long_int *)mp

#define rpc_convert_long_int(src_drep, dst_drep, mp, dst)\
    if (src_drep.int_rep == dst_drep.int_rep)\
        rpc_unmarshall_long_int(mp, dst);\
    else {\
        ndr_byte *_d = (ndr_byte *) &dst;\
        ndr_byte *_s = (ndr_byte *) mp;\
        _d[0]=_s[3]; _d[1]=_s[2]; _d[2]=_s[1]; _d[3]=_s[0];\
        }



#define rpc_marshall_ulong_int(mp, src)\
    *(ndr_ulong_int *)mp = (ndr_ulong_int)src

#define rpc_unmarshall_ulong_int(mp, dst)\
    dst = *(ndr_ulong_int *)mp

#define rpc_convert_ulong_int(src_drep, dst_drep, mp, dst)\
    if (src_drep.int_rep == dst_drep.int_rep)\
        rpc_unmarshall_ulong_int(mp, dst);\
    else {\
        ndr_byte *_d = (ndr_byte *) &dst;\
        ndr_byte *_s = (ndr_byte *) mp;\
        _d[0]=_s[3]; _d[1]=_s[2]; _d[2]=_s[1]; _d[3]=_s[0];\
        }



#define rpc_marshall_hyper_int(mp, src) {\
    *(ndr_hyper_int *)mp = *(ndr_hyper_int *)&src;\
    }

#define rpc_unmarshall_hyper_int(mp, dst) {\
    *(ndr_hyper_int *)&dst = *(ndr_hyper_int *)mp;\
    }

#define rpc_convert_hyper_int(src_drep, dst_drep, mp, dst)\
    if (src_drep.int_rep == dst_drep.int_rep)\
        rpc_unmarshall_hyper_int(mp, dst)\
    else {\
        ndr_byte *_d = (ndr_byte *) &dst;\
        ndr_byte *_s = (ndr_byte *) mp;\
        _d[0]=_s[7]; _d[1]=_s[6]; _d[2]=_s[5]; _d[3]=_s[4];\
        _d[4]=_s[3]; _d[5]=_s[2]; _d[6]=_s[1]; _d[7]=_s[0];\
        }



#define rpc_marshall_uhyper_int(mp, src) {\
    *(ndr_uhyper_int *)mp = *(ndr_uhyper_int *)&src;\
    }

#define rpc_unmarshall_uhyper_int(mp, dst) {\
    *(ndr_uhyper_int *)&dst = *(ndr_uhyper_int *)mp;\
    }

#define rpc_convert_uhyper_int(src_drep, dst_drep, mp, dst)\
    if (src_drep.int_rep == dst_drep.int_rep)\
        rpc_unmarshall_uhyper_int(mp, dst)\
    else {\
        ndr_byte *_d = (ndr_byte *) &dst;\
        ndr_byte *_s = (ndr_byte *) mp;\
        _d[0]=_s[7]; _d[1]=_s[6]; _d[2]=_s[5]; _d[3]=_s[4];\
        _d[4]=_s[3]; _d[5]=_s[2]; _d[6]=_s[1]; _d[7]=_s[0];\
        }



#define rpc_marshall_short_float(mp, src) {\
    ndr_short_float tmp;\
    tmp = src;\
    *(ndr_short_float *)mp = tmp;\
    }

#define rpc_unmarshall_short_float(mp, dst)\
    dst = *(ndr_short_float *)mp

#define rpc_convert_short_float(src_drep, dst_drep, mp, dst)\
    if ((src_drep.float_rep == dst_drep.float_rep) &&\
        (src_drep.int_rep   == dst_drep.int_rep))\
        rpc_unmarshall_short_float(mp, dst);\
    else {\
        ndr_cvt_short_float (src_drep, dst_drep,\
            (short_float_p_t)mp,\
            (short_float_p_t)&dst);\
        }



#define rpc_marshall_long_float(mp, src)\
    *(ndr_long_float *)mp = src

#define rpc_unmarshall_long_float(mp, dst)\
    dst = *(ndr_long_float *)mp

#define rpc_convert_long_float(src_drep, dst_drep, mp, dst)\
    if ((src_drep.float_rep == dst_drep.float_rep) &&\
        (src_drep.int_rep   == dst_drep.int_rep))\
        rpc_unmarshall_long_float(mp, dst);\
    else\
        ndr_cvt_long_float (src_drep, dst_drep,\
            (long_float_p_t)mp,\
            (long_float_p_t)&dst)

#endif  /* USE_DEFAULT_MACROS */

/***************************************************************************/
/***************************************************************************/

/*
 *  Macros to give independence from threading package
 */

#define RPC_SS_EXC_MATCHES dcethread_exc_matches

#ifdef STUBS_USE_PTHREADS

#define RPC_SS_THREADS_INIT

#define RPC_SS_THREADS_ONCE_T dcethread_oncectl

#define RPC_SS_THREADS_ONCE_INIT DCETHREAD_ONCE_INIT

#define RPC_SS_THREADS_ONCE(once_block_addr,init_routine) dcethread_once_throw( \
    once_block_addr,(dcethread_initroutine)init_routine)


#define RPC_SS_THREADS_MUTEX_T dcethread_mutex

#define RPC_SS_THREADS_MUTEX_CREATE(mutex_addr) dcethread_mutex_init_throw(  \
    mutex_addr, NULL )

#define RPC_SS_THREADS_MUTEX_LOCK(mutex_addr) dcethread_mutex_lock_throw(mutex_addr)

#define RPC_SS_THREADS_MUTEX_UNLOCK(mutex_addr) dcethread_mutex_unlock_throw(mutex_addr)

#define RPC_SS_THREADS_MUTEX_DELETE(mutex_addr) dcethread_mutex_destroy_throw(mutex_addr)

typedef void *rpc_ss_threads_dest_arg_t;

#define RPC_SS_THREADS_KEY_T dcethread_key

#define RPC_SS_THREADS_KEY_CREATE(key_addr,destructor) dcethread_keycreate_throw( \
    (key_addr),(destructor))

#define RPC_SS_THREADS_KEY_SET_CONTEXT(key,value) dcethread_setspecific_throw( \
    (key),(dcethread_addr)(value))

#define RPC_SS_THREADS_KEY_GET_CONTEXT(key,value_addr) dcethread_getspecific_throw( \
    key,(dcethread_addr)(value_addr))

#define RPC_SS_THREADS_CONDITION_T dcethread_cond

#define RPC_SS_THREADS_CONDITION_CREATE(condition_addr) dcethread_cond_init_throw( \
    condition_addr, NULL)

#define RPC_SS_THREADS_CONDITION_SIGNAL(condition_addr) dcethread_cond_signal_throw( \
    condition_addr)

#define RPC_SS_THREADS_CONDITION_WAIT(condition_addr,mutex_addr) dcethread_cond_wait_throw( \
    condition_addr,mutex_addr)

#define RPC_SS_THREADS_CONDITION_DELETE(condition_addr) dcethread_cond_destroy_throw( \
    condition_addr)

#define RPC_SS_THREADS_X_CANCELLED dcethread_interrupt_e

#define RPC_SS_THREADS_CANCEL_STATE_T int
#define RPC_SS_THREADS_CANCEL_STATE_T_INITIALIZER 0

#define RPC_SS_THREADS_DISABLE_ASYNC(state) state=dcethread_enableasync_throw(0)

#define RPC_SS_THREADS_RESTORE_ASYNC(state) dcethread_enableasync_throw(state)

#define RPC_SS_THREADS_DISABLE_GENERAL(state) state=dcethread_enableinterrupt_throw(0)

#define RPC_SS_THREADS_ENABLE_GENERAL(state) state=dcethread_enableinterrupt_throw(1)

#define RPC_SS_THREADS_RESTORE_GENERAL(state) state=dcethread_enableinterrupt_throw(state)

#else

#define RPC_SS_THREADS_INIT cma_init()

#define RPC_SS_THREADS_ONCE_T cma_t_once

#define RPC_SS_THREADS_ONCE_INIT cma_once_init

#define RPC_SS_THREADS_ONCE(once_block_addr,init_routine) cma_once( \
    once_block_addr,(cma_t_init_routine)init_routine,&cma_c_null)

#define RPC_SS_THREADS_MUTEX_T cma_t_mutex

#define RPC_SS_THREADS_MUTEX_CREATE(mutex_addr) cma_mutex_create( \
    mutex_addr, &cma_c_null )

#define RPC_SS_THREADS_MUTEX_LOCK(mutex_addr) cma_mutex_lock((mutex_addr))

#define RPC_SS_THREADS_MUTEX_UNLOCK(mutex_addr) cma_mutex_unlock((mutex_addr))

#define RPC_SS_THREADS_MUTEX_DELETE(mutex_addr) cma_mutex_delete(mutex_addr)

typedef cma_t_address rpc_ss_threads_dest_arg_t;

#define RPC_SS_THREADS_KEY_T cma_t_key

#define RPC_SS_THREADS_KEY_CREATE(key_addr,destructor) cma_key_create( \
    key_addr,&cma_c_null,destructor)

#define RPC_SS_THREADS_KEY_SET_CONTEXT(key,value) cma_key_set_context( \
    key,(cma_t_address)value)

#define RPC_SS_THREADS_KEY_GET_CONTEXT(key,value_addr) cma_key_get_context( \
    key,(cma_t_address *)value_addr)

#define RPC_SS_THREADS_CONDITION_T cma_t_cond

#define RPC_SS_THREADS_CONDITION_CREATE(condition_addr) cma_cond_create( \
    condition_addr,&cma_c_null)

#define RPC_SS_THREADS_CONDITION_SIGNAL(condition_addr) cma_cond_signal( \
    condition_addr)

#define RPC_SS_THREADS_CONDITION_WAIT(condition_addr,mutex_addr) cma_cond_wait( \
    condition_addr,mutex_addr)

#define RPC_SS_THREADS_CONDITION_DELETE(condition_addr) cma_cond_delete( \
    condition_addr)

#define RPC_SS_THREADS_X_CANCELLED cma_e_alerted

#define RPC_SS_THREADS_CANCEL_STATE_T cma_t_alert_state
#define RPC_SS_THREADS_CANCEL_STATE_T_INITIALIZER {0}

#define RPC_SS_THREADS_DISABLE_ASYNC(state) cma_alert_disable_asynch(&state)

#define RPC_SS_THREADS_RESTORE_ASYNC(state) cma_alert_restore(&state)

#define RPC_SS_THREADS_DISABLE_GENERAL(state) cma_alert_disable_general(&state)

#define RPC_SS_THREADS_ENABLE_GENERAL(state) cma_alert_enable_general(&state)

#define RPC_SS_THREADS_RESTORE_GENERAL(state) cma_alert_restore(&state)

#endif  /* STUBS_USE_PTHREADS */

/*
 * A macro to set up an iovector of appropriate size
 */

#define IoVec_t(n)                 \
    struct                         \
    {                              \
        unsigned16 num_elt;        \
        rpc_iovector_elt_t elt[n]; \
    }

#define NULL_FREE_RTN (rpc_buff_dealloc_fn_t)0

typedef rpc_buff_dealloc_fn_t rpc_ss_dealloc_t;

/*
 * Memory allocation stub support definitions
 */

/*
 * Wrapper needed for free() routine assignments in generated files.
 */

void rpc_ss_call_free   ( rpc_void_p_t );

/*
 * Table of caller nodes that are marshalled during an operation
 */

typedef byte_p_t rpc_ss_node_table_t;

/*
 * An allocation handle; initialize to NULL
 */

typedef volatile struct {
    rpc_void_p_t memory;
    rpc_ss_node_table_t node_table;
    rpc_void_p_t (*alloc)(idl_size_t size);
    void (*free)(rpc_void_p_t obj);
} rpc_ss_mem_handle;

/*
 * rpc_ss_mem_alloc
 *
 * Allocates and returns a pointer to a block of memory
 * aligned to an eight-byte boundary containing bytes bytes
 * Returns NULL if unable to allocate
 */

byte_p_t rpc_ss_mem_alloc   (
    rpc_ss_mem_handle *,  /* The (initially NULL) allocation handle */
    unsigned               /* Number of bytes to allocate */
);

/*
 * rpc_sm_mem_alloc
 *
 * Allocates and returns a pointer to a block of memory
 * aligned to an eight-byte boundary containing bytes bytes
 * Returns NULL if unable to allocate
 * returns rpc_s_no_memory instead of Raise( rpc_x_no_memory)
 */
byte_p_t rpc_sm_mem_alloc   (
    rpc_ss_mem_handle *,    /* The (initially NULL) allocation handle */
    unsigned,               /* Number of bytes to allocate */
    error_status_t *        /*The status parameter if alloc returns NULL */
);


/*
 * rpc_ss_mem_free
 *
 * Frees all memory associated with the handle passed
 */

void rpc_ss_mem_free   (rpc_ss_mem_handle *);



/*
 * rpc_ss_mem_release
 *
 * Removes a block of memory from the list associated with a handle
 * Frees the memory if 'free' is non-zero
 */

void rpc_ss_mem_release   (
    rpc_ss_mem_handle *,
    byte_p_t ,/* The aligned address of the memory to be
                          released */
    int /*freeit*/         /* Non-zero if the memory should be freed */
);

#ifdef MIA
/*
 * rpc_ss_mem_item_free
 *
 * Same functionality as rpc_ss_mem_release with freeit != 0
 */
void rpc_ss_mem_item_free   (
    rpc_ss_mem_handle *,
    byte_p_t  /* The aligned address of the memory to be
                          released */
);
#endif /* MIA  */

#if 0
/*
 * rpc_ss_mem_dealloc
 *
 * A routine to pass to the runtime: passed an aligned data address,
 * frees the original block of memory containing the address.
 */
void rpc_ss_mem_dealloc   ( byte_p_t );
#endif


/*
 * To avoid passing parameter lists of excessive length, a set of values
 * needed while unmarshalling a complex parameter is held in a parameter
 * block
 */
typedef struct rpc_ss_marsh_state_t
{
    rpc_mp_t mp;                      /* unmarshalling pointer */
    unsigned long op;                 /* offset from start of parameters */
    ndr_format_t src_drep;            /* sender's data representation */
    rpc_iovector_elt_t *p_rcvd_data;  /* address of received data descriptor */
    rpc_ss_mem_handle *p_mem_h;       /* ptr to stub memory management handle */
    rpc_call_handle_t call_h;
    rpc_void_p_t (*p_allocate) ( idl_size_t );
    void (*p_free) ( rpc_void_p_t ) ;
    rpc_ss_node_table_t node_table;   /* node number to pointer table */
    unsigned long space_in_buff;      /* Space left in buffer */
    rpc_iovector_t  *p_iovec;         /* Address of I/O vector */
    error_status_t  *p_st;            /* Return status */
    unsigned long   version;          /* Version number field */
} rpc_ss_marsh_state_t;


/*
 * MARSHALLING AND UNMARSHALLING OF NODES
 */

typedef enum { rpc_ss_mutable_node_k, rpc_ss_old_ref_node_k,
               rpc_ss_new_ref_node_k, rpc_ss_alloc_ref_node_k,
               rpc_ss_unique_node_k
} rpc_ss_node_type_k_t;

/* 
 * Use address of a function as "an address which cannot be a valid data
 *   address" 
 */

#define RPC_SS_NEW_UNIQUE_NODE (-1)

/*
 * Support operations for pointer <-> node number mapping
 */

void rpc_ss_init_node_table   (
    volatile rpc_ss_node_table_t *, 
    rpc_ss_mem_handle *
);

void rpc_ss_enable_reflect_deletes  (
 rpc_ss_node_table_t 
);

idl_ulong_int rpc_ss_register_node   (
    rpc_ss_node_table_t  /*tab*/,
    byte_p_t  /*ptr*/,
    long  /*marshalling*/,
    long * /*has_been_marshalled*/
);


byte_p_t rpc_ss_lookup_node_by_num   (
    rpc_ss_node_table_t  /*tab*/,
    idl_ulong_int /*num*/
);

byte_p_t rpc_ss_lookup_pointer_to_node   (
    rpc_ss_node_table_t  /*tab*/,
    idl_ulong_int  /*num*/,
    long * /*has_been_unmarshalled*/
);

byte_p_t rpc_ss_return_pointer_to_node   (
    rpc_ss_node_table_t  /*tab*/,
    idl_ulong_int  /*num*/,
    idl_ulong_int  /*size*/,
    rpc_void_p_t (*p_allocate) ( idl_size_t),
    long * /*has_been_unmarshalled*/,
    long * /*new_node*/
);

void rpc_ss_unregister_node   (
    rpc_ss_node_table_t  /*tab*/,
    byte_p_t  /*ptr*/
);


#define NIDL_BUFF_SIZE 2048

void rpc_ss_marsh_change_buff   (
    rpc_ss_marsh_state_t * /*msp*/,          /* marshalling state */
    unsigned long /*size_next_structure*/   /* Size needed in marshalling buffer */
);

/*
 *  Stub initialization macros
 */

extern ndr_boolean rpc_ss_client_is_set_up;
extern ndr_boolean rpc_ss_server_is_set_up;
extern ndr_boolean rpc_ss_allocate_is_set_up;
extern ndr_boolean rpc_ss_context_is_set_up;

void rpc_ss_init_client_once   (void);

void rpc_ss_init_server_once   (void);


void rpc_ss_init_allocate_once  (void);

void rpc_ss_init_context_once   (void);


#define RPC_SS_INIT_CLIENT if(!rpc_ss_client_is_set_up)rpc_ss_init_client_once();

#define RPC_SS_INIT_SERVER if(!rpc_ss_server_is_set_up)rpc_ss_init_server_once();

#define RPC_SS_INIT_ALLOCATE if(!rpc_ss_allocate_is_set_up)rpc_ss_init_allocate_once();

#define RPC_SS_INIT_CONTEXT if(!rpc_ss_context_is_set_up)rpc_ss_init_context_once();

#if defined(VMS) || defined(__VMS)
#    define IDL_ENABLE_STATUS_MAPPING
#    undef RPC_SS_INIT_CLIENT
#    define RPC_SS_INIT_CLIENT if(!rpc_ss_client_is_set_up) \
        { \
            rpc_ss_init_client_once(); \
            rpc_ss_client_is_set_up = ndr_true; \
        }
#    undef RPC_SS_INIT_SERVER
#    define RPC_SS_INIT_SERVER if(!rpc_ss_server_is_set_up) \
        { \
            rpc_ss_init_server_once(); \
            rpc_ss_server_is_set_up = ndr_true; \
        }
#    undef RPC_SS_INIT_ALLOCATE
#    define RPC_SS_INIT_ALLOCATE if(!rpc_ss_allocate_is_set_up) \
        { \
            rpc_ss_init_allocate_once(); \
            rpc_ss_allocate_is_set_up = ndr_true; \
        }
#    undef RPC_SS_INIT_CONTEXT
#    define RPC_SS_INIT_CONTEXT if(!rpc_ss_context_is_set_up) \
        { \
            rpc_ss_init_context_once(); \
            rpc_ss_context_is_set_up = ndr_true; \
        }
#endif /* VMS */

#ifdef MEMORY_NOT_WRITTEN_SERIALLY

#    undef RPC_SS_INIT_CLIENT
#    define RPC_SS_INIT_CLIENT rpc_ss_init_client_once();

#    undef RPC_SS_INIT_SERVER
#    define RPC_SS_INIT_SERVER rpc_ss_init_server_once();
#endif

/*
 *  CMA SUPPORT ROUTINES
 */

void rpc_ss_send_server_exception   (
    rpc_call_handle_t,
    dcethread_exc *
);

void rpc_ss_report_error   (
    ndr_ulong_int  /*fault_code*/,
    ndr_ulong_int  /*result_code*/,
    RPC_SS_THREADS_CANCEL_STATE_T  /*async_cancel_state*/,
    error_status_t * /*p_comm_status*/,
    error_status_t * /*p_fault_status*/
);

/*
 * OPTIMIZATION SUPPORT
 */
void rpc_ss_new_recv_buff   (
    rpc_iovector_elt_t * /*elt*/,
    rpc_call_handle_t  /*call_h*/,
    rpc_mp_t * /*p_mp*/,
    volatile error_status_t * /*st*/
);


/*
 * SUPPORT FOR MULTI-THREADING
 */

/*
 * Thread local structure to give node support routines access to stub
 * variables
 */

typedef struct rpc_ss_thread_support_ptrs_t
{
    RPC_SS_THREADS_MUTEX_T mutex;    /* Helper thread protection */
    rpc_ss_mem_handle *p_mem_h;
    rpc_void_p_t (*p_allocate) (idl_size_t);
    void (*p_free) ( rpc_void_p_t) ;
} rpc_ss_thread_support_ptrs_t;

/*
 *  Thread local storage to indirect to the support pointers structure
 */
typedef struct rpc_ss_thread_indirection_t
{
    rpc_ss_thread_support_ptrs_t *indirection;
    idl_boolean free_referents;     /* TRUE => release the referents of
                                    indirection at thread termination */
} rpc_ss_thread_indirection_t;

extern RPC_SS_THREADS_KEY_T rpc_ss_thread_supp_key;

void rpc_ss_build_indirection_struct  (
    rpc_ss_thread_support_ptrs_t * /*p_thread_support_ptrs*/,
    rpc_ss_mem_handle * /*p_mem_handle*/,
    idl_boolean  /*free_referents*/
);

void rpc_ss_create_support_ptrs   (
    rpc_ss_thread_support_ptrs_t * /*p_thread_support_ptrs*/,
    rpc_ss_mem_handle * /*p_mem_handle*/
);

void rpc_ss_get_support_ptrs  (
    rpc_ss_thread_support_ptrs_t ** /*p_p_thread_support_ptrs*/
);

void rpc_ss_destroy_support_ptrs   (void);


void rpc_ss_client_establish_alloc  (rpc_ss_marsh_state_t *);

/*
 *    MARSHALLING AND UNMARSHALLING PIPES
 */

#define NIDL_PIPE_BUFF_SIZE 2048

/*
 * Data structure for maintaining pipe status
 */

typedef struct rpc_ss_ee_pipe_state_t
{
    long pipe_number;
    long next_in_pipe;               /* if -ve, next pipe is [out] */
    long next_out_pipe;
    long *p_current_pipe;            /* +ve curr pipe is [in], -ve for [out] */
    unsigned long left_in_wire_array;
    rpc_mp_t *p_mp;                  /* address of marshalling pointer */
    rpc_op_t *p_op;                  /* address of offset pointer */
    ndr_format_t src_drep;           /* sender's data representation */
    rpc_iovector_elt_t *p_rcvd_data; /* addr of received data descriptor */
    rpc_ss_mem_handle *p_mem_h;      /* ptr to stub memory management handle */
    rpc_call_handle_t call_h;
    ndr_boolean pipe_drained;         /* used only when pipe is [in] */
    ndr_boolean pipe_filled;          /* used only when pipe is [out] */
    error_status_t *p_st;             /* address of status in the stub */
} rpc_ss_ee_pipe_state_t;

void rpc_ss_initialize_callee_pipe   (
    long ,        /* index of pipe in set of pipes in the
                                        operation's parameter list */
    long ,      /* index of next [in] pipe to process */
    long ,     /* index of next [out] pipe to process */
    long *,            /* ptr to index num and dirn of active pipe */
    rpc_mp_t *,                  /* ptr to marshalling pointer */
    rpc_op_t *,                  /* ptr to offset pointer */
    ndr_format_t ,           /* sender's data representation */
    rpc_iovector_elt_t *, /* Addr of received data descriptor */
    rpc_ss_mem_handle *,      /* ptr to stub memory allocation handle */
    rpc_call_handle_t ,
    rpc_ss_ee_pipe_state_t **, /* address of ptr to pipe state block */
    error_status_t *
);

#define rpc_p_pipe_state ((rpc_ss_ee_pipe_state_t *)state)


/*
 *    CONTEXT HANDLE STUFF
 */

typedef struct
{
    ndr_context_handle context_on_wire;
    handle_t using_handle;
} rpc_ss_caller_context_element_t;

extern RPC_SS_THREADS_MUTEX_T rpc_ss_context_table_mutex;

/*
 * typedef for context rundown procedure 
 */

typedef void (*ctx_rundown_fn_p_t)(rpc_ss_context_t);

void rpc_ss_er_ctx_to_wire   (
    rpc_ss_context_t        ,  /* [in] opaque pointer */
    ndr_context_handle      *, /* [out] ndr_context_handle */
    handle_t                ,               /* binding handle */
    ndr_boolean             ,          /* TRUE for [in, out] parameters */
    volatile error_status_t *
);

void rpc_ss_er_ctx_from_wire   (
    ndr_context_handle      *,   /* [in] ndr_context_handle */
    rpc_ss_context_t        *, /* [out] opaque pointer */
    handle_t                ,                 /* binding handle */
    ndr_boolean             ,            /* TRUE for [in, out] parameters */
    volatile error_status_t *
);



void rpc_ss_ee_ctx_to_wire   (
    rpc_ss_context_t        ,   /* [in] opaque pointer */
    ndr_context_handle      *,  /* [out] ndr_context_handle */
    handle_t                ,   /* binding handle */
    ctx_rundown_fn_p_t      ,   /* Pointer to context rundown routine */
    ndr_boolean             ,   /* TRUE for [in, out] parameters */
    volatile error_status_t *
);

void rpc_ss_ee_ctx_from_wire   (
    ndr_context_handle      *,   /* [in] ndr_context_handle */
    rpc_ss_context_t        *, /* [out] opaque pointer */
    volatile error_status_t *
);

void rpc_ss_ctx_client_ref_count_inc   (
  handle_t ,
  error_status_t *
);

void rpc_ss_ctx_client_ref_count_dec   (
  handle_t h, 
  error_status_t *
);

void rpc_ss_ctx_client_ref_count_i_2   (
    handle_t ,
    rpc_client_handle_t *,
    error_status_t *
);

void rpc_ss_ctx_client_ref_count_d_2 (
    handle_t ,
    rpc_client_handle_t
);

void rpc_ss_init_callee_ctx_tables   (void);

#define uuid_tOmr rpc_ss_m_uuid
#define uuid_tOur rpc_ss_u_uuid
#define uuid_tOme rpc_ss_m_uuid
#define uuid_tOue rpc_ss_u_uuid

void rpc_ss_m_uuid  (
   dce_uuid_t *, 
   rpc_ss_marsh_state_t *
);

void rpc_ss_u_uuid   (
   dce_uuid_t *, 
   rpc_ss_marsh_state_t *
);

/*
 *    MARSHALLING AND UNMARSHALLING OF POINTED AT SCALARS
 */

void rpc_ss_mr_boolean   (
     idl_boolean *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_boolean   (
    idl_boolean **,
    rpc_ss_node_type_k_t, 
    rpc_ss_marsh_state_t *
);

void rpc_ss_me_boolean   (
    idl_boolean *,
    rpc_ss_node_type_k_t,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_boolean   (
    idl_boolean **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_mr_byte   (
    idl_byte *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_byte   (
    idl_byte **, 
    rpc_ss_node_type_k_t,
    rpc_ss_marsh_state_t *
);

void rpc_ss_me_byte   (
    idl_byte *,
    rpc_ss_node_type_k_t,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_byte   (
    idl_byte **, 
    rpc_ss_node_type_k_t , 
    rpc_ss_marsh_state_t *
);

void rpc_ss_mr_char   (
    idl_char *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_char   (
    idl_char **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_me_char   (
    idl_char *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_char   (
    idl_char **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);


void rpc_ss_mr_enum   (
    int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_enum   (
    int **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_me_enum   (
    int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_enum   (
    int **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_mr_small_int   (
    idl_small_int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_small_int   (
    idl_small_int **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_me_small_int   (
    idl_small_int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_small_int   (
    idl_small_int **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_mr_short_int   (
    idl_short_int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_short_int   (
    idl_short_int **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_me_short_int  (
    idl_short_int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_short_int  (
    idl_short_int **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_mr_long_int  (
    idl_long_int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_long_int  (
    idl_long_int **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_me_long_int  (
    idl_long_int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_long_int  (
    idl_long_int **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);


void  rpc_ss_mr_hyper_int  (
    idl_hyper_int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_hyper_int  (
    idl_hyper_int **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_me_hyper_int  (
    idl_hyper_int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_hyper_int  (
    idl_hyper_int **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_mr_usmall_int  (
    idl_usmall_int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_usmall_int  (
    idl_usmall_int **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);


void rpc_ss_me_usmall_int   (
    idl_usmall_int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_usmall_int  (
    idl_usmall_int **, 
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_mr_ushort_int  (
    idl_ushort_int *, 
    rpc_ss_node_type_k_t,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_ushort_int  (
    idl_ushort_int **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_me_ushort_int  (
    idl_ushort_int *,
    rpc_ss_node_type_k_t, 
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_ushort_int  (
    idl_ushort_int **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_mr_ulong_int  (
    idl_ulong_int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_ulong_int  (
    idl_ulong_int **, 
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_me_ulong_int  (
    idl_ulong_int *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_ulong_int  (
    idl_ulong_int **, 
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_mr_uhyper_int  (
    idl_uhyper_int *, 
    rpc_ss_node_type_k_t,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_uhyper_int  (
    idl_uhyper_int **, 
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_me_uhyper_int (
    idl_uhyper_int *,
    rpc_ss_node_type_k_t,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_uhyper_int  (
    idl_uhyper_int **, 
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_mr_short_float  (
    idl_short_float *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_short_float  (
    idl_short_float **,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_me_short_float  (
    idl_short_float *,
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_short_float  (
    idl_short_float **, 
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_mr_long_float  (
    idl_long_float *, 
    rpc_ss_node_type_k_t,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ur_long_float   (
    idl_long_float **,
    rpc_ss_node_type_k_t,
    rpc_ss_marsh_state_t *
);

void rpc_ss_me_long_float  (
    idl_long_float *, 
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

void rpc_ss_ue_long_float  (
    idl_long_float **, 
    rpc_ss_node_type_k_t ,
    rpc_ss_marsh_state_t *
);

/*
 *  AUTO_HANDLE SUPPORT
 */

void rpc_ss_make_import_cursor_valid  (
     RPC_SS_THREADS_MUTEX_T *, 
     rpc_ns_import_handle_t  *, 
     rpc_if_handle_t ,
     error_status_t *
);

void rpc_ss_import_cursor_advance   (
    RPC_SS_THREADS_MUTEX_T *,
    idl_boolean *,
    rpc_ns_import_handle_t *,
    rpc_if_handle_t ,
    ndr_boolean *,
    rpc_binding_handle_t *,
    rpc_binding_handle_t *,
    error_status_t *,
    error_status_t *
);

void rpc_ss_flag_error_on_binding   (
   RPC_SS_THREADS_MUTEX_T *,
    ndr_boolean *,
    rpc_binding_handle_t *,
    rpc_binding_handle_t *
);

/*
 *  CALL END, GETTING FAULT IF THERE IS ONE
 */

void rpc_ss_call_end   (
    volatile rpc_call_handle_t *,
    volatile ndr_ulong_int *,
    volatile error_status_t *
);

void rpc_ss_call_end_2   (
    volatile rpc_call_handle_t *,
    volatile ndr_ulong_int *,
    volatile ndr_ulong_int *,
    volatile error_status_t *
);


/*
 * NDR CONVERSIONS
 */

globalref rpc_trans_tab_p_t ndr_g_ascii_to_ebcdic;

globalref rpc_trans_tab_p_t ndr_g_ebcdic_to_ascii;

void ndr_cvt_string   (
        ndr_format_t ,
        ndr_format_t ,
        char_p_t ,
        char_p_t 
);

void ndr_cvt_short_float   (
        ndr_format_t, 
        ndr_format_t, 
        short_float_p_t,
        short_float_p_t
);

void ndr_cvt_long_float   (
        ndr_format_t ,
        ndr_format_t ,
        long_float_p_t ,
        long_float_p_t 
);


/*
 *  Support routines for marshalling and unmarshalling [string]
 */

idl_ulong_int rpc_ss_strsiz   ( idl_char *, idl_ulong_int);


/*
 * STATUS CODE CONVERSIONS
 */

#ifdef IDL_ENABLE_STATUS_MAPPING

void rpc_ss_map_dce_to_local_status  (
   error_status_t *   /* [in,out] pointer to DCE status -> local status */
);

void rpc_ss_map_local_to_dce_status    (
    error_status_t *   /* [in,out] pointer to local status -> DCE status */
);

#endif /* IDL_ENABLE_STATUS_MAPPING */

/*
 * Canned routines that can be used in an ACF [binding_callout] attribute.
 * These routines are called from the client stub.
 */
void rpc_ss_bind_authn_client (
    rpc_binding_handle_t    *,      /* [io] Binding handle */
    rpc_if_handle_t         ,       /* [in] Interface handle */
    error_status_t          *       /*[out] Return status */
);


#endif  /* !defined(NCK) || defined(NCK_NEED_MARSHALLING) */

/* Matching pragma for one specified above */

#if defined(VMS) || defined(__VMS)

#if defined(__DECC) || defined(__cplusplus)
#pragma extern_model __restore
#endif /* DEC C or C++ */

#pragma standard
#endif /* VMS  */

#ifdef __cplusplus
}
#endif

#endif  /* _STUBBASE_H */
