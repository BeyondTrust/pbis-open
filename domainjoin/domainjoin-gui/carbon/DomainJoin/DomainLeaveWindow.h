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
 *  DomainLeaveWindow.h
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/8/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
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


