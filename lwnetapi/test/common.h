/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

#ifndef _TEST_COMMON_H_
#define _TEST_COMMON_H_


typedef enum {
    RPC_LSA_BINDING       = 1,
    RPC_SAMR_BINDING,
    RPC_NETLOGON_BINDING,
    RPC_DSSETUP_BINDING,
    RPC_WKSSVC_BINDING
} RPC_BINDING_TYPE;


BOOLEAN
CreateRpcBinding(
    PVOID            *phBinding,
    RPC_BINDING_TYPE  eBindingType,
    PCWSTR            pwszHostname,
    PCWSTR            pwszBinding,
    PCREDENTIALS      pCredentials
    );


VOID
FreeRpcBinding(
    PVOID            *phBinding,
    RPC_BINDING_TYPE  eBindingType
    );


DWORD
GetMachinePassword(
    OUT OPTIONAL PWSTR* ppwszDnsDomainName,
    OUT OPTIONAL PWSTR* ppwszDomainName,
    OUT OPTIONAL PWSTR* ppwszMachineSamAccountName,
    OUT OPTIONAL PWSTR* ppwszMachinePassword,
    OUT OPTIONAL PWSTR* ppwszComputerName
    );

#endif /* _TEST_COMMON_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
