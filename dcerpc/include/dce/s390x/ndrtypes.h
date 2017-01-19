/****
 **** NDR types for the S/390x Architecture - 64bit, Big Endian Mode
 ****  This is a modified version of the original header.
 ****/

#ifndef _NDRTYPES_H
#define _NDRTYPES_H

/*
 * Use C99 stdint types if possible, as they have known sizes.
 * The DCE/RPC porting guide defines the expected size of each
 * of these types. Depending on compiler flags, int, long and
 * pointer types may be 32 or 64 bit. (See gcc -m31 -m64 options.)
 *
 * n.b. for gcc this requires passing --std=gnu99/c99 (or greater)
 *  or else __STDC_VERSION__ is not defined.
 */
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

typedef unsigned char 		ndr_boolean;

#define ndr_false false
#define ndr_true  true

typedef unsigned char 		ndr_byte;

typedef unsigned char 		ndr_char;

typedef signed char 		ndr_small_int;

typedef unsigned char 		ndr_usmall_int;

#if defined(C_STD_99)
typedef int16_t 		ndr_short_int;

typedef uint16_t		ndr_ushort_int;

typedef int32_t			ndr_long_int;

typedef uint32_t		ndr_ulong_int;

typedef int64_t 		ndr_hyper_int;

typedef uint64_t		ndr_uhyper_int;

#else
typedef short int 		ndr_short_int;

typedef unsigned short int 	ndr_ushort_int;

typedef int 	                ndr_long_int;

typedef unsigned int 	        ndr_ulong_int;

typedef signed long int         ndr_hyper_int;

typedef unsigned long int       ndr_uhyper_int;
#endif


typedef float 		        ndr_short_float;

typedef double 		        ndr_long_float;

#endif /* NDRTYPES_H */
