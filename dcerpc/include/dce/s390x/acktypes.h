/****
 **** NDR types for the PowerPC RISC Architecture - 64bit, Big Endian Mode
 ****/

#ifndef _NDRTYPES_H
#define _NDRTYPES_H

/* 
 * Use C99 stdint types if possible,  as they have known sizes. 
 * The DCE/RPC porting guide defines the expected size of each 
 * of these types. Depending on how PPC software is compiled, 
 * int, long and pointer types may be 32 or 64 bit. (See gcc 
 * -m32 -m64 power pc options.)
 * 
 * n.b. for gcc this requires passing --std=c99 (or greater)
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

typedef uint16_t        ndr_ushort_int;

typedef int32_t			ndr_long_int;

typedef uint32_t		ndr_ulong_int;

#else
/* without C99 make some assumptions about sizes */
typedef short int 		ndr_short_int;

typedef unsigned short int 	ndr_ushort_int;

typedef int 	        ndr_long_int;

typedef unsigned int 	ndr_ulong_int;

#endif

struct ndr_hyper_int_rep_s_t   {
    ndr_long_int high; 
    ndr_ulong_int low;
};

struct ndr_uhyper_int_rep_s_t  {
    ndr_ulong_int high; 
    ndr_ulong_int low;
};

#ifdef __GNUC__
typedef long long int		ndr_hyper_int;
typedef unsigned long long int	ndr_uhyper_int;
#else
typedef struct ndr_hyper_int_rep_s_t ndr_hyper_int;
typedef struct ndr_uhyper_int_rep_s_t ndr_uhyper_int;
#endif /* __GNUC__ */

typedef float 		ndr_short_float;
typedef double 		ndr_long_float;

#endif /* NDRTYPES_H */
