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
