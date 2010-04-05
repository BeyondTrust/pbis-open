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
 *  UserMigrateException.h
 *  UserMigrate
 *
 *  Created by Chuck Mount on 8/8/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
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
    
        static const int CENTERROR_DOMAINJOIN_NON_ROOT_USER;
        static const int CENTERROR_DOMAINJOIN_INVALID_HOSTNAME;
        static const int CENTERROR_DOMAINJOIN_INVALID_DOMAIN_NAME;
        static const int CENTERROR_DOMAINJOIN_INVALID_USERID;
        static const int CENTERROR_DOMAINJOIN_UNRESOLVED_DOMAIN_NAME;
        static const int CENTERROR_DOMAINJOIN_INVALID_OU;
        static const int CENTERROR_DOMAINJOIN_FAILED_ADMIN_PRIVS;

    private:
    
        std::string _shortErrorMsg;
        std::string _longErrorMsg;
        int _errCode;

};

class NonRootUserException : public UserMigrateException
{
    public:
        NonRootUserException()
        : UserMigrateException(
              UserMigrateException::CENTERROR_DOMAINJOIN_NON_ROOT_USER
              )
        {
        }
};

class InvalidHostnameException : public UserMigrateException
{
    public:
           InvalidHostnameException()
               : UserMigrateException(
                     UserMigrateException::CENTERROR_DOMAINJOIN_INVALID_HOSTNAME
                 )
               {
               }
};

class InvalidDomainnameException : public UserMigrateException
{
    public:
           InvalidDomainnameException()
               : UserMigrateException(
                    UserMigrateException::CENTERROR_DOMAINJOIN_INVALID_DOMAIN_NAME)
           {
           }
};

class InvalidUsernameException : public UserMigrateException
{
    public:
           InvalidUsernameException()
               : UserMigrateException(
                     UserMigrateException::CENTERROR_DOMAINJOIN_INVALID_USERID
                     )
               {
               }
};

class InvalidOUPathException : public UserMigrateException
{
    public:
           InvalidOUPathException()
               : UserMigrateException(
                     UserMigrateException::CENTERROR_DOMAINJOIN_INVALID_OU
                     )
               {
               }
};

class FailedAdminPrivilegeException : public UserMigrateException
{
    public:
          FailedAdminPrivilegeException()
              : UserMigrateException(
                     UserMigrateException::CENTERROR_DOMAINJOIN_FAILED_ADMIN_PRIVS)
            {
            }
           FailedAdminPrivilegeException(const std::string& errMsg)
               : UserMigrateException(
					 UserMigrateException::CENTERROR_DOMAINJOIN_FAILED_ADMIN_PRIVS,
                     errMsg
                     )
            {
            }
};

class UnresolvedDomainNameException : public UserMigrateException
{
    public:
           UnresolvedDomainNameException()
               : UserMigrateException(UserMigrateException::CENTERROR_DOMAINJOIN_UNRESOLVED_DOMAIN_NAME)
               {
               }
};

#endif /* __USERMIGRATEEXCEPTION_H__ */


