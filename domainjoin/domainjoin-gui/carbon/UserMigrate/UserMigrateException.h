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
 *  UserMigrateException.h
 *  UserMigrate
 *
 *  Created by Chuck Mount on 8/8/07.
 *  Copyright (c) BeyondTrust Software. All rights reserved.
 *
 */

#ifndef __USERMIGRATEEXCEPTION_H__
#define __USERMIGRATEEXCEPTION_H__

#include <Carbon/Carbon.h>
#include <string>
#include <exception>

class UserMigrateException : public std::exception
{
    public:

        UserMigrateException();
        UserMigrateException(int errCode,
                            const std::string& shortErrMsg="Domain Join Error",
                            const std::string& longErrMsg="Internal Error");
        virtual ~UserMigrateException() throw() {}

    public:

        inline int GetErrorCode() { return _errCode; }
        inline void SetErrorCode(int code) { _errCode = code; }

        virtual const char* what() const throw()
        {
                return _shortErrorMsg.c_str();
        }

        virtual const char* GetLongErrorMessage() const throw()
        {
                return _longErrorMsg.c_str();
        }

    public:

        static const int ERROR_INVALID_COMPUTERNAME;
        static const int ERROR_INVALID_DOMAINNAME;
        static const int ERROR_BAD_FORMAT;
        static const int LW_ERROR_FAILED_TO_LOOKUP_DC;
        static const int LW_ERROR_INVALID_OU;
        static const int LW_ERROR_FAILED_ADMIN_PRIVS;

    private:

        std::string _shortErrorMsg;
        std::string _longErrorMsg;
        int _errCode;

};

class InvalidHostnameException : public UserMigrateException
{
    public:
           InvalidHostnameException()
               : UserMigrateException(
                     UserMigrateException::ERROR_INVALID_COMPUTERNAME
                 )
               {
               }
};

class InvalidDomainnameException : public UserMigrateException
{
    public:
           InvalidDomainnameException()
               : UserMigrateException(
                    UserMigrateException::ERROR_INVALID_DOMAINNAME)
           {
           }
};

class InvalidUsernameException : public UserMigrateException
{
    public:
           InvalidUsernameException()
               : UserMigrateException(
                     UserMigrateException::ERROR_BAD_FORMAT
                     )
               {
               }
};

class InvalidOUPathException : public UserMigrateException
{
    public:
           InvalidOUPathException()
               : UserMigrateException(
                     UserMigrateException::LW_ERROR_INVALID_OU
                     )
               {
               }
};

class FailedAdminPrivilegeException : public UserMigrateException
{
    public:
          FailedAdminPrivilegeException()
              : UserMigrateException(
                     UserMigrateException::LW_ERROR_FAILED_ADMIN_PRIVS)
            {
            }
           FailedAdminPrivilegeException(const std::string& errMsg)
               : UserMigrateException(
					 UserMigrateException::LW_ERROR_FAILED_ADMIN_PRIVS,
                     errMsg
                     )
            {
            }
};

class UnresolvedDomainNameException : public UserMigrateException
{
    public:
           UnresolvedDomainNameException()
               : UserMigrateException(UserMigrateException::LW_ERROR_FAILED_TO_LOOKUP_DC)
               {
               }
};

#endif /* __USERMIGRATEEXCEPTION_H__ */
