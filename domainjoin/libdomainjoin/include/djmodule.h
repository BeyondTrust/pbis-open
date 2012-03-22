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

struct _JoinModule;
typedef struct _JoinModule JoinModule;

//Used inside of JoinProcessOptions
typedef struct
{
    BOOLEAN runModule;
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

//Options filled in by the UI in order to communicate with the join modules.
struct _JoinProcessOptions
{
    PSTR domainName;
    PSTR shortDomainName;
    PSTR computerName;
    PSTR ouName;
    PSTR username;
    PSTR password;
    DWORD uacFlags;
    void *userData;
    //TRUE if joining to AD, FALSE if leaving
    BOOLEAN joiningDomain;
    BOOLEAN showTraces;
    BOOLEAN disableTimeSync;
    BOOLEAN enableMultipleJoins;
    BOOLEAN ignorePam;

    BOOLEAN setAssumeDefaultDomain;
    // TRUE means that lsass will be configured to append a domain prefix so user logons
    // can be with short account names.
    // FALSE means that logons will require a UPN or NT4 style domain user account.
    BOOLEAN assumeDefaultDomain;
    // Optional alternate user domain prefix used with assumeDefaultDomain (true) setting.
    // If null, the computer domain name prefix is assumed.
    PSTR userDomainPrefix;

    WarningFunction warningCallback;
    /* Contains modules that are enabled and disabled by the user, but does
     * not contain NA modules. This list is populated from the moduleTable.
     * The data type inside of the array is ModuleState.
    */
    DynamicArray moduleStates;
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

void DJEnableModule(JoinProcessOptions *options, PCSTR shortName, BOOLEAN enable, LWException **exc);

void DJCheckRequiredEnabled(const JoinProcessOptions *options, LWException **exc);

#endif // __DJMODULE_H__
