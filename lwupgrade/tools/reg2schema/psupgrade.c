/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name: psupgrade (pstore v6.0 -> 6.1+ registry upgrade program)
 *
 * Abstract:
 * Program to move Pstore registry entries from 6.0 Pstore\Default location
 * to per-domain location. 6.1 supports joining multiple domains, but
 * there is only one default domain in this upgrade scenario. Move Pstore
 * entries to a subkey under that domainname, and set domain Default entry.
 * 
 * Authors:
 *     Adam Bernstein (abernstein@likewise.com)
 */


#include "includes.h"


static
DWORD
ParsePstoreSections(
    FILE *fp,
    PSTR domainDnsName)
{

    CHAR cBuf[1024] = {0};
    PSTR pszStr = NULL;
    PSTR pszPrintBuf = NULL;
    PSTR pszNewHkey = NULL;
    DWORD dwLen = 0;
    DWORD state = 0;
    BOOLEAN bWroteDefaultDomain = FALSE;

    while (!feof(fp))
    {
        pszStr = fgets(cBuf, sizeof(cBuf)-1, fp);
        if (pszStr)
        {
            pszPrintBuf = cBuf;

            /* Search for line: [HKEY_THIS_MACHINE.*Pstore.*] */
            if (cBuf[0] == '[' && (pszStr = strstr(cBuf, "Pstore")))
            {
                state = 1;

                /* 
                 * Ignore line: [HKEY_THIS_MACHINE\Services.*Pstore] 
                 * The location of the default values has not changed
                 */
                if (pszStr[strlen("Pstore")] == ']')
                {
                    state = 0;
                }

                if (state)
                {
                   /*
                    * transform these:
                    * [HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ \
                    *  ActiveDirectory\Pstore\Default]
                    * [HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ \
                    *  ActiveDirectory\Pstore\Default\MachinePassword]
                    * into these:
                    * [HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ \
                    *  ActiveDirectory\DomainJoin\domainDnsName\Pstore]
                    * [HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ \
                    * ActiveDirectory\DomainJoin\domainDnsName\Pstore\MachinePassword]
                    */

                    dwLen = strlen(cBuf) + sizeof("DomainJoin") + 
                          sizeof("MachinePassword") + strlen(domainDnsName);
                    pszNewHkey = (char *) calloc(dwLen, sizeof(char));
                    if (pszNewHkey)
                    {
                        strcpy(pszNewHkey, cBuf);
                        pszStr = strstr(pszNewHkey, "Pstore");
                        *pszStr = '\0';
                        pszStr = strstr(cBuf, "Default");
                        if (pszStr)
                        {
                            pszStr += strlen("Default");
                        }
                        strcat(pszNewHkey, "DomainJoin");

                        if (!bWroteDefaultDomain)
                        {
                            /* Emit subkey that contains default domain */
                            printf("%s]\n", pszNewHkey);
                            printf("\"Default\"=\"%s\"\n\n", domainDnsName);
                            bWroteDefaultDomain = 1;
                        }

                        /* Subkeys that contain per-domain Pstore information */
                        strcat(pszNewHkey, "\\");
                        strcat(pszNewHkey, domainDnsName);
                        strcat(pszNewHkey, "\\Pstore");
                        if (pszStr && *pszStr == '\\')
                        {
                            strcat(pszNewHkey, pszStr);
                        }
                        else
                        {
                            strcat(pszNewHkey, "]\r\n");
                        }
                        pszPrintBuf = pszNewHkey;
                    }
                }
            }
        
            if (state)
            {
                printf("%s", pszPrintBuf);
                if (cBuf[0] == '\r' || cBuf[0] == '\n')
                {
                    printf("\n");
                    state = 0;
                }
            }
        }
        LW_SAFE_FREE_STRING(pszNewHkey);
    }
    return 0;
}


/*
 * Find "DomainDnsName" in current registry export. It is assumed this 
 * only occurs once and under a Pstore subkey. This could break should 
 * this valueName be duplicated under another subkey.
 */
static
PSTR
FindDomain(
    FILE *fp)
{
    CHAR cBuf[1024] = {0};
    PSTR pszStr = NULL;
    PSTR pszRetDomain = NULL;

    while (!feof(fp))
    {
        pszStr = fgets(cBuf, sizeof(cBuf)-1, fp);
        if (pszStr)
        {
            pszStr = strstr(cBuf, "DomainDnsName");
            if (pszStr)
            {
                pszStr = strchr(pszStr, '=');
                if (pszStr)
                {
                    pszStr++;
                    if (*pszStr == '"')
                    {
                        pszStr++;
                        pszRetDomain = strdup(pszStr);
                        if (!pszRetDomain)
                        {
                            return NULL;
                        }
                        pszStr = pszRetDomain;
                        pszStr = strchr(pszStr, '"');
                        if (pszStr)
                        {
                            *pszStr = '\0';
                            break;
                        }
                    }
                }
            }
        }
    }
    return pszRetDomain;
}


int main(int argc, char *argv[])
{
    FILE *fp = NULL;
    PSTR pszDomainDnsName = NULL;

    if (argc == 1)
    {
        fprintf(stderr, "usage: %s registry.txt\n", argv[0]);
        return 0;
    }

    fp = fopen(argv[1], "r");
    if (!fp)
    {
        return 1;
    }
    
    pszDomainDnsName = FindDomain(fp);
    if (!pszDomainDnsName)
    {
        return 1;
    }
    rewind(fp);

    ParsePstoreSections(fp, pszDomainDnsName);
    LW_SAFE_FREE_STRING(pszDomainDnsName);
    
    fclose(fp);
    return 0;
}
