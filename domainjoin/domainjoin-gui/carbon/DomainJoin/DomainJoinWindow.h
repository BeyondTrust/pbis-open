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
 *  DomainJoinWindow.h
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/8/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
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
	
	protected:
	    DomainJoinWindow(const DomainJoinWindow& other);
		DomainJoinWindow& operator=(const DomainJoinWindow& other);
		
	public:

		void SetComputerName(const std::string& name);
		void SetDomainName(const std::string& name);
        
		virtual void Close();

    protected:
    
        virtual Boolean     HandleCommand( const HICommandExtended& inCommand );
		
		std::string GetComputerName();
		std::string GetDomainName();
		std::string GetOUPath();
		
		void HandleJoinDomain();
		void GetCredentials();
		bool ValidateData();
		
		void ValidateDomainname(const std::string& domainName);
		void ValidateUsername(const std::string& userName);
		void ValidateHostname(const std::string& hostName);
		void ValidateOUPath(const std::string& ouPath);
		
		void ShowDomainWelcomeDialog(const std::string& domainName);
		
		TWindow& GetCredentialsDialog();
		
	protected:
	
	    static const int COMPUTER_NAME_ID;
		static const int DOMAIN_NAME_ID;
		static const int DEFAULT_OU_RADIO_ID;
		static const int OU_PATH_RADIO_ID;
		static const int OU_PATH_TEXT_ID;
		static const int CANCEL_ID;
		static const int JOIN_ID;
		
		static const int COMPUTER_NAME_CMD_ID;
		static const int DOMAIN_NAME_CMD_ID;
		static const int DEFAULT_OU_CMD_ID;
		static const int OU_PATH_RADIO_CMD_ID;
		static const int OU_PATH_TEXT_CMD_ID;
		static const int CANCEL_CMD_ID;
		static const int JOIN_CMD_ID;
		
	private:
	
	    std::string _originalComputerName;
	    std::string _userName;
	    std::string _password;
		
		TWindow*    _credentialsDialog;
};

#endif /* __DOMAINJOINWINDOW_H__ */


