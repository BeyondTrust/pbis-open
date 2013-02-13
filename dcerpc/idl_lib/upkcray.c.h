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
**      upkcray.c.h
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      This module contains code to extract information from a cray
**      floating number and to initialize an UNPACKED_REAL structure
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
**  This module contains code to extract information from a cray
**  floating number and to initialize an UNPACKED_REAL structure
**  with those bits.
**
**  See the header files for a description of the UNPACKED_REAL
**  structure.
**
**  A normalized CRAY floating number looks like:
**
**      [0]: Sign bit, 15 exp bits (bias 16384), 16 fraction bits
**      [1]: 32 low order fraction bits
**
**      0.5 <= fraction < 1.0, MSB explicit
**      Since CRAY has no hidden bits the MSB must always be set.
**
**  Some of the CRAY exponent range is not used.
**  Exponents < 0x2000 and >= 0x6000 are invalid.
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



	memcpy(r, input_value, 8);

	/* Shuffle bytes to little endian format */
#if (NDR_LOCAL_INT_REP == ndr_c_int_big_endian)
	if (options & CVT_C_BIG_ENDIAN) {
        
                r[2] = r[1];
                r[1] = r[0];
        
        } else {
        
		r[2] = r[0];
		r[1] = r[1];

	}
#else
	r[2]  = ((r[1] << 24) | (r[1] >> 24));
	r[2] |= ((r[1] << 8) & 0x00FF0000L);
	r[2] |= ((r[1] >> 8) & 0x0000FF00L);
	r[1]  = ((r[0] << 24) | (r[0] >> 24));
	r[1] |= ((r[0] << 8) & 0x00FF0000L);
	r[1] |= ((r[0] >> 8) & 0x0000FF00L);
#endif

	/* Initialize FLAGS and perhaps set NEGATIVE bit */

	r[U_R_FLAGS] = (r[1] >> 31);

	/* Clear sign bit */

	r[1] &= 0x7FFFFFFFL;

	/* Extract CRAY biased exponent */

	r[U_R_EXP] = r[1] >> 16;

	if ((r[1] == 0) && (r[2] == 0)) {

		r[U_R_FLAGS] |= U_R_ZERO;

	} else if (    (r[U_R_EXP] <  0x2000)
				|| (r[U_R_EXP] >= 0x6000)
				|| (!(r[1] & 0x00008000L)) ) {

		r[U_R_FLAGS] |= U_R_INVALID;

	} else {

		/* Adjust bias */

		r[U_R_EXP] += (U_R_BIAS - 16384);

		/* Left justify fraction bits */

		r[1] <<= 16;
		r[1] |= (r[2] >> 16);
		r[2] <<= 16;

		/* Clear uninitialized part of unpacked real */

		r[3] = 0;
		r[4] = 0;

	}
