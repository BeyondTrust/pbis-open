/*
 * 
 * (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
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
**
**  NAME:
**
**      cvt.h
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**  Header file for floating point conversion routines.
**
**  VERSION: DCE 1.0
**
*/

#ifndef CVT
#define CVT 1

/*
 *
 *    Type Definitions
 *
 */

typedef unsigned char CVT_BYTE;
typedef CVT_BYTE *CVT_BYTE_PTR;

typedef CVT_BYTE CVT_VAX_F[4];
typedef CVT_BYTE CVT_VAX_D[8];
typedef CVT_BYTE CVT_VAX_G[8];
typedef CVT_BYTE CVT_VAX_H[16];
typedef CVT_BYTE CVT_IEEE_SINGLE[4];
typedef CVT_BYTE CVT_IEEE_DOUBLE[8];
typedef CVT_BYTE CVT_IBM_SHORT[4];
typedef CVT_BYTE CVT_IBM_LONG[8];
typedef CVT_BYTE CVT_CRAY[8];
typedef float    CVT_SINGLE;
typedef double   CVT_DOUBLE;
typedef long     CVT_SIGNED_INT;
typedef unsigned long CVT_UNSIGNED_INT;
typedef unsigned long CVT_STATUS;

/*
 *
 *    Constant Definitions
 *
 */

#define CVT_C_ROUND_TO_NEAREST		     1
#define CVT_C_TRUNCATE			     2
#define CVT_C_ROUND_TO_POS		     4
#define CVT_C_ROUND_TO_NEG		     8
#define CVT_C_VAX_ROUNDING                  16
#define CVT_C_BIG_ENDIAN		    32
#define CVT_C_ERR_UNDERFLOW		    64
#define CVT_C_ZERO_BLANKS		   128
#define CVT_C_SKIP_BLANKS		   256
#define CVT_C_SKIP_UNDERSCORES		   512
#define CVT_C_SKIP_UNDERSCORE		   512
#define CVT_C_SKIP_TABS			  1024
#define CVT_C_ONLY_E                      2048
#define CVT_C_EXP_LETTER_REQUIRED	  4096
#define CVT_C_FORCE_SCALE		  8192
#define CVT_C_EXPONENTIAL_FORMAT	 16384
#define CVT_C_FORCE_PLUS		 32768
#define CVT_C_FORCE_EXPONENT_SIGN	 65536
#define CVT_C_SUPPRESS_TRAILING_ZEROES	131072
#define CVT_C_FORCE_EXPONENTIAL_FORMAT	262144
#define CVT_C_FORCE_FRACTIONAL_FORMAT	524288
#define CVT_C_EXPONENT_D		1048576
#define CVT_C_EXPONENT_E		2097152
#define CVT_C_SEMANTICS_FORTRAN		4194304
#define CVT_C_SEMANTICS_PASCAL		8388608


#define cvt__normal 			1
#define cvt__invalid_character 		2
#define cvt__invalid_option 		3
#define cvt__invalid_radix 		4
#define cvt__invalid_size 		5
#define cvt__invalid_value 		6
#define cvt__neg_infinity 		7
#define cvt__output_conversion_error 	8
#define cvt__overflow 			9
#define cvt__pos_infinity 		10
#define cvt__underflow 			11
#define cvt__input_conversion_error 	12

#define cvt_s_normal 			cvt__normal
#define cvt_s_invalid_character 	cvt__invalid_character
#define cvt_s_invalid_option 		cvt__invalid_option
#define cvt_s_invalid_radix 		cvt__invalid_radix
#define cvt_s_invalid_size 		cvt__invalid_size
#define cvt_s_invalid_value 		cvt__invalid_value
#define cvt_s_neg_infinity 		cvt__neg_infinity
#define cvt_s_input_conversion_error 	cvt__input_conversion_error
#define cvt_s_output_conversion_error 	cvt__output_conversion_error
#define cvt_s_overflow 			cvt__overflow
#define cvt_s_pos_infinity 		cvt__pos_infinity
#define cvt_s_underflow 		cvt__underflow

#define CVT_C_BIN  2
#define CVT_C_OCT  8
#define CVT_C_DEC 10
#define CVT_C_HEX 16

#endif
