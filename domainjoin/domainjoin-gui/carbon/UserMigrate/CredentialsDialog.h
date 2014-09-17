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
 *  CredentialsDialog.h
 *  UserMigrate
 *
 *  Created by Sriram Nambakam on 8/13/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */
#ifndef __CREDENTIALSDIALOG_H__
#define __CREDENTIALSDIALOG_H__

#include "TSheet.h"

#include <Carbon/Carbon.h>
#include <string>

class CredentialsDialog : public TSheet
{
    public:
	     CredentialsDialog(int inAppSignature, TWindow& parentWindow);
		 virtual ~CredentialsDialog() {}
	
	protected:
	     CredentialsDialog(const CredentialsDialog& other);
		 CredentialsDialog& operator=(const CredentialsDialog& other);
		 
	public:
		 		 
	     void SetUsername(const std::string& userName);
		 void SetPassword(const std::string& password);
		 
		 std::string GetUsername();
		 std::string GetPassword();
		 
	protected:
	
	     virtual Boolean HandleCommand(const HICommandExtended& inCommand);
		
	protected:
	
	    static const int USERNAME_ID;
		static const int PASSWORD_ID;
		static const int CANCEL_BUTTON_ID;
		static const int OK_BUTTON_ID;
		
		static const int CANCEL_CMD_ID;
		static const int OK_CMD_ID;
};

#endif /* ___CREDENTIALSDIALOG_H__ */


