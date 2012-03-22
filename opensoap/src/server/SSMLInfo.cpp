/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SSMLInfo.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <iostream>
#include "SSMLInfo.h"

using namespace OpenSOAP;

SSMLInfo::SSMLInfo()
    : _syncTTL(0)
      , _asyncTTL(0)
      , _hoptimesTTL(0)
      , _port(0)
      , _ssmlType(EXTERNAL_SERVICES) //added 2004/01/04
{
}

const std::string& 
SSMLInfo::getOperationName() const
{
  return _operationName;
}

void 
SSMLInfo::setOperationName(const std::string& operationName)
{
  _operationName = operationName;
}

const std::string& 
SSMLInfo::getNamespace() const
{
  return _namespace;
}

void 
SSMLInfo::setNamespace(const std::string& ns)
{
  _namespace = ns;
}


void 
SSMLInfo::setOperationType(const std::string& operationType)
{
  _operationType = operationType;
}

const std::string& 
SSMLInfo::getOperationType() const
{
  return _operationType;
}

const std::string& 
SSMLInfo::getExecProg() const
{
  return _execProg;
}

void 
SSMLInfo::setExecProg(const std::string& execProg)
{
  _execProg = execProg;
}


unsigned int 
SSMLInfo::getSyncTTL() const
{
  return _syncTTL;
}

void 
SSMLInfo::setSyncTTL(const unsigned int ttl)
{
  _syncTTL = ttl;
}

unsigned int 
SSMLInfo::getAsyncTTL() const
{
  return _asyncTTL;
}

void 
SSMLInfo::setAsyncTTL(const unsigned int ttl)
{
  _asyncTTL = ttl;
}


unsigned int 
SSMLInfo::getTTLHoptimes() const
{
    return _hoptimesTTL;
}

bool 
SSMLInfo::setTTLHoptimes(const unsigned int ttl)
{
    _hoptimesTTL = ttl;
    return true;
}

const std::string& 
SSMLInfo::getConnectionMethod() const
{
  return _connectionMethod;
}

bool 
SSMLInfo::setConnectionMethod(const std::string& connectionMethod)
{
  _connectionMethod = connectionMethod;
  return true;
}

const std::string& 
SSMLInfo::getSendFIFOName() const
{
  return _sendFIFOName;
}

void 
SSMLInfo::setSendFIFOName(const std::string& sendFIFOName)
{
  _sendFIFOName = sendFIFOName;
}

const std::string& 
SSMLInfo::getRecvFIFOName() const
{
  return _recvFIFOName;
}

void 
SSMLInfo::setRecvFIFOName(const std::string& recvFIFOName)
{
  _recvFIFOName = recvFIFOName;
}


const std::string& 
SSMLInfo::getHostname() const
{
  return _hostname;
}

void 
SSMLInfo::setHostname(const std::string& hostname)
{
  _hostname = hostname;
}


unsigned int 
SSMLInfo::getPort() const
{
  return _port;
}


void 
SSMLInfo::setPort(const unsigned int port)
{
  _port = port;
}

//added 2003/06/18
const std::string& 
SSMLInfo::getEndPointUrl() const
{
  return _endPointUrl;
}

void 
SSMLInfo::setEndPointUrl(const std::string& endpointUrl)
{
  _endPointUrl = endpointUrl;
}

//added 2004/01/04
const SSMLType&
SSMLInfo::getSSMLType() const
{
    return _ssmlType;
}

void
SSMLInfo::setSSMLType(const SSMLType ssmlType)
{
    _ssmlType = ssmlType;
}

// End of SSMLInfo.cpp

