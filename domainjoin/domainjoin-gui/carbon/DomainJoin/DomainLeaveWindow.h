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
 *  DomainLeaveWindow.h
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/8/07.
 *  Copyright (c) BeyondTrust Software. All rights reserved.
 *
 */

#ifndef __DOMAINLEAVEWINDOW_H__
#define __DOMAINLEAVEWINDOW_H__

#include <Carbon/Carbon.h>
#include <string>

#include "TWindow.h"

class DomainLeaveWindow : public TWindow
{
    public:
	DomainLeaveWindow(int inAppSignature) : TWindow( inAppSignature, CFSTR("Window"), CFSTR("Leave") ) {}
    virtual  ~DomainLeaveWindow() {}

    protected:
    DomainLeaveWindow(const DomainLeaveWindow& other);
    DomainLeaveWindow& operator=(const DomainLeaveWindow& other);

    public:
    void SetComputerName(const std::string& name);
    void SetDomainName(const std::string& name);
    void SetOU(const std::string& ou);
    virtual void Close();

    protected:
    virtual Boolean     HandleCommand( const HICommandExtended& inCommand );
	std::string GetComputerName();
	std::string GetDomainName();
	bool ConfirmLeave(const std::string& domainName);
	void HandleLeaveDomain();
    void HandleMigrateUser();
    void ShowLeftDomainDialog(const std::string& domainName);

    protected:

	static const int COMPUTER_NAME_ID;
	static const int DOMAIN_NAME_ID;
    static const int OU_ID;
	static const int LEAVE_ID;
	static const int CLOSE_ID;
    static const int MIGRATE_ID;

	static const int COMPUTER_NAME_CMD_ID;
	static const int DOMAIN_NAME_CMD_ID;
	static const int LEAVE_CMD_ID;
    static const int MIGRATE_CMD_ID;
	static const int CLOSE_CMD_ID;
};

#endif /* __DOMAINLEAVEWINDOW_H__ */
