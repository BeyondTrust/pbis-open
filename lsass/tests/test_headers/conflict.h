/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        confict.h
 *
 * Abstract:
 *
 *        Header test.
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 * 
 */

#define BOOL conflicting_bool
#define BOOLEAN conflicting_boolean
#define CHAR conflicting_char
#define UCHAR conflicting_uchar
#define SHORT conflicting_short
#define USHORT conflicting_ushort
#define LONG conflicting_long
#define ULONG conflicting_ulong
#define LONG64 conflicting_long64
#define ULONG64 conflicting_ulong64
#define DWORD conflicting_dword

#define PBOOL conflicting_pbool
#define PBOOLEAN conflicting_pboolean
#define PCHAR conflicting_pchar
#define PUCHAR conflicting_puchar
#define PSHORT conflicting_pshort
#define PUSHORT conflicting_pushort
#define PLONG conflicting_plong
#define PULONG conflicting_pulong
#define PLONG64 conflicting_plong64
#define PULONG64 conflicting_pulong64
#define PDWORD conflicting_pdword

#define IN conflicting_in
#define OUT conflicting_out
#define OPTIONAL conflicting_optional

#define PSTR conflicting_pstr
#define PCSTR conflicting_pcstr

#define HANDLE conflicting_handle
#define PHANDLE conflicting_phandle
#define VOID conflicting_void
#define PVOID conflicting_pvoid

// Conflicting from Samba headers (as well as BOOL from above)
#define LSA_TRUSTED_DOMAIN_INFO conflicting_lsa_trusted_domain_info
#define PLSA_TRUSTED_DOMAIN_INFO conflicting_plsa_trusted_domain_info
#define LSA_DATA_BLOB conflicting_lsa_data_blob
#define PLSA_DATA_BLOB conflicting_plsa_data_blob
