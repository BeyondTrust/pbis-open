/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Samba defines and structures necessary for building an IDMap
 * plugin for Winbind outside of the Samba source tree.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __SAMBA_H
#define __SAMBA_H

#include <config.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

typedef struct {uint32_t v;} NTSTATUS;

#define NT_STATUS(x) ((NTSTATUS) { x })
#define NT_STATUS_V(x) ((x).v)

/* Small set of nt_status error codes returned by the lwicomp modules */

#define NT_STATUS_OK                    NT_STATUS(0x0000)
#define NT_STATUS_UNSUCCESSFUL          NT_STATUS(0xC0000000 | 0x0001)
#define NT_STATUS_NOT_IMPLEMENTED       NT_STATUS(0xC0000000 | 0x0002)
#define NT_STATUS_INVALID_PARAMETER     NT_STATUS(0xC0000000 | 0x000d)
#define NT_STATUS_NONE_MAPPED           NT_STATUS(0xC0000000 | 0x0073)

#ifndef MAXSUBAUTHS
#define MAXSUBAUTHS 15 /* max sub authorities in a SID */
#endif

typedef struct dom_sid {
	uint8_t  sid_rev_num;             /**< SID revision number */
	uint8_t  num_auths;               /**< Number of sub-authorities */
	uint8_t  id_auth[6];              /**< Identifier Authority */

	uint32_t sub_auths[MAXSUBAUTHS];  
} DOM_SID;

typedef union unid_t {
	uid_t uid;
	gid_t gid;
} unid_t;

#endif
