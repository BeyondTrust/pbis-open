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
**      pkieees.c.h
**
**  FACILITY:
**
**      IDL Stub Runtime Support
**
**  ABSTRACT:
**
**      This module contains code to extract information from an
**      UNPACKED_REAL structure and to create an IEEE single floating number
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
**  UNPACKED_REAL structure and to create an IEEE single number
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
**      options: a word of flags, see include files.
**
**      output_value: a pointer to the input parameter.
**
**      r: an UNPACKED_REAL structure.
**
**--
*/



if (r[U_R_FLAGS] & U_R_UNUSUAL) {

        if (r[U_R_FLAGS] & U_R_ZERO)

                if (r[U_R_FLAGS] & U_R_NEGATIVE)
                        memcpy(output_value, IEEE_S_NEG_ZERO, 4);
                else
                        memcpy(output_value, IEEE_S_POS_ZERO, 4);

        else if (r[U_R_FLAGS] & U_R_INFINITY) {

                if (r[U_R_FLAGS] & U_R_NEGATIVE)
                        memcpy(output_value, IEEE_S_NEG_INFINITY, 4);
                else
                        memcpy(output_value, IEEE_S_POS_INFINITY, 4);

        } else if (r[U_R_FLAGS] & U_R_INVALID) {

                memcpy(output_value, IEEE_S_INVALID, 4);
                DCETHREAD_RAISE(dcethread_aritherr_e);    /* Invalid value */

        }

} else {

        /* Precision varies if value will be a denorm */
        /* So, figure out where to round (0 <= i <= 24). */

        round_bit_position = r[U_R_EXP] - ((U_R_BIAS - 126) - 23);
        if (round_bit_position < 0)
                round_bit_position = 0;
        else if (round_bit_position > 24)
                round_bit_position = 24;

#include "round.c.h"

        if (r[U_R_EXP] < (U_R_BIAS - 125)) {

                /* Denorm or underflow */

                if (r[U_R_EXP] < ((U_R_BIAS - 125) - 23)) {

                        /* Value is too small for a denorm, so underflow */

                        if (r[U_R_FLAGS] & U_R_NEGATIVE)
                                memcpy(output_value, IEEE_S_NEG_ZERO, 4);
                        else
                                memcpy(output_value, IEEE_S_POS_ZERO, 4);
                        if (options & CVT_C_ERR_UNDERFLOW) {
                                DCETHREAD_RAISE(dcethread_fltund_e);  /* Underflow */
                        }

                } else {

                        /* Figure leading zeros for denorm and right-justify fraction */

                        i = 32 - (r[U_R_EXP] - ((U_R_BIAS - 126) - 23));
                        r[1] >>= i;

                        /* Set sign bit */

                        r[1] |= (r[U_R_FLAGS] << 31);

                        if (options & CVT_C_BIG_ENDIAN) {

                                r[0]  = ((r[1] << 24) | (r[1] >> 24));
                                r[0] |= ((r[1] << 8) & 0x00FF0000L);
                                r[0] |= ((r[1] >> 8) & 0x0000FF00L);
                                memcpy(output_value, r, 4);

                        } else {

                                memcpy(output_value, &r[1], 4);

                        }
                }

        } else if (r[U_R_EXP] > (U_R_BIAS + 128)) {

                /* Overflow */

                if (options & CVT_C_TRUNCATE) {

                        if (r[U_R_FLAGS] & U_R_NEGATIVE)
                                memcpy(output_value, IEEE_S_NEG_HUGE, 4);
                        else
                                memcpy(output_value, IEEE_S_POS_HUGE, 4);

                } else if ((options & CVT_C_ROUND_TO_POS)
                                        && (r[U_R_FLAGS] & U_R_NEGATIVE)) {

                                memcpy(output_value, IEEE_S_NEG_HUGE, 4);

                } else if ((options & CVT_C_ROUND_TO_NEG)
                                        && !(r[U_R_FLAGS] & U_R_NEGATIVE)) {

                                memcpy(output_value, IEEE_S_POS_HUGE, 4);

                } else {

                        if (r[U_R_FLAGS] & U_R_NEGATIVE)
                                memcpy(output_value, IEEE_S_NEG_INFINITY, 4);
                        else
                                memcpy(output_value, IEEE_S_POS_INFINITY, 4);

                }

                DCETHREAD_RAISE(dcethread_fltovf_e);  /* Overflow */

        } else {

                /* Adjust bias of exponent */

                r[U_R_EXP] -= (U_R_BIAS - 126);

                /* Make room for exponent and sign bit */

                r[1] >>= 8;

                /* Clear implicit bit */

                r[1] &= 0x007FFFFFL;

                /* OR in exponent and sign bit */

                r[1] |= (r[U_R_EXP] << 23);
                r[1] |= (r[U_R_FLAGS] << 31);

#if (NDR_LOCAL_INT_REP == ndr_c_int_big_endian)

                memcpy(output_value, &r[1], 4);

#else
                if (options & CVT_C_BIG_ENDIAN) {

                        r[0]  = ((r[1] << 24) | (r[1] >> 24));
                        r[0] |= ((r[1] << 8) & 0x00FF0000L);
                        r[0] |= ((r[1] >> 8) & 0x0000FF00L);
                        memcpy(output_value, r, 4);

                } else {

                        memcpy(output_value, &r[1], 4);

                }
#endif
        }

}
