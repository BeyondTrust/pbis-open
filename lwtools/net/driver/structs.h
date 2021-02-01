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
