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
 *        main.c
 *
 * Abstract:
 *
 *        BeyondTrust System NET Utilities
 *
 *        Driver Dispatch
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"

#define NET_COMMAND_UNKNOWN 0
#define NET_COMMAND_HELP 1
#define NET_COMMAND_SHARE 2
#define NET_COMMAND_SESSION 3
#define NET_COMMAND_USER 4
#define NET_COMMAND_VIEW 5
#define NET_COMMAND_LOCALGROUP 6
#define NET_COMMAND_TIME 7
#define NET_COMMAND_FILE 8

#define NET_COMMAND_HELP_NAME "HELP"
#define NET_COMMAND_SHARE_NAME "SHARE"
#define NET_COMMAND_SESSION_NAME "SESSION"
#define NET_COMMAND_USER_NAME "USER"
#define NET_COMMAND_VIEW_NAME "VIEW"
#define NET_COMMAND_LOCALGROUP_NAME "LOCALGROUP"
#define NET_COMMAND_TIME_NAME "TIME"
#define NET_COMMAND_FILE_NAME "FILE"


static
VOID
NetShowUsage(
	VOID
	)
{
    printf(
        "lwnet commands available: \n"
        "       lwnet file \n"
        "       lwnet session \n"
        "       lwnet share \n");

    printf("\n"
           "lwnet help command - shows usage of a particular lwnet command)\n"
           "\n");
}

static
VOID
NetCommandShow(
	VOID
	)
{
    printf(
        "The syntax of lwnet command is: \n"
        "       lwnet [ file | session | share | use ]\n");
}

static
VOID
NetShowCommandUsage(
	char *param
	)
{
	if (!strcasecmp(param, NET_COMMAND_SHARE_NAME))
	{
		NetShareShowUsage();
	}
	else if (!strcasecmp(param, NET_COMMAND_SESSION_NAME))
	{
		NetSessionShowUsage();
	}
	else if (!strcasecmp(param, NET_COMMAND_USER_NAME))
	{
		//NetUserShowUsage();
	}
	else if (!strcasecmp(param, NET_COMMAND_VIEW_NAME))
	{
		//NetViewShowUsage();
	}
	else if (!strcasecmp(param, NET_COMMAND_LOCALGROUP_NAME))
	{
		//NetLocalGroupShowUsage();
	}
	else if (!strcasecmp(param, NET_COMMAND_TIME_NAME))
	{
		//NetTimeShowUsage();
	}
	else if (!strcasecmp(param, NET_COMMAND_FILE_NAME))
	{
		NetFileShowUsage();
	}
	else
	{
		NetShowUsage();
	}

	return;
}

static
DWORD
NetMapSubCommand(
    char *param,
    PNET_SUB_COMMAND pdwSubCommand
    )
{
	NET_SUB_COMMAND dwSubCommand = NET_COMMAND_UNKNOWN;

	if (!strcasecmp(param, NET_COMMAND_HELP_NAME))
	{
		dwSubCommand = NET_COMMAND_HELP;
	}
	else if (!strcasecmp(param, NET_COMMAND_SHARE_NAME))
	{
		dwSubCommand = NET_COMMAND_SHARE;
	}
	else if (!strcasecmp(param, NET_COMMAND_SESSION_NAME))
	{
		dwSubCommand = NET_COMMAND_SESSION;
	}
	else if (!strcasecmp(param, NET_COMMAND_USER_NAME))
	{
		dwSubCommand = NET_COMMAND_USER;
	}
	else if (!strcasecmp(param, NET_COMMAND_VIEW_NAME))
	{
		dwSubCommand = NET_COMMAND_VIEW;
	}
	else if (!strcasecmp(param, NET_COMMAND_LOCALGROUP_NAME))
	{
		dwSubCommand = NET_COMMAND_LOCALGROUP;
	}
	else if (!strcasecmp(param, NET_COMMAND_TIME_NAME))
	{
		dwSubCommand = NET_COMMAND_TIME;
	}
	else if (!strcasecmp(param, NET_COMMAND_FILE_NAME))
	{
		dwSubCommand = NET_COMMAND_FILE;
	}

	*pdwSubCommand = dwSubCommand;

	return 0;
}


int 
main(
    int argc,
    char ** argv
    )
{
	DWORD dwError = 0;
	NET_SUB_COMMAND dwSubCommand = NET_COMMAND_UNKNOWN;

	if (argc == 1)
	{
		NetCommandShow();
		return dwError;
	}

    dwError = NetMapSubCommand(
                    argv[1],
                    &dwSubCommand
                    );
    BAIL_ON_LTNET_ERROR(dwError);

    switch (dwSubCommand)
    {
        case NET_COMMAND_HELP:
            if (!argv[2])
            {
            	NetShowUsage();
            }
            else
            {
            	NetShowCommandUsage(argv[2]);
            }

        	break;

        case NET_COMMAND_SHARE:

            dwError = NetShare(argc, argv);

            break;

        case NET_COMMAND_SESSION:

            dwError = NetSession(argc, argv);

        	break;

        case NET_COMMAND_USER:
        	printf("net user\n");
        	break;

        case NET_COMMAND_VIEW:
        	printf("net view\n");
        	break;

        case NET_COMMAND_LOCALGROUP:
        	printf("net localgroup\n");
        	break;

        case NET_COMMAND_TIME:
        	printf("net time\n");
        	break;

        case NET_COMMAND_FILE:

            dwError = NetFile(argc, argv);

        	break;

        case NET_COMMAND_UNKNOWN:
        default:
        	NetShowUsage();
        	break;
    }
    BAIL_ON_LTNET_ERROR(dwError);

 error:

     return dwError;
}
