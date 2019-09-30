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
