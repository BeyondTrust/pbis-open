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

#include "domainjoin.h"
#include "djmodule.h"
#include "djdaemonmgr.h"
#include "djkrb5conf.h"
#include "djauthinfo.h"
#include "djnsswitch.h"
#include "djsshconf.h"
#include "djpamconf.h"
#include "djsystemscripts.h"
#include "djlogincfg.h"
#include "djsecuser.h"
#include "djfirewall.h"
#include "djauditing.h"
#include "djconfig_mac.h"
#include "djddns.h"

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

//Make sure all modules are included in both lists, even if the module
//doesn't apply to start or stop
#if defined(MINIMAL_JOIN)
const JoinModule *startList[] = {
    &DJDoJoinModule,
    &DJKrb5Module,
    &DJDoLeaveModule,
    &DJCacheModule,
    NULL };

const JoinModule *stopList[] = {
    &DJCacheModule,
    &DJDoLeaveModule,
    &DJKrb5Module,
    &DJDoJoinModule,
    NULL };
#else
const JoinModule *startList[] = {
    &DJDaemonStopModule,
    &DJSetHostname,
    &DJKeytabModule,
    &DJDoJoinModule,
    &DJDoLeaveModule,
    &DJNsswitchModule,
    &DJCacheModule,
    &DJDaemonStartModule,
    &DJKrb5Module,
    &DJBashPrompt,
    &DJGdmPresession,
    &DJPamMode,
    &DJPamModule,
    &DJLamAuth,
    &DJSshModule,
    &DJDSPlugin,
    &DJDDNSModule,
    NULL };

const JoinModule *stopList[] = {
    &DJDDNSModule,
    &DJDSPlugin,
    &DJSshModule,
    &DJLamAuth,
    &DJPamModule,
    &DJPamMode,
    &DJGdmPresession,
    &DJBashPrompt,
    &DJDaemonStartModule,
    &DJCacheModule,
    &DJNsswitchModule,
    &DJKrb5Module,
    &DJDaemonStopModule,
    &DJDoLeaveModule,
    &DJDoJoinModule,
    &DJKeytabModule,
    &DJSetHostname,
    NULL };
#endif

void DJZeroJoinProcessOptions(JoinProcessOptions *options)
{
    memset(options, 0, sizeof(*options));
}

void DJFreeJoinProcessOptions(JoinProcessOptions *options)
{
    size_t i;
    CT_SAFE_FREE_STRING(options->domainName);
    CT_SAFE_FREE_STRING(options->shortDomainName);
    CT_SAFE_FREE_STRING(options->computerName);
    CT_SAFE_FREE_STRING(options->ouName);
    CT_SAFE_FREE_STRING(options->userDomainPrefix);
    CT_SAFE_FREE_STRING(options->username);
    CT_SAFE_FREE_STRING(options->password);

    for(i = 0; i < options->moduleStates.size; i++)
    {
        ModuleState *state = DJGetModuleState(options, i);
        if(state->module->FreeModuleData != 0)
            state->module->FreeModuleData(options, state);
    }
    CTArrayFree(&options->moduleStates);
}

ModuleState *DJGetModuleState(const JoinProcessOptions *options, size_t index)
{
    if(index >= options->moduleStates.size)
        return NULL;

    return ((ModuleState *)options->moduleStates.data) + index;
}

ModuleState *DJGetModuleStateByName(const JoinProcessOptions *options, const char *shortName)
{
    size_t i;
    if(shortName == NULL)
        return NULL;
    for(i = 0; i < options->moduleStates.size; i++)
    {
        ModuleState *state = DJGetModuleState(options, i);
        if(!strcmp(state->module->shortName, shortName))
            return state;
    }
    return NULL;
}

void DJRefreshModuleStates(JoinProcessOptions *options, LWException **exc)
{
    size_t i;
    for(i = 0; i < options->moduleStates.size; i++)
    {
        ModuleState *state = DJGetModuleState(options, i);
        LW_TRY(exc, state->lastResult = state->module->QueryState(options, &LW_EXC));
    }
cleanup: ;
}

void NormalizeUsername(PSTR *username, PCSTR domainName, LWException **exc)
{
    PSTR newUsername = NULL;
    PSTR upperDomainName = NULL;
    if(*username == NULL)
        goto cleanup;
    if(strchr(*username, '@') != NULL)
    {
        CTStrToUpper(strrchr(*username, '@'));
        goto cleanup;
    }

    if(IsNullOrEmptyString(domainName))
    {
        LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Unable to determine user domain", "The domain that '%s' belongs to could not be automatically determined. Please pass the user name in user@domain.com syntax.", *username);
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, CTStrdup(domainName, &upperDomainName));
    CTStrToUpper(upperDomainName);

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&newUsername, "%s@%s", *username, upperDomainName));
    CT_SAFE_FREE_STRING(*username);
    *username = newUsername;
    newUsername = NULL;

cleanup:
    CT_SAFE_FREE_STRING(newUsername);
    CT_SAFE_FREE_STRING(upperDomainName);
}

void DJInitModuleStates(JoinProcessOptions *options, LWException **exc)
{
    size_t i;
    PCSTR userDomain;
    PDOMAINJOININFO joinedInfo = NULL;

    const JoinModule **moduleTable = NULL;

    if(options->joiningDomain)
    {
        userDomain = options->domainName;
        moduleTable = startList;
    }
    else
    {
        if (options->domainName)
        {
            userDomain = options->domainName;
        }
        else
        {
            LW_TRY(exc, QueryInformation(&joinedInfo, &LW_EXC));
            userDomain = joinedInfo->pszDomainName;
        }
        moduleTable = stopList;
    }
    LW_TRY(exc, NormalizeUsername(&options->username, userDomain, &LW_EXC));

    for(i = 0; moduleTable[i] != NULL; i++)
    {
        const JoinModule *module = moduleTable[i];
        ModuleState state;
        ModuleState *arrayState;

        memset(&state, 0, sizeof(state));
        state.module = module;
        state.lastResult = NotApplicable;

        LW_CLEANUP_CTERR(exc, CTArrayAppend(&options->moduleStates, sizeof(state), &state, 1));
        arrayState = DJGetModuleState(options, options->moduleStates.size - 1);
        arrayState->runModule = module->runByDefault;

        LW_TRY(exc, arrayState->lastResult = module->QueryState(options, &LW_EXC));

        switch(arrayState->lastResult)
        {
            case NotApplicable:
                LW_CLEANUP_CTERR(exc, CTArrayRemove(&options->moduleStates,
                        options->moduleStates.size - 1, sizeof(state), 1));
                break;
            case FullyConfigured:
            case CannotConfigure:
                arrayState->runModule = FALSE;
                break;
            case SufficientlyConfigured:
            case NotConfigured:
                break;
            case ApplePluginInUse:
                LW_RAISE_EX(exc, ERROR_INVALID_OPERATION, "Apple AD Directory Plugin in use.", "The configuration of module '%s' detected that the computer is already joined to Active Directory with the built in Apple AD plugin. To use Likewise, please first unbind your Mac from Active Directory by using the Directory Utility of your system.\n", state.module->shortName);
                goto cleanup;
            default:
                LW_RAISE_EX(exc, ERROR_INVALID_OPERATION, "Invalid module state", "The configuration of module '%s' returned an invalid configuration state.\n", state.module->shortName);
                goto cleanup;
        }
    }
    return;

cleanup:
    if(joinedInfo != NULL)
        FreeDomainJoinInfo(joinedInfo);
    CTArrayFree(&options->moduleStates);
}

void DJCheckRequiredEnabled(const JoinProcessOptions *options, LWException **exc)
{
    PSTR exceptionMessage = NULL;
    size_t i;
    for(i = 0; i < options->moduleStates.size; i++)
    {
        const ModuleState *state = DJGetModuleState((JoinProcessOptions *)options, i);
        switch(state->lastResult)
        {
            case CannotConfigure:
                LW_TRY(exc, exceptionMessage = state->module->GetChangeDescription(options, &LW_EXC));
                LW_RAISE_EX(exc, LW_ERROR_MODULE_NOT_ENABLED, "Manual configuration required", "The configuration stage '%s' cannot be completed automatically. Please manually perform the following steps and rerun the domain join:\n\n%s", state->module->longName, exceptionMessage);
                goto cleanup;
            case NotConfigured:
                if(!state->runModule)
                {
                    LW_TRY(exc, exceptionMessage = state->module->GetChangeDescription(options, &LW_EXC));
                    LW_RAISE_EX(exc, LW_ERROR_MODULE_NOT_ENABLED, "Required configuration stage not enabled", "The configuration of module '%s' is required. Please either allow this configuration stage to be performed automatically (by passing '--enable %s'), or manually perform these configuration steps and rerun the domain join:\n\n%s", state->module->longName, state->module->shortName, exceptionMessage);
                    goto cleanup;
                }
                break;
            case SufficientlyConfigured:
                break;
            case NotApplicable:
            case FullyConfigured:
                if(state->runModule)
                {
                    //The UI shouldn't even let this happen.
                    LW_RAISE_EX(exc, LW_ERROR_MODULE_ALREADY_DONE, "Invalid module enabled", "Running module '%s' is not valid at this time because it is already configured. Please disable it and try again.", state->module->longName);
                    goto cleanup;
                }
                break;
            case ApplePluginInUse:
                LW_RAISE_EX(exc, ERROR_INVALID_OPERATION, "Apple AD Directory Plugin in use.", "The configuration of module '%s' detected that the computer is already joined to Active Directory with the built in Apple AD plugin. To use Likewise, please first unbind your Mac from Active Directory by using the Directory Utility of your system.\n", state->module->shortName);
                goto cleanup;
            default:
                LW_RAISE_EX(exc, ERROR_INVALID_OPERATION, "Invalid module state", "The configuration of module '%s' returned an invalid configuration state.\n", state->module->shortName);
                goto cleanup;
        }
    }

cleanup:
    CT_SAFE_FREE_STRING(exceptionMessage);
}

void DJRunJoinProcess(JoinProcessOptions *options, LWException **exc)
{
    size_t i;
    PSTR exceptionMessage = NULL;
    PSTR exceptionTitle = NULL;
    LWException *moduleException = NULL;

    //Before running the modules, make sure that all of the necessary modules
    //are enabled.
    LW_TRY(exc, DJCheckRequiredEnabled(options, &LW_EXC));

    for(i = 0; i < options->moduleStates.size; i++)
    {
        ModuleState *state = DJGetModuleState(options, i);
        if(!state->runModule)
            continue;
	DJ_LOG_INFO("Running module %s", state->module->shortName);
        state->module->MakeChanges(options, &moduleException);
        LW_TRY(exc, state->lastResult = state->module->QueryState(options, &LW_EXC));
        if(!LW_IS_OK(moduleException))
        {
            switch(state->lastResult)
            {
                case NotConfigured:
                case CannotConfigure:
                    LW_CLEANUP(exc, moduleException);
                case NotApplicable:
                case FullyConfigured:
                case SufficientlyConfigured:
                    LW_CLEANUP_CTERR(exc, LWExceptionToString(moduleException, NULL, FALSE, options->showTraces, &exceptionMessage));
                    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
                            &exceptionTitle,
                            "A resumable error occurred while processing the '%s' module",
                            state->module->shortName));
                    if (options->warningCallback != NULL)
                    {
                        options->warningCallback(options, exceptionTitle, exceptionMessage);
                    }
                    DJLogException(LOG_LEVEL_WARNING, moduleException);
                    LWHandle(&moduleException);
                    CT_SAFE_FREE_STRING(exceptionMessage);
                    CT_SAFE_FREE_STRING(exceptionTitle);
                    break;
                case ApplePluginInUse:
                    LW_RAISE_EX(exc, ERROR_INVALID_OPERATION, "Apple AD Directory Plugin in use.", "The configuration of module '%s' detected that the computer is already joined to Active Directory with the built in Apple AD plugin. To use Likewise, please first unbind your Mac from Active Directory by using the Directory Utility of your system.\n", state->module->shortName);
                    goto cleanup;
                default:
                    LW_RAISE_EX(exc, ERROR_INVALID_OPERATION, "Invalid module state", "The configuration of module '%s' returned an invalid configuration state.\n", state->module->shortName);
                    goto cleanup;
            }
        }
        else
        {
            switch(state->lastResult)
            {
                case FullyConfigured:
                case NotApplicable:
                    state->runModule = FALSE;
                    break;
                case NotConfigured:
                case CannotConfigure:
                    LW_RAISE_EX(exc,
                            ERROR_CAN_NOT_COMPLETE, "Module not configured",
                            "Even though the configuration of '%s' was executed, the configuration is not complete. Please contact Likewise support.",
                            state->module->shortName);
                    goto cleanup;
                case SufficientlyConfigured:
                    LW_CLEANUP_CTERR(exc,
                            CTAllocateStringPrintf(&exceptionMessage,
                            "Even though the configuration of '%s' was executed, the configuration did not fully complete. Please contact Likewise support.",
                            state->module->shortName));
                    if (options->warningCallback != NULL)
                    {
                        options->warningCallback(options,
                                                 "A resumable error occurred while processing a module",
                                                 exceptionMessage);
                    }
                    CT_SAFE_FREE_STRING(exceptionMessage);
                    break;
                case ApplePluginInUse:
                    LW_RAISE_EX(exc, ERROR_INVALID_OPERATION, "Apple AD Directory Plugin in use.", "The configuration of module '%s' detected that the computer is already joined to Active Directory with the built in Apple AD plugin. To use Likewise, please first unbind your Mac from Active Directory by using the Directory Utility of your system.\n", state->module->shortName);
                    goto cleanup;
                default:
                    LW_RAISE_EX(exc, ERROR_INVALID_OPERATION, "Invalid module state", "The configuration of module '%s' returned an invalid configuration state.\n", state->module->shortName);
                    goto cleanup;
            }
        }
    }

cleanup:
    LWHandle(&moduleException);
    CT_SAFE_FREE_STRING(exceptionMessage);
    CT_SAFE_FREE_STRING(exceptionTitle);
}

void DJEnableModule(JoinProcessOptions *options, PCSTR shortName, BOOLEAN enable, LWException **exc)
{
    ModuleState *state = DJGetModuleStateByName(options, shortName);
    if(!state)
    {
        LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Unable to enable/disable module", "Unable to enable/disable module '%s'. This module could not be found. Please check the name and try again. Keep in mind that some domainjoin modules may not be applicable on all platforms.", shortName);
        return;
    }
    state->runModule = enable;
}
