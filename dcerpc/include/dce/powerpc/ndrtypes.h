/****
 **** NDR types for the PowerPC RISC Architecture - 32bit, Big Endian Mode
 ****/

#ifndef _NDRTYPES_H
#define _NDRTYPES_H

typedef unsigned char 		ndr_boolean;

#define ndr_false false
#define ndr_true  true

typedef unsigned char 		ndr_byte;

typedef unsigned char 		ndr_char;

typedef signed char 		ndr_small_int;

typedef unsigned char 		ndr_usmall_int;

typedef short int 		ndr_short_int;

typedef unsigned short int 	ndr_ushort_int;

typedef long int 	        ndr_long_int;

typedef unsigned long int 	ndr_ulong_int;

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
