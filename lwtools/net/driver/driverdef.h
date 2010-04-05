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

#ifndef DRIVERDEF_H_
#define DRIVERDEF_H_

typedef DWORD NET_SUB_COMMAND, *PNET_SUB_COMMAND;

#define NET_COMMAND_UNKNOWN 0
#define NET_COMMAND_HELP 1
#define NET_COMMAND_SHARE 2
#define NET_COMMAND_SESSION 3
#define NET_COMMAND_USER 4
#define NET_COMMAND_VIEW 5
#define NET_COMMAND_LOCALGROUP 6
#define NET_COMMAND_TIME 7
#define NET_COMMAND_FILE 8


#define NET_COMMAND_HELP_PARAM "HELP"
#define NET_COMMAND_SHARE_PARAM "SHARE"
#define NET_COMMAND_SESSION_PARAM "SESSION"
#define NET_COMMAND_USER_PARAM "USER"
#define NET_COMMAND_VIEW_PARAM "VIEW"
#define NET_COMMAND_LOCALGROUP_PARAM "LOCALGROUP"
#define NET_COMMAND_TIME_PARAM "TIME"
#define NET_COMMAND_FILE_PARAM "FILE"

#endif /* DRIVERDEF_H_ */
