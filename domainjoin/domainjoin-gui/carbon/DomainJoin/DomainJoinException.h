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
 *  DomainJoinException.h
 *  DomainJoin
 *
 *  Created by Chuck Mount on 8/8/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __DOMAINJOINEXCEPTION_H__
#define __DOMAINJOINEXCEPTION_H__

#include <Carbon/Carbon.h>
#include <string>
#include <exception>

class DomainJoinException : public std::exception
{
    public:
    
        DomainJoinException();
        DomainJoinException(int errCode,
                            const std::string& shortErrMsg="Domain Join Error",
                            const std::string& longErrMsg="Internal Error");
        virtual ~DomainJoinException() throw() {}
    
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
    
        int _errCode;
        std::string _shortErrorMsg;
        std::string _longErrorMsg;

};

class NonRootUserException : public DomainJoinException
{
    public:
        NonRootUserException()
        : DomainJoinException(
              DomainJoinException::CENTERROR_DOMAINJOIN_NON_ROOT_USER
              )
        {
        }
};

class InvalidHostnameException : public DomainJoinException
{
    public:
           InvalidHostnameException()
               : DomainJoinException(
                     DomainJoinException::CENTERROR_DOMAINJOIN_INVALID_HOSTNAME
                 )
               {
               }
};

class InvalidDomainnameException : public DomainJoinException
{
    public:
           InvalidDomainnameException()
               : DomainJoinException(
                    DomainJoinException::CENTERROR_DOMAINJOIN_INVALID_DOMAIN_NAME)
           {
           }
};

class InvalidUsernameException : public DomainJoinException
{
    public:
           InvalidUsernameException()
               : DomainJoinException(
                     DomainJoinException::CENTERROR_DOMAINJOIN_INVALID_USERID
                     )
               {
               }
};

class InvalidOUPathException : public DomainJoinException
{
    public:
           InvalidOUPathException()
               : DomainJoinException(
                     DomainJoinException::CENTERROR_DOMAINJOIN_INVALID_OU
                     )
               {
               }
};

class FailedAdminPrivilegeException : public DomainJoinException
{
    public:
          FailedAdminPrivilegeException()
              : DomainJoinException(
                     DomainJoinException::CENTERROR_DOMAINJOIN_FAILED_ADMIN_PRIVS)
            {
            }
           FailedAdminPrivilegeException(const std::string& errMsg)
               : DomainJoinException(
					 DomainJoinException::CENTERROR_DOMAINJOIN_FAILED_ADMIN_PRIVS,
                     errMsg
                     )
            {
            }
};

class UnresolvedDomainNameException : public DomainJoinException
{
    public:
           UnresolvedDomainNameException()
               : DomainJoinException(DomainJoinException::CENTERROR_DOMAINJOIN_UNRESOLVED_DOMAIN_NAME)
               {
               }
};

#endif /* __DOMAINJOINEXCEPTION_H__ */


