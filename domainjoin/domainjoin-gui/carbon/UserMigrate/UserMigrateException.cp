/*
 *  UserMigrateException.cpp
 *  UserMigrate
 *
 *  Created by Sriram Nambakam on 8/8/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "UserMigrateException.h"

const int UserMigrateException::CENTERROR_DOMAINJOIN_NON_ROOT_USER          = 524289;
const int UserMigrateException::CENTERROR_DOMAINJOIN_INVALID_HOSTNAME       = 524290;
const int UserMigrateException::CENTERROR_DOMAINJOIN_INVALID_DOMAIN_NAME    = 524323;
const int UserMigrateException::CENTERROR_DOMAINJOIN_INVALID_USERID         = 524322;
const int UserMigrateException::CENTERROR_DOMAINJOIN_UNRESOLVED_DOMAIN_NAME = 524326;
const int UserMigrateException::CENTERROR_DOMAINJOIN_INVALID_OU             = 524334;
const int UserMigrateException::CENTERROR_DOMAINJOIN_FAILED_ADMIN_PRIVS     = 524343;

UserMigrateException::UserMigrateException()
: _errCode(0)
{
}

UserMigrateException::UserMigrateException(
					int errCode,
					const std::string& shortErrMsg,
					const std::string& longErrMsg)
: _errCode(errCode),
  _shortErrorMsg(shortErrMsg),
  _longErrorMsg(longErrMsg)
{
}

