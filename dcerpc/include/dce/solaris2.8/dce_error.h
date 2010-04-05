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
#if	!defined(DCE_ERROR_H)
#define DCE_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#define dce_c_error_string_len 160

typedef char dce_error_string_t[dce_c_error_string_len];
void dce_error_inq_text (unsigned long status_to_convert,
		unsigned char* error_text, int* status);

#ifdef __cplusplus
}
#endif

#endif	/* !defined(DCE_ERROR_H) */
