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
    
        static const int DWORD_DOMAINJOIN_NON_ROOT_USER;
        static const int ERROR_INVALID_COMPUTERNAME;
        static const int ERROR_INVALID_DOMAINNAME;
        static const int ERROR_BAD_FORMAT;
        static const int LW_ERROR_FAILED_TO_LOOKUP_DC;
        static const int LW_ERROR_INVALID_OU;
        static const int LW_ERROR_FAILED_ADMIN_PRIVS;

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
              DomainJoinException::DWORD_DOMAINJOIN_NON_ROOT_USER
              )
        {
        }
};

class InvalidHostnameException : public DomainJoinException
{
    public:
           InvalidHostnameException()
               : DomainJoinException(
                     DomainJoinException::ERROR_INVALID_COMPUTERNAME
                 )
               {
               }
};

class InvalidDomainnameException : public DomainJoinException
{
    public:
           InvalidDomainnameException()
               : DomainJoinException(
                    DomainJoinException::ERROR_INVALID_DOMAINNAME)
           {
           }
};

class InvalidUsernameException : public DomainJoinException
{
    public:
           InvalidUsernameException()
               : DomainJoinException(
                     DomainJoinException::ERROR_BAD_FORMAT
                     )
               {
               }
};

class InvalidOUPathException : public DomainJoinException
{
    public:
           InvalidOUPathException()
               : DomainJoinException(
                     DomainJoinException::LW_ERROR_INVALID_OU
                     )
               {
               }
};

class FailedAdminPrivilegeException : public DomainJoinException
{
    public:
          FailedAdminPrivilegeException()
              : DomainJoinException(
                     DomainJoinException::LW_ERROR_FAILED_ADMIN_PRIVS)
            {
            }
           FailedAdminPrivilegeException(const std::string& errMsg)
               : DomainJoinException(
					 DomainJoinException::LW_ERROR_FAILED_ADMIN_PRIVS,
                     errMsg
                     )
            {
            }
};

class UnresolvedDomainNameException : public DomainJoinException
{
    public:
           UnresolvedDomainNameException()
               : DomainJoinException(DomainJoinException::LW_ERROR_FAILED_TO_LOOKUP_DC)
               {
               }
};

#endif /* __DOMAINJOINEXCEPTION_H__ */


