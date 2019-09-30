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
#ifndef DEFS_H
#define DEFS_H

#define BAIL_ON_ERROR(x) if (x) goto error

#define APP_ERROR_BAD_XML                  0x20000000
#define APP_ERROR_XML_DUPLICATED_ELEMENT   0x20000001
#define APP_ERROR_XML_MISSING_ELEMENT      0x20000002
#define APP_ERROR_XPATH_EVAL_FAILED        0x20000003
#define APP_ERROR_CAPABILITY_NOT_FOUND     0x20000004
#define APP_ERROR_INVALID_DWORD            0x20000005
#define APP_ERROR_INVALID_BOOLEAN          0x20000006
#define APP_ERROR_UNKNOWN_TYPE             0x20000007
#define APP_ERROR_INVALID_SUFFIX           0x20000008
#define APP_ERROR_PARAMETER_REQUIRED       0x20000009
#define APP_ERROR_UNEXPECTED_VALUE         0x2000000a
#define APP_ERROR_COULD_NOT_FORK           0x2000000b
#define APP_ERROR_INVALID_ESCAPE_SEQUENCE  0x2000000c
#define APP_ERROR_UNTERMINATED_QUOTE       0x2000000d
#define APP_ERROR_BAD_REGISTRY_PATH        0x2000000e
#define APP_ERROR_VALUE_NOT_ACCEPTED       0x2000000f
#define APP_ERROR_PROGRAM_ERROR            0x20000010
#define APP_ERROR_CAPABILITY_MULTIPLE_MATCHES 0x20000011
#define APP_ERROR_XML_MISSING_ATTRIBUTE       0x20000012
#endif
