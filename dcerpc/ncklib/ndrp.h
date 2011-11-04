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
#ifndef _NDRP_H
#define _NDRP_H	1
/*
**
**  NAME:
**
**      ndrp.h
**
**  FACILITY:
**
**      Network Data Representation (NDR)
**
**  ABSTRACT:
**
**  System (machine-architecture) -dependent definitions.
**
**
*/

/*
 * Data representation descriptor (drep)
 *
 * Note that the form of a drep "on the wire" is not captured by the
 * the "ndr_format_t" data type.  The actual structure -- a "packed drep"
 * -- is a vector of four bytes:
 *
 *      | MSB           LSB |
 *      |<---- 8 bits ----->|
 *      |<-- 4 -->|<-- 4 -->|
 *
 *      +---------+---------+
 *      | int rep | chr rep |
 *      +---------+---------+
 *      |     float rep     |
 *      +-------------------+
 *      |     reserved      |
 *      +-------------------+
 *      |     reserved      |
 *      +-------------------+
 *
 * The following macros manipulate data representation descriptors.
 * "NDR_COPY_DREP" copies one packed drep into another.  "NDR_UNPACK_DREP"
 * copies from a packed drep into a variable of the type "ndr_format_t".
 * 
 */

#ifdef CONVENTIONAL_ALIGNMENT
#  define NDR_COPY_DREP(dst, src) \
    (*((signed32 *) (dst)) = *((signed32 *) (src)))
#else
#  define NDR_COPY_DREP(dst, src) { \
    (dst)[0] = (src)[0]; \
    (dst)[1] = (src)[1]; \
    (dst)[2] = (src)[2]; \
    (dst)[3] = (src)[3]; \
  }
#endif

#define NDR_DREP_INT_REP(drep)   ((drep)[0] >> 4)
#define NDR_DREP_CHAR_REP(drep)  ((drep)[0] & 0xf)
#define NDR_DREP_FLOAT_REP(drep) ((drep)[1])

#define NDR_UNPACK_DREP(dst, src) {             \
    (dst)->int_rep   = NDR_DREP_INT_REP(src);   \
    (dst)->char_rep  = NDR_DREP_CHAR_REP(src);  \
    (dst)->float_rep = NDR_DREP_FLOAT_REP(src); \
    (dst)->reserved  = 0;                   \
}

#endif /* _NDRP_H */
