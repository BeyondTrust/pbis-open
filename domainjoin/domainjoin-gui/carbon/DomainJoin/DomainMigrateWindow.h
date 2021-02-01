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
 *  DomainMigrateWindow.h
 *  DomainJoin
 *
 *  Created by Glenn Curtis on 1/13/09.
 *  Copyright 2009 Likewise. All rights reserved.
 *
 */
#ifndef __DOMAINMIGRATEWINDOW_H__
#define __DOMAINMIGRATEWINDOW_H__

#include <Carbon/Carbon.h>

#include "TApplication.h"
#include "TWindow.h"

#include <string.h>

typedef struct _USER_LIST
{
    char * pszUsername;
    char * pszUserRealName;
    char * pszUserUID;
    char * pszUserGID;
    char * pszUserHomeDir;
    struct _USER_LIST * pNext;
    
} USER_LIST, * PUSER_LIST;

// Helper routines for this class
long
GetADUserInfo(
    const char * pszUsername,
    char ** ppszRealName,
    char ** ppszUserHomeDir,
    char ** ppszUserUID,
    char ** ppszUserGID
    );

long
GetLocalUserList(
    PUSER_LIST * ppLocalUsers
    );
    
void
FreeLocalUserList(
    PUSER_LIST pLocalUsers
    );

long
CallCommandWithOutputAndErr(
    const char *  pszCommand,
    const char ** ppszArgs,
    bool          bCaptureStderr,
    char **       ppszOutput,
    int *         pExitCode
    );

class DomainMigrateWindow : public TWindow
{
    public:
        DomainMigrateWindow(int inAppSignature);
        virtual ~DomainMigrateWindow();

    protected:
        DomainMigrateWindow(const DomainMigrateWindow& other);
        DomainMigrateWindow& operator=(const DomainMigrateWindow& other);

    public:

        void SetLocalUserRealName(const std::string& value);
        void SetLocalUserHomeDirectory(const std::string& value);
        void SetLocalUserUID(const std::string& value);
        void SetLocalUserGID(const std::string& value);
        void SetADUserRealName(const std::string& value);
        void SetADUserHomeDirectory(const std::string& value);
        void SetADUserUID(const std::string& value);
        void SetADUserGID(const std::string& value);
        void SetADUserEdit(const std::string& value);
        void MigrateOff();
        void MigrateOn();
        void SetLocalUsers();
        virtual void Close();

    protected:
    
        virtual Boolean HandleCommand( const HICommandExtended& inCommand );
        void ClearLocalUsersCombo();
        void AddUserToLocalUsersCombo(const std::string& value);
        void SetTitleToLocalUsersCombo();
        std::string GetLocalUserName();
        std::string GetLocalUserHomeDir();
        std::string GetADUserName();
        std::string GetADUserHomeDir();
        std::string GetADUserUID();
        std::string GetADUserGID();
        bool IsMoveOptionSelected();
        bool IsDeleteOptionSelected();
        bool IsKeepAdminOptionSelected();
        bool IsUseSpotlightOptionSelected();
        void HandleMigration();
        bool HandleValidateUser();
        bool ConfirmMigration(const std::string& localUserName,
                              const std::string& localUserHomeDir,
                              const std::string& adUserName,
                              const std::string& adUserHomeDir,
                              const std::string& adUserUID,
                              const std::string& adUserGID,
                              bool  bMoveProfile,
                              bool  bDeleteAccount,
                              bool  bKeepAdmin,
                              bool  bUseSpotlight);
        int CallMigrateCommand(const std::string& localUserName,
                               const std::string& adUserName,
                               const std::string& logFileName,
                               bool bMoveProfile,
                               bool bDeleteAccount,
                               bool bKeepAdmin,
                               bool bUseSpotlight,
                               char ** ppszOutput);
        void ShowMigrateCompleteDialog(const std::string& value);
        void ShowMigrateCompleteErrorDialog(const std::string& value, int code, const std::string& resultMessage);
        void HideMigrateProgressBar();
        void ShowMigrateProgressBar();

    protected:
	
        static const int LOCAL_USER_COMBO_ID;
        static const int LOCAL_USER_PATH_ID;
        static const int LOCAL_USER_REAL_NAME_ID;
        static const int LOCAL_USER_UID_ID;
        static const int LOCAL_USER_GID_ID;
        static const int AD_USER_EDIT_ID;
        static const int AD_USER_PATH_ID;
        static const int AD_USER_REAL_NAME_ID;
        static const int AD_USER_UID_ID;
        static const int AD_USER_GID_ID;
        static const int COPY_RADIO_ID;
        static const int MOVE_RADIO_ID;
        static const int DELETE_ACCOUNT_ID;
        static const int KEEP_ADMIN_ID;
        static const int USE_SPOTLIGHT_ID;
        static const int VALIDATE_ID;
        static const int CANCEL_ID;
        static const int MIGRATE_ID;
        static const int MIGRATE_PROGRESS_ID;

        static const int LOCAL_USER_NAME_CMD_ID;
        static const int AD_USER_NAME_CMD_ID;
        static const int COPY_RADIO_CMD_ID;
        static const int MOVE_RADIO_CMD_ID;
        static const int VALIDATE_CMD_ID;
        static const int CANCEL_CMD_ID;
        static const int MIGRATE_CMD_ID;
		
    private:

        std::string _localUsersFirstItem;
        std::string _localUserRealName;
        std::string _localUserHomeDir;
        std::string _localUserUID;
        std::string _localUserGID;
        std::string _adUserRealName;
        std::string _adUserHomeDir;
        std::string _adUserUID;
        std::string _adUserGID;
        PUSER_LIST  _pLocalUsers;
};

#endif /* __DOMAINMIGRATEWINDOW_H__ */


