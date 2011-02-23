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
#if !defined(_DCE_H)
#define _DCE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Common definitions for DCE
 * This is a machine specific file that must be ported to each platform.
 */ 

#define DCE_VERSION "1.1"
#define DCE_MAJOR_VERSION 1
#define DCE_MINOR_VERSION 1

/* 
 * Define the endianess of the platform. Pulled in from machine/endian.h.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <sys/byteorder.h>

/* Only one place needed for DCE to define these */
#define FALSE 0
#define TRUE 1

#if !defined(MIN)
#  define MIN(x, y)         ((x) < (y) ? (x) : (y))
#endif

#if !defined(MAX)
#  define MAX(x, y)         ((x) > (y) ? (x) : (y))
#endif

/* 
 * For those components wishing to support platforms where void 
 * pointers are not available, they can use the following typedef for 
 * a generic pointer type.  If they are supporting such platforms they 
 * must use this.
 */
#  define _DCE_VOID_

#if defined(_DCE_VOID_)
  typedef void * pointer_t;
#else                                   /* defined(_DCE_VOID_) */
  typedef char * pointer_t;
#endif                                  /* defined(_DCE_VOID_) */

/* 
 * Here is a macro that can be used to support token concatenation in 
 * an ANSI and non-ANSI environment.  Support of non-ANSI environments 
 * is not required, but where done, this macro must be used.
 */
#  define _DCE_TOKENCONCAT_

#if defined(_DCE_TOKENCONCAT_)
#  define DCE_CONCAT(a, b)      a ## b
#else                                   /* defined(_DCE_TOKENCONCAT_) */
#  define DCE_CONCAT(a, b)      a/**/b
#endif                                  /* defined(_DCE_TOKENCONCAT_) */

/*
 * Define the dcelocal and dceshared directories
 */
extern const char *dcelocal_path;
extern const char *dceshared_path;

/* If DCE_DEBUG is defined then debugging code is activated. */
/* #define DCE_DEBUG */

/* 
 * Machine dependent typedefs for boolean, byte, and (un)signed integers.
 * All DCE code should be using these typedefs where applicable.
 * The following are defined in nbase.h:
 *     unsigned8       unsigned  8 bit integer
 *     unsigned16      unsigned 16 bit integer
 *     unsigned32      unsigned 32 bit integer
 *     signed8           signed  8 bit integer       
 *     signed16          signed 16 bit integer
 *     signed32          signed 32 bit integer
 * Define the following from idl types in idlbase.h (which is included 
 * by nbase.h:
 *     byte            unsigned  8 bits
 *     boolean         unsigned  8 bits   
 * Define (un)signed64 to be used with the U64* macros
 */
#include <dce/nbase.h>
typedef idl_byte        byte;
typedef idl_boolean     boolean;
typedef struct unsigned64_s_t {
    unsigned long hi;
    unsigned long lo;
} unsigned64;

typedef struct signed64_s_t {
    unsigned long hi;
    unsigned long lo;
} signed64;

typedef struct unsigned48_s_t {
    unsigned long  int  lo;             /* least significant 32 bits */
	unsigned short int  hi;             /* most significant 16 bits */
} unsigned48;

typedef struct unsigned128_s_t {
    unsigned long lolo;
    unsigned long lohi;
    unsigned long hilo;
    unsigned long hihi;
} unsigned128;

/* 
 * Serviceability and perhaps other DCE-wide include files 
 * will be included here.  This is a sample only.
 */
#if 0
#include <dce/dce_svc.h>
#endif

#ifdef __cplusplus
}
#endif

#endif                                  /* _DCE_H */
