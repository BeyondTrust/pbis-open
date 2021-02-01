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

#ifndef __MACERROR_H__
#define __MACERROR_H__


LONG
LWGetMacError(
    DWORD dwError
    );

#ifndef IsNullOrEmptyString

#define IsNullOrEmptyString(pszStr)             \
    (pszStr == NULL || *pszStr == '\0')

#endif

#define LW_SAFE_LOG_STR(s) ((s)?(s):"(null)")

//#ifdef BAIL_ON_MAC_ERROR
//#undef BAIL_ON_MAC_ERROR
//#endif

#define BAIL_ON_MAC_ERROR(dwError) \
    if (dwError) {                                                                                                  \
       LOG_ERROR("Error %d (%s) at %s:%d", dwError, LW_SAFE_LOG_STR(MacErrorToString(dwError)), __FILE__, __LINE__); \
       goto error;                                                                                                  \
    }

#define BAIL_ON_MAC_ERROR_NO_LOG(dwError) \
    if (dwError) { \
       goto error; \
    }

#if defined(WORDS_BIGENDIAN)
#define CONVERT_ENDIAN_DWORD(ui32val)           \
    ((ui32val & 0x000000FF) << 24 |             \
     (ui32val & 0x0000FF00) << 8  |             \
     (ui32val & 0x00FF0000) >> 8  |             \
     (ui32val & 0xFF000000) >> 24)

#define CONVERT_ENDIAN_WORD(ui16val)            \
    ((ui16val & 0x00FF) << 8 |                  \
     (ui16val & 0xFF00) >> 8)

#else
#define CONVERT_ENDIAN_DWORD(ui32val) (ui32val)
#define CONVERT_ENDIAN_WORD(ui16val) (ui16val)
#endif

/* ERRORS */
#define MAC_AD_ERROR_SUCCESS                                0x0000
#define MAC_AD_ERROR_NOT_IMPLEMENTED                        0xC001 // 49153
#define MAC_AD_ERROR_INVALID_PARAMETER                      0xC002 // 49154
#define MAC_AD_ERROR_NOT_SUPPORTED                          0xC003 // 49155
#define MAC_AD_ERROR_LOGON_FAILURE                          0xC004 // 49156
#define MAC_AD_ERROR_INVALID_NAME                           0xC005 // 49157
#define MAC_AD_ERROR_NULL_PARAMETER                         0xC006 // 49158
#define MAC_AD_ERROR_INVALID_TAG                            0xC007 // 49159
#define MAC_AD_ERROR_NO_SUCH_ATTRIBUTE                      0xC008 // 49160
#define MAC_AD_ERROR_INVALID_RECORD_TYPE                    0xC009 // 49161
#define MAC_AD_ERROR_INVALID_ATTRIBUTE_TYPE                 0xC00A // 49162
#define MAC_AD_ERROR_INSUFFICIENT_BUFFER                    0xC00B // 49163
#define MAC_AD_ERROR_IPC_FAILED                             0xC00C // 49164
#define MAC_AD_ERROR_NO_PROC_STATUS                         0xC00D // 49165
#define MAC_AD_ERROR_KRB5_PASSWORD_MISMATCH                 0xC00E // 49166
#define MAC_AD_ERROR_KRB5_PASSWORD_EXPIRED                  0xC00F // 49167
#define MAC_AD_ERROR_KRB5_CLOCK_SKEW                        0xC010 // 49168
#define MAC_AD_ERROR_KRB5_ERROR                             0xC011 // 49169
#define MAC_AD_ERROR_GSS_API_FAILED                         0xC012 // 49170
#define MAC_AD_ERROR_NO_SUCH_POLICY                         0xC013 // 49171
#define MAC_AD_ERROR_LDAP_OPEN                              0xC014 // 49172
#define MAC_AD_ERROR_LDAP_SET_OPTION                        0xC015 // 49173
#define MAC_AD_ERROR_LDAP_QUERY_FAILED                      0xC016 // 49174
#define MAC_AD_ERROR_COMMAND_FAILED                         0xC017 // 49175
#define MAC_AD_ERROR_UPN_NOT_FOUND                          0xC018 // 49176
#define MAC_AD_ERROR_LOAD_LIBRARY_FAILED                    0xC019 // 49177
#define MAC_AD_ERROR_LOOKUP_SYMBOL_FAILED                   0xC01A // 49178
#define MAC_AD_ERROR_CREATE_FAILED                          0xC01B // 49179
#define MAC_AD_ERROR_WRITE_FAILED                           0xC01C // 49180
#define MAC_AD_ERROR_READ_FAILED                            0xC01D // 49181

#define MAC_AD_ERROR_MASK(_e_)                              (_e_ & 0xC000)

#endif /* __MACERROR_H__ */
