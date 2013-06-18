/*
 *  DomainJoinInterface.cpp
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/7/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "DomainJoinInterface.h"

DomainJoinInterface* DomainJoinInterface::_instance = NULL;

DomainJoinInterface::DomainJoinInterface()
: _libHandle(NULL),
  _pDJApiFunctionTable(NULL),
  _pfnShutdownJoinInterface(NULL)
{
}

DomainJoinInterface::~DomainJoinInterface()
{
    Cleanup();
}

DomainJoinInterface&
DomainJoinInterface::getInstance()
{
    if (_instance == NULL)
    {
       _instance = new DomainJoinInterface();
       _instance->Initialize();
    }
    
    return *_instance;
}

void
DomainJoinInterface::Initialize()
{
    const char* LIBDOMAINJOIN = "/opt/pbis/lib/libdomainjoin-mac.so";
    std::string szShortError = "Failed to load domain join interface";
    void* pLibHandle = NULL;
    PFNInitJoinInterface     pfnInitJoinInterface = NULL;
    PFNShutdownJoinInterface pfnShutdownJoinInterface = NULL;
    PDJ_API_FUNCTION_TABLE   pFunctionTable = NULL;

    try
    {
        if (geteuid() != 0)
        {
           throw DomainJoinException(-1,
                                     szShortError,
                                     "Failed to initialize domain join interface due to insufficient privileges");
        }   

        if (getuid() != 0)
        {
           throw DomainJoinException(-1,
                                     szShortError,
                                     "Failed to initialize domain join interface due to insufficient privileges");
        }   

        dlerror();
        pLibHandle = dlopen(LIBDOMAINJOIN, RTLD_GLOBAL|RTLD_NOW);
        if (!pLibHandle)
        {
           std::string errMsg = dlerror();
           throw DomainJoinException(-1,
                                     szShortError,
                                     errMsg);
        }
    
        LoadFunction(pLibHandle, DJ_INITIALIZE_JOIN_INTERFACE, (void**)&pfnInitJoinInterface);
    
        if (pfnInitJoinInterface(&pFunctionTable)) {

           throw DomainJoinException(-1,
                                     szShortError,
                                     "Failed to initialize domain join interface");

        } else if ( !pFunctionTable->pfnJoinDomain ||
                    !pFunctionTable->pfnLeaveDomain ||
                    !pFunctionTable->pfnSetComputerName ||
                    !pFunctionTable->pfnQueryInformation ||
                    !pFunctionTable->pfnIsDomainNameResolvable ||
                    !pFunctionTable->pfnFreeDomainJoinInfo ||
                    !pFunctionTable->pfnFreeDomainJoinError ) {
            throw DomainJoinException(-1,
                                      szShortError,
                                      "The domain join interface is invalid");
        }
       
        LoadFunction(pLibHandle, DJ_SHUTDOWN_JOIN_INTERFACE, (void**)&pfnShutdownJoinInterface);

        Cleanup();

        _libHandle = pLibHandle;
        _pDJApiFunctionTable = pFunctionTable;
        _pfnShutdownJoinInterface = pfnShutdownJoinInterface;
    }
    catch(std::exception& e)
    {
         if (pLibHandle) {
            if (pfnShutdownJoinInterface &&
                pFunctionTable)
            {
               pfnShutdownJoinInterface(pFunctionTable);
            }
            dlclose(pLibHandle);
         }
         throw;
    }
}

void
DomainJoinInterface::Cleanup()
{
  if (_libHandle)
  {
      if (_pfnShutdownJoinInterface) {
          _pfnShutdownJoinInterface(_pDJApiFunctionTable);
      }
  
      dlclose(_libHandle);
      _libHandle = NULL;
      
      _pfnShutdownJoinInterface = NULL;
      _pDJApiFunctionTable = NULL;
  }
}

void
DomainJoinInterface::LoadFunction(
	void*       pLibHandle,
	const char* pszFunctionName,
	void**      functionPointer
	)
{
    void* function;
    
    dlerror();
    
    function = dlsym(pLibHandle, pszFunctionName);
    if (!function)
    {
       std::string errMsg = dlerror();
       throw DomainJoinException(-1,
                                 "Failed to load symbol",
                                 errMsg);
    }
    
    *functionPointer = function;
}

void
DomainJoinInterface::JoinDomain(std::string& pszDomainName,
                                std::string& pszUserName,
                                std::string& pszPassword,
                                std::string& pszOU,
                                std::string& pszUserDomainPrefix,
                                bool bAssumeDefaultDomain,
                                bool bNoHosts)
{
     PDOMAIN_JOIN_ERROR pError = NULL;
     
     int errCode = getInstance()._pDJApiFunctionTable->pfnJoinDomain(
                                    const_cast<char*>(pszDomainName.c_str()),
                                    const_cast<char*>(pszOU.c_str()),
                                    const_cast<char*>(pszUserName.c_str()), 
                                    const_cast<char*>(pszPassword.c_str()), 
                                    const_cast<char*>(pszUserDomainPrefix.c_str()),
                                    bAssumeDefaultDomain,
                                    bNoHosts,
                                    &pError);
    if (pError) {
       DomainJoinException exc(pError->code,
                               pError->pszShortError,
                               pError->pszLongError);
       
       getInstance()._pDJApiFunctionTable->pfnFreeDomainJoinError(pError);
     
       throw exc;  
    }
    
    if (errCode) {
       DomainJoinException exc(errCode, "Domain Join Error", "Failed to join domain");
       throw exc;
    }
}
                                        
void
DomainJoinInterface::LeaveDomain()
{
    PDOMAIN_JOIN_ERROR pError = NULL;
     
    int errCode = getInstance()._pDJApiFunctionTable->pfnLeaveDomain(
                                    NULL,
                                    NULL,
                                    &pError);
    if (pError) {
       DomainJoinException exc(pError->code,
                               pError->pszShortError,
                               pError->pszLongError);
       
       getInstance()._pDJApiFunctionTable->pfnFreeDomainJoinError(pError);
     
       throw exc;  
    }
    
    if (errCode) {
       DomainJoinException exc(-1, "Domain Join Error", "Failed to leave domain");
       throw exc;
    }
}

bool
DomainJoinInterface::IsDomainNameResolvable(const std::string& domainName)
{
    PDOMAIN_JOIN_ERROR pError = NULL;
    short bResolvable = 0;
     
    int errCode = getInstance()._pDJApiFunctionTable->pfnIsDomainNameResolvable(
                                    const_cast<char*>(domainName.c_str()),
                                    &bResolvable,
                                    &pError);
                                    
    if (pError) {
       DomainJoinException exc(pError->code,
                               pError->pszShortError,
                               pError->pszLongError);
       
       getInstance()._pDJApiFunctionTable->pfnFreeDomainJoinError(pError);
     
       throw exc;  
    }
    
    if (errCode) {
       DomainJoinException exc(errCode, "Domain Join Error", "Failed to determine if domain name is resolvable through DNS");
       throw exc;
    }
    
    return bResolvable;
}
        
void
DomainJoinInterface::SetComputerName(std::string& pszComputerName,
                                     std::string& pszDomainName)
{
    PDOMAIN_JOIN_ERROR pError = NULL;

    int errCode = getInstance()._pDJApiFunctionTable->pfnSetComputerName(
                                    const_cast<char*>(pszComputerName.c_str()),
                                    const_cast<char*>(pszDomainName.c_str()),
                                    &pError);
                                    
    if (pError) {
       DomainJoinException exc(pError->code,
                               pError->pszShortError,
                               pError->pszLongError);
       
       getInstance()._pDJApiFunctionTable->pfnFreeDomainJoinError(pError);
     
       throw exc;  
    }
    
    if (errCode) {
       DomainJoinException exc(errCode, "Domain Join Error", "Failed to set the computer name");
       throw exc;
    }
}

void
DomainJoinInterface::GetDomainJoinStatus(DomainJoinStatus& joinStatus)
{
    PDOMAIN_JOIN_INFO pInfo = NULL;
    PDOMAIN_JOIN_ERROR pError = NULL;

    int errCode = getInstance()._pDJApiFunctionTable->pfnQueryInformation(
                                    &pInfo,
                                    &pError);
                                    
    if (pError) {
       DomainJoinException exc(pError->code,
                               pError->pszShortError,
                               pError->pszLongError);
       
       getInstance()._pDJApiFunctionTable->pfnFreeDomainJoinError(pError);
     
       throw exc;  
    }
    
    if (errCode) {
       DomainJoinException exc(errCode, "Domain Join Error", "Failed to query domain join status");
       throw exc;
    }
    
    joinStatus.Name = (pInfo->pszName ? pInfo->pszName : "");
    joinStatus.DnsDomain = (pInfo->pszDnsDomain ? pInfo->pszDnsDomain : "");
    joinStatus.DomainName = (pInfo->pszDomainName ? pInfo->pszDomainName : "");
    joinStatus.ShortDomainName = (pInfo->pszDomainShortName ? pInfo->pszDomainShortName : "");
    joinStatus.LogFilePath = (pInfo->pszLogFilePath ? pInfo->pszLogFilePath : "");
    joinStatus.OUPath = (pInfo->pszOU ? pInfo->pszOU : "");
    
    getInstance()._pDJApiFunctionTable->pfnFreeDomainJoinInfo(pInfo);
}
