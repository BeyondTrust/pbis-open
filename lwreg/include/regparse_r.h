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
 *        regparse.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */

#ifndef REGPARSE_R_H
#define REGPARSE_R_H


typedef struct _REG_PARSE_ITEM
{
    /* 
     * Use these values below when "type" != REG_ATTRIBUTES.
     * When type == REG_ATTRIBUTES, these values are not valid,
     * and regAttributeEntry should be used.
     */
    REG_DATA_TYPE type;       /* Type of value name */
    REG_DATA_TYPE valueType;  /* Type of data value */
    PSTR keyName;
    PSTR valueName;
    DWORD lineNumber;
    void *value;
    DWORD valueLen;
    DWORD status;             /* status of data consistency check */

    /* valid when type = REG_ATTRIBUTES. */
    LWREG_VALUE_ATTRIBUTES regAttr;
} REG_PARSE_ITEM, *PREG_PARSE_ITEM;

#endif /* __REGPARSE_R_H__ */
