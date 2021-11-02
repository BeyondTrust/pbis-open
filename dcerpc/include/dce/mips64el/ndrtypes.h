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
**      ndrtypes.h
**
**  FACILITY:
**
**      IDL Stub Support Include File
**
**  ABSTRACT:
**
**  This file is new for DCE 1.1. This is a platform specific file that
**  defines the base level ndr types. This file is indirectly included 
**  in all files via the idlbase.h file. 
**
*/

/*
 * This particular file defines the NDR types for a little-endian 
 * architecture. This file also depends on the presence of a ANSI 
 * C compiler, in that it uses the signed keyword to create the 
 * ndr_small_int type.
 */

#ifndef _NDR_TYPES_H 
#define  _NDR_TYPES_H

#if defined(__STDC__)
# if defined(__STDC_VERSION__)
#  if (__STDC_VERSION__ >= 19901L)
#    define C_STD_99
#  endif
# endif
#endif
#if defined(C_STD_99)
#include <stdint.h>
#endif

typedef unsigned char           ndr_boolean;

#define ndr_false       false
#define ndr_true        true

typedef unsigned char           ndr_byte;

typedef unsigned char           ndr_char;

typedef signed char             ndr_small_int;

typedef unsigned char           ndr_usmall_int;

#if defined(C_STD_99)
typedef int16_t                 ndr_short_int;

typedef uint16_t                ndr_ushort_int;

typedef int32_t                 ndr_long_int;

typedef uint32_t                ndr_ulong_int;

#else
/* without C99 make some assumptions about sizes */
typedef short int               ndr_short_int;

typedef unsigned short int      ndr_ushort_int;

typedef long int                ndr_long_int;

typedef unsigned int            ndr_ulong_int;

#endif /* defined(C_STD_99) */

/* 
 * the reps for hyper must match the little-endian NDR rep since
 *  defined(vax) || defined(M_I86) => defined(ALIGNED_SCALAR_ARRAYS) 
 */

struct ndr_hyper_int_rep_s_t {
    ndr_ulong_int low;
    ndr_long_int high;
};

struct ndr_uhyper_int_rep_s_t {
    ndr_ulong_int low;
    ndr_ulong_int high;
};

#ifdef __GNUC__
typedef long long int           ndr_hyper_int;
typedef unsigned long long int  ndr_uhyper_int;
#else
typedef struct ndr_hyper_int_rep_s_t ndr_hyper_int;
typedef struct ndr_uhyper_int_rep_s_t ndr_uhyper_int;
#endif /* __GNUC__ */

typedef float                   ndr_short_float;
typedef double                  ndr_long_float;

#endif /* _NDR_TYPES_H */

