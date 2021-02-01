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
 *  DomainJoinWindow.h
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/8/07.
 *  Copyright (c) BeyondTrust Software. All rights reserved.
 *
 */
#ifndef __DOMAINJOINWINDOW_H__
#define __DOMAINJOINWINDOW_H__

#include <Carbon/Carbon.h>

#include "TApplication.h"
#include "TWindow.h"

#include <string.h>

class DomainJoinWindow : public TWindow
{
    public:
        DomainJoinWindow(int inAppSignature);
        virtual ~DomainJoinWindow();
        void SetComputerName(const std::string& name);
        void SetDomainName(const std::string& name);
        virtual void Close();

    protected:
        DomainJoinWindow(const DomainJoinWindow& other);
        DomainJoinWindow& operator=(const DomainJoinWindow& other);
        virtual Boolean     HandleCommand( const HICommandExtended& inCommand );
        std::string GetComputerName();
        std::string GetDomainName();
        std::string GetOUPath();
        std::string GetUserDomainPrefix();

        void HandleJoinDomain();
        void GetCredentials();
        bool ValidateData();
        void ValidateDomainname(const std::string& domainName);
        void ValidateUsername(const std::string& userName);
        void ValidateHostname(const std::string& hostName);
        void ValidateOUPath(const std::string& ouPath);
        void ShowDomainWelcomeDialog(const std::string& domainName);
        TWindow& GetCredentialsDialog();

        static const int COMPUTER_NAME_ID;
        static const int DOMAIN_NAME_ID;
        static const int DEFAULT_OU_RADIO_ID;
        static const int OU_PATH_RADIO_ID;
        static const int OU_PATH_TEXT_ID;
        static const int CANCEL_ID;
        static const int JOIN_ID;
        static const int DEFAULT_USER_DOMAIN_PREFIX_ID;
        static const int USE_SHORT_NAME_FOR_LOGON_ID;
        static const int COMPUTER_NAME_CMD_ID;
        static const int DOMAIN_NAME_CMD_ID;
        static const int DEFAULT_OU_CMD_ID;
        static const int OU_PATH_RADIO_CMD_ID;
        static const int OU_PATH_TEXT_CMD_ID;
        static const int CANCEL_CMD_ID;
        static const int JOIN_CMD_ID;
        static const int USE_SHORT_NAME_CMD_ID;

    private:

        std::string _originalComputerName;
        std::string _userName;
        std::string _password;
        TWindow*    _credentialsDialog;
};

#endif /* __DOMAINJOINWINDOW_H__ */
