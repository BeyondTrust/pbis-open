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


 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsastr.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        String Utilities
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#ifndef __LSAUNISTR_H__
#define __LSAUNISTR_H__

#if HAVE_WC16STR_H
#include <wc16str.h>
#endif
#if HAVE_WC16PRINTF_H
#include <wc16printf.h>
#endif

#ifdef HAVE_WCHAR16_T

typedef struct _LSA_STRING {
    USHORT   length;
    USHORT   max;
    wchar16_t *buffer;
} LSA_STRING, *PLSA_STRING;
	
#define LSASTR_BUFF_PROBE	        0x001F
#define LSASTR_BUFF_MAXBYTE     	0xFFFF
#define LSASTR_BUFF_MAXCONVERTED	0X00FF

#define LSASTR_CCH_BYTECOUNT(s) ((s) * sizeof(wchar16_t))
#define LSASTR_WCHAR_COUNT(s)   ((s)/sizeof(wchar16_t))
#define LSASTR_BYTECOUNT(s)	(_wc16slen(input) * sizeof(wchar16_t))	

void
LsaFreeLsaString(
    PLSA_STRING lsaString
    );

BOOLEAN
LsaEqualLsaString(
    PLSA_STRING s1,
    PLSA_STRING s2
    );

BOOLEAN
LsaEqualLsaStringNoCase(
    PLSA_STRING s1,
    PLSA_STRING s2
    );

VOID
LsaUpperCaseLsaString(
    PLSA_STRING s
    );

DWORD
LsaCopyLsaString(
    PLSA_STRING dest,
    const PLSA_STRING src
    );

DWORD
LsaCopyLsaStringNullTerm(
    PLSA_STRING dest,
    const PLSA_STRING src
    );

DWORD 
LsaInitializeLsaStringW(
    wchar16_t *input, 
    BOOLEAN copy,
    PLSA_STRING lsaStringOut
    );

DWORD
LsaStringToAnsi(
    PLSA_STRING lsaString,
    PSTR *ansiString
    );

DWORD
LsaInitializeLsaStringA(
    PCSTR wcString, 
    PLSA_STRING lsaStringOut
    );

DWORD
LsaMbsToWc16s(
    PCSTR pszInput,
    PWSTR* ppszOutput
    );

DWORD
LsaWc16sToMbs(
    PCWSTR pwszInput,
    PSTR*  ppszOutput
    );

DWORD
LsaWc16snToMbs(
    PCWSTR pwszInput,
    PSTR*  ppszOutput,
    size_t sMaxChars
    );

DWORD
LsaWc16sLen(
    PCWSTR  pwszInput,
    size_t* psLen
    );

DWORD
LsaSW16printf(
    PWSTR*  ppwszStrOutput,
    PCSTR   pszFormat,
    ...);

DWORD
LsaWc16ToUpper(
    IN OUT PWSTR pwszString
    );


#endif
#endif /* __LSAUNISTR_H__ */

