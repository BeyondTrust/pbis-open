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
 *  main.h
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/9/07.
 *  Copyright (c) BeyondTrust Software. All rights reserved.
 *
 */
#ifndef __MAIN_H__
#define __MAIN_H__

#include <Carbon/Carbon.h>
#include <Security/Security.h>

#include "DomainMigrateWindow.h"
#include "DomainJoinWindow.h"
#include "DomainLeaveWindow.h"
#include "MainWindow.h"

class DomainJoinApp : public TApplication
{
    public:

		DomainJoinApp();
        virtual ~DomainJoinApp();

	protected:

		DomainJoinApp(const DomainJoinApp& other);
		DomainJoinApp& operator=(const DomainJoinApp& other);

	public:

	    virtual void Run();

    protected:

        virtual Boolean     HandleCommand( const HICommandExtended& inCommand );

		void JoinOrLeaveDomain();
        void MigrateUser();

		DomainJoinWindow& GetJoinWindow();
		DomainLeaveWindow& GetLeaveWindow();
        DomainMigrateWindow& GetMigrateWindow();

        void EnsureAuthorization();

		void FixProcessEnvironment();

	protected:

	    static const int ApplicationSignature;

	private:

	    MainWindow*          _mainWindow;
	    DomainJoinWindow*    _joinWindow;
		DomainLeaveWindow*   _leaveWindow;
        DomainMigrateWindow* _migrateWindow;

		char* _envPath;
};

#endif /* __MAIN_H__ */
