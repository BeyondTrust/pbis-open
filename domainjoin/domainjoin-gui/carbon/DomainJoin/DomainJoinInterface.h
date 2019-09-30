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
 *  DomainJoinInterface.h
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/7/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __DOMAINJOININTERFACE_H__
#define __DOMAINJOININTERFACE_H__

#include <Carbon/Carbon.h>

#include <string>
#include <dlfcn.h>

#include "DomainJoinStatus.h"
#include "DomainJoinException.h"
#include "djitf.h"

class DomainJoinInterface
{  
    private:
       DomainJoinInterface();
       ~DomainJoinInterface();
          
    public:
    
        static void JoinDomain(std::string& szDomainName,
                               std::string& pszUserName,
                               std::string& pszPassword,
                               std::string& pszOU,
                               std::string& pszUserDomainPrefix,
                               bool bAssumeDefaultDomain,
                               bool bNoHosts);
        
        static void LeaveDomain();
        
        static void SetComputerName(std::string& pszComputerName,
                                    std::string& pszDomainName);
        
        static void GetDomainJoinStatus(DomainJoinStatus& joinStatus);
		
		static bool IsDomainNameResolvable(const std::string& domainName);
        
    protected:
    
        static DomainJoinInterface& getInstance();
        
        void Initialize();
        void Cleanup();
        
        static void LoadFunction(
						void*       pLibHandle,
						const char* pszFunctionName,
						void**      FunctionPointer
						);
        
    private:
    
        static DomainJoinInterface* _instance;
    
        void* _libHandle;
		PDJ_API_FUNCTION_TABLE _pDJApiFunctionTable;
        PFNShutdownJoinInterface _pfnShutdownJoinInterface;
};

#endif /* __DOMAINJOININTERFACE_H__ */
