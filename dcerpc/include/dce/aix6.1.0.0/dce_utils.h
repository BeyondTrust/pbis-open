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
#include <dce/rpc.h>
/* For dce_get_802_addr() [on some platforms] */
#ifndef IEEE_802_FILE
#define IEEE_802_FILE   "/etc/ieee_802_addr"
#endif

#define utils_s_802_cant_read 0x1460101e
#define utils_s_802_addr_format 0x1460101f

typedef struct dce_802_addr_s_t {
    unsigned_char_t	eaddr[6];
} dce_802_addr_t;

void dce_get_802_addr(dce_802_addr_t*, error_status_t*);
