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
 *  CredentialsDialog.h
 *  UserMigrate
 *
 *  Created by Sriram Nambakam on 8/13/07.
 *  Copyright (c) BeyondTrust Software. All rights reserved.
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
