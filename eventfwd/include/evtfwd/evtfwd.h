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
 *        evtfwd.h
 *
 * Abstract:
 *
 *        Unnamed Description
 * 
 *        Active Directory Site API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#ifndef __EFD_H__
#define __EFD_H__

#ifndef _WIN32

#include <lw/types.h>
#include <lw/attrs.h>
#include <evtfwd/logging.h>

#endif

/* ERRORS */
#define EFD_ERROR_SUCCESS                             0x0000
#define EFD_ERROR_INVALID_CACHE_PATH                  0xA000 // 40960
#define EFD_ERROR_INVALID_CONFIG_PATH                 0xA001 // 40961
#define EFD_ERROR_INVALID_PREFIX_PATH                 0xA002 // 40962
#define EFD_ERROR_INSUFFICIENT_BUFFER                 0xA003 // 40963
#define EFD_ERROR_OUT_OF_MEMORY                       0xA004 // 40964
#define EFD_ERROR_INVALID_MESSAGE                     0xA005 // 40965
#define EFD_ERROR_UNEXPECTED_MESSAGE                  0xA006 // 40966
#define EFD_ERROR_DATA_ERROR                          0xA007 // 40967
#define EFD_ERROR_NOT_IMPLEMENTED                     0xA008 // 40968
#define EFD_ERROR_NO_CONTEXT_ITEM                     0xA009 // 40969
#define EFD_ERROR_REGEX_COMPILE_FAILED                0xA00A // 40970
#define EFD_ERROR_INTERNAL                            0xA00B // 40971
#define EFD_ERROR_INVALID_DNS_RESPONSE                0xA00C // 40972
#define EFD_ERROR_DNS_RESOLUTION_FAILED               0xA00D // 40973
#define EFD_ERROR_FAILED_TIME_CONVERSION              0xA00E // 40974
#define EFD_ERROR_INVALID_SID                         0xA00F // 40975
#define EFD_ERROR_UNEXPECTED_DB_RESULT                0xA010 // 40976
#define EFD_ERROR_INVALID_EFD_CONNECTION            0xA011 // 40977
#define EFD_ERROR_INVALID_PARAMETER                   0xA012 // 40978
#define EFD_ERROR_LDAP_NO_PARENT_DN                   0xA013 // 40979
#define EFD_ERROR_LDAP_ERROR                          0xA014 // 40980
#define EFD_ERROR_NO_SUCH_DOMAIN                      0xA015 // 40981
#define EFD_ERROR_LDAP_FAILED_GETDN                   0xA016 // 40982
#define EFD_ERROR_DUPLICATE_DOMAINNAME                0xA017 // 40983
#define EFD_ERROR_FAILED_FIND_DC                      0xA018 // 40984
#define EFD_ERROR_LDAP_GET_DN_FAILED                  0xA019 // 40985
#define EFD_ERROR_INVALID_SID_REVISION                0xA01A // 40986
#define EFD_ERROR_LOAD_LIBRARY_FAILED                 0xA01B // 40987
#define EFD_ERROR_LOOKUP_SYMBOL_FAILED                0xA01C // 40988
#define EFD_ERROR_INVALID_EVENTLOG                    0xA01D // 40989
#define EFD_ERROR_INVALID_CONFIG                      0xA01E // 40990
#define EFD_ERROR_UNEXPECTED_TOKEN                    0xA01F // 40991
#define EFD_ERROR_LDAP_NO_RECORDS_FOUND               0xA020 // 40992
#define EFD_ERROR_STRING_CONV_FAILED                  0xA021 // 40993
#define EFD_ERROR_QUERY_CREATION_FAILED               0xA022 // 40994
#define EFD_ERROR_NOT_JOINED_TO_AD                    0xA023 // 40995
#define EFD_ERROR_FAILED_TO_SET_TIME                  0xA024 // 40996
#define EFD_ERROR_NO_NETBIOS_NAME                     0xA025 // 40997
#define EFD_ERROR_INVALID_OBJECTGUID                  0xA027 // 40999
#define EFD_ERROR_INVALID_DOMAIN                      0xA028 // 41000
#define EFD_ERROR_NO_DEFAULT_REALM                    0xA029 // 41001
#define EFD_ERROR_NOT_SUPPORTED                       0xA02A // 41002
#define EFD_ERROR_NO_HANDLER                          0xA02C // 41004
#define EFD_ERROR_NO_MATCHING_CACHE_ENTRY             0xA02D // 41005
#define EFD_ERROR_MAC_FLUSH_DS_CACHE_FAILED           0xA033 // 41011
#define EFD_ERROR_ACCESS_DENIED                       0xA033 // 41012
#define EFD_ERROR_SENTINEL                            0xA035 // 41013


//
// Note: When you add errors here, please remember to update
//       the corresponding error strings in unnamed-error.c
//
#define EFD_ERROR_PREFIX                 0xA000 // 40960
#define EFD_ERROR_MASK(_e_)             (_e_ & EFD_ERROR_PREFIX)

#define EFD_API

//Standard GUID's are 16 bytes long.
#define EFD_GUID_SIZE 16

#define _EFD_MAKE_SAFE_FREE(Pointer, FreeFunction) \
    do { \
        if (Pointer) \
        { \
            (FreeFunction)(Pointer); \
            (Pointer) = NULL; \
        } \
    } while (0)

DWORD
EfdOpenServer(
    PHANDLE phConnection
    );

DWORD
EfdCloseServer(
    HANDLE hConnection
    );

EFD_API
size_t
EfdGetErrorString(
    DWORD  dwErrorCode,
    PSTR   pszBuffer,
    size_t stBufSize
    );

#endif /* __EFD_H__ */
