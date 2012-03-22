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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        utils.c
 *
 * Abstract:
 *
 *        Likewise System NET Utilities
 *
 *        Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"

BOOLEAN
LwNetCheckUnsignedInteger(
    PCSTR pszIntegerCandidate
    )
{
    typedef enum {
        PARSE_MODE_LEADING_SPACE = 0,
        PARSE_MODE_INTEGER,
        PARSE_MODE_TRAILING_SPACE
    } ParseMode;

    ParseMode parseMode = PARSE_MODE_LEADING_SPACE;
    BOOLEAN bIsUnsignedInteger = TRUE;
    INT iLength = 0;
    INT iCharIdx = 0;
    CHAR cNext = '\0';

    if (LW_IS_NULL_OR_EMPTY_STR(pszIntegerCandidate))
    {
        bIsUnsignedInteger = FALSE;
        goto error;
    }

    iLength = strlen(pszIntegerCandidate);

    do {

      cNext = pszIntegerCandidate[iCharIdx++];

      switch(parseMode) {

          case PARSE_MODE_LEADING_SPACE:
          {
              if (isdigit((int)cNext))
              {
                  parseMode = PARSE_MODE_INTEGER;
              }
              else if (!isspace((int)cNext))
              {
                  bIsUnsignedInteger = FALSE;
              }
              break;
          }

          case PARSE_MODE_INTEGER:
          {
              if (isspace((int)cNext))
              {
                  parseMode = PARSE_MODE_TRAILING_SPACE;
              }
              else if (!isdigit((int)cNext))
              {
                  bIsUnsignedInteger = FALSE;
              }
              break;
          }

          case PARSE_MODE_TRAILING_SPACE:
          {
              if (!isspace((int)cNext))
              {
                  bIsUnsignedInteger = FALSE;
              }
              break;
          }
      }

    } while (iCharIdx < iLength && bIsUnsignedInteger == TRUE);


error:

    return bIsUnsignedInteger;
}

DWORD
LwNetAppendStringArray(
    PDWORD pdwCount,
    PWSTR** pppwszArray,
    PWSTR pwszString
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszNewArray = NULL;

    dwError = LwReallocMemory(
        *pppwszArray,
        OUT_PPVOID(&ppwszNewArray),
        sizeof(*ppwszNewArray) * (*pdwCount + 1));
    BAIL_ON_LTNET_ERROR(dwError);

    ppwszNewArray[(*pdwCount)++] = pwszString;

    *pppwszArray = ppwszNewArray;

error:

    return dwError;
}
