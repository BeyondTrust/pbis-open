/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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
