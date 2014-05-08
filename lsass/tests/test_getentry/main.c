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

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwsecurityidentifier.h"
#include "lsautils.h"
#include "lsaclient.h"
#include <usersec.h>

#include <stdio.h>
#include <syslog.h>

static
void
ShowUsage()
{
printf("\
Usage: \n\n\
test-getentry getuserattr <user-name> <userattr> \n\
<userattr> = id | pgid | password | home | shell | registry | gecos | pgrp | groups | groupsids | account_locked | SID \n\n");
}

static
void
ValidateArgs(
    int   argc,
    char* argv[]
    )
{
    PSTR pszCommand = NULL;

    if( argc!=4 ) {
        ShowUsage();
        exit(0);
    }

    pszCommand = argv[1];
    if( strcmp(pszCommand, "getuserattr")!=0 ) {
        ShowUsage();
        exit(0);
    }
}

static
PSTR
GetUserAttribAsString (
    PSTR pszUname,
    PSTR pszAttr,
    int  type,
    int  *errNo
    ) 
{
    PSTR value = NULL;
    
    if( type==SEC_INT )  {

        int  ret;
        char arr[10];

        *errNo = getuserattr (pszUname, pszAttr, &ret, type);

        sprintf(arr, "%i", ret);
        LwAllocateString(arr, &value);

        return value;
    }

    if( type==SEC_CHAR ) {
        *errNo = getuserattr (pszUname, pszAttr, &value, type);
        return value;
    }

    if( type==SEC_LIST ) {
        PSTR pszList = NULL;
        PSTR ch      = NULL;

        *errNo = getuserattr (pszUname, pszAttr, &pszList, type);
        ch = pszList;
        while(1) {
            if(*ch=='\0') {
                if( *(ch+1)=='\0' )
                    break;
                else
                    *ch = ',';
            }
            ch++;
        }
        return pszList;
    }

    if( type==SEC_BOOL ) {
        int ret;
        *errNo = getuserattr (pszUname, pszAttr, &ret, type);
        return (0==ret)? "FALSE" : "TRUE";
    }
}

static
int
GetType(
    PSTR pszAttribute
    )
{
    if( !strcmp(pszAttribute, "id")   ||
        !strcmp(pszAttribute, "pgid") ) {
            return SEC_INT;
    }

    if( !strcmp(pszAttribute, "password") ||
        !strcmp(pszAttribute, "home")     ||
        !strcmp(pszAttribute, "shell")    ||
        !strcmp(pszAttribute, "registry") ||
        !strcmp(pszAttribute, "gecos")    ||
        !strcmp(pszAttribute, "pgrp")     ||
        !strcmp(pszAttribute, "SID") ) {
            return SEC_CHAR;
    }

    if( !strcmp(pszAttribute, "groups") ) {
        return SEC_LIST;
    }

    if( !strcmp(pszAttribute, "account_locked") ) {
        return SEC_BOOL;
    }
}

void
PrintUserAttrib(PSTR pszUname, PSTR pszAttr, PSTR pszValue) {
    printf("GetUserAttr:\n");
    printf("============\n");
    printf("User Name: %s\n", pszUname);
    printf("%s: %s\n" , pszAttr, LW_IS_NULL_OR_EMPTY_STR(pszValue) ? "<null>" : pszValue);
}

void
DoGetUserAttr(
    int   argc,
    char* argv[]
    )
{
    PSTR pszValue = NULL;
    PSTR pszUname = argv[2];
    PSTR pszAttr  = argv[3];
    int  type     = GetType( pszAttr );
    int  errNo    = 0;
  
    pszValue = GetUserAttribAsString(pszUname, pszAttr, type, &errNo);

    PrintUserAttrib(pszUname, pszAttr, pszValue);
}



int
main(
    int   argc,
    char* argv[]
    )
{
    PSTR pszCommand = NULL;

    ValidateArgs(argc, argv);

    pszCommand = argv[1];

    if( strcmp(pszCommand, "getuserattr")==0 ) {
        DoGetUserAttr(argc, argv);
    }
}
