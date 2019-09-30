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
 * Module Name:
 *
 *        ids.h
 *
 * Abstract:
 *        Methods fo ID manipulations.
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: May 7, 2010
 *
 */

#ifndef _AD_TOOL_IDS_H_
#define _AD_TOOL_IDS_H_

/**
 * Covert object GUID to hex string.
 *
 * @param pszStr Bytes to convert.
 * @param pszStr Converted string.
 * @return 0 on success; error code on failure.
 */
extern DWORD Guid2Str(IN PVOID pszStr, OUT PSTR* ppszHexStr);

/**
 * Convert char to hex.
 *
 * @param in Char to convert.
 * @param s Preallocated 2 chars array.
 */
extern VOID Char2Hex(IN UCHAR in, OUT PSTR s);

/**
 * Covert AD object's SID to an ASCII string.
 *
 * @param s Bytes to convert.
 * @param out Converted string.
 * @return 0 on success; error code on failure.
 */
extern DWORD Sid2Str(IN PVOID s, OUT PSTR *out);

/**
 * Generate UID from SID.
 *
 * @param s SID bytes.
 * @param out UID (dynamically allocated).
 * @return 0 on success; error code on failure.
 */
extern DWORD Sid2Id(IN PVOID s, OUT PDWORD out);

#endif /* _AD_TOOL_IDS_H_ */
