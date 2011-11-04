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
**
**  NAME:
**
**      upkieees.c.h
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      This module contains code to extract information from a IEEE
**      single floating number and to initialize an UNPACKED_REAL structure
**      with those bits.
**
**		This module is meant to be used as an include file.
**
**  VERSION: DCE 1.0
**
*/
#if HAVE_CONFIG_H
#include <config.h>
#endif


/*
**++
**  Functional Description:
**
**  This module contains code to extract information from a IEEE
**  single floating number and to initialize an UNPACKED_REAL structure
**  with those bits.
**
**  See the header files for a description of the UNPACKED_REAL
**  structure.
**
**  A normalized IEEE single precision floating number looks like:
**
**      Sign bit, 8 exp bits (bias 127), 23 fraction bits
**
**      1.0 <= fraction < 2.0, MSB implicit
**
**  For more details see "Mips R2000 Risc Architecture"
**  by Gerry Kane, page 6-8 or ANSI/IEEE Std 754-1985.
**
**
**  Implicit parameters:
**
**  	options: a word of flags, see include files.
**
**  	input_value: a pointer to the input parameter.
**
**  	r: an UNPACKED_REAL structure.
**
**  	i: a temporary integer variable.
**
**--
*/


#if (NDR_LOCAL_INT_REP == ndr_c_int_big_endian)

	memcpy(&r[1], input_value, 4);

#else
	if (options & CVT_C_BIG_ENDIAN) {

		memcpy(r, input_value, 4);

		/* Shuffle bytes to little endian format */

		r[1]  = ((r[0] << 24) | (r[0] >> 24));
		r[1] |= ((r[0] << 8) & 0x00FF0000L);
		r[1] |= ((r[0] >> 8) & 0x0000FF00L);

	} else {

		memcpy(&r[1], input_value, 4);

	}
#endif

	/* Initialize FLAGS and perhaps set NEGATIVE bit */
	
	r[U_R_FLAGS] = (r[1] >> 31);

	/* Extract biased exponent */

	r[U_R_EXP] = (r[1] >> 23) & 0x000000FFL;

	/* Check for denormalized values */

	if (r[U_R_EXP] == 0) {

		/* Clear sign bit */

		r[1] &= 0x7FFFFFFFL;

		/* If fraction is non-zero then normalize it */

		if (r[1]) {

			/* Count leading zeros */

			i = 0;
			while (!(r[1] & 0x00400000L)) {
				r[1] <<= 1;
				i += 1;
			}

			/* Adjust exponent and normalize fraction */

			r[U_R_EXP] = U_R_BIAS - 126 - i;
			r[1] <<= 9;

			/* Clear uninitialized part of unpacked real */

			r[2] = 0;
			r[3] = 0;
			r[4] = 0;

		} else {

			r[U_R_FLAGS] |= U_R_ZERO;

		}


	/* Check for NANs and INFINITIES */

	} else if (r[U_R_EXP] == 255) {

		/* Clear sign and exponent */

		r[1] &= 0x007FFFFFL;

		if (r[1])
			r[U_R_FLAGS] |= U_R_INVALID;
		else
			r[U_R_FLAGS] |= U_R_INFINITY;

	} else {

		/* Adjust exponent bias */

		r[U_R_EXP] += (U_R_BIAS - 126);

		/* Set hidden bit */

		r[1] |= 0x00800000L;

		/* Left justify fraction bits */

		r[1] <<= 8;

		/* Clear uninitialized part of unpacked real */

		r[2] = 0;
		r[3] = 0;
		r[4] = 0;

	}
