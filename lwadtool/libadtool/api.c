/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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

/*
 * Module Name:
 *
 *        app.c
 *
 * Abstract:
 *
 *        Application data and adtool syntax definitions.
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 23, 2010
 *
 */

#include "includes.h"

/**
 * Error message buffer.
 */
CHAR adtLastErrMsg[ADT_ERROR_MSG_SIZE_MAX + 1];

/**
 * Reset action data.
 *
 * @param context Context reference returned by AdtOpen().
 * @param argc Number of arguments.
 * @param argv Array of sting arguments.
 * @return 0 on success; error code on failure.
 */
static DWORD ResetAction(IN AppContextTP appContext, IN INT argc, IN PCSTR *argv) {
    DWORD dwError = 0;

    if(appContext->fullCon) {
        poptFreeContext(appContext->fullCon);
    }

    if(appContext->optionsCon) {
        poptFreeContext(appContext->optionsCon);
    }

    appContext->optionsCon = poptGetContext("options", argc, argv, appContext->optionsTable, 0);
    ADT_BAIL_ON_NULL(appContext->optionsCon);
    poptSetOtherOptionHelp(appContext->optionsCon, ADT_APP_USAGE);

    appContext->fullCon = poptGetContext("full", argc, argv, appContext->fullTable, 0);
    ADT_BAIL_ON_NULL(appContext->fullCon);
    /* poptSetOtherOptionHelp(appContext->fullCon, ADT_APP_USAGE); */
    appContext->actionExecState = ActionNotReady;
    appContext->execResult.base.returnCode = 0;

    LW_SAFE_FREE_MEMORY(appContext->stdoutStr);
    LW_SAFE_FREE_MEMORY(appContext->stderrStr);
    LW_SAFE_FREE_MEMORY(appContext->execResult.base.resultStr);

    /* Free misc stuff */
    LW_SAFE_FREE_MEMORY(appContext->oID);
    LW_SAFE_FREE_MEMORY(appContext->oName);
    LW_SAFE_FREE_MEMORY(appContext->oUID);
    LW_SAFE_FREE_MEMORY(appContext->oGID);
    LW_SAFE_FREE_MEMORY(appContext->oHomeDir);
    LW_SAFE_FREE_MEMORY(appContext->oShell);
    LW_SAFE_FREE_MEMORY(appContext->oCellDn);

    cleanup:
      return(dwError);

    error:
      dwError = ADT_ERR_FAILED_POPT_CTX;
      goto cleanup;
}

/**
 * Set references to Init(), Validate(), CleanUp(), and Execute() functions.
 *
 * @param action Action reference.
 * return 0 on success; error code on failure.
 */
static DWORD SetUpAction(IN AdtActionTP action) {
    DWORD dwError = 0;

    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    appContext->workConn = &(appContext->modifyConn);

    switch (((AdtActionBaseTP) action)->actionCode) {
        case (AdtMoveObjectAction):
            appContext->actionInitMethod = &InitAdtMoveObjectAction;
            appContext->actionValidateMethod = &ValidateAdtMoveObjectAction;
            appContext->actionExecuteMethod = &ExecuteAdtMoveObjectAction;
            appContext->actionCleanUpMethod = &CleanUpAdtMoveObjectAction;
            break;

        case (AdtNewUserAction):
            appContext->actionInitMethod = &InitAdtNewUserAction;
            appContext->actionValidateMethod = &ValidateAdtNewUserAction;
            appContext->actionExecuteMethod = &ExecuteAdtNewUserAction;
            appContext->actionCleanUpMethod = &CleanUpAdtNewUserAction;
            break;

        case (AdtNewGroupAction):
            appContext->actionInitMethod = &InitAdtNewGroupAction;
            appContext->actionValidateMethod = &ValidateAdtNewGroupAction;
            appContext->actionExecuteMethod = &ExecuteAdtNewGroupAction;
            appContext->actionCleanUpMethod = &CleanUpAdtNewGroupAction;
            break;

        case (AdtNewOuAction):
            appContext->actionInitMethod = &InitAdtNewOuAction;
            appContext->actionValidateMethod = &ValidateAdtNewOuAction;
            appContext->actionExecuteMethod = &ExecuteAdtNewOuAction;
            appContext->actionCleanUpMethod = &CleanUpAdtNewOuAction;
            break;

        case (AdtNewComputerAction):
            appContext->actionInitMethod = &InitAdtNewComputerAction;
            appContext->actionValidateMethod = &ValidateAdtNewComputerAction;
            appContext->actionExecuteMethod = &ExecuteAdtNewComputerAction;
            appContext->actionCleanUpMethod = &CleanUpAdtNewComputerAction;
            break;

        case (AdtDeleteObjectAction):
            appContext->actionInitMethod = &InitAdtDeleteObjectAction;
            appContext->actionValidateMethod = &ValidateAdtDeleteObjectAction;
            appContext->actionExecuteMethod = &ExecuteAdtDeleteObjectAction;
            appContext->actionCleanUpMethod = &CleanUpAdtDeleteObjectAction;
            break;

        case (AdtSearchUserAction):
            appContext->actionInitMethod = &InitAdtSearchUserAction;
            appContext->actionValidateMethod = &ValidateAdtSearchUserAction;
            appContext->actionExecuteMethod = &ExecuteAdtSearchUserAction;
            appContext->actionCleanUpMethod = &CleanUpAdtSearchUserAction;
            break;

        case (AdtSearchGroupAction):
            appContext->actionInitMethod = &InitAdtSearchGroupAction;
            appContext->actionValidateMethod = &ValidateAdtSearchGroupAction;
            appContext->actionExecuteMethod = &ExecuteAdtSearchGroupAction;
            appContext->actionCleanUpMethod = &CleanUpAdtSearchGroupAction;
            break;

        case (AdtSearchOuAction):
            appContext->actionInitMethod = &InitAdtSearchOuAction;
            appContext->actionValidateMethod = &ValidateAdtSearchOuAction;
            appContext->actionExecuteMethod = &ExecuteAdtSearchOuAction;
            appContext->actionCleanUpMethod = &CleanUpAdtSearchOuAction;
            break;

        case (AdtSearchComputerAction):
            appContext->actionInitMethod = &InitAdtSearchComputerAction;
            appContext->actionValidateMethod = &ValidateAdtSearchComputerAction;
            appContext->actionExecuteMethod = &ExecuteAdtSearchComputerAction;
            appContext->actionCleanUpMethod = &CleanUpAdtSearchComputerAction;
            break;

        case (AdtSearchObjectAction):
            appContext->actionInitMethod = &InitAdtSearchObjectAction;
            appContext->actionValidateMethod = &ValidateAdtSearchObjectAction;
            appContext->actionExecuteMethod = &ExecuteAdtSearchObjectAction;
            appContext->actionCleanUpMethod = &CleanUpAdtSearchObjectAction;
            break;

        case (AdtLookupObjectAction):
            appContext->actionInitMethod = &InitAdtLookupObjectAction;
            appContext->actionValidateMethod = &ValidateAdtLookupObjectAction;
            appContext->actionExecuteMethod = &ExecuteAdtLookupObjectAction;
            appContext->actionCleanUpMethod = &CleanUpAdtLookupObjectAction;
            break;

        case (AdtEnableUserAction):
            appContext->actionInitMethod = &InitAdtEnableUserAction;
            appContext->actionValidateMethod = &ValidateAdtEnableUserAction;
            appContext->actionExecuteMethod = &ExecuteAdtEnableUserAction;
            appContext->actionCleanUpMethod = &CleanUpAdtEnableUserAction;
            break;

        case (AdtEnableComputerAction):
            appContext->actionInitMethod = &InitAdtEnableComputerAction;
            appContext->actionValidateMethod = &ValidateAdtEnableComputerAction;
            appContext->actionExecuteMethod = &ExecuteAdtEnableComputerAction;
            appContext->actionCleanUpMethod = &CleanUpAdtEnableComputerAction;
            break;

        case (AdtDisableUserAction):
            appContext->actionInitMethod = &InitAdtDisableUserAction;
            appContext->actionValidateMethod = &ValidateAdtDisableUserAction;
            appContext->actionExecuteMethod = &ExecuteAdtDisableUserAction;
            appContext->actionCleanUpMethod = &CleanUpAdtDisableUserAction;
            break;

        case (AdtDisableComputerAction):
            appContext->actionInitMethod = &InitAdtDisableComputerAction;
            appContext->actionValidateMethod
                    = &ValidateAdtDisableComputerAction;
            appContext->actionExecuteMethod = &ExecuteAdtDisableComputerAction;
            appContext->actionCleanUpMethod = &CleanUpAdtDisableComputerAction;
            break;

        case (AdtResetUserPasswordAction):
            appContext->actionInitMethod = &InitAdtResetUserPasswordAction;
            appContext->actionValidateMethod
                    = &ValidateAdtResetUserPasswordAction;
            appContext->actionExecuteMethod
                    = &ExecuteAdtResetUserPasswordAction;
            appContext->actionCleanUpMethod
                    = &CleanUpAdtResetUserPasswordAction;
            break;

        case (AdtResetComputerPasswordAction):
            appContext->actionInitMethod = &InitAdtResetComputerPasswordAction;
            appContext->actionValidateMethod
                    = &ValidateAdtResetComputerPasswordAction;
            appContext->actionExecuteMethod
                    = &ExecuteAdtResetComputerPasswordAction;
            appContext->actionCleanUpMethod
                    = &CleanUpAdtResetComputerPasswordAction;
            break;

        case (AdtAddToGroupAction):
            appContext->actionInitMethod = &InitAdtAddToGroupAction;
            appContext->actionValidateMethod = &ValidateAdtAddToGroupAction;
            appContext->actionExecuteMethod = &ExecuteAdtAddToGroupAction;
            appContext->actionCleanUpMethod = &CleanUpAdtAddToGroupAction;
            break;

        case (AdtRemoveFromGroupAction):
            appContext->actionInitMethod = &InitAdtRemoveFromGroupAction;
            appContext->actionValidateMethod
                    = &ValidateAdtRemoveFromGroupAction;
            appContext->actionExecuteMethod = &ExecuteAdtRemoveFromGroupAction;
            appContext->actionCleanUpMethod = &CleanUpAdtRemoveFromGroupAction;
            break;

        case (AdtUnlockAccountAction):
            appContext->actionInitMethod = &InitAdtUnlockAccountAction;
            appContext->actionValidateMethod
                    = &ValidateAdtUnlockAccountAction;
            appContext->actionExecuteMethod = &ExecuteAdtUnlockAccountAction;
            appContext->actionCleanUpMethod = &CleanUpAdtUnlockAccountAction;
            break;

        default:
            break;
    }

    PrintStderr(appContext,
                LogLevelTrace,
                "%s ---> ProcessAuthArgs()\n",
                appContext->actionName);
    dwError = ProcessAuthArgs(appContext);

    PrintStderr(appContext,
                LogLevelTrace,
                "%s <--- Returned %d\n",
                appContext->actionName,
                dwError);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext,
                LogLevelTrace,
                "%s ---> ProcessConnectArgs()\n",
                appContext->actionName);

    if(!appContext->copts.serverAddress && appContext->copts.domainName) {
        dwError = LwStrDupOrNull((PCSTR) appContext->copts.domainName,
                                 &(appContext->copts.serverAddress));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

        LW_SAFE_FREE_MEMORY(appContext->copts.domainName);
    }

    dwError = ProcessConnectArgs(appContext,
                                 appContext->copts.serverAddress,
                                 appContext->copts.port,
                                 appContext->copts.domainName);

    PrintStderr(appContext,
                LogLevelTrace,
                "%s ---> Returned %d\n",
                appContext->actionName,
                dwError);
    ADT_BAIL_ON_ERROR_NP(dwError);

    appContext->actionExecState = ActionReady;

    error:
       return dwError;
}

/********************************************************************/
/*                      AdTool API start                            */
/********************************************************************/

/**
 * Initialize ADTool library. Call this method before using any other
 * AdTool API methods. In multi-threaded environment each thread must
 * have its context, so this method must be called from each thread.
 *
 * @param context Application context reference.
 * @return 0 on success; error code on failure.
 */
DWORD AdtOpen(OUT HANDLE *context)
{
    DWORD dwError = 0;
    AppContextTP appContext; /* application context. */

    dwError = LwAllocateMemory(sizeof(AppContextT), OUT_PPVOID(&appContext));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);
    appContext->gopts.logLevel = LogLevelWarning;

    dwError = MakeFullArgsTable(appContext, NULL);
    ADT_BAIL_ON_ERROR(dwError);

    appContext->actionExecState = ActionNotReady;
    appContext->action.base.opaque = (VOID *) appContext;

    *context = appContext;

   cleanup:
      return(dwError);

   error:
      goto cleanup;
}

/**
 * Close ADTool library. This method frees internally allocated memory.
 *
 * @param context Application context reference.
 * @return 0 on success; error code on failure.
 */
DWORD AdtClose(OUT HANDLE context)
{
    AppContextTP appContext = (AppContextTP) context;

    if(appContext->optionsCon) {
        appContext->optionsCon = poptFreeContext(appContext->optionsCon);
    }

    if(appContext->fullCon) {
        appContext->fullCon = poptFreeContext(appContext->fullCon);
    }

    LW_SAFE_FREE_MEMORY(appContext->stdoutStr);
    LW_SAFE_FREE_MEMORY(appContext->stderrStr);
    LW_SAFE_FREE_MEMORY(appContext->execResult.base.resultStr);

    FreeOptions(appContext->fullTable);

    CloseADConnections(appContext);
    LW_SAFE_FREE_MEMORY(appContext->oID);
    LW_SAFE_FREE_MEMORY(appContext->oName);
    LW_SAFE_FREE_MEMORY(appContext->oUID);
    LW_SAFE_FREE_MEMORY(appContext->oGID);
    LW_SAFE_FREE_MEMORY(appContext->oHomeDir);
    LW_SAFE_FREE_MEMORY(appContext->oShell);
    LW_SAFE_FREE_MEMORY(appContext->oCellDn);

    return 0;
}

/**
 * Create a new action object from command line arguments.
 *
 * @param context Context reference returned by AdtOpen().
 * @param argc Number of arguments.
 * @param argv Array of sting arguments.
 * @param action Created action.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtCreateActionArgV(IN HANDLE context, IN INT argc, IN PCSTR *argv, OUT AdtActionTPP action)
{
    AppContextTP appContext = (AppContextTP) context;
    DWORD dwError = 0;
    INT isActionPresent = 0;
    AdtActionCode code;
    INT c;
    PCSTR str = NULL;
    BOOL extraArgs = FALSE;

    *action = &(appContext->action);
    ResetAction(context, argc, argv);

    appContext->execResult.base.returnCode = 0;

    while ((c = poptGetNextOpt(appContext->optionsCon)) >= 0) {
        if(c == 'a') {
            isActionPresent = 1;

            if (appContext->actionName) {
                code = AdtGetActionCode((PSTR) appContext->actionName);

                if (code == AdtBaseAction) {
                    dwError = ADT_ERR_INVALID_ACTION;
                    ADT_BAIL_ON_ERROR(dwError);
                }

                appContext->action.base.actionCode = code;
                appContext->actionName = AdtGetActionName(code);
            }
        }
    }

    if (appContext->hopts.isPrintUsage) {
        PrintUsage(appContext->fullCon, NULL, NULL, appContext->optionsCon);
        appContext->actionExecState = ActionComplete;
        EXIT_NORMALLY;
    }

    if (appContext->hopts.isPrintVersion) {
        fprintf(stdout, ADT_VERSION);
        appContext->actionExecState = ActionComplete;
        EXIT_NORMALLY;
    }

    if (!isActionPresent) {
        if (appContext->hopts.isHelp) {
            poptPrintHelp(appContext->optionsCon, stdout, 0);
            PrintExamples();
            appContext->actionExecState = ActionComplete;
            EXIT_NORMALLY;
        }
        else {
            dwError = ADT_ERR_ACTION_MISSING;
            ADT_BAIL_ON_ERROR(dwError);
        }
    }
    else {
        if (appContext->hopts.isHelp) {
            if (appContext->actionName != NULL) {
                PrintActionHelp(appContext->actionsTable, appContext->action.base.actionCode, argc, argv);
            }
            else {
                PrintActionsList(appContext->actionsTable, argc, argv); /* help on all actions */
            }
            appContext->actionExecState = ActionComplete;
            EXIT_NORMALLY;
        }
        else {
            if (appContext->actionName == NULL) {
                dwError = ADT_ERR_ACTION_MISSING;
                ADT_BAIL_ON_ERROR(dwError);
            }

            struct poptOption *tmp = CloneActionSubtable(appContext->actionsTable, appContext->action.base.actionCode);
            FreeOptions(appContext->fullTable);
            appContext->fullTable = NULL;

            if(appContext->fullCon) {
                poptFreeContext(appContext->fullCon);
            }

            dwError = MakeFullArgsTable(appContext, tmp);
            ADT_BAIL_ON_ERROR(dwError);

            appContext->fullCon = poptGetContext(appContext->actionName, argc, argv, appContext->fullTable, 0);
            ADT_BAIL_ON_NULL(appContext->fullCon);

            while ((c = poptGetNextOpt(appContext->fullCon)) >= 0) {
            }

            while ((str = poptGetArg(appContext->fullCon)) != NULL) {
                fprintf(stdout, "Unrecognized argument: %s\n", str);
                extraArgs = TRUE;
            }

            if(extraArgs == TRUE) {
                dwError = ADT_ERR_INVALID_USAGE;
                goto error2;
            }

            if(c < -1) { /* parsing was not successful */
                fprintf(stderr, "%s: %s\n",
                        poptBadOption(appContext->fullCon, POPT_BADOPTION_NOALIAS),
                        poptStrerror(c));

                dwError = ADT_ERR_INVALID_USAGE;
                EXIT_ABNORMALLY;
            }

            dwError = SetUpAction(&(appContext->action));
            ADT_BAIL_ON_ERROR2(dwError);

            EXIT_NORMALLY;
        }
    }

    cleanup:
      appContext->execResult.base.returnCode = dwError;
      return(dwError);

    error:
      PrintUsage(appContext->fullCon, NULL, NULL, appContext->optionsCon);
      goto cleanup;

    error2:
      goto cleanup;
}

/**
 * Create a new action from a command line string.
 * This string a normal lw-adtool command line without the
 * command component (lw-adtool), e.g. "-a new-cell -dn Enterprise"
 *
 * @param context Context reference returned by AdtOpen().
 * @param command Command line string.
 * @param action Created action.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtCreateActionArgS(IN HANDLE context, IN PSTR command, OUT AdtActionTPP action)
{
    AppContextTP appContext = (AppContextTP) context;
    int argc;
    const char **argv;
    PSTR cmd = command;
    PSTR cmdbuf;

    if(cmd == NULL) {
        cmd = "";
    }

    DWORD dwError = LwAllocateStringPrintf(&cmdbuf, "%s %s", ADTOOL, cmd);
    ADT_BAIL_ON_ERROR(dwError);

    poptParseArgvString(cmd, &argc, &argv);

    dwError = AdtCreateActionArgV(context, argc, argv, action);
    ADT_BAIL_ON_ERROR(dwError);

    cleanup:
       LW_SAFE_FREE_STRING(cmdbuf);
       return(dwError);

    error:
       goto cleanup;
}

/**
 * Set output mode of the adtool library.
 *
 * @param context Context reference returned by AdtOpen().
 * @param mode Output mode.
 */
VOID AdtSetOutputMode(IN HANDLE context, IN AdtOutputModeT mode)
{
    AppContextTP appContext = (AppContextTP) context;
    appContext->outputMode = mode;
}

/**
 * Create a new action from a command line string, using an existing
 * authenticated connection to an Active Directory server.
 * This string a normal lw-adtool command line without the
 * command component (lw-adtool), e.g. "-a new-cell -dn Enterprise"
 *
 * @param context Context reference returned by AdtOpen().
 * @param command Command line string.
 * @param serverConn Active Directory server connection.
 * @param action Created action.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtCreateActionArgSC(IN HANDLE context, IN PSTR command, IN HANDLE serverConn, OUT AdtActionTPP action)
{
    return AdtCreateActionArgS(context, command, action);
}

/**
 * Initialize action. Among other things, this method set references to Validate(),
 * CleanUp(), and Execute() functions. In order to use this method, you have to
 * allocate action memory first and set action fields as needed.
 * AdtGetAction() will finish initialization and return a ready to use action
 * object. When the action object is no longer needed, call AdtDisposeAction()
 * and then free the previously set fields and action memory.
 *
 * @param context Context reference returned by AdtOpen().
 * @param prePopulatedAction prepopulated action.
 * @param serverConn Authenticated Active Directory server connection.
 * @param action Created action.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtGetAction(IN HANDLE context, IN OUT AdtActionTP prePopulatedAction, IN HANDLE serverConn)
{
    return ADT_ERR_ACTION_NOT_SUPPORTED; /* TODO: implement */
}

/**
 * Excute action.
 *
 * @param action Action to execute.
 * return 0 on success; error code on failure.
 */
DWORD AdtExecuteAction(IN AdtActionTP action)
{
    DWORD dwError = 0;

    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    if(appContext->actionExecState == ActionComplete) {
        return 0;
    }

    if(appContext->actionExecState != ActionReady) {
        dwError = ADT_ERR_ACTION_INVALID_STATE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s ---> Init()\n", appContext->actionName);
    dwError = appContext->actionInitMethod(action);
    PrintStderr(appContext, LogLevelTrace, "%s <--- Returned %d\n", appContext->actionName, dwError);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelTrace, "%s ---> Validate()\n", appContext->actionName);
    dwError = appContext->actionValidateMethod(&(appContext->action));
    PrintStderr(appContext, LogLevelTrace, "%s <--- Returned %d\n", appContext->actionName, dwError);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelTrace, "%s ---> Execute()\n", appContext->actionName);
    dwError = appContext->actionExecuteMethod(action);
    PrintStderr(appContext, LogLevelTrace, "%s <--- Returned %d\n", appContext->actionName, dwError);
    ADT_BAIL_ON_ERROR_NP(dwError);

    cleanup:
        ((AdtResultBaseTP) &(appContext->execResult))->returnCode = dwError;
        appContext->actionExecState = ActionComplete;
        return dwError;

    error:
        goto cleanup;
}

/**
 * Get result of action execution. The result is action dependent and must
 * be cast to an action-specific type.
 *
 * @param action Executed action.
 * @param execResult Execution result.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtGetExecResult(IN AdtActionTP action, OUT AdtResultTPP execResult) {
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    if(appContext->actionExecState != ActionComplete) {
        return ADT_ERR_ACTION_INVALID_STATE;
    }

    *execResult = &(appContext->execResult);

    return 0;
}

/**
 * Get full stdout output of action creation and execution.
 *
 * @param context Context reference returned by AdtOpen().
 * @param str Stdout string.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtGetStdoutStr(IN HANDLE context, OUT PCSTR *str)
{
    AppContextTP appContext = (AppContextTP) context;

    *str = (PCSTR) appContext->stdoutStr;

    return 0;
}

/**
 * Get full stderr output of action creation and execution.
 *
 * @param context Context reference returned by AdtOpen().
 * @param str Stdout string.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtGetStderrStr(IN HANDLE context, OUT PCSTR *str)
{
    AppContextTP appContext = (AppContextTP) context;

    *str = (PCSTR) appContext->stderrStr;

    return 0;
}

/**
 * Free action memory.
 *
 * @param action Action to be disposed.
 *
 * return 0 on success; error code on failure.
 */
DWORD AdtDisposeAction(IN AdtActionTP action)
{
    DWORD dwError = 0;

    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    if((appContext->actionName != NULL) && (appContext->actionCleanUpMethod != NULL)) {
        PrintStderr(appContext, LogLevelTrace, "%s ---> CleanUp()\n", appContext->actionName);
        dwError = appContext->actionCleanUpMethod(action);
        PrintStderr(appContext, LogLevelTrace, "%s <--- Returned %d\n", appContext->actionName, dwError);
    }

    if(appContext->optionsCon) {
        appContext->optionsCon = poptFreeContext(appContext->optionsCon);
    }

    if(appContext->fullCon) {
        appContext->fullCon = poptFreeContext(appContext->fullCon);
    }

    appContext->actionExecState = ActionNotReady;
    appContext->execResult.base.returnCode = 0;
    LW_SAFE_FREE_MEMORY(appContext->stdoutStr);
    LW_SAFE_FREE_MEMORY(appContext->stderrStr);
    LW_SAFE_FREE_MEMORY(appContext->execResult.base.resultStr);

    // We shall not free the following data when we implement
    // connection sharing.
    CloseADConnections(appContext);
    LW_SAFE_FREE_MEMORY(appContext->oID);
    LW_SAFE_FREE_MEMORY(appContext->oName);
    LW_SAFE_FREE_MEMORY(appContext->oUID);
    LW_SAFE_FREE_MEMORY(appContext->oGID);
    LW_SAFE_FREE_MEMORY(appContext->oHomeDir);
    LW_SAFE_FREE_MEMORY(appContext->oShell);
    LW_SAFE_FREE_MEMORY(appContext->oCellDn);

    return dwError;
}

/**
 * This method checks whether this is an AdTool-specific error and
 * remaps it as needed. It delegate the call to LW error mapper if
 * the error code is not known to AdTool.
 *
 * @param dwError Error code.
 * @return Error message.
 */
PCSTR AdtGetErrorMsg(IN DWORD dwError)
{
    DWORD reqSize = 0;

    PCSTR ret = GetErrorMsg(dwError);

    if(ret) {
        return ret;
    }

    /* Check if this is an LDAP error */
    if((dwError - ADT_LDAP_ERR_BASE) < ADT_LDAP_ERR_RANGE) {
        return ldap_err2string(dwError - ADT_LDAP_ERR_BASE);
    }

    /* Check if this is a Kerberos error. We use global buffer for Kerberos errors. */
    if((dwError - ADT_KRB5_ERR_BASE) < ADT_KRB5_ERR_RANGE) {
        return adtLastErrMsg;
    }

    /* Check if this is a Windoes error. */
    if((dwError - ADT_WIN_ERR_BASE) < ADT_WIN_ERR_RANGE) {
        if(LwNtStatusToErrno(dwError - ADT_WIN_ERR_BASE) == -1) {
            dwError -= ADT_WIN_ERR_BASE;
        }
        else {
            return NtStatusToDescription(dwError - ADT_WIN_ERR_BASE);
        }
    }

    /* Default to lwadvapi error */
    reqSize = LwGetErrorString(dwError, adtLastErrMsg, ADT_ERROR_MSG_SIZE_MAX);

    if (reqSize > ADT_ERROR_MSG_SIZE_MAX) {
        PrintStderr(NULL, LogLevelTrace, "Error message is too long [dwError=%d]", dwError);
        return NULL;
    }

    return adtLastErrMsg;
}

/**
 * Look up action name by code.
 *
 * @param code Action code.
 * @return Action name or NULL if the code is unknown.
 */
PCSTR AdtGetActionName(IN AdtActionCode code) {
    switch (code) {
        /**
         * Enterprise edition.
         */
        case (AdtNewCellAction):
            return ADT_NEW_CELL_ACT;

        case (AdtEditCellAction):
            return ADT_EDIT_CELL_ACT;

        case (AdtEditCellUserAction):
            return ADT_EDIT_CELL_USER_ACT;

        case (AdtEditCellGroupAction):
            return ADT_EDIT_CELL_GROUP_ACT;

        case (AdtAddToCellAction):
            return ADT_ADD_TO_CELL_ACT;

        case (AdtRemoveFromCellAction):
            return ADT_REMOVE_FROM_CELL_ACT;

        case (AdtLinkCellAction):
            return ADT_LINK_CELL_ACT;

        case (AdtUnlinkCellAction):
            return ADT_UNLINK_CELL_ACT;

        case (AdtSearchCellsAction):
            return ADT_SEARCH_CELLS_ACT;

        case (AdtLookupCellAction):
            return ADT_LOOKUP_CELL_ACT;

        case (AdtLookupCellUserAction):
            return ADT_LOOKUP_CELL_USER_ACT;

        case (AdtLookupCellGroupAction):
            return ADT_lOOKUP_CELL_GROUP_ACT;

        case (AdtMoveObjectAction):
            return ADT_MOVE_OBJECT_ACT;

        case (AdtNewUserAction):
            return ADT_NEW_USER_ACT;

        case (AdtNewGroupAction):
            return ADT_NEW_GROUP_ACT;

        case (AdtNewOuAction):
            return ADT_NEW_OU_ACT;

        case (AdtNewComputerAction):
            return ADT_NEW_COMPUTER_ACT;

        case (AdtDeleteObjectAction):
            return ADT_DELETE_OBJECT_ACT;

        case (AdtDeleteCellAction):
            return ADT_DELETE_CELL_ACT;

        case (AdtSearchUserAction):
            return ADT_SEARCH_USER_ACT;

        case (AdtSearchGroupAction):
            return ADT_SEARCH_GROUP_ACT;

        case (AdtSearchOuAction):
            return ADT_SEARCH_OU_ACT;

        case (AdtSearchComputerAction):
            return ADT_SEARCH_COMPUTER_ACT;

        case (AdtSearchObjectAction):
            return ADT_SEARCH_OBJECT_ACT;

        case (AdtLookupObjectAction):
            return ADT_LOOKUP_OBJECT_ACT;

        case (AdtEnableUserAction):
            return ADT_ENABLE_USER_ACT;

        case (AdtEnableComputerAction):
            return ADT_ENABLE_COMPUTER_ACT;

        case (AdtDisableUserAction):
            return ADT_DISABLE_USER_ACT;

        case (AdtDisableComputerAction):
            return ADT_DISABLE_COMPUTER_ACT;

        case (AdtResetUserPasswordAction):
            return ADT_RESET_USER_PASSWORD_ACT;

        case (AdtResetComputerPasswordAction):
            return ADT_RESET_COMPUTER_PASSWORD_ACT;

        case (AdtAddToGroupAction):
            return ADT_ADD_TO_GROUP_ACT;

        case (AdtRemoveFromGroupAction):
            return ADT_REMOVE_FROM_GROUP_ACT;

        case (AdtUnlockAccountAction):
            return ADT_UNLOCK_ACCOUNT_ACT;

        default:
            break;
    }

    return NULL;
}

/**
 * Look up action code by name.
 *
 * @param s Action name.
 * @return Action code or AdtBaseAction if the name is not recognized.
 */
AdtActionCode AdtGetActionCode(IN PSTR name) {
    if((name == NULL) || (name[0] == '\0')) {
        return AdtBaseAction;
    }

    int i;

    for(i = 0; name[i]; i++) {
      name[i] = tolower(name[i]);
    }

    if(!strcmp(name, ADT_NEW_CELL_ACT)) {
        return AdtNewCellAction;
    }

    if(!strcmp(name, ADT_EDIT_CELL_ACT)) {
        return AdtEditCellAction;
    }

    if(!strcmp(name, ADT_EDIT_CELL_USER_ACT)) {
        return AdtEditCellUserAction;
    }

    if(!strcmp(name, ADT_EDIT_CELL_GROUP_ACT)) {
        return AdtEditCellGroupAction;
    }

    if(!strcmp(name, ADT_ADD_TO_CELL_ACT)) {
        return AdtAddToCellAction;
    }

    if(!strcmp(name, ADT_REMOVE_FROM_CELL_ACT)) {
        return AdtRemoveFromCellAction;
    }

    if(!strcmp(name, ADT_LINK_CELL_ACT)) {
        return AdtLinkCellAction;
    }

    if(!strcmp(name, ADT_UNLINK_CELL_ACT)) {
        return AdtUnlinkCellAction;
    }

    if(!strcmp(name, ADT_SEARCH_CELLS_ACT)) {
        return AdtSearchCellsAction;
    }

    if(!strcmp(name, ADT_LOOKUP_CELL_ACT)) {
        return AdtLookupCellAction;
    }

    if(!strcmp(name, ADT_LOOKUP_CELL_USER_ACT)) {
        return AdtLookupCellUserAction;
    }

    if(!strcmp(name, ADT_lOOKUP_CELL_GROUP_ACT)) {
        return AdtLookupCellGroupAction;
    }

    if(!strcmp(name, ADT_MOVE_OBJECT_ACT)) {
        return AdtMoveObjectAction;
    }

    if(!strcmp(name, ADT_NEW_USER_ACT)) {
        return AdtNewUserAction;
    }

    if(!strcmp(name, ADT_NEW_GROUP_ACT)) {
        return AdtNewGroupAction;
    }

    if(!strcmp(name, ADT_NEW_OU_ACT)) {
        return AdtNewOuAction;
    }

    if(!strcmp(name, ADT_NEW_COMPUTER_ACT)) {
        return AdtNewComputerAction;
    }

    if(!strcmp(name, ADT_DELETE_OBJECT_ACT)) {
        return AdtDeleteObjectAction;
    }

    if(!strcmp(name, ADT_DELETE_CELL_ACT)) {
        return AdtDeleteCellAction;
    }

    if(!strcmp(name, ADT_SEARCH_USER_ACT)) {
        return AdtSearchUserAction;
    }

    if(!strcmp(name, ADT_SEARCH_GROUP_ACT)) {
        return AdtSearchGroupAction;
    }

    if(!strcmp(name, ADT_SEARCH_OU_ACT)) {
        return AdtSearchOuAction;
    }

    if(!strcmp(name, ADT_SEARCH_COMPUTER_ACT)) {
        return AdtSearchComputerAction;
    }

    if(!strcmp(name, ADT_SEARCH_OBJECT_ACT)) {
        return AdtSearchObjectAction;
    }

    if(!strcmp(name, ADT_LOOKUP_OBJECT_ACT)) {
        return AdtLookupObjectAction;
    }

    if(!strcmp(name, ADT_ENABLE_USER_ACT)) {
        return AdtEnableUserAction;
    }

    if(!strcmp(name, ADT_ENABLE_COMPUTER_ACT)) {
        return AdtEnableComputerAction;
    }

    if(!strcmp(name, ADT_DISABLE_USER_ACT)) {
        return AdtDisableUserAction;
    }

    if(!strcmp(name, ADT_DISABLE_COMPUTER_ACT)) {
        return AdtDisableComputerAction;
    }

    if(!strcmp(name, ADT_RESET_USER_PASSWORD_ACT)) {
        return AdtResetUserPasswordAction;
    }

    if(!strcmp(name, ADT_RESET_COMPUTER_PASSWORD_ACT)) {
        return AdtResetComputerPasswordAction;
    }

    if(!strcmp(name, ADT_ADD_TO_GROUP_ACT)) {
        return AdtAddToGroupAction;
    }

    if(!strcmp(name, ADT_REMOVE_FROM_GROUP_ACT)) {
        return AdtRemoveFromGroupAction;
    }

    if(!strcmp(name, ADT_UNLOCK_ACCOUNT_ACT)) {
        return AdtUnlockAccountAction;
    }

    return AdtBaseAction;
}

/********************************************************************/
/*                        AdTool API end                            */
/********************************************************************/
