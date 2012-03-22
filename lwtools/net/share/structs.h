/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
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
