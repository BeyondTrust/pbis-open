/*
 *  DomainJoinException.cpp
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/8/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "DomainJoinException.h"

const int DomainJoinException::ERROR_INVALID_COMPUTERNAME       = 1210;
const int DomainJoinException::ERROR_INVALID_DOMAINNAME    = 1212;
const int DomainJoinException::ERROR_BAD_FORMAT         = 11;
const int DomainJoinException::LW_ERROR_FAILED_TO_LOOKUP_DC = 40108;
const int DomainJoinException::LW_ERROR_INVALID_OU             = 42500;
const int DomainJoinException::LW_ERROR_FAILED_ADMIN_PRIVS     = 42508;

DomainJoinException::DomainJoinException()
: _errCode(0)
{
}

DomainJoinException::DomainJoinException(
					int errCode,
					const std::string& shortErrMsg,
					const std::string& longErrMsg)
: _errCode(errCode),
  _shortErrorMsg(shortErrMsg),
  _longErrorMsg(longErrMsg)
{
}

