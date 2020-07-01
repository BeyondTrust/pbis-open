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

#include "domainjoin.h"
#include "djdistroinfo.h"
#include "djsshconf.h"
#include "djpamconf.h"
#include "djcli.h"
#include "djauditing.h"
#include "djpbps.h"
#include "ctprocutils.h"
#include "lwexc.h"
#include "linenoise.h"
#include <lw/rtllog.h>
#include <lwstr.h>

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

const char *LN_COMPUTER_NAME_HISTORY_FILE = ".pbis-djname-hist";
const char *LN_JOINLEAVE_HISTORY_FILE = ".pbis-djjoin-hist";
const int LN_MAX_HISTORY_LEN = 64;

static
void
ShowUsage(const BOOLEAN isEnterprise)
{
    fprintf(stdout, "usage: domainjoin-cli [options] command [args...]\n\n");
    fprintf(stdout, "  where options are:\n\n");
    fprintf(stdout, "    --help                                     Display this help information.\n");
    fprintf(stdout, "    --help-internal                            Display help for debug commands.\n");
    fprintf(stdout, "    --logfile {.|path}                         Log to a file (or \".\" to log\n"
                    "                                               to console).\n");
    fprintf(stdout, "    --loglevel {error|warning|info|verbose}    Adjusts how much logging is\n"
                    "                                               produced by domainjoin.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  and commands are:\n\n");
    fprintf(stdout, "    query\n");
    fprintf(stdout, "    setname <computer name>\n");
    fprintf(stdout, (isEnterprise)
            ? "    join [join options] [--ou <organizationalUnit>] <domain name> <user name> [<password>]\n"
              "    join [join options] [--ou <organizationalUnit>] --configFile <configuration file> <domain name>\n"
            : "    join [join options] [--ou <organizationalUnit>] <domain name> <user name> [<password>]\n");
    fprintf(stdout, "    join [--advanced] --preview [--ou <organizationalUnit>] <domain name>\n");
    fprintf(stdout, "    join [--ou <organizationalUnit>] --details <module> <domain name>\n");
    fprintf(stdout, "    join options: [--notimesync] [--nohosts] [--nogssapi] [--ignore-pam] [--enable <module> --disable <module> ...] [--no-pre-account]\n");
    fprintf(stdout, (isEnterprise)
            ?       "                  [--assumeDefaultCell {auto|no|force}] [--assumeDefaultDomain {yes|no}] [--userDomainPrefix <short domain name>]\n"
            :       "                  [--assumeDefaultDomain {yes|no}] [--userDomainPrefix <short domain name>]\n");
    fprintf(stdout, (isEnterprise)
            ?       "                  [--uac-flags <flags>] [--trustEnumerationWaitSeconds <seconds>] [--unprovisioned {auto|no|force}]\n\n"
            :       "                  [--uac-flags <flags>] [--trustEnumerationWaitSeconds <seconds>]\n\n");
    fprintf(stdout, (isEnterprise)
            ? "    leave [--enable <module> --disable <module> ...] [--keepLicense] [user name] [password]\n"
              "    leave [--enable <module> --disable <module> ...] [--deleteAccount <user name> [<password>]]\n"
              "    leave [--enable <module> --disable <module> ...] [--deleteAccount --configFile <configuration file>]\n"
            : "    leave [--enable <module> --disable <module> ...] [--deleteAccount <user name> [<password>]]\n");
    fprintf(stdout, "    leave [--advanced] --preview [user name] [password]\n");
    fprintf(stdout, "    leave --details <module>\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  Examples:\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  This example shows three different non-interactive domain joins. Each uses the supplied domain\n");
    fprintf(stdout, "  and AD user name. The second and third examples show joining to an OU within the domain, specified\n");
    fprintf(stdout, "  in AD canonical name (without the domain prefix), and Distinguised Name (DN) formats.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "    domainjoin-cli join MYDOMAIN.COM MyJoinAccount\n");
    fprintf(stdout, "    domainjoin-cli join --ou Eng/Dev MYDOMAIN.COM MyJoinAccount\n");
    fprintf(stdout, "    domainjoin-cli join --ou OU=Dev,OU=Eng,DC=MYDOMAIN,DC=COM MYDOMAIN.COM MyJoinAccount\n\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  This example shows how domainjoin-cli will prompt for the domain, user name and OU parameters if\n");
    fprintf(stdout, "  they are not supplied. In the first example, domainjoin-cli will prompt for domain and user name.\n");
    fprintf(stdout, "  In the second, domainjoin-cli will prompt for the domain, user name and OU. In the final example,\n");
    fprintf(stdout, "  domainjoin-cli will prompt for the OU and user name. Here -- is used to indicate the OU parameter\n");
    fprintf(stdout, "  is not supplied, otherwise domainjoin-cli would parse MYDOMAIN.COM as the OU.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "    domainjoin-cli join\n");
    fprintf(stdout, "    domainjoin-cli join --ou\n");
    fprintf(stdout, "    domainjoin-cli join --ou -- MYDOMAIN.COM \n");
    fprintf(stdout, "\n");
}

static
void
ShowUsageInternal(const BOOLEAN isEnterprise)
{
    ShowUsage(isEnterprise);

    fprintf(stdout, "  Internal debug commands:\n");
    fprintf(stdout, "    fixfqdn\n");
    fprintf(stdout, "    configure { --enable | --disable } [--testprefix <dir>] pam\n");
    fprintf(stdout, "    configure { --enable | --disable } [--testprefix <dir>] nsswitch\n");
    fprintf(stdout, "    configure { --enable | --disable } [--testprefix <dir>] ssh\n");
    fprintf(stdout, "    configure { --enable | --disable } [--testprefix <dir>] [--long <longdomain>] [--short <shortdomain>] krb5\n");
    fprintf(stdout, "    configure { --enable | --disable } eventfwdd\n");
    fprintf(stdout, "    configure { --enable | --disable } reapsysld\n");
    fprintf(stdout, "    get_os_type\n");
    fprintf(stdout, "    get_arch\n");
    fprintf(stdout, "    get_distro\n");
    fprintf(stdout, "    get_distro_version\n");
    fprintf(stdout, "\n");
}

/* Strip leading/trailing whitespace from user entry */
static
char *
linenoise_strip_whitespace(
    const char *prompt
    )
{
    char *input = NULL;
    input = linenoise(prompt);
    if (input)
    {
        LwStripWhitespace(input, TRUE, TRUE);
    }

    return input;
}

/* return the home directory or NULL if there is
 * any error,
 *
 * the returned string should be freed */
char *homeDirectory()
{
    char *home = NULL;
    char *ret = NULL;
    struct passwd *pw = NULL;

    if (!(home = getenv("HOME")))
    {
        pw = getpwuid(geteuid());
        if (pw)
        {
            home = pw->pw_dir;
        }
    }

    LwStrDupOrNull(home, &ret);
    return ret;
}

static
char *
linenoise_history_file(const char *filename)
{
    PSTR historyFile = NULL;
    DWORD dwError = ERROR_SUCCESS;

    char *homeDir = homeDirectory();
    if (homeDir)
    {
        dwError = LwAllocateStringPrintf(&historyFile, "%s/%s", homeDir, filename);
    }
    else
    {
        dwError = LwStrDupOrNull(filename, &historyFile);
    }
    BAIL_ON_CENTERIS_ERROR(dwError);

cleanup:
    CT_SAFE_FREE_STRING(homeDir);
    return historyFile;


error:
    CT_SAFE_FREE_STRING(historyFile);
    goto cleanup;
}

/* Add to linenoise history if not null or empty */
static
void
linenoise_history_add(const char *line)
{
    if (line && *line)
    {
        linenoiseHistoryAdd(line);
    }
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
        DJ_LOG_INFO("Using user entered password");
    }
    else
    {
        DJ_LOG_WARNING("Retrieved password from environmental variable");
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
    PSTR wrapped = NULL;
    int columns;

    if(CTGetTerminalWidth(fileno(stdout), &columns))
        columns = -1;

    // This function doesn't return a DWORD, so we have to recover as much
    // as possible.
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
        fprintf(stdout, "[%c] ", state->disposition == EnableModule ? 'X' : ' ');
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
    BOOLEAN isEnterprise;

    isEnterprise = DJGetIsEnterprise();

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
        if (!options->releaseLicense && isEnterprise) {
            fprintf(stdout, "Leaving domain without releasing license\n");
        }
        if (options->deleteAccount) {
            fprintf(stdout, "Attempting to delete account\n");
        }
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
            if(state->disposition == EnableModule)
            {
                fprintf(stdout, "%-15s- %s\n", state->module->shortName,
                        state->module->longName);
            }
        }
    }
}

static
char * ouHintsCallback(const char *buffer, int *color, int *bold)
{
    *color = 34; /* blue */
    *bold = 1;
    return " [e.g. Eng/pbis/dev | OU=Dev,OU=Eng,DC=MYDOMAIN,DC=COM]";
}

static
BOOLEAN isOptionValueEnabled(const char *value) {
    return (!strcasecmp(value, "auto")
            || !strcasecmp(value, "automatic")
            || !strcasecmp(value, "dynamic")
            || !strcasecmp(value, "on"));
}

static
BOOLEAN isOptionValueDisabled(const char *value) {
    return (!strcasecmp(value, "no")
            || !strcasecmp(value, "false")
            || !strcasecmp(value, "disable")
            || !strcasecmp(value, "disabled")
            || !strcasecmp(value, "off"));
}

static
BOOLEAN isOptionValueOn(const char *value) {
    return (!strcasecmp(value, "yes")
            || !strcasecmp(value, "on")
            || !strcasecmp(value, "true"));
}

static
BOOLEAN isOptionValueOff(const char *value) {
    return isOptionValueDisabled(value);
}

static
BOOLEAN isOptionValueForced(const char *value) {
    return (!strcasecmp(value, "force"));
}

static void DJValidateJoinOptions(JoinProcessOptions *pOptions, LWException **exc)
{
    if (pOptions->isEnterprise
            && pOptions->assumeDefaultCellMode > False
            && pOptions->unprovisionedMode > False) {
        LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "You cannot enable both --assumeDefaultCell and --unprovisioned modes; please specify only one.", "");
    }
}

void DoJoin(int argc, char **argv, int columns, BOOLEAN isEnterprise, LWException **exc)
{
    JoinProcessOptions options;
    BOOLEAN advanced = FALSE;
    BOOLEAN preview = FALSE;
    BOOLEAN mustSupplyOU = FALSE;
    BOOLEAN mustSupplyUsername = TRUE;
    BOOLEAN mustSupplyConfigFile = FALSE;
    BOOLEAN haveConfigFile = FALSE;
    DynamicArray enableModules, disableModules, ignoreModules;
    DynamicArray detailModules;
    size_t i;
    PSTR moduleDetails = NULL;
    PSTR wrapped = NULL;
    PSTR joinLeaveHistoryFile = NULL;
    PPbpsApiHandle_t pPbpsApiHandle = NULL;

    DJZeroJoinProcessOptions(&options);
    options.isEnterprise = isEnterprise;

    memset(&enableModules, 0, sizeof(enableModules));
    memset(&disableModules, 0, sizeof(disableModules));
    memset(&ignoreModules, 0, sizeof(ignoreModules));
    memset(&detailModules, 0, sizeof(detailModules));

    /* initialize linenoise history; if we couldn't allocate a history
     * file name, we just won't be able to load/persist history between
     * runs */
    linenoiseHistorySetMaxLen(LN_MAX_HISTORY_LEN);
    joinLeaveHistoryFile = linenoise_history_file(LN_JOINLEAVE_HISTORY_FILE);
    if (joinLeaveHistoryFile)
    {
        linenoiseHistoryLoad(joinLeaveHistoryFile);
    }

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
        else if(!strcmp(argv[0], "--no-pre-account"))
            options.noPreAccount = TRUE;
        else if(!strcmp(argv[0], "--notimesync"))
            options.disableTimeSync = TRUE;
        else if(!strcmp(argv[0], "--multiple"))
            options.enableMultipleJoins = TRUE;
        else if(!strcmp(argv[0], "--nogssapi"))
            options.disableGSSAPI = TRUE;
        else if(!strcmp(argv[0], "--nohosts"))
        {
            PCSTR module = "hostname";
            LW_CLEANUP_CTERR(exc, CTArrayAppend(&disableModules, sizeof(PCSTR), &module, 1));
        }
        else if(!strcmp(argv[0], "--ou"))
        {
            /* set the prompt for the ou flag if necessary, either no more options or the
             * next argument is another option; we will prompt for it later to
             * allow option processing validation to finish */
            if (argc > 1 && strncmp("--", argv[1], 2))
            {
                DJ_LOG_INFO("Domainjoin invoked with option --ou %s", argv[1]);
                CT_SAFE_FREE_STRING(options.ouName);
                LW_CLEANUP_CTERR(exc, CTStrdup(argv[1], &options.ouName));
                argv++;
                argc--;
            }
            else
            {
                mustSupplyOU = TRUE;
            }
        }
        else if(!strcmp(argv[0], "--"))
        {
            /* marks the "end" of the options */
        }
        // remaining options take two parameters
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
            if(!strcmp(argv[1], "ssh")){
                options.ignoreSsh = TRUE;
            }
            else {
                options.ignoreSsh = FALSE;
                LW_CLEANUP_CTERR(exc, CTArrayAppend(&disableModules, sizeof(PCSTR), &argv[1], 1));
            }
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--ignore"))
        {
            LW_CLEANUP_CTERR(exc, CTArrayAppend(&ignoreModules, sizeof(PCSTR), &argv[1], 1));
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--details"))
        {
            LW_CLEANUP_CTERR(exc, CTArrayAppend(&detailModules, sizeof(PCSTR), &argv[1], 1));
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--uac-flags"))
        {
            DJ_LOG_INFO("Domainjoin invoked with option --uac-flags %s", argv[1]);
            CT_SAFE_FREE_STRING(options.ouName);
            options.uacFlags = strtoul(argv[1], NULL, 0);
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--assumeDefaultDomain"))
        {
            DJ_LOG_INFO("Domainjoin invoked with option --assumeDefaultDomain");
            options.setAssumeDefaultDomain = TRUE;

            if (isOptionValueOn(argv[1]))
            {
                options.assumeDefaultDomain = TRUE;
            }
            else if (isOptionValueOff(argv[1]))
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
        else if(!strcmp(argv[0], "--trustEnumerationWaitSeconds"))
        {
            DJ_LOG_INFO("Domainjoin invoked with option --trustEnumerationWaitSeconds %s", argv[1]);
            options.dwTrustEnumerationWaitSeconds = strtoul(argv[1], NULL, 0);
            // Verify the supported range. Zero disables the functionality.
            // Range is the same as the GPO.
            if (options.dwTrustEnumerationWaitSeconds > 1000)
            {
                LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
                goto cleanup;
            }
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--assumeDefaultCell") && isEnterprise)
        {
            DJ_LOG_INFO("Domainjoin invoked with option %s %s", argv[0], argv[1]);

            if (isOptionValueEnabled(argv[1]))
            {
                options.assumeDefaultCellMode = True;
            }
            else if (isOptionValueDisabled(argv[1]))
            {
                options.assumeDefaultCellMode = False;
            }
            else if (isOptionValueForced(argv[1]))
            {
                options.assumeDefaultCellMode = Force;
            }
            else
            {
                LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
                goto cleanup;
            }
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--unprovisioned") && isEnterprise)
        {
            DJ_LOG_INFO("Domainjoin invoked with option %s %s", argv[0], argv[1]);

            if (isOptionValueEnabled(argv[1]))
            {
                options.unprovisionedMode = True;
            }
            else if (isOptionValueDisabled(argv[1]))
            {
                options.unprovisionedMode = False;
            }
            else if (isOptionValueForced(argv[1]))
            {
                options.unprovisionedMode = Force;
            }
            else
            {
                LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
                goto cleanup;
            }
            argv++;
            argc--;
        }
        else if ((!strcmp(argv[0], "--configFile")) && isEnterprise)
        {
            if (argc > 1 && strncmp("--", argv[1], 2))
            {
               DJ_LOG_INFO("Domainjoin invoked with option --configFile %s", argv[1]);
               CT_SAFE_FREE_STRING(options.pszConfigFile);
               LW_CLEANUP_CTERR(exc, CTStrdup(argv[1], &options.pszConfigFile));
               haveConfigFile = TRUE;
               argv++;
               argc--;
            }
            else
            {
               mustSupplyConfigFile = TRUE;
            }
        }
        else
        {
            LW_RAISE(exc, LW_ERROR_SHOW_USAGE);
            goto cleanup;
        }
        argv++;
        argc--;
    }

    DJ_LOG_INFO("Validating join options.");
    LW_TRY(exc, DJValidateJoinOptions(&options, &LW_EXC));

    DJ_LOG_INFO("Domainjoin invoked with %d arg(s) to the join command:", argc);

    /*  prompt for the OU if required */
    if (mustSupplyOU)
    {
        linenoiseSetHintsCallback(ouHintsCallback);
        options.ouName = linenoise_strip_whitespace("Computer object location (OU): ");
        linenoise_history_add(options.ouName);
        linenoiseSetHintsCallback(NULL);

        if (LW_IS_NULL_OR_EMPTY_STR(options.ouName))
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Invalid OU", "");
            goto cleanup;
        }
    }

    if (argc > 0)
    {
        DJ_LOG_INFO("    [%s]", argv[0]);
        LW_CLEANUP_CTERR(exc, CTStrdup(argv[0], &options.domainName));
        argv++;
        argc--;
    }
    else
    {
        options.domainName = linenoise_strip_whitespace("AD Domain: ");
        linenoise_history_add(options.domainName);
        DJ_LOG_INFO("    Domain: [%s]", options.domainName);
    }

    if (mustSupplyConfigFile)
    {
        options.pszConfigFile = linenoise_strip_whitespace("configFile: ");
        linenoise_history_add(options.pszConfigFile);
        DJ_LOG_INFO("    configFile: [%s]", options.pszConfigFile);
        haveConfigFile = TRUE;
    }

    if (!haveConfigFile)
    {
       mustSupplyUsername = (!(preview || detailModules.size != 0));
       if (argc > 0)
       {
           DJ_LOG_INFO("    [%s]", argv[0]);
           LW_CLEANUP_CTERR(exc, CTStrdup(argv[0], &options.username));
           argv++;
           argc--;
       }
       else if (mustSupplyUsername)
       {
           options.username = linenoise_strip_whitespace("Username: ");
           linenoise_history_add(options.username);
           DJ_LOG_INFO("    Username: [%s]", options.username);
       }

       /*  user will be prompted later if password is needed and not supplied */
       if (argc > 0)
       {
           DJ_LOG_INFO("    [<password>]");
           LW_CLEANUP_CTERR(exc, CTStrdup(argv[0], &options.password));
           argv++;
           argc--;
       }
    }

    if (isEnterprise && haveConfigFile && options.pszConfigFile)
    {
        CT_SAFE_FREE_STRING(options.username);
        CT_SAFE_FREE_STRING(options.password);
        LW_CLEANUP_CTERR(exc, PbpsApiCredentialGet(
                                 options.pszConfigFile,
                                 &options.username,
                                 &options.password,
                                 &pPbpsApiHandle));
    }

    options.joiningDomain = TRUE;
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
        if(CTArrayFindString(&ignoreModules, module) != -1)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Module already specified", "The module '%s' is listed as being ignored and enabled", module);
            goto cleanup;
        }
        LW_TRY(exc, DJSetModuleDisposition(&options, module, EnableModule, &LW_EXC));
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
        if(CTArrayFindString(&ignoreModules, module) != -1)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Module already specified", "The module '%s' is listed as being ignored and enabled", module);
            goto cleanup;
        }
        LW_TRY(exc, DJSetModuleDisposition(&options, module, DisableModule, &LW_EXC));
    }

    for(i = 0; i < ignoreModules.size; i++)
    {
        PCSTR module = *(PCSTR *)CTArrayGetItem(
                    &ignoreModules, i, sizeof(PCSTR));
        if(CTArrayFindString(&enableModules, module) != -1)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Module already specified", "The module '%s' is listed as being enabled and ignored", module);
            goto cleanup;
        }
        if(CTArrayFindString(&disableModules, module) != -1)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Module already specified", "The module '%s' is listed as being disabled and ignored", module);
            goto cleanup;
        }
        LW_TRY(exc, DJSetModuleDisposition(&options, module, IgnoreModule, &LW_EXC));
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
    DJ_LOG_INFO("Join SUCCESS");

cleanup:
    if (joinLeaveHistoryFile)
    {
        linenoiseHistorySave(joinLeaveHistoryFile);
    }

    PbpsApiCredentialRelease(&pPbpsApiHandle);

    DJFreeJoinProcessOptions(&options);
    CTArrayFree(&enableModules);
    CTArrayFree(&disableModules);
    CTArrayFree(&ignoreModules);
    CTArrayFree(&detailModules);
    CT_SAFE_FREE_STRING(moduleDetails);
    CT_SAFE_FREE_STRING(wrapped);
    CT_SAFE_FREE_STRING(joinLeaveHistoryFile);
}

void DoLeaveNew(int argc, char **argv, int columns, BOOLEAN isEnterprise, LWException **exc)
{
    JoinProcessOptions options;
    BOOLEAN advanced = FALSE;
    BOOLEAN preview = FALSE;
    BOOLEAN mustSupplyUsername = FALSE;
    BOOLEAN haveConfigFile = FALSE;
    BOOLEAN mustSupplyConfigFile = FALSE;
    DynamicArray enableModules, disableModules, ignoreModules;
    DynamicArray detailModules;
    ssize_t i;
    PSTR moduleDetails = NULL;
    PSTR wrapped = NULL;
    int passwordIndex = -1;
    PSTR joinLeaveHistoryFile = NULL;
    PPbpsApiHandle_t pPbpsApiHandle = NULL;

    DJZeroJoinProcessOptions(&options);
    options.isEnterprise = isEnterprise;

    memset(&enableModules, 0, sizeof(enableModules));
    memset(&disableModules, 0, sizeof(disableModules));
    memset(&ignoreModules, 0, sizeof(ignoreModules));
    memset(&detailModules, 0, sizeof(detailModules));

    // Enterprise default is to release the license
    options.releaseLicense = isEnterprise;

    /* initialize linenoise history; if we couldn't allocate a history
     * file name, we just won't be able to load/persist history between
     * runs */
    linenoiseHistorySetMaxLen(LN_MAX_HISTORY_LEN);
    joinLeaveHistoryFile = linenoise_history_file(LN_JOINLEAVE_HISTORY_FILE);
    if (joinLeaveHistoryFile)
    {
        linenoiseHistoryLoad(joinLeaveHistoryFile);
    }

    while(argc > 0 && CTStrStartsWith(argv[0], "--"))
    {
        if(!strcmp(argv[0], "--advanced"))
            advanced = TRUE;
        else if(!strcmp(argv[0], "--preview"))
            preview = TRUE;
        else if (!strcmp(argv[0], "--keepLicense"))
            options.releaseLicense = FALSE;
        else if (!strcmp(argv[0], "--deleteAccount"))
        {
            options.deleteAccount = TRUE;

            if (haveConfigFile)
               mustSupplyUsername = FALSE;
            else
               mustSupplyUsername = TRUE;
        }
        else if ((!strcmp(argv[0], "--configFile")) && isEnterprise)
        {
           if (options.deleteAccount)
              mustSupplyUsername = FALSE;

            if (argc > 1 && strncmp("--", argv[1], 2))
            {
               DJ_LOG_INFO("Domain leave invoked with option --configFile %s", argv[1]);
               CT_SAFE_FREE_STRING(options.pszConfigFile);
               LW_CLEANUP_CTERR(exc, CTStrdup(argv[1], &options.pszConfigFile));
               haveConfigFile = TRUE;

               argv++;
               argc--;
            }
            else
               mustSupplyConfigFile = TRUE;
        }
        // remaining options require at least one parameter
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
            if(!strcmp(argv[1], "ssh")){
                options.ignoreSsh = TRUE;
            }
            else {
                options.ignoreSsh = FALSE;
                LW_CLEANUP_CTERR(exc, CTArrayAppend(&disableModules, sizeof(PCSTR *), &argv[1], 1));
            }
            argv++;
            argc--;
        }
        else if(!strcmp(argv[0], "--ignore"))
        {
            LW_CLEANUP_CTERR(exc, CTArrayAppend(&ignoreModules, sizeof(PCSTR *), &argv[1], 1));
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

    if ((mustSupplyConfigFile) && (!haveConfigFile))
    {
      fprintf(stdout, "Missing config file\n");
      goto cleanup;
    }


    if (!haveConfigFile)
    {
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

       DJ_LOG_INFO("Domainjoin invoked with %d arg(s) to the leave command:", argc);
       for(i = 0; i < argc; i++)
       {
           DJ_LOG_INFO("    [%s]", i == passwordIndex ? "<password>" : argv[i]);
       }

       /* n.b. must set username before initializing module states */
       if (argc > 0)
       {
           LW_CLEANUP_CTERR(exc, CTStrdup(argv[0], &options.username));
       }
       else if (mustSupplyUsername)
       {
           options.username = linenoise_strip_whitespace("Username: ");
           DJ_LOG_INFO("    Username: [%s]", options.username);
       }
    }

    if (isEnterprise && haveConfigFile && options.pszConfigFile)
    {
        CT_SAFE_FREE_STRING(options.username);
        CT_SAFE_FREE_STRING(options.password);
        LW_CLEANUP_CTERR(exc, PbpsApiCredentialGet(
                                 options.pszConfigFile,
                                 &options.username,
                                 &options.password,
                                 &pPbpsApiHandle));
    }


    options.joiningDomain = FALSE;
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
        if(CTArrayFindString(&ignoreModules, module) != -1)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Module already specified", "The module '%s' is listed as being ignored and enabled", module);
            goto cleanup;
        }
        LW_TRY(exc, DJSetModuleDisposition(&options, module, EnableModule, &LW_EXC));
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
        if(CTArrayFindString(&ignoreModules, module) != -1)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Module already specified", "The module '%s' is listed as being disabled and ignored", module);
            goto cleanup;
        }
        LW_TRY(exc, DJSetModuleDisposition(&options, module, DisableModule, &LW_EXC));
    }

    for(i = 0; i < ignoreModules.size; i++)
    {
        PCSTR module = *(PCSTR *)CTArrayGetItem(
                    &ignoreModules, i, sizeof(PCSTR));
        if(CTArrayFindString(&enableModules, module) != -1)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Module already specified", "The module '%s' is listed as being ignored and enabled", module);
            goto cleanup;
        }
        if(CTArrayFindString(&disableModules, module) != -1)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_PARAMETER, "Module already specified", "The module '%s' is listed as being ignored and disabled", module);
            goto cleanup;
        }
        LW_TRY(exc, DJSetModuleDisposition(&options, module, IgnoreModule, &LW_EXC));
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
    DJ_LOG_INFO("Leave SUCCESS");

cleanup:
    if (joinLeaveHistoryFile)
    {
        linenoiseHistorySave(joinLeaveHistoryFile);
    }

    PbpsApiCredentialRelease(&pPbpsApiHandle);

    DJFreeJoinProcessOptions(&options);
    CTArrayFree(&enableModules);
    CTArrayFree(&disableModules);
    CTArrayFree(&ignoreModules);
    CTArrayFree(&detailModules);
    CT_SAFE_FREE_STRING(moduleDetails);
    CT_SAFE_FREE_STRING(wrapped);
    CT_SAFE_FREE_STRING(joinLeaveHistoryFile);
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
    DJ_LOG_INFO("Configure SUCCESS");
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

static
LW_VOID
RtlLogCallback(
    LW_IN LW_OPTIONAL LW_PVOID Context,
    LW_IN LW_RTL_LOG_LEVEL Level,
    LW_IN LW_OPTIONAL LW_PCSTR ComponentName,
    LW_IN LW_PCSTR FunctionName,
    LW_IN LW_PCSTR FileName,
    LW_IN LW_ULONG LineNumber,
    LW_IN LW_PCSTR Format,
    LW_IN ...
    )
{
    va_list args;
    PSTR pCombined = NULL;
    DWORD djLevel = 0;

    va_start(args, Format);

    switch(Level)
    {
        case LW_RTL_LOG_LEVEL_ALWAYS:
            djLevel = LOG_LEVEL_ALWAYS;
            break;
        case LW_RTL_LOG_LEVEL_ERROR:
            djLevel = LOG_LEVEL_ERROR;
            break;
        case LW_RTL_LOG_LEVEL_WARNING:
            djLevel = LOG_LEVEL_WARNING;
            break;
        case LW_RTL_LOG_LEVEL_INFO:
            djLevel = LOG_LEVEL_INFO;
            break;
        case LW_RTL_LOG_LEVEL_VERBOSE:
        case LW_RTL_LOG_LEVEL_DEBUG:
        case LW_RTL_LOG_LEVEL_TRACE:
        case LW_RTL_LOG_LEVEL_UNDEFINED:
        default:
            djLevel = LOG_LEVEL_VERBOSE;
            break;
    }

    if (!LwAllocateStringPrintfV(
                &pCombined,
                Format,
                args))
    {
        dj_log_message(
            djLevel,
            "%s:%s():%s:%d: %s",
            ComponentName,
            FunctionName,
            FileName,
            LineNumber,
            pCombined);
    }
    LW_SAFE_FREE_STRING(pCombined);

    va_end(args);
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
    BOOLEAN isEnterprise = FALSE;

    isEnterprise = DJGetIsEnterprise();

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
        ShowUsageInternal(isEnterprise);
        goto cleanup;
    }

    if (showHelp) {
        ShowUsage(isEnterprise);
        goto cleanup;
    }

    if (!strcasecmp(logLevel, "error"))
    {
        dwLogLevel = LOG_LEVEL_ERROR;
        LwRtlLogSetLevel(LW_RTL_LOG_LEVEL_ERROR);
    }
    else if (!strcasecmp(logLevel, "warning"))
    {
        dwLogLevel = LOG_LEVEL_WARNING;
        LwRtlLogSetLevel(LW_RTL_LOG_LEVEL_WARNING);
    }
    else if (!strcasecmp(logLevel, "info"))
    {
        dwLogLevel = LOG_LEVEL_INFO;
        LwRtlLogSetLevel(LW_RTL_LOG_LEVEL_INFO);
    }
    else if (!strcasecmp(logLevel, "verbose"))
    {
        dwLogLevel = LOG_LEVEL_VERBOSE;
        LwRtlLogSetLevel(LW_RTL_LOG_LEVEL_VERBOSE);
    }
    else if (!strcasecmp(logLevel, "debug"))
    {
        dwLogLevel = LOG_LEVEL_VERBOSE;
        LwRtlLogSetLevel(LW_RTL_LOG_LEVEL_DEBUG);
    }
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
            pszLogFilePath = "/var/log/domainjoin-cli.log";
        }
        else
        {
            pszLogFilePath = "/var/adm/domainjoin-cli.log";
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
    LwRtlLogSetCallback(RtlLogCallback, NULL);

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
        PSTR pComputerName = NULL;
        PSTR pDomainSuffix = NULL;

        argPos++;
        remainingArgs--;

        if (remainingArgs > 1)
        {
            ShowUsage(isEnterprise);
            goto cleanup;
        }

        if (remainingArgs == 1)
        {
            pComputerName = argPos[0];
        }
        else
        {
            PSTR setNameHistoryFile = NULL;
            setNameHistoryFile = linenoise_history_file(LN_COMPUTER_NAME_HISTORY_FILE);

            linenoiseHistorySetMaxLen(LN_MAX_HISTORY_LEN);
            if (setNameHistoryFile)
            {
                linenoiseHistoryLoad(setNameHistoryFile);
            }

            /* n.b. ignoring this leak */
            pComputerName = linenoise_strip_whitespace("Computer name: ");
            if (!pComputerName)
            {
                pComputerName = "";
            }

            linenoise_history_add(pComputerName);

            if (setNameHistoryFile)
            {
                linenoiseHistorySave(setNameHistoryFile);
            }

            CT_SAFE_FREE_STRING(setNameHistoryFile);
        }

        pDomainSuffix = strchr(pComputerName, '.');
        if (pDomainSuffix)
        {
            *pDomainSuffix = 0;
            pDomainSuffix++;
        }
        else
        {
            pDomainSuffix = "";
        }
        LW_TRY(&exc, DJSetComputerName(pComputerName, pDomainSuffix, &LW_EXC));
    }
    else if(!strcmp(argPos[0], "join"))
    {
        argPos++;
        remainingArgs--;
        LW_TRY(&exc, DoJoin(remainingArgs, argPos, columns, isEnterprise, &LW_EXC));
    }
    else if(!strcmp(argPos[0], "leave"))
    {
        argPos++;
        remainingArgs--;
        LW_TRY(&exc, DoLeaveNew(remainingArgs, argPos, columns, isEnterprise, &LW_EXC));
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

    if (!LW_IS_OK(exc))
    {
        if (exc->code == LW_ERROR_SHOW_USAGE)
        {
            ShowUsage(isEnterprise);
        }
        else
        {
            //Ignoring the return value from this because we can't do anything
            //if there is an error
            fprintf(stdout, "\n");
            LWPrintException(stdout, exc, FALSE);
            DJLogException(LOG_LEVEL_ERROR, exc);
        }

        LWHandle(&exc);
        dj_close_log();
        return 1;
    }

    dj_close_log();

    return 0;
}
