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
**      upkibms.c.h
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      This module contains code to extract information from an IBM
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
**  This module contains code to extract information from an IBM
**  single floating number and to initialize an UNPACKED_REAL structure
**  with those bits.
**
**  See the header files for a description of the UNPACKED_REAL
**  structure.
**
**  A normalized IBM short floating number looks like:
**
**      Sign bit, 7 exp bits (bias 64), 24 fraction bits
**
**      0.0625 <= fraction < 1.0, from 0 to 3 leading zeros
**      to compensate for the hexadecimal exponent.
**
**
**  Implicit parameters:
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
	memcpy(r, input_value, 4);

	/* Shuffle bytes to little endian format */

	r[1]  = ((r[0] << 24) | (r[0] >> 24));
	r[1] |= ((r[0] << 8) & 0x00FF0000L);
	r[1] |= ((r[0] >> 8) & 0x0000FF00L);

#endif

	/* Initialize FLAGS and perhaps set NEGATIVE bit */

	r[U_R_FLAGS] = (r[1] >> 31);

	/* Clear sign bit */

	r[1] &= 0x7FFFFFFFL;

	if (r[1] == 0) {

		r[U_R_FLAGS] |= U_R_ZERO;

	} else {

		/* Get unbiased hexadecimal exponent and convert it to binary */

		r[U_R_EXP] = U_R_BIAS + (((r[1] >> 24) - 64) * 4);

		/* Count leading zeros */

		i = 0;
		while (!(r[1] & 0x00800000L)) {
			i += 1;
			if (i > 3)
				break;
			r[1] <<= 1;
		}

		if (i > 3) {

			r[U_R_FLAGS] |= U_R_INVALID;

		} else {

			/* Adjust exponent to compensate for leading zeros */

			r[U_R_EXP] -= i;

			/* Left justify fraction bits */

			r[1] <<= 8;

			/* Clear uninitialized part of unpacked real */

			r[2] = 0;
			r[3] = 0;
			r[4] = 0;
		}

	}
