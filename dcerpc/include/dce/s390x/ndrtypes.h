/****
 **** NDR types for the S/390 Architecture - 32bit, Big Endian Mode
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

typedef int 	                ndr_long_int;

typedef unsigned int 	        ndr_ulong_int;

typedef signed long int         ndr_hyper_int;

typedef unsigned long int       ndr_uhyper_int;

typedef float 		        ndr_short_float;

typedef double 		        ndr_long_float;

#endif /* NDRTYPES_H */
