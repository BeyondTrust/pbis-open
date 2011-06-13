/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

#include "domainjoin.h"
#include "djdistroinfo.h"
#include "djsshconf.h"
#include "djpamconf.h"
#include "djcli.h"
#include "djfirewall.h"
#include "djauditing.h"
#include "ctprocutils.h"
#include "lwexc.h"

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

static
void
ShowUsage()
{
    fprintf(stdout, "usage: domainjoin-cli [options] command [args...]\n\n");
    fprintf(stdout, "  where options are:\n\n");
    fprintf(stdout, "    --help                                     Display this help information.\n");
    fprintf(stdout, "    --help-internal                            Display help for debug commands\n");
    fprintf(stdout, "    --logfile {.|path}                         Log to a file (or \".\" to log\n"
                    "                                               to console).\n");
    fprintf(stdout, "    --loglevel {error|warning|info|verbose}    Adjusts how much logging is\n"
                    "                                               produced by domainjoin.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  and commands are:\n\n");
    fprintf(stdout, "    query\n");
    fprintf(stdout, "    setname <computer name>\n");
    fprintf(stdout, "    join [--notimesync] [--enable <module> --disable <module> ...] [--ou <organizationalUnit>] [--multiple] <domain name> <user name> [<password>]\n");
    fprintf(stdout, "    join [--advanced] --preview [--ou <organizationalUnit>] <domain name>\n");
    fprintf(stdout, "    join [--assumeDefaultDomain {yes|no}] [--userDomainPrefix <short domain name>] [--ou <organizationalUnit>] <domain name>\n");
    fprintf(stdout, "    join [--ou <organizationalUnit>] --details <module> <domain name>\n");
    fprintf(stdout, "    leave [--enable <module> --disable <module> ...] [--multiple <domain name>] [user name] [password]\n");
    fprintf(stdout, "    leave [--advanced] --preview [user name] [password]\n");
    fprintf(stdout, "    leave --details <module>\n\n");

    fprintf(stdout, "  Example:\n\n");
    fprintf(stdout, "    domainjoin-cli join likewisedemo.com Administrator\n\n");
}

static
void
ShowUsageInternal()
{
    ShowUsage();

    fprintf(stdout, "  Internal debug commands:\n");
    fprintf(stdout, "    fixfqdn\n");
    fprintf(stdout, "    configure { --enable | --disable } pam [--testprefix <dir>]\n");
    fprintf(stdout, "    configure { --enable | --disable } nsswitch [--testprefix <dir>]\n");
    fprintf(stdout, "    configure { --enable | --disable } ssh [--testprefix <dir>]\n");
    fprintf(stdout, "    configure { --enable | --disable } [--testprefix <dir>] [--long <longdomain>] [--short <shortdomain>] krb5\n");
    fprintf(stdout, "    configure { --enable | --disable } firewall [--testprefix <dir>]\n");
    fprintf(stdout, "    configure { --enable | --disable } eventfwdd\n");
    fprintf(stdout, "    configure { --enable | --disable } reapsysld\n");
    fprintf(stdout, "    get_os_type\n");
    fprintf(stdout, "    get_arch\n");
    fprintf(stdout, "    get_distro\n");
    fprintf(stdout, "    get_distro_version\n");
    fprintf(stdout, "    raise_error <error code | error name | 0xhex error code>\n");
    fprintf(stdout, "\n");
}

static
DWORD
FillMissingPassword(
    PCSTR username,
    PSTR* ppszPassword
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR pszPassword = NULL;
    PCSTR pszEnvPassword = NULL;

    pszEnvPassword = getenv("PASSWORD");
    if (pszEnvPassword == NULL)
    {
        fprintf(stdout, "%s's password: ",username);
        fflush(stdout);
        ceError = GetPassword(&pszPassword);
        BAIL_ON_CENTERIS_ERROR(ceError);
        fprintf(stdout, "\n");
    }
    else
    {
        DJ_LOG_WARNING("Retrieved password from envionmental variable");
        ceError = CTStrdup(pszEnvPassword, &pszPassword);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (!IsNullOrEmptyString(pszPassword)) {
        *ppszPassword = pszPassword;
        pszPassword = NULL;
    }

error:

    if (pszPassword)
        CTFreeString(pszPassword);

    return ceError;
}

BOOLEAN GetEnableBoolean(EnableType dwEnable)
{
    PDOMAINJOININFO pDomainJoinInfo = NULL;
    if(dwEnable == ENABLE_TYPE_AUTO)
    {
        LW_TRY(NULL, QueryInformation(&pDomainJoinInfo, &LW_EXC));

        if(IsNullOrEmptyString(pDomainJoinInfo->pszDomainName))
            dwEnable = ENABLE_TYPE_DISABLE;
        else
            dwEnable = ENABLE_TYPE_ENABLE;

    }

cleanup:
    if(pDomainJoinInfo != NULL)
        FreeDomainJoinInfo(pDomainJoinInfo);

    return dwEnable == ENABLE_TYPE_ENABLE;
}

void PrintWarning(const JoinProcessOptions *options, const char *title, const char *message)
{
    PSTR      wrapped = NULL;
    int columns;
    if(CTGetTerminalWidth(fileno(stdout), &columns))
        columns = -1;

    //This function doesn't return a DWORD, so we have to recover as much
    //as possible.
    if(!CTWordWrap(message, &wrapped, 4, columns))
        fprintf(stdout, "Warning: %s\n%s\n\n", title, wrapped);
    else
        fprintf(stdout, "Warning: %s\n%s\n\n", title, message);
    CT_SAFE_FREE_STRING(wrapped);
    DJ_LOG_WARNING("%s\n%s", title, message);
}

void PrintModuleState(ModuleState *state)
{
    char resultChar;
    if(state->lastResult != FullyConfigured &&
            state->lastResult != CannotConfigure)
    {
        fprintf(stdout, "[%c] ", state->runModule? 'X' : ' ');
    }
    else
    {
        fprintf(stdout, "    ");
    }
    switch(state->lastResult)
    {
        default:
        case NotApplicable:
            //This case should not occur
            resultChar = 'E';
            break;
        case FullyConfigured:
            resultChar = 'F';
            break;
        case SufficientlyConfigured:
            resultChar = 'S';
            break;
        case NotConfigured:
            resultChar = 'N';
            break;
        case CannotConfigure:
            //This is distinguishable from NotConfigured because the
            //checkbox to the left doesn't appear.
            resultChar = 'N';
            break;
    }
    fprintf(stdout, "[%c] %-15s- %s\n", resultChar, state->module->shortName,
            state->module->longName);
}

void PrintStateKey()
{
    fprintf(stdout,
"\n"
"Key to flags\n"
"[F]ully configured        - the system is already configured for this step\n"
"[S]ufficiently configured - the system meets the minimum configuration\n"
"                            requirements for this step\n"
"[N]ecessary               - this step must be run or manually performed.\n"
"\n"
"[X]                       - this step is enabled and will make changes\n"
"[ ]                       - this step is disabled and will not make changes\n");
}

void PrintJoinHeader(const JoinProcessOptions *options, LWException **exc)
{
    PSTR fqdn = NULL;
    PDOMAINJOININFO pDomainJoinInfo = NULL;
    PCSTR domain;

    if(options->joiningDomain)
    {
        LW_CLEANUP_CTERR(exc, DJGetFinalFqdn(options, &fqdn));
        fprintf(stdout,
                "Joining to AD Domain:   %s\n"
                "With Computer DNS Name: %s\n\n",
                options->domainName,
                fqdn);
    }
    else
    {
        if (options->domainName)
        {
            domain = options->domainName;
        }
        else
        {
            LW_TRY(exc, QueryInformation(&pDomainJoinInfo, &LW_EXC));
            domain = pDomainJoinInfo->pszDomainName;
        }
        if(domain == NULL)
            domain = "(unknown)";
        fprintf(stdout, "Leaving AD Domain:   %s\n", domain);
    }

cleanup:
    CT_SAFE_FREE_STRING(fqdn);
}

void PrintModuleStates(BOOLEAN showTristate, JoinProcessOptions *options)
{
    size_t i;
    if(showTristate)
    {
        for(i = 0; i < options->moduleStates.size; i++)
        {
            PrintModuleState(DJGetModuleState(options, i));
        }
        PrintStateKey();
    }
    else
    {
        fprintf(stdout, "The following stages are currently configured to be run during the domain join:\n");
        for(i = 0; i < options->moduleStates.size; i++)
        {
            ModuleState *state = DJGetModuleState(options, i);
            if(state->runModule)
            {
                fprintf(stdout, "%-15s- %s\n", state->module->shortName,
                        state->module->longName);
            }
        }
    }
}

void DoJoin(int argc, char **argv, int columns, LWException **exc)
{
    JoinProcessOptions options;
    BOOLEAN advanced = FALSE;
    BOOLEAN preview = FALSE;
    DynamicArray enableModules, disableModules;
    DynamicArray detailModules;
    size_t i;
    int passwordIndex = -1;
    PSTR moduleDetails = NULL;
    PSTR wrapped = NULL;

    DJZeroJoinProcessOptions(&options);
    memset(&enableModules, 0, sizeof(enableModules));
    memset(&disableModules, 0, sizeof(disableModules));
    memset(&detailModules, 0, sizeof(detailModules));

    while(argc > 0 && CTStrStartsWith(argv[0], "--"))
    {
        if(!strcmp(argv[0], "--advanced"))
            advanced = TRUE;
        else if(!strcmp(argv[0], "--preview"))
            preview = TRUE;
        else if(!strcmp(argv[0], "--ignore-firewall-ntp"))
        {
            printf("Warning: --ignore-firewall-ntp is deprecated. This behavior is now default.\n");
        }
        else if(!strcmp(argv[0], "--ignore-pam"))
            options.ignorePam = TRUE;
        else if(!strcmp(argv[0], "--notimesync"))
            options.disableTimeSync = TRUE;
        else if(!strcmp(argv[0], "--multiple"))
            options.enableMultipleJoins = TRUE;
        else if(!strcmp(argv[0], "--nohosts"))
        {
            PCSTR module = "hostname";
            LW_CLEANUP_CTERR(exc, CTArrayAppend(&disableModules, sizeof(PCSTR), &module, 1));
        }
        else if(argc < 2)
        {
            LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
            goto cleanup;
        }
        else if(!strcmp(argv[0], "--enable"))
        {
            LW_CLEANUP_CTERR(exc, CTArrayAppend(&enableModules, sizeof(PCSTR), &argv[1], 1));
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--disable"))
        {
            LW_CLEANUP_CTERR(exc, CTArrayAppend(&disableModules, sizeof(PCSTR), &argv[1], 1));
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--details"))
        {
            LW_CLEANUP_CTERR(exc, CTArrayAppend(&detailModules, sizeof(PCSTR), &argv[1], 1));
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--ou"))
        {
            DJ_LOG_INFO("Domainjoin invoked with option --ou %s", argv[1]);
            CT_SAFE_FREE_STRING(options.ouName);
            LW_CLEANUP_CTERR(exc, CTStrdup(argv[1], &options.ouName));
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--assumeDefaultDomain"))
        {
            DJ_LOG_INFO("Domainjoin invoked with option --assumeDefaultDomain");
            options.setAssumeDefaultDomain = TRUE;

            if (!strcasecmp(argv[1], "yes") || !strcasecmp(argv[1], "true") ||
                !strcasecmp(argv[1], "on"))
            {
                options.assumeDefaultDomain = TRUE;
            }
            else if (!strcasecmp(argv[1], "no") ||
                !strcasecmp(argv[1], "false") ||
                !strcasecmp(argv[1], "off"))
            {
                options.assumeDefaultDomain = FALSE;
            }
            else
            {
                LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
                goto cleanup;
            }
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--userDomainPrefix"))
        {
            DJ_LOG_INFO("Domainjoin invoked with option --userDomainPrefix %s", argv[1]);
            options.setAssumeDefaultDomain = TRUE;
            options.assumeDefaultDomain = TRUE;
            CT_SAFE_FREE_STRING(options.userDomainPrefix);
            LW_CLEANUP_CTERR(exc, CTStrdup(argv[1], &options.userDomainPrefix));
            CTStrToUpper(options.userDomainPrefix);
            argv++;
            argc--;
        }
        else
        {
            LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
            goto cleanup;
        }
        argv++;
        argc--;
    }

    if(argc == 3)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup(argv[2], &options.password));
        passwordIndex = 2;
    }
    // The join username is not required in preview or details mode.
    else if(argc == 1 && (preview || detailModules.size != 0) )
        ;
    else if(argc != 2)
    {
        LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
        goto cleanup;
    }
    options.joiningDomain = TRUE;

    DJ_LOG_INFO("Domainjoin invoked with %d arg(s) to the join command:", argc);
    for(i = 0; i < argc; i++)
    {
        DJ_LOG_INFO("    [%s]", i == passwordIndex ? "<password>" : argv[i]);
    }

    LW_CLEANUP_CTERR(exc, CTStrdup(
        argv[0], &options.domainName));
    if(argc > 1)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup(argv[1], &options.username));
    }

    options.warningCallback = PrintWarning;
    options.showTraces = advanced;
    LW_CLEANUP_CTERR(exc, DJGetComputerName(&options.computerName));

    LW_TRY(exc, DJInitModuleStates(&options, &LW_EXC));

    for(i = 0; i < enableModules.size; i++)
    {
        PCSTR module = *(PCSTR *)CTArrayGetItem(
                    &enableModules, i, sizeof(PCSTR));
        if(CTArrayFindString(&disableModules, module) != -1)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Module already specified", "The module '%s' is listed as being disabled and enabled", module);
            goto cleanup;
        }
        LW_TRY(exc, DJEnableModule(&options, module, TRUE, &LW_EXC));
    }

    for(i = 0; i < disableModules.size; i++)
    {
        PCSTR module = *(PCSTR *)CTArrayGetItem(
                    &disableModules, i, sizeof(PCSTR));
        if(CTArrayFindString(&enableModules, module) != -1)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Module already specified", "The module '%s' is listed as being disabled and enabled", module);
            goto cleanup;
        }
        LW_TRY(exc, DJEnableModule(&options, module, FALSE, &LW_EXC));
    }

    for(i = 0; i < detailModules.size; i++)
    {
        PCSTR module = *(PCSTR *)CTArrayGetItem(
                    &detailModules, i, sizeof(PCSTR));
        ModuleState *state = DJGetModuleStateByName(&options, module);
        if(state == NULL)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Unable to find module.", "Please check the spelling of '%s'. This module cannot be found", module);
            goto cleanup;
        }
        PrintModuleState(state);
    }
    if(detailModules.size > 0)
    {
        PrintStateKey();
    }
    for(i = 0; i < detailModules.size; i++)
    {
        PCSTR module = *(PCSTR *)CTArrayGetItem(
                    &detailModules, i, sizeof(PCSTR));
        ModuleState *state = DJGetModuleStateByName(&options, module);
        CT_SAFE_FREE_STRING(moduleDetails);
        CT_SAFE_FREE_STRING(wrapped);
        LW_TRY(exc, moduleDetails = state->module->GetChangeDescription(&options, &LW_EXC));
        LW_CLEANUP_CTERR(exc, CTWordWrap(moduleDetails, &wrapped, 4, columns));
        fprintf(stdout, "\nDetails for '%s':\n%s\n", state->module->longName, wrapped);
    }
    if(detailModules.size > 0)
        goto cleanup;

    LW_TRY(exc, PrintJoinHeader(&options, &LW_EXC));

    if(preview)
    {
        PrintModuleStates(advanced, &options);
        if(!advanced)
            LW_TRY(exc, DJCheckRequiredEnabled(&options, &LW_EXC));
        goto cleanup;
    }

    LW_TRY(exc, DJCheckRequiredEnabled(&options, &LW_EXC));

    if (IsNullOrEmptyString(options.password))
    {
        CT_SAFE_FREE_STRING(options.password);

        LW_CLEANUP_CTERR(exc, FillMissingPassword(options.username,
                    &options.password));
    }

    LW_TRY(exc, DJRunJoinProcess(&options, &LW_EXC));
    fprintf(stdout, "SUCCESS\n");

cleanup:
    DJFreeJoinProcessOptions(&options);
    CTArrayFree(&enableModules);
    CTArrayFree(&disableModules);
    CTArrayFree(&detailModules);
    CT_SAFE_FREE_STRING(moduleDetails);
    CT_SAFE_FREE_STRING(wrapped);
}

void DoLeaveNew(int argc, char **argv, int columns, LWException **exc)
{
    JoinProcessOptions options;
    BOOLEAN advanced = FALSE;
    BOOLEAN preview = FALSE;
    DynamicArray enableModules, disableModules;
    DynamicArray detailModules;
    size_t i;
    PSTR moduleDetails = NULL;
    PSTR wrapped = NULL;
    int passwordIndex = -1;

    DJZeroJoinProcessOptions(&options);
    memset(&enableModules, 0, sizeof(enableModules));
    memset(&disableModules, 0, sizeof(disableModules));
    memset(&detailModules, 0, sizeof(detailModules));

    while(argc > 0 && CTStrStartsWith(argv[0], "--"))
    {
        if(!strcmp(argv[0], "--advanced"))
            advanced = TRUE;
        else if(!strcmp(argv[0], "--preview"))
            preview = TRUE;
        else if(argc < 2)
        {
            LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
            goto cleanup;
        }
        else if(!strcmp(argv[0], "--enable"))
        {
            LW_CLEANUP_CTERR(exc, CTArrayAppend(&enableModules, sizeof(PCSTR *), &argv[1], 1));
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--disable"))
        {
            LW_CLEANUP_CTERR(exc, CTArrayAppend(&disableModules, sizeof(PCSTR *), &argv[1], 1));
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--details"))
        {
            LW_CLEANUP_CTERR(exc, CTArrayAppend(&detailModules, sizeof(PCSTR *), &argv[1], 1));
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--multiple"))
        {
            options.enableMultipleJoins = TRUE;
            LW_CLEANUP_CTERR(exc, CTStrdup(argv[1], &options.domainName));
            argv++;
            argc--;
        }
        else
        {
            LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
            goto cleanup;
        }
        argv++;
        argc--;
    }

    if(argc == 2)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup(argv[1], &options.password));
        passwordIndex = 1;
    }
    else if(argc > 2)
    {
        LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
        goto cleanup;
    }
    options.joiningDomain = FALSE;

    DJ_LOG_INFO("Domainjoin invoked with %d arg(s) to the leave command:", argc);
    for(i = 0; i < argc; i++)
    {
        DJ_LOG_INFO("    [%s]", i == passwordIndex ? "<password>" : argv[i]);
    }

    if(argc > 0)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup(argv[0], &options.username));
    }

    options.warningCallback = PrintWarning;
    options.showTraces = advanced;
    LW_CLEANUP_CTERR(exc, DJGetComputerName(&options.computerName));

    LW_TRY(exc, DJInitModuleStates(&options, &LW_EXC));

    for(i = 0; i < enableModules.size; i++)
    {
        PCSTR module = *(PCSTR *)CTArrayGetItem(
                    &enableModules, i, sizeof(PCSTR));
        if(CTArrayFindString(&disableModules, module) != -1)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Module already specified", "The module '%s' is listed as being disabled and enabled", module);
            goto cleanup;
        }
        LW_TRY(exc, DJEnableModule(&options, module, TRUE, &LW_EXC));
    }

    for(i = 0; i < disableModules.size; i++)
    {
        PCSTR module = *(PCSTR *)CTArrayGetItem(
                    &disableModules, i, sizeof(PCSTR));
        if(CTArrayFindString(&enableModules, module) != -1)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Module already specified", "The module '%s' is listed as being disabled and enabled", module);
            goto cleanup;
        }
        LW_TRY(exc, DJEnableModule(&options, module, FALSE, &LW_EXC));
    }

    for(i = 0; i < detailModules.size; i++)
    {
        PCSTR module = *(PCSTR *)CTArrayGetItem(
                    &detailModules, i, sizeof(PCSTR));
        ModuleState *state = DJGetModuleStateByName(&options, module);
        if(state == NULL)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Unable to find module.", "Please check the spelling of '%s'. This module cannot be found", module);
            goto cleanup;
        }
        PrintModuleState(state);
    }
    if(detailModules.size > 0)
    {
        PrintStateKey();
    }
    for(i = 0; i < detailModules.size; i++)
    {
        PCSTR module = *(PCSTR *)CTArrayGetItem(
                    &detailModules, i, sizeof(PCSTR));
        ModuleState *state = DJGetModuleStateByName(&options, module);
        CT_SAFE_FREE_STRING(moduleDetails);
        CT_SAFE_FREE_STRING(wrapped);
        LW_TRY(exc, moduleDetails = state->module->GetChangeDescription(&options, &LW_EXC));
        LW_CLEANUP_CTERR(exc, CTWordWrap(moduleDetails, &wrapped, 4, columns));
        fprintf(stdout, "\nDetails for '%s':\n%s\n", state->module->longName, wrapped);
    }
    if(detailModules.size > 0)
        goto cleanup;

    LW_TRY(exc, PrintJoinHeader(&options, &LW_EXC));

    if(preview)
    {
        PrintModuleStates(advanced, &options);
        if(!advanced)
            LW_TRY(exc, DJCheckRequiredEnabled(&options, &LW_EXC));
        goto cleanup;
    }

    LW_TRY(exc, DJCheckRequiredEnabled(&options, &LW_EXC));

    if (options.username != NULL && IsNullOrEmptyString(options.password))
    {
        CT_SAFE_FREE_STRING(options.password);

        LW_CLEANUP_CTERR(exc, FillMissingPassword(options.username,
                    &options.password));
    }

    LW_TRY(exc, DJRunJoinProcess(&options, &LW_EXC));
    fprintf(stdout, "SUCCESS\n");

cleanup:
    DJFreeJoinProcessOptions(&options);
    CTArrayFree(&enableModules);
    CTArrayFree(&disableModules);
    CTArrayFree(&detailModules);
    CT_SAFE_FREE_STRING(moduleDetails);
    CT_SAFE_FREE_STRING(wrapped);
}

#ifndef ENABLE_MINIMAL
void DoConfigure(int argc, char **argv, LWException **exc)
{
    EnableType dwEnable = ENABLE_TYPE_AUTO;
    PCSTR testPrefix = NULL;
    PCSTR longDomain = NULL;
    PCSTR shortDomain = NULL;
    JoinProcessOptions options = { 0 };

    while(argc > 0 && CTStrStartsWith(argv[0], "--"))
    {
        if(!strcmp(argv[0], "--autoenable"))
            dwEnable = ENABLE_TYPE_AUTO;
        else if(!strcmp(argv[0], "--enable"))
            dwEnable = ENABLE_TYPE_ENABLE;
        else if(!strcmp(argv[0], "--disable"))
            dwEnable = ENABLE_TYPE_DISABLE;
        else if(argc < 2)
        {
            LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
            goto cleanup;
        }
        else if(!strcmp(argv[0], "--testprefix"))
        {
            testPrefix = argv[1];
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--long"))
        {
            longDomain = argv[1];
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--short"))
        {
            shortDomain = argv[1];
            argv++;
            argc--;
        }
        else
        {
            LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
            goto cleanup;
        }
        argv++;
        argc--;
    }

    if(argc < 1)
    {
        LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
        goto cleanup;
    }

    options.joiningDomain = GetEnableBoolean(dwEnable);
    options.warningCallback = PrintWarning;

    if(!strcmp(argv[0], "pam"))
        LW_TRY(exc, DJNewConfigurePamForADLogin(testPrefix, NULL, PrintWarning, GetEnableBoolean(dwEnable), &LW_EXC));
    else if(!strcmp(argv[0], "nsswitch"))
    {
        LW_TRY(exc, DoNsswitch(&options, &LW_EXC));
    }
    else if(!strcmp(argv[0], "ssh"))
        LW_TRY(exc, DJConfigureSshForADLogin(GetEnableBoolean(dwEnable), NULL, &LW_EXC));
    else if(!strcmp(argv[0], "krb5"))
        LW_CLEANUP_CTERR(exc, DJModifyKrb5Conf(testPrefix,
            GetEnableBoolean(dwEnable), longDomain, shortDomain, NULL, NULL));
    else if(!strcmp(argv[0], "eventfwdd"))
        LW_CLEANUP_CTERR(exc, DJConfigureEventFwd(testPrefix, GetEnableBoolean(dwEnable)));
    else if(!strcmp(argv[0], "reapsysld"))
        LW_CLEANUP_CTERR(exc, DJConfigureReapSyslog(testPrefix, GetEnableBoolean(dwEnable)));
    else
    {
        LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
        goto cleanup;
    }
    fprintf(stdout, "SUCCESS\n");
cleanup:
    ;
}
#endif

void DoGetDistroInfo(int argc, char **argv, LWException **exc)
{
    PSTR str = NULL;
    PCSTR requestType = argv[0];
    PCSTR testPrefix = NULL;
    LwDistroInfo distro = {0};

    argc--;
    argv++;

    if(argc > 0 && !strcmp(argv[0], "--testprefix"))
    {
        testPrefix = argv[1];
        argv += 2;
        argc -= 2;
    }
    if(argc > 0)
    {
        LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, DJGetDistroInfo(testPrefix, &distro));
    if(!strcmp(requestType, "get_os_type"))
    {
        LW_CLEANUP_CTERR(exc, DJGetOSString(distro.os, &str));
    }
    else if(!strcmp(requestType, "get_arch"))
    {
        LW_CLEANUP_CTERR(exc, DJGetArchString(distro.arch, &str));
    }
    else if(!strcmp(requestType, "get_distro"))
    {
        LW_CLEANUP_CTERR(exc, DJGetDistroString(distro.distro, &str));
    }
    else if(!strcmp(requestType, "get_distro_version"))
    {
        LW_CLEANUP_CTERR(exc, CTStrdup(distro.version, &str));
    }
    else
    {
        LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
        goto cleanup;
    }
    fprintf(stdout, "%s\n", str);

cleanup:
    DJFreeDistroInfo(&distro);
    CT_SAFE_FREE_STRING(str);
}

int main(
    int argc,
    char* argv[]
    )
{
    LWException *exc = NULL;
    int columns;
    PSTR pszLogFilePath = NULL;
    BOOLEAN bNoLog = FALSE;
    PSTR logLevel = "warning";
    DWORD dwLogLevel;
    BOOLEAN showHelp = FALSE;
    BOOLEAN showInternalHelp = FALSE;
    int remainingArgs = argc;
    char **argPos = argv;
    int i;
    BOOLEAN directoryExists = FALSE;

    if(CTGetTerminalWidth(fileno(stdout), &columns))
        columns = -1;

    /* Skip the program name */
    argPos++;
    remainingArgs--;

    setlocale(LC_ALL, "");

    while(remainingArgs > 0 && CTStrStartsWith(argPos[0], "--"))
    {
        if(!strcmp(argPos[0], "--help"))
            showHelp = TRUE;
        else if(!strcmp(argPos[0], "--help-internal"))
            showInternalHelp = TRUE;
        else if(!strcmp(argPos[0], "--nolog"))
            bNoLog = TRUE;
        //All options after this point take an argument
        else if(remainingArgs < 2)
            showHelp = TRUE;
        else if(!strcmp(argPos[0], "--logfile"))
        {
            pszLogFilePath = (++argPos)[0];
            remainingArgs--;
        }
        else if(!strcmp(argPos[0], "--loglevel"))
        {
            logLevel = (++argPos)[0];
            remainingArgs--;
        }
        else
            break;
        remainingArgs--;
        argPos++;
    }

    if(remainingArgs < 1)
        showHelp = TRUE;

    if (showInternalHelp) {
        ShowUsageInternal();
        goto cleanup;
    }

    if (showHelp) {
        ShowUsage();
        goto cleanup;
    }

    if (!strcasecmp(logLevel, "error"))
        dwLogLevel = LOG_LEVEL_ERROR;
    else if (!strcasecmp(logLevel, "warning"))
        dwLogLevel = LOG_LEVEL_WARNING;
    else if (!strcasecmp(logLevel, "info"))
        dwLogLevel = LOG_LEVEL_INFO;
    else if (!strcasecmp(logLevel, "verbose"))
        dwLogLevel = LOG_LEVEL_VERBOSE;
    else {
        LW_CLEANUP_CTERR(&exc, LW_ERROR_INVALID_LOG_LEVEL);
    }

    if (pszLogFilePath == NULL)
    {
        // Determine the default log path
        LW_CLEANUP_CTERR(&exc,
                CTCheckDirectoryExists("/var/log", &directoryExists));
        if (directoryExists)
        {
            pszLogFilePath = "/var/log/likewise-join.log";
        }
        else
        {
            pszLogFilePath = "/var/adm/likewise-join.log";
        }
    }

    if (bNoLog) {
        LW_CLEANUP_CTERR(&exc, dj_disable_logging());
    } else if (!strcmp(pszLogFilePath, ".")) {
        LW_CLEANUP_CTERR(&exc, dj_init_logging_to_console(dwLogLevel));
    } else {
        DWORD ceError = dj_init_logging_to_file(dwLogLevel, pszLogFilePath);
        if(ceError == ERROR_ACCESS_DENIED)
        {
            fprintf(stderr, "Warning: insufficient permissions to log to %s. To enable logging, please specify a different filename with --logfile <file>.\n",
                    pszLogFilePath);
            ceError = ERROR_SUCCESS;
            LW_CLEANUP_CTERR(&exc, dj_disable_logging());
        }
        else if (ceError == ERROR_FILE_NOT_FOUND)
        {
            fprintf(stderr, "Warning: parent directory of log file %s does not exist. To enable logging, please specify a different filename with --logfile <file>.\n",
                    pszLogFilePath);
            ceError = ERROR_SUCCESS;
            LW_CLEANUP_CTERR(&exc, dj_disable_logging());
        }
        LW_CLEANUP_CTERR(&exc, ceError);
    }

    if (!strcmp(argPos[0], "join") || !strcmp(argPos[0], "leave"))
    {
        DJ_LOG_INFO("Domainjoin invoked with the %s command (remaining arguments will be printed later):", argPos[0]);
        // Only print up to the 'join' part
        for (i = 0; i <= argPos - argv; i++)
        {
            DJ_LOG_INFO("    [%s]", argv[i]);
        }
    }
    else
    {
        DJ_LOG_INFO("Domainjoin invoked with %d arg(s):", argc);
        for (i = 0; i < argc; i++)
        {
            DJ_LOG_INFO("    [%s]", argv[i]);
        }
    }

    if(!strcmp(argPos[0], "setname"))
    {
        argPos++;
        if(--remainingArgs != 1)
        {
            ShowUsage();
            goto cleanup;
        }
        //Needed so that DJGetMachineSid does not call winbind
        LW_TRY(&exc, DJSetComputerName(argPos[0], NULL, &LW_EXC));
    }
    else if(!strcmp(argPos[0], "join"))
    {
        argPos++;
        remainingArgs--;
        LW_TRY(&exc, DoJoin(remainingArgs, argPos, columns, &LW_EXC));
    }
    else if(!strcmp(argPos[0], "leave"))
    {
        argPos++;
        remainingArgs--;
        LW_TRY(&exc, DoLeaveNew(remainingArgs, argPos, columns, &LW_EXC));
    }
    else if(!strcmp(argPos[0], "query"))
    {
        LW_TRY(&exc, DoQuery(&LW_EXC));
    }
    else if(!strcmp(argPos[0], "fixfqdn"))
        LW_TRY(&exc, DoFixFqdn(&LW_EXC));
#ifndef ENABLE_MINIMAL
    else if(!strcmp(argPos[0], "configure"))
    {
        argPos++;
        remainingArgs--;
        LW_TRY(&exc, DoConfigure(remainingArgs, argPos, &LW_EXC));
    }
#endif
    else if(!strcmp(argPos[0], "get_os_type") ||
        !strcmp(argPos[0], "get_arch") ||
        !strcmp(argPos[0], "get_distro") ||
        !strcmp(argPos[0], "get_distro_version"))
    {
        LW_TRY(&exc, DoGetDistroInfo(remainingArgs, argPos, &LW_EXC));
    }
    else
    {
        LW_RAISE(&exc, LW_ERROR_SHOW_USAGE);
        goto cleanup;
    }

cleanup:

    if (!LW_IS_OK(exc) && exc->code == LW_ERROR_SHOW_USAGE)
    {
        ShowUsage();
        LWHandle(&exc);
    }
    else if (!LW_IS_OK(exc))
    {
        //Ignoring the return value from this because we can't do anything
        //if there is an error
        fprintf(stdout, "\n");
        LWPrintException(stdout, exc, FALSE);
        DJLogException(LOG_LEVEL_ERROR, exc);
        LWHandle(&exc);
        dj_close_log();
        return 1;
    }

    dj_close_log();

    return 0;
}
