/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "config.h"
#include "ctbase.h"

CENTERROR
CTVerifyGID(
    gid_t gid
    )
{
  CENTERROR ceError = CENTERROR_SUCCESS;
  CHAR szBuf[1024];
  struct group group;
  struct group* pResult = NULL;
  
  if (getgrgid_r(gid, &group, szBuf, sizeof(szBuf), &pResult) < 0) {
    ceError = CTMapSystemError(errno);
    BAIL_ON_CENTERIS_ERROR(ceError);
  }

  if (!pResult) {
     ceError = CENTERROR_INVALID_GID;
     BAIL_ON_CENTERIS_ERROR(ceError);
  }
  
 error:
  
  return ceError;
}

CENTERROR
CTGetGID(
    PCSTR pszGID,
    gid_t* pGID
    )
{
  CENTERROR ceError = CENTERROR_SUCCESS;

  if (IsNullOrEmptyString(pszGID)) {
    ceError = CENTERROR_INVALID_GID;
    BAIL_ON_CENTERIS_ERROR(ceError);
  }

  if (CTIsAllDigit(pszGID)) {

    gid_t gid = atoi(pszGID);

    ceError = CTVerifyGID(gid);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *pGID = gid;

  }
  else {

    CHAR szBuf[1024];
    struct group group;
    struct group* pResult = NULL;

    memset(&group, 0, sizeof(struct group));

    if (getgrnam_r(pszGID, &group, szBuf, sizeof(szBuf), &pResult) < 0) {
      ceError = CTMapSystemError(errno);
      BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    if (!pResult) {
       ceError = CENTERROR_INVALID_GID;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *pGID = group.gr_gid;

  }

 error:
   
  return ceError;
}
