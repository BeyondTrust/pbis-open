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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        task.c
 *
 * Abstract:
 *
 *        BeyondTrust Task Client
 *
 *        Task Request Handler
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
DWORD
LwTaskCliList(
    int   argc,
    char* argv[]
    );

static
DWORD
LwTaskCliAdd(
    int   argc,
    char* argv[]
    );

static
DWORD
LwTaskCliDel(
    int   argc,
    char* argv[]
    );

static
DWORD
LwTaskCliExec(
    int   argc,
    char* argv[]
    );

static
DWORD
LwTaskCliGetSchema(
    int   argc,
    char* argv[]
    );

static
DWORD
LwTaskCliGetStatus(
    int   argc,
    char* argv[]
    );

DWORD
LwTaskHandleTaskRequest(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;

	if (!argc)
	{
		goto cleanup;
	}

	if (!strcmp(argv[0], "list"))
	{
		dwError = LwTaskCliList(argc-1, &argv[1]);
	}
	else if (!strcmp(argv[0], "add"))
	{
		dwError = LwTaskCliAdd(argc-1, &argv[1]);
	}
	else if (!strcmp(argv[0], "del"))
	{
		dwError = LwTaskCliDel(argc-1, &argv[1]);
	}
	else if (!strcmp(argv[0], "exec"))
	{
		dwError = LwTaskCliExec(argc-1, &argv[1]);
	}
	else if (!strcmp(argv[0], "schema"))
	{
		dwError = LwTaskCliGetSchema(argc-1, &argv[1]);
	}
	else if (!strcmp(argv[0], "status"))
	{
		dwError = LwTaskCliGetStatus(argc-1, &argv[1]);
	}
	else
	{
		dwError = ERROR_BAD_ARGUMENTS;
	}
	BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

	return dwError;

error:

	goto cleanup;
}

static
DWORD
LwTaskCliList(
    int   argc,
    char* argv[]
    )
{
	DWORD dwError = 0;
	PLW_TASK_CLIENT_CONNECTION pConnection = NULL;
	LW_TASK_TYPE taskType = LW_TASK_TYPE_MIGRATE;
	DWORD dwNumTasks = 0;
	DWORD dwNumTotalTasks = 0;
	DWORD dwResume = 0;
	PLW_TASK_INFO pTaskInfoArray = NULL;
	BOOLEAN bContinue = TRUE;

	if (argc > 0)
	{
		if (!strcmp(argv[0], "migrate-share"))
		{
			taskType = LW_TASK_TYPE_MIGRATE;
		}
		else
		{
			fprintf(stderr, "Error: unknown task type [%s]\n", argv[0]);

			dwError = ERROR_BAD_ARGUMENTS;
			BAIL_ON_LW_TASK_ERROR(dwError);
		}
	}

	dwError = LwTaskOpenServer(&pConnection);
	BAIL_ON_LW_TASK_ERROR(dwError);

	do
	{
		DWORD iTask = 0;

		if (pTaskInfoArray)
		{
			LwTaskFreeTaskInfoArray(pTaskInfoArray, dwNumTasks);
			pTaskInfoArray = NULL;
		}

		dwError = LwTaskEnum(
						pConnection,
						taskType,
						&pTaskInfoArray,
						&dwNumTasks,
						&dwNumTotalTasks,
						&dwResume);
		switch (dwError)
		{
		case ERROR_MORE_DATA:

			bContinue = TRUE;

			dwError = ERROR_SUCCESS;

			break;

		case ERROR_SUCCESS:

			bContinue = FALSE;

			break;

		default:

			break;
		}
		BAIL_ON_LW_TASK_ERROR(dwError);

		for (; iTask < dwNumTasks; iTask++)
		{
			PLW_TASK_INFO pTaskInfo = &pTaskInfoArray[iTask];

			fprintf(stdout, "\nTask id: [%s]\n", pTaskInfo->pszTaskId);
			fprintf(stdout, "\nArguments: \n");

			if (pTaskInfo->dwNumArgs > 0)
			{
				DWORD iArg = 0;

				for (; iArg < pTaskInfo->dwNumArgs; iArg++)
				{
					PLW_TASK_ARG pArg = &pTaskInfo->pArgArray[iArg];

					fprintf(stdout, "Name:  %s\n", LW_SAFE_LOG_STRING(pArg->pszArgName));
					fprintf(stdout, "Value: %s\n", LW_SAFE_LOG_STRING(pArg->pszArgValue));
				}
			}
		}

	} while (bContinue);

cleanup:

	if (pTaskInfoArray)
	{
		LwTaskFreeTaskInfoArray(pTaskInfoArray, dwNumTasks);
		pTaskInfoArray = NULL;
	}

	if (pConnection)
	{
		LwTaskCloseServer(pConnection);
	}

	return dwError;

error:

	goto cleanup;
}

static
DWORD
LwTaskCliAdd(
    int   argc,
    char* argv[]
    )
{
	return 0;
}

static
DWORD
LwTaskCliDel(
    int   argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PLW_TASK_CLIENT_CONNECTION pConnection = NULL;

    if (!argc)
    {
    	dwError = ERROR_BAD_ARGUMENTS;
    	BAIL_ON_LW_TASK_ERROR(dwError);
    }

	dwError = LwTaskOpenServer(&pConnection);
	BAIL_ON_LW_TASK_ERROR(dwError);

	dwError = LwTaskDelete(pConnection, argv[0]);
	BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

	if (pConnection)
	{
		LwTaskCloseServer(pConnection);
	}

	return dwError;

error:

	goto cleanup;

}

static
DWORD
LwTaskCliExec(
    int   argc,
    char* argv[]
    )
{
	return 0;
}

static
DWORD
LwTaskCliGetSchema(
    int   argc,
    char* argv[]
    )
{
	return 0;
}

static
DWORD
LwTaskCliGetStatus(
    int   argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PLW_TASK_CLIENT_CONNECTION pConnection = NULL;
    LW_TASK_STATUS taskStatus = {0};
    struct tm tmbuf;
    CHAR szBuf[128];

    if (!argc)
    {
    	dwError = ERROR_BAD_ARGUMENTS;
    	BAIL_ON_LW_TASK_ERROR(dwError);
    }

	dwError = LwTaskOpenServer(&pConnection);
	BAIL_ON_LW_TASK_ERROR(dwError);

	dwError = LwTaskGetStatus(pConnection, argv[0], &taskStatus);
	BAIL_ON_LW_TASK_ERROR(dwError);

	fprintf(stdout, "Task status for [id: %s]\n", argv[0]);
	fprintf(stdout, "Error code:      [%u]\n", taskStatus.dwError);
	fprintf(stdout, "Percent complete:[%u]\n", taskStatus.dwPercentComplete);

	if (taskStatus.startTime > 0)
	{
		localtime_r(&taskStatus.startTime, &tmbuf);
		strftime(szBuf, sizeof(szBuf), "%m/%d/%Y %H:%M:%S %Z", &tmbuf);

		fprintf(stdout, "Start time:      [%s]\n", szBuf);
	}
	else
	{
		fprintf(stdout, "Start time:      [0]\n");
	}

	if (taskStatus.endTime > 0)
	{
		localtime_r(&taskStatus.endTime, &tmbuf);
		strftime(szBuf, sizeof(szBuf), "%m/%d/%Y %H:%M:%S %Z", &tmbuf);

		fprintf(stdout, "End time:        [%s]\n", szBuf);
	}
	else
	{
		fprintf(stdout, "Start time:      [0]\n");
	}

cleanup:

	if (pConnection)
	{
		LwTaskCloseServer(pConnection);
	}

	return dwError;

error:

	goto cleanup;
}
