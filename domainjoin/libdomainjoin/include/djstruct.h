/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#ifndef __DJSTRUCT_H__
#define __DJSTRUCT_H__

typedef struct __DOMAINJOININFO
{

    PSTR pszName;
    PSTR pszDnsDomain;
    PSTR pszDomainName;      /* Null if not joined  */
    PSTR pszDomainShortName; /* Null if not joined  */
    PSTR pszLogFilePath;     /* Null if not logging */

} DOMAINJOININFO, *PDOMAINJOININFO;

typedef struct __PROCBUFFER
{
    BOOLEAN            bEndOfFile;
    CHAR               szOutBuf[MAX_PROC_BUF_LEN];
    DWORD              dwOutBytesRead;
    CHAR               szErrBuf[MAX_PROC_BUF_LEN];
    DWORD              dwErrBytesRead;
} PROCBUFFER, *PPROCBUFFER;

typedef enum
{
    SERVER_LICENSE = 0x1,
    WORKSTATION_LICENSE = 0x2,
    EVALUATION_LICENSE = 0x4,
    SITE_LICENSE = 0x8
} LicenseMagic;

typedef struct __LICENSEINFO
{
    uint16_t crc16;
    BYTE licenseVersion;
    BYTE productId;
    uint16_t variable;
    uint32_t licenseExpiration;
    LicenseMagic licenseMagic;

} LICENSEINFO, *PLICENSEINFO;

#endif /* __DJSTRUCT_H__ */
