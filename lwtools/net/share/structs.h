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


#ifndef STRUCTS_H_
#define STRUCTS_H_

typedef DWORD NET_SHARE_CTRL_CODE;

#define NET_SHARE_UNKNOWN 0
#define NET_SHARE_HELP 1
#define NET_SHARE_ADD 2
#define NET_SHARE_DEL 3
#define NET_SHARE_ENUM 4
#define NET_SHARE_SETINFO 5
#define NET_SHARE_GETINFO 6

#define NET_SHARE_COMMAND_HELP "HELP"
#define NET_SHARE_COMMAND_ADD "ADD"
#define NET_SHARE_COMMAND_DEL "DEL"
#define NET_SHARE_COMMAND_SETINFO "SET-INFO"
#define NET_SHARE_COMMAND_GETINFO "GET-INFO"


#define NET_SHARE_NAME_TITLE "Share name"
#define NET_SHARE_PATH_TITLE "Resource"
#define NET_SHARE_COMMENT_TITLE "Remark"


typedef struct _NET_SHARE_ADD_OR_SET_INFO_PARAMS
{
	PWSTR pwszServerName;
	PWSTR pwszShareName;
	PWSTR pwszPath;

    DWORD dwAllowUserCount;
    PWSTR* ppwszAllowUsers;
    DWORD dwDenyUserCount;
    PWSTR* ppwszDenyUsers;
    PWSTR pwszComment;
    PWSTR pwszTarget;
    BOOLEAN bReadOnly;
    BOOLEAN bReadWrite;
    BOOLEAN bClearAllow;
    BOOLEAN bClearDeny;
}NET_SHARE_ADD_OR_SET_INFO_PARAMS, *PNET_SHARE_ADD_OR_SET_INFO_PARAMS;


typedef struct _NET_SHARE_DEL_INFO_PARAMS
{
	PWSTR pwszServerName;
	PWSTR pwszShareName;
}NET_SHARE_DEL_INFO_PARAMS, *PNET_SHARE_DEL_INFO_PARAMS;

typedef struct _NET_SHARE_ENUM_INFO_PARAMS
{
	PWSTR pwszServerName;
}NET_SHARE_ENUM_INFO_PARAMS, *PNET_SHARE_ENUM_INFO_PARAMS;


typedef struct _NET_SHARE_COMMAND_INFO {
	NET_SHARE_CTRL_CODE dwControlCode;

    union {
		NET_SHARE_ENUM_INFO_PARAMS ShareEnumInfo;
		NET_SHARE_ADD_OR_SET_INFO_PARAMS ShareAddOrSetInfo;
    	NET_SHARE_DEL_INFO_PARAMS ShareDelInfo;
    	//NET_SHARE_GET_INFO_PARAMS ShareGetInfo;
    };
}NET_SHARE_COMMAND_INFO, *PNET_SHARE_COMMAND_INFO;

#endif /* STRUCTS_H_ */
