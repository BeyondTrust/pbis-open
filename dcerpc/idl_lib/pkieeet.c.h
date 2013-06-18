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
**
**  NAME:
**
**      pkieeet.c.h
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      This module contains code to extract information from an
**      UNPACKED_REAL structure and to create an IEEE double floating number
**      with those bits.
**
**              This module is meant to be used as an include file.
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
**  This module contains code to extract information from an
**  UNPACKED_REAL structure and to create an IEEE double floating number
**  with those bits.
**
**  See the header files for a description of the UNPACKED_REAL
**  structure.
**
**  A normalized IEEE double precision floating number looks like:
**
**      [0]: 32 low order fraction bits
**      [1]: Sign bit, 11 exp bits (bias 1023), 20 fraction bits
**
**      1.0 <= fraction < 2.0, MSB implicit
**
**  For more details see "Mips R2000 Risc Architecture"
**  by Gerry Kane, page 6-8 or ANSI/IEEE Std 754-1985.
**
**
**  Implicit parameters:
**
**      options: a word of flags, see include files.
**
**      output_value: a pointer to the input parameter.
**
**      r: an UNPACKED_REAL structure.
**
**              i: a temporary integer variable
**
**--
*/



if (r[U_R_FLAGS] & U_R_UNUSUAL) {

        if (r[U_R_FLAGS] & U_R_ZERO)

                if (r[U_R_FLAGS] & U_R_NEGATIVE)
                        memcpy(output_value, IEEE_T_NEG_ZERO, 8);
                else
                        memcpy(output_value, IEEE_T_POS_ZERO, 8);

        else if (r[U_R_FLAGS] & U_R_INFINITY) {

                if (r[U_R_FLAGS] & U_R_NEGATIVE)
                        memcpy(output_value, IEEE_T_NEG_INFINITY, 8);
                else
                        memcpy(output_value, IEEE_T_POS_INFINITY, 8);

        } else if (r[U_R_FLAGS] & U_R_INVALID) {

                memcpy(output_value, IEEE_T_INVALID, 8);
                DCETHREAD_RAISE(dcethread_aritherr_e);    /* Invalid value */

        }

} else {

        /* Precision varies if value will be a denorm */
        /* So, figure out where to round (0 <= i <= 53). */

        round_bit_position = r[U_R_EXP] - ((U_R_BIAS - 1022) - 52);
        if (round_bit_position < 0)
                round_bit_position = 0;
        else if (round_bit_position > 53)
                round_bit_position = 53;

#include "round.c.h"

        if (r[U_R_EXP] < (U_R_BIAS - 1021)) {

                /* Denorm or underflow */

                if (r[U_R_EXP] < ((U_R_BIAS - 1021) - 52)) {

                        /* Value is too small for a denorm, so underflow */

                        if (r[U_R_FLAGS] & U_R_NEGATIVE)
                                memcpy(output_value, IEEE_T_NEG_ZERO, 8);
                        else
                                memcpy(output_value, IEEE_T_POS_ZERO, 8);
                        if (options & CVT_C_ERR_UNDERFLOW) {
                                DCETHREAD_RAISE(dcethread_fltund_e);  /* Underflow */
                        }

                } else {

                        /* Figure leading zeros for denorm and right-justify fraction */

                        i = 64 - (r[U_R_EXP] - ((U_R_BIAS - 1022) - 52));

                        if (i > 31) {
                                i -= 32;
                                r[2] = (r[1] >> i);
                                r[1] = 0;
                        } else {
                                r[2] >>= i;
                                r[2] |= (r[1] << (32 - i));
                                r[1] >>= i;
                        }

                        /* OR in sign bit */

                        r[1] |= (r[U_R_FLAGS] << 31);

#if (NDR_LOCAL_INT_REP == ndr_c_int_big_endian)
                        if (options & CVT_C_BIG_ENDIAN) {

				r[0] = r[1];
                                r[1] = r[2];

                        } else {

				r[0] = r[2];

                        }
 
#else
                        if (options & CVT_C_BIG_ENDIAN) {

                                r[0]  = ((r[1] << 24) | (r[1] >> 24));
                                r[0] |= ((r[1] << 8) & 0x00FF0000L);
                                r[0] |= ((r[1] >> 8) & 0x0000FF00L);
                                r[1]  = ((r[2] << 24) | (r[2] >> 24));
                                r[1] |= ((r[2] << 8) & 0x00FF0000L);
                                r[1] |= ((r[2] >> 8) & 0x0000FF00L);

                        } else {

                                r[0] = r[2];

                        }
#endif 
                        memcpy(output_value, r, 8);
                }

        } else if (r[U_R_EXP] > (U_R_BIAS + 1024)) {

                /* Overflow */

                if (options & CVT_C_TRUNCATE) {

                        if (r[U_R_FLAGS] & U_R_NEGATIVE)
                                memcpy(output_value, IEEE_T_NEG_HUGE, 8);
                        else
                                memcpy(output_value, IEEE_T_POS_HUGE, 8);

                } else if ((options & CVT_C_ROUND_TO_POS)
                                        && (r[U_R_FLAGS] & U_R_NEGATIVE)) {

                                memcpy(output_value, IEEE_T_NEG_HUGE, 8);

                } else if ((options & CVT_C_ROUND_TO_NEG)
                                        && !(r[U_R_FLAGS] & U_R_NEGATIVE)) {

                                memcpy(output_value, IEEE_T_POS_HUGE, 8);

                } else {

                        if (r[U_R_FLAGS] & U_R_NEGATIVE)
                                memcpy(output_value, IEEE_T_NEG_INFINITY, 8);
                        else
                                memcpy(output_value, IEEE_T_POS_INFINITY, 8);

                }

                DCETHREAD_RAISE(dcethread_fltovf_e);  /* Overflow */

        } else {

                /* Adjust bias of exponent */

                r[U_R_EXP] -= (U_R_BIAS - 1022);

                /* Make room for exponent and sign bit */

                r[2] >>= 11;
                r[2] |= (r[1] << 21);
                r[1] >>= 11;

                /* Clear implicit bit */

                r[1] &= 0x000FFFFFL;

                /* OR in exponent and sign bit */

                r[1] |= (r[U_R_EXP] << 20);
                r[1] |= (r[U_R_FLAGS] << 31);


#if (NDR_LOCAL_INT_REP == ndr_c_int_big_endian)
                if (options & CVT_C_BIG_ENDIAN) {

			r[0] = r[1];
                        r[1] = r[2];

                } else {

			r[0] = r[2];

                }
#else
                if (options & CVT_C_BIG_ENDIAN) {

                        r[0]  = ((r[1] << 24) | (r[1] >> 24));
                        r[0] |= ((r[1] << 8) & 0x00FF0000L);
                        r[0] |= ((r[1] >> 8) & 0x0000FF00L);
                        r[1]  = ((r[2] << 24) | (r[2] >> 24));
                        r[1] |= ((r[2] << 8) & 0x00FF0000L);
                        r[1] |= ((r[2] >> 8) & 0x0000FF00L);

                } else {

                        r[0] = r[2];

                }
#endif
                memcpy(output_value, r, 8);
        }

}
