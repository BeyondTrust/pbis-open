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

/*
 *  DomainMigrateWindow.h
 *  UserMigrate
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
        void HandleMigration();
        bool HandleValidateUser();
        bool ConfirmMigration(const std::string& localUserName,
                              const std::string& localUserHomeDir,
                              const std::string& adUserName,
                              const std::string& adUserHomeDir,
                              const std::string& adUserUID,
                              const std::string& adUserGID,
                              bool  bMoveProfile);
        int CallMigrateCommand(const std::string& localUserHomeDir,
                               const std::string& adUserName,
                               const std::string& logFileName,
                               bool bMoveProfile,
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


