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

#ifndef __DJMODULE_H__
#define __DJMODULE_H__

#include "ctarray.h"
#include "lwexc.h"

typedef enum _QueryResult
{
    CannotConfigure = 0,
    NotConfigured = 1,
    SufficientlyConfigured = 2,
    FullyConfigured = 3,
    NotApplicable = 4,
    ApplePluginInUse = 5
} QueryResult;

typedef enum _ModuleDisposition
{
    DisableModule = 0,
    EnableModule = 1,
    IgnoreModule = 2
} ModuleDisposition;

// a tri-bool for enable/force
typedef enum _EnableForceBoolean
{
    False = 0,
    True  = 1,
    Force = 2
} EnableForceBoolean;

struct _JoinModule;
typedef struct _JoinModule JoinModule;

//Used inside of JoinProcessOptions
typedef struct
{
    ModuleDisposition disposition;
    QueryResult lastResult;
    const JoinModule *module;
    //Can be used by domainjoin frontend.
    void *userData;
    //Can be used by domainjoin module.
    void *moduleData;
} ModuleState;

struct _JoinProcessOptions;
typedef struct _JoinProcessOptions JoinProcessOptions;
typedef void (*WarningFunction)(const JoinProcessOptions *options, const char *title, const char *message);

// Options filled in by the UI in order to communicate with the join modules.
struct _JoinProcessOptions
{
    BOOLEAN isEnterprise;

    PSTR domainName;
    PSTR shortDomainName;
    PSTR computerName;
    PSTR ouName;
    PSTR username;
    PSTR password;
    DWORD uacFlags;
    void *userData;
    // TRUE if joining to AD, FALSE if leaving
    BOOLEAN joiningDomain;
    BOOLEAN showTraces;
    BOOLEAN disableTimeSync;
    BOOLEAN enableMultipleJoins;
    BOOLEAN ignorePam;
    BOOLEAN noPreAccount;
    BOOLEAN ignoreSsh;
    BOOLEAN releaseLicense;
    BOOLEAN deleteAccount;

    BOOLEAN disableGSSAPI;

    BOOLEAN setAssumeDefaultDomain;

    // TRUE - lsass will be configured to append a domain prefix so user logons
    // can use short account names.
    // FALSE - logons will require a UPN or NT4 style domain user account.
    BOOLEAN assumeDefaultDomain;

    // Optional alternate user domain prefix used with assumeDefaultDomain (true) setting.
    // If null, the computer domain name prefix is assumed.
    PSTR userDomainPrefix;

    WarningFunction warningCallback;

    // Contains modules that are enabled and disabled by the user, but does not
    // contain NA modules. This list is populated from the moduleTable.  The
    // data type inside of the array is ModuleState.
    DynamicArray moduleStates;

    // Range is zero to 3600 seconds. Zero disables the functionality.
    DWORD dwTrustEnumerationWaitSeconds;

    // assumed default cell mode - act like default cell mode even if no cells exist
    // FALSE - no support for this mode
    // TRUE  - enable this mode if a named/default cell is not found
    // FORCE - force this mode even if a named/default cell exists
    EnableForceBoolean assumeDefaultCellMode;

    // unprovisioned mode
    // FALSE - no support for this mode
    // TRUE  - enable this mode if a named/default cell is not found
    // FORCE - force this mode even if a named/default cell exists
    EnableForceBoolean unprovisionedMode;

    PSTR pszConfigFile;
};

//A struct that defines a join module.
struct _JoinModule
{
    BOOLEAN runByDefault;
    PCSTR shortName;
    PCSTR longName;

    //Every module has the following entry points
    QueryResult (*QueryState)(const JoinProcessOptions *, LWException **);
    void (*MakeChanges)(JoinProcessOptions *, LWException **);
    PSTR (*GetChangeDescription)(const JoinProcessOptions *, LWException **);
    void (*FreeModuleData)(const JoinProcessOptions *, ModuleState *state);
};

DWORD
DJInit(
    VOID
    );

DWORD
DJSetComputerNameEx(
    PCSTR pszComputerName
    );

DWORD
DJJoinDomain(
    PCSTR pszDomain,
    PCSTR pszOU,
    PCSTR pszUsername,
    PCSTR pszPassword
    );

DWORD
DJQueryJoinInformation(
    PSTR* ppszComputerName,
    PSTR* ppszDomainName,
    PSTR* ppszComptuterDN
    );

DWORD
DJUnjoinDomain(
    PCSTR pszUsername,
    PCSTR pszPassword
    );

VOID
DJFreeMemory(
    PVOID pMemory
    );

DWORD
DJShutdown(
    VOID
    );

void DJZeroJoinProcessOptions(JoinProcessOptions *options);
void DJFreeJoinProcessOptions(JoinProcessOptions *options);
void DJRefreshModuleStates(JoinProcessOptions *options, LWException **err);
void DJInitModuleStates(JoinProcessOptions *options, LWException **err);
void DJRunJoinProcess(JoinProcessOptions *options, LWException **err);

ModuleState *DJGetModuleState(const JoinProcessOptions *options, size_t index);
ModuleState *DJGetModuleStateByName(const JoinProcessOptions *options, const char *shortName);

void DJSetModuleDisposition(JoinProcessOptions *options, PCSTR shortName, ModuleDisposition disposition, LWException **exc);

void DJCheckRequiredEnabled(const JoinProcessOptions *options, LWException **exc);

#endif // __DJMODULE_H__
