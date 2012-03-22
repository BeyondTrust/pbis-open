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
 */
/*
 * ndr_rep.h 
 * platform dependent (OS + Architecture) file split out from stubbase.h 
 * for DCE 1.1 code cleanup.
 * This file contains the architecture specific definitions of the 
 * local scaler data representation used 
 *
 * This file is always included as part of stubbase.h 
 */

#ifndef _NDR_REP_H 
#define _NDR_REP_H

#ifdef __BIG_ENDIAN__
#define NDR_LOCAL_INT_REP     ndr_c_int_big_endian
#else
#define NDR_LOCAL_INT_REP     ndr_c_int_little_endian
#endif
#define NDR_LOCAL_FLOAT_REP   ndr_c_float_ieee
#define NDR_LOCAL_CHAR_REP    ndr_c_char_ascii

#endif /* _NDR_REP_H */

