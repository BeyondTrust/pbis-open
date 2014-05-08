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

#include "domainjoin.h"
#include "djparsehosts.h"

#if defined(__LWI_DARWIN__)
static
DWORD
RunSilentWithStatus(
    PCSTR Command,
    PSTR* Args,
    PLONG Status
    )
{
    DWORD ceError = ERROR_SUCCESS;
    int EE = 0;
    LONG status = 0;
    PPROCINFO procInfo = NULL;

    ceError = DJSpawnProcessSilent(Command, Args, &procInfo);
    GOTO_CLEANUP_ON_DWORD_EE(ceError, EE);

    ceError = DJGetProcessStatus(procInfo, &status);
    GOTO_CLEANUP_ON_DWORD_EE(ceError, EE);

cleanup:
    if (procInfo)
    {
        FreeProcInfo(procInfo);
    }

    *Status = status;

    if (EE || ceError)
    {
        DJ_LOG_VERBOSE("RunSiledWithStatus LEAVE -> 0x%08x (EE = %d)", ceError, EE);
    }

    return ceError;
}


static
void
KickLookupd(
    )
{
    DWORD ceError = ERROR_SUCCESS;
    int EE = 0;
    char stopCommand[] = "/usr/bin/killall";
    PSTR stopArgs[3] = { stopCommand, "lookupd" };
#if 0
    char startCommand[] = "/usr/sbin/lookupd";
    PSTR startArgs[2] = { startCommand };
#endif
    LONG status = 0;

    ceError = RunSilentWithStatus(stopCommand, stopArgs, &status);
    GOTO_CLEANUP_ON_DWORD_EE(ceError, EE);
    if (status != 0)
    {
        DJ_LOG_ERROR("%s failed [Status code: %d]", stopCommand, status);
        goto cleanup;
    }

    // We do not technically need to restart it since it will restart on
    // demand.
#if 0
    ceError = RunSilentWithStatus(startCommand, startArgs, &status);
    GOTO_CLEANUP_ON_DWORD_EE(ceError, EE);
    if (status != 0)
    {
        DJ_LOG_ERROR("%s failed [Status code: %d]", startCommand, status);
    }
#endif

cleanup:
    DJ_LOG_VERBOSE("KickLookupd LEAVE -> 0x%08x (EE = %d)", ceError, EE);
}
#endif

static
void
DJFreeAlias(
    PHOSTFILEALIAS pAlias
    )
{
    if (pAlias->pszAlias)
        CTFreeString(pAlias->pszAlias);
    CTFreeMemory(pAlias);
}

static
void
DJFreeAliasList(
    PHOSTFILEALIAS pAliasList
    )
{
    PHOSTFILEALIAS pAlias = NULL;
    while (pAliasList) {
        pAlias = pAliasList;

        pAliasList = pAliasList->pNext;

        DJFreeAlias(pAlias);
    }
}

static
void
DJFreeHostsLine(
    PHOSTSFILELINE pHostsLine
    )
{
    PHOSTSFILEENTRY pEntry = NULL;

    if (pHostsLine->pszComment)
        CTFreeString(pHostsLine->pszComment);
    pEntry = pHostsLine->pEntry;
    if (pEntry) {

        DJFreeAliasList(pEntry->pAliasList);
        pEntry->pAliasList = NULL;

        if (pEntry->pszIpAddress)
            CTFreeString(pEntry->pszIpAddress);

        if (pEntry->pszCanonicalName)
            CTFreeString(pEntry->pszCanonicalName);

        CTFreeMemory(pEntry);
    }
    CTFreeMemory(pHostsLine);
}

static
PHOSTSFILELINE *
DJGetLastHostsLine(
    PHOSTSFILELINE *pHostsLine
    )
{
    while(*pHostsLine != NULL)
    {
        pHostsLine = &(*pHostsLine)->pNext;
    }
    return pHostsLine;
}

void
DJFreeHostsFileLineList(
    PHOSTSFILELINE pHostsLineList
    )
{
    PHOSTSFILELINE pHostsLine = NULL;

    while (pHostsLineList) {

        pHostsLine = pHostsLineList;
        pHostsLineList = pHostsLineList->pNext;

        DJFreeHostsLine(pHostsLine);
    }
}

static
PHOSTFILEALIAS
DJReverseAliasList(
    PHOSTFILEALIAS pAliasList
    )
{
    PHOSTFILEALIAS pP = NULL;
    PHOSTFILEALIAS pQ = pAliasList;
    PHOSTFILEALIAS pR = NULL;

    while( pQ ) {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}

DWORD
DJParseHostsFile(
    const char *filename,
    PHOSTSFILELINE* ppHostsFileLineList
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PHOSTSFILELINE pLineHead = NULL;
    PHOSTSFILELINE pHostsLine = NULL;
    PHOSTFILEALIAS pAlias = NULL;
    FILE* fp = NULL;
    CHAR szBuf[1024+1];
    PSTR pszTmp = NULL;
    DWORD iToken = 0;
    PHOSTSFILELINE pLineTail = NULL;
    BOOLEAN exists;

    BAIL_ON_CENTERIS_ERROR(ceError = CTCheckFileOrLinkExists(filename, &exists));
    if(!exists)
        BAIL_ON_CENTERIS_ERROR(ceError = ERROR_FILE_NOT_FOUND);

    fp = fopen(filename, "r");
    if (fp == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    while (1) {

        if (fgets(szBuf, 1024, fp) == NULL) {

            if (!feof(fp)) {

                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);

            } else {

                break;

            }
        }

        CTStripWhitespace(szBuf);

        ceError = CTAllocateMemory(sizeof(HOSTSFILELINE),
                                   (PVOID*)(PVOID)&pHostsLine);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pHostsLine->pEntry = NULL;
        pHostsLine->pszComment = NULL;
        pszTmp = strchr(szBuf, '#');
        if (pszTmp != NULL) {
            ceError = CTAllocateString(pszTmp,
                                       &pHostsLine->pszComment);
            BAIL_ON_CENTERIS_ERROR(ceError);

            *pszTmp = '\0';
        }

        if(szBuf[0] != '\0')
        {
            ceError = CTAllocateMemory(sizeof(HOSTSFILEENTRY),
                                       (PVOID*)(PVOID)&pHostsLine->pEntry);
            BAIL_ON_CENTERIS_ERROR(ceError);


            iToken = 0;
            pszTmp = strtok(szBuf, " \t");
            while (pszTmp != NULL) {

                if (iToken == 0) {

                    ceError = CTAllocateString(pszTmp,
                                               &pHostsLine->pEntry->pszIpAddress);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                } else if (iToken == 1) {

                    ceError = CTAllocateString(pszTmp,
                                               &pHostsLine->pEntry->pszCanonicalName);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                } else {

                    ceError = CTAllocateMemory(sizeof(HOSTFILEALIAS),
                                               (PVOID*)(PVOID)&pAlias);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    ceError = CTAllocateString(pszTmp, &pAlias->pszAlias);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    //The alias list is first built in reverse
                    pAlias->pNext = pHostsLine->pEntry->pAliasList;
                    pHostsLine->pEntry->pAliasList = pAlias;
                    pAlias = NULL;
                }

                iToken++;

                pszTmp = strtok(NULL, " \t");
            }

            if (pHostsLine->pEntry->pAliasList) {
                pHostsLine->pEntry->pAliasList =
                    DJReverseAliasList(pHostsLine->pEntry->pAliasList);
            }
        }

        if(pLineTail != NULL)
            pLineTail->pNext = pHostsLine;
        else
            pLineHead = pHostsLine;
        pLineTail = pHostsLine;

        pHostsLine = NULL;
    }

    *ppHostsFileLineList = pLineHead;
    pLineHead = NULL;

error:

    if (pAlias)
        DJFreeAlias(pAlias);

    if (pHostsLine)
        DJFreeHostsLine(pHostsLine);

    if (pLineHead)
        DJFreeHostsFileLineList(pLineHead);

    if (fp)
        fclose(fp);

    return ceError;
}

static
BOOLEAN
DJEntryHasAlias(
    PHOSTFILEALIAS pAliasList,
    PCSTR pszName
    )
{
    BOOLEAN bResult = FALSE;

    while (pAliasList) {

        if (pAliasList->pszAlias &&
            !strcasecmp(pAliasList->pszAlias, pszName)) {
            bResult = TRUE;
            break;
        }

        pAliasList = pAliasList->pNext;
    }

    return bResult;
}

//Adds an alias to the head of the alias list
static
DWORD
DJAddAlias(
    PHOSTSFILELINE pLine,
    PCSTR pszName,
    BOOLEAN alwaysAdd,
    PBOOLEAN pbAdded
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PHOSTFILEALIAS pAlias = NULL;

    if (alwaysAdd || !DJEntryHasAlias(pLine->pEntry->pAliasList, pszName)) {

        ceError = CTAllocateMemory(sizeof(HOSTFILEALIAS), (PVOID*)(PVOID)&pAlias);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString(pszName, &pAlias->pszAlias);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pAlias->pNext = pLine->pEntry->pAliasList;
        pLine->pEntry->pAliasList = pAlias;

        pAlias = NULL;

        if (pbAdded != NULL)
            *pbAdded = TRUE;

        pLine->bModified = TRUE;

    } else if (pbAdded != NULL) {

        *pbAdded = FALSE;

    }

error:

    if (pAlias)
        DJFreeAlias(pAlias);

    return ceError;
}

#if 0
// Currently unused
static
DWORD
DJGetDnsDomain(
    PSTR pszHostname,
    PSTR* ppszDnsDomain
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR pszDnsDomain = NULL;
    PSTR pszTmp = strchr(pszHostname, '.');

    if (pszTmp && *(pszTmp+1) != '\0') {
        ceError = CTAllocateString(pszTmp+1, &pszDnsDomain);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *ppszDnsDomain = pszDnsDomain;

    return ceError;

error:

    if (pszDnsDomain)
        CTFreeString(pszDnsDomain);

    return ceError;
}
#endif

static
PHOSTSFILELINE
DJFindLineByIPAddress(
    PHOSTSFILELINE pHostsFileLineList,
    PSTR pszIpAddress
    )
{
    while(pHostsFileLineList) {
        if (pHostsFileLineList->pEntry != NULL &&
            !strcmp(pHostsFileLineList->pEntry->pszIpAddress, pszIpAddress))
            return pHostsFileLineList;
        pHostsFileLineList = pHostsFileLineList->pNext;
    }

    return NULL;
}

BOOLEAN
DJHostsFileWasModified(
    PHOSTSFILELINE pHostFileLineList
    )
{
    BOOLEAN bResult = FALSE;

    while (pHostFileLineList) {
        if (pHostFileLineList->bModified) {
            bResult = TRUE;
            break;
        }

        pHostFileLineList = pHostFileLineList->pNext;
    }

    return bResult;
}

static
DWORD
DJWriteHostsFileIfModified(
    const char *filename,
    PHOSTSFILELINE pHostFileLineList
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PHOSTSFILELINE pLine = pHostFileLineList;
    FILE* fp = NULL;
    PHOSTFILEALIAS pAlias = NULL;
    BOOLEAN bRemoveFile = FALSE;
    char *tempName = NULL;
    char *finalName = NULL;

    ceError = CTGetFileTempPath(
                        filename,
                        &finalName,
                        &tempName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (DJHostsFileWasModified(pHostFileLineList))
    {
        DJ_LOG_INFO("Writing out updated %s file", finalName);
        fp = fopen(tempName, "w");
        if (fp == NULL) {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        bRemoveFile = TRUE;

        while (pLine) {
            if (pLine->pEntry != NULL) {

                if (!IsNullOrEmptyString(pLine->pEntry->pszIpAddress))
                {
                    if (fprintf(fp, "%s", pLine->pEntry->pszIpAddress) < 0)
                    {
                        ceError = LwMapErrnoToLwError(errno);
                        BAIL_ON_CENTERIS_ERROR(ceError);
                    }
                }

                if (!IsNullOrEmptyString(pLine->pEntry->pszCanonicalName))
                {
                    if (fprintf(fp, " %s", pLine->pEntry->pszCanonicalName) < 0)
                    {
                        ceError = LwMapErrnoToLwError(errno);
                        BAIL_ON_CENTERIS_ERROR(ceError);
                    }
                }

                pAlias = pLine->pEntry->pAliasList;

                while (pAlias) {

                    if (!IsNullOrEmptyString(pAlias->pszAlias))
                    {
                        if (fprintf(fp, " %s", pAlias->pszAlias) < 0)
                        {
                            ceError = LwMapErrnoToLwError(errno);
                            BAIL_ON_CENTERIS_ERROR(ceError);
                        }
                    }

                    pAlias = pAlias->pNext;
                }

                if (pLine->pszComment != NULL)
                {
                    if (fprintf(fp, " %s", pLine->pszComment) < 0)
                    {
                        ceError = LwMapErrnoToLwError(errno);
                        BAIL_ON_CENTERIS_ERROR(ceError);
                    }
                }

                if (fprintf(fp, "\n") < 0)
                {
                    ceError = LwMapErrnoToLwError(errno);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
            }
            else if(pLine->pszComment != NULL)
            {
                if (fprintf(fp, "%s\n", pLine->pszComment) < 0)
                {
                    ceError = LwMapErrnoToLwError(errno);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
            }

            pLine = pLine->pNext;
        }

        if (fp) {
            fclose(fp);
            fp = NULL;
        }

        ceError = CTSafeReplaceFile(finalName, tempName);
        BAIL_ON_CENTERIS_ERROR(ceError);

#if defined(__LWI_DARWIN__)
        // Work around Mac bug where lookupd sometimes fails to pick up
        // changes in /etc/hosts, even with -flushcache.
        KickLookupd();
#endif

        bRemoveFile = FALSE;
    }
    else
        DJ_LOG_INFO("%s file was not modified; not rewriting", finalName);

error:

    if (fp)
        fclose(fp);

    if (bRemoveFile)
        CTRemoveFile(tempName);

    CT_SAFE_FREE_STRING(finalName);
    CT_SAFE_FREE_STRING(tempName);

    return ceError;
}

PCSTR Disp(PCSTR in)
{
    if(in == NULL)
        return "(null)";
    return in;
}

static
DWORD
DJUpdateHostEntry(
    PHOSTSFILELINE pLine,
    PCSTR pszShortName,
    PCSTR pszFqdnName,
    PCSTR pszRemoveName1,
    PCSTR pszRemoveName2
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PHOSTFILEALIAS *aliasPos;

    //This updates our hostname in a line of the hosts file
    // 1. pszFqdnName will be added as the primary name in the host entry
    // 2. pszShortName will be added as the first alias
    // 3. Whatever was previously the primary name and first alias will be
    // added as the second and third aliases
    // 4. If pszRemoveName1 and or pszRemoveName2 are not null, they will
    // be removed from the alias list

    if (pszFqdnName == NULL && pszShortName != NULL)
    {
        pszFqdnName = pszShortName;
        pszShortName = NULL;
    }
    else if (pszFqdnName != NULL && pszShortName != NULL &&
             !strcasecmp(pszFqdnName, pszShortName))
    {
        pszShortName = NULL;
    }
    DJ_LOG_INFO("Adding %s (fqdn %s) to /etc/hosts ip %s, "
                "removing %s, %s, %s, %s",
                Disp(pszShortName), Disp(pszFqdnName),
                Disp(pLine->pEntry->pszIpAddress),
                Disp(pszShortName), Disp(pszFqdnName),
                Disp(pszRemoveName1), Disp(pszRemoveName2));

    if (pszFqdnName != NULL && (pLine->pEntry->pszCanonicalName == NULL ||
                                strcasecmp(pLine->pEntry->pszCanonicalName, pszFqdnName)))
    {
        if (pLine->pEntry->pszCanonicalName != NULL)
        {
            ceError = DJAddAlias(pLine, pLine->pEntry->pszCanonicalName, FALSE, NULL);
            BAIL_ON_CENTERIS_ERROR(ceError);
            CTFreeString(pLine->pEntry->pszCanonicalName);
            pLine->pEntry->pszCanonicalName = NULL;
        }

        ceError = CTAllocateString(pszFqdnName, &pLine->pEntry->pszCanonicalName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pLine->bModified = TRUE;
    }

    aliasPos = &pLine->pEntry->pAliasList;
    if (pszShortName != NULL)
    {
        if (pLine->pEntry->pAliasList == NULL || strcasecmp(pLine->pEntry->pAliasList->pszAlias, pszShortName))
        {
            ceError = DJAddAlias(pLine, pszShortName, TRUE, NULL);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        //Skip over this so we don't delete it
        aliasPos = &(*aliasPos)->pNext;
    }

    {
        PCSTR removeStrings[] = {
            pszShortName,
            pszFqdnName,
            pszRemoveName1,
            pszRemoveName2 };
        while (*aliasPos != NULL)
        {
            int i;
            for(i=0; i<sizeof(removeStrings)/sizeof(removeStrings[0]); i++)
            {
                if(removeStrings[i] != NULL &&
                   !strcasecmp((*aliasPos)->pszAlias, removeStrings[i]))
                {
                    //Remove it
                    PHOSTFILEALIAS remove = *aliasPos;
                    (*aliasPos) = remove->pNext;
                    DJFreeAlias(remove);
                    pLine->bModified = TRUE;
                    goto removed_entry;
                }
            }
            //Advance to the next entry because nothing was removed.
            aliasPos = &(*aliasPos)->pNext;
removed_entry:
            ;
        }
    }
error:
    return ceError;
}

// newFdqnHostname = <shortHostname>.<dnsDomainName>
DWORD
DJReplaceHostnameInMemory(
    PHOSTSFILELINE pHostsFileLineList,
    PCSTR oldShortHostname,
    PCSTR oldFqdnHostname,
    PCSTR shortHostname,
    PCSTR dnsDomainName
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR pszDomainName = NULL;
    PSTR pszHostName = NULL;
    PSTR pszCanonicalName = NULL;
    PHOSTSFILELINE pLine = NULL;
    PHOSTSFILELINE pCreatedLine = NULL;
    BOOLEAN bFound = FALSE;
    BOOLEAN bModified = FALSE;
    PHOSTFILEALIAS pAlias = NULL;

    //
    // Ideal algorithm:
    //
    // 1) Find any lines with hostname.
    // 2) Make sure the the FQDN is present as the first
    //    name in each of those lines.
    // 3) If no lines were found, then add hostname to 127.0.0.1
    //    and put FQDN first.
    // 4) If 127.0.0.2 line is present, edit that to just have our info.
    //

    if (IsNullOrEmptyString(shortHostname)) {
        ceError = ERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (strchr(shortHostname, '.')) {
        ceError = ERROR_INVALID_COMPUTERNAME;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTAllocateString(shortHostname, &pszHostName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    CTStripWhitespace(pszHostName);

    CTStrToLower(pszHostName);

    if (dnsDomainName != NULL)
    {
        ceError = CTAllocateString(dnsDomainName, &pszDomainName);
        BAIL_ON_CENTERIS_ERROR(ceError);
        CTStripWhitespace(pszDomainName);
        CTStrToLower(pszDomainName);
        ceError = CTAllocateMemory(strlen(pszHostName)+strlen(pszDomainName)+2,
                                   (PVOID*)(PVOID)&pszCanonicalName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        sprintf(pszCanonicalName, "%s.%s", pszHostName, pszDomainName);
    }
    else
    {
        ceError = CTAllocateString(pszHostName, &pszCanonicalName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    for (pLine = pHostsFileLineList; pLine; pLine = pLine->pNext) {

        if (pLine->pEntry != NULL) {
            if (pLine->pEntry->pszCanonicalName != NULL &&
                (!strcasecmp(pLine->pEntry->pszCanonicalName, pszHostName) ||
                 !strcasecmp(pLine->pEntry->pszCanonicalName, oldShortHostname ? oldShortHostname : "") ||
                 !strcasecmp(pLine->pEntry->pszCanonicalName, oldFqdnHostname ? oldFqdnHostname : "") ||
                 !strcasecmp(pLine->pEntry->pszCanonicalName, pszCanonicalName))) {

                ceError = DJUpdateHostEntry( pLine, pszHostName, pszCanonicalName, oldShortHostname, oldFqdnHostname);
                BAIL_ON_CENTERIS_ERROR(ceError);
                bFound = TRUE;
            }
            else if (DJEntryHasAlias(pLine->pEntry->pAliasList, pszHostName)) {

                bFound = TRUE;
                ceError = DJUpdateHostEntry( pLine, pszHostName, pszCanonicalName, oldShortHostname, oldFqdnHostname);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
    }

    if (!bFound) {
        //First try to setup ip address on the loop back device which are not
        //127.0.0.1
        for (pLine = pHostsFileLineList; pLine; pLine = pLine->pNext) {
            if (pLine->pEntry != NULL &&
                !strncmp(pLine->pEntry->pszIpAddress, "127.0.", strlen("127.0.")) &&
                strcmp(pLine->pEntry->pszIpAddress, "127.0.0.1"))
            {
                ceError = DJUpdateHostEntry( pLine, pszHostName, pszCanonicalName, oldShortHostname, oldFqdnHostname);
                BAIL_ON_CENTERIS_ERROR(ceError);
                bFound = TRUE;
            }
        }
        if (!bFound)
        {
            //Have to add it to the 127.0.0.1 address
            pLine = DJFindLineByIPAddress(pHostsFileLineList, "127.0.0.1");
            if(pLine == NULL)
            {
                //We have to create the 127.0.0.1 address
                ceError = CTAllocateMemory(sizeof(HOSTSFILELINE), (PVOID*)(PVOID)&pCreatedLine);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ceError = CTAllocateMemory(sizeof(HOSTSFILEENTRY),
                                           (PVOID*)(PVOID)&pCreatedLine->pEntry);

                ceError = CTAllocateString("127.0.0.1",
                                           &pCreatedLine->pEntry->pszIpAddress);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ceError = DJAddAlias(pCreatedLine, "localhost", FALSE, &bModified);
                BAIL_ON_CENTERIS_ERROR(ceError);

                *DJGetLastHostsLine(&pHostsFileLineList) = pCreatedLine;
                pLine = pCreatedLine;
                pCreatedLine = NULL;
            }
            ceError = DJUpdateHostEntry( pLine, pszHostName, pszCanonicalName, oldShortHostname, oldFqdnHostname);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

error:

    if (pAlias)
        DJFreeAlias(pAlias);

    if (pszHostName)
        CTFreeString(pszHostName);

    if (pszDomainName)
        CTFreeString(pszDomainName);

    if (pszCanonicalName)
        CTFreeMemory(pszCanonicalName);

    if(pCreatedLine)
        DJFreeHostsLine(pCreatedLine);

    return ceError;
}

DWORD
DJCopyLine(
        PHOSTSFILELINE pSrc, 
        PHOSTSFILELINE *ppDest)
{
    PHOSTSFILELINE ret = NULL;
    PHOSTFILEALIAS srcAlias = NULL, *destAlias = NULL;
    DWORD ceError = ERROR_SUCCESS;

    ceError = CTAllocateMemory(sizeof(HOSTSFILELINE),
                               (PVOID*)(PVOID)&ret);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ret->bModified = pSrc->bModified;
    ceError = CTDupOrNullStr(pSrc->pszComment, &ret->pszComment);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(pSrc->pEntry != NULL)
    {
        ceError = CTAllocateMemory(sizeof(HOSTSFILEENTRY),
                                   (PVOID*)(PVOID)&ret->pEntry);
        BAIL_ON_CENTERIS_ERROR(ceError);
        ceError = CTDupOrNullStr(pSrc->pEntry->pszIpAddress, &ret->pEntry->pszIpAddress);
        BAIL_ON_CENTERIS_ERROR(ceError);
        ceError = CTDupOrNullStr(pSrc->pEntry->pszCanonicalName, &ret->pEntry->pszCanonicalName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        srcAlias = pSrc->pEntry->pAliasList;
        destAlias = &ret->pEntry->pAliasList;
    }

    while(srcAlias != NULL)
    {
        ceError = CTAllocateMemory(sizeof(HOSTFILEALIAS),
                                   (PVOID*)(PVOID)destAlias);
        BAIL_ON_CENTERIS_ERROR(ceError);
        ceError = CTDupOrNullStr(srcAlias->pszAlias, &(*destAlias)->pszAlias);
        BAIL_ON_CENTERIS_ERROR(ceError);

        srcAlias = srcAlias->pNext;
        destAlias = &(*destAlias)->pNext;
    }

    *ppDest = ret;
    ret = NULL;

error:
    if(ret != NULL)
        DJFreeHostsFileLineList(ret);
    return ceError;
}

DWORD
DJCopyMissingHostsEntry(
        PCSTR destFile, PCSTR srcFile,
        PCSTR entryName1, PCSTR entryName2)
{
    DWORD ceError = ERROR_SUCCESS;
    PHOSTSFILELINE pDestList = NULL;
    PHOSTSFILELINE pSrcList = NULL;
    PHOSTSFILELINE pLine = NULL;
    PHOSTSFILELINE pCopiedLine = NULL;

    ceError = DJParseHostsFile(destFile, &pDestList);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJParseHostsFile(srcFile, &pSrcList);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(entryName2 == NULL)
        entryName2 = "";

    for (pLine = pDestList; pLine; pLine = pLine->pNext) {

        if (pLine->pEntry != NULL) {
            if ((pLine->pEntry->pszCanonicalName != NULL &&
                (!strcasecmp(pLine->pEntry->pszCanonicalName, entryName1) ||
                 !strcasecmp(pLine->pEntry->pszCanonicalName, entryName2))) ||
                DJEntryHasAlias(pLine->pEntry->pAliasList, entryName1) ||
                DJEntryHasAlias(pLine->pEntry->pAliasList, entryName1))
            {
                //The dest file already has this entry
                goto done;
            }
        }
    }

    for (pLine = pSrcList; pLine; pLine = pLine->pNext) {

        if (pLine->pEntry != NULL) {
            if ((pLine->pEntry->pszCanonicalName != NULL &&
                (!strcasecmp(pLine->pEntry->pszCanonicalName, entryName1) ||
                 !strcasecmp(pLine->pEntry->pszCanonicalName, entryName2))) ||
                DJEntryHasAlias(pLine->pEntry->pAliasList, entryName1) ||
                DJEntryHasAlias(pLine->pEntry->pAliasList, entryName1))
            {
                //Copy this line to dest
                ceError = DJCopyLine(pLine, &pCopiedLine);
                BAIL_ON_CENTERIS_ERROR(ceError);

                pCopiedLine->pNext = pDestList;
                pDestList = pCopiedLine;
                pCopiedLine->bModified = TRUE;
                pCopiedLine = NULL;
            }
        }
    }

    ceError = DJWriteHostsFileIfModified(destFile, pDestList);
    BAIL_ON_CENTERIS_ERROR(ceError);

done:
error:
    if (pDestList)
        DJFreeHostsFileLineList(pDestList);
    if (pSrcList)
        DJFreeHostsFileLineList(pSrcList);
    if (pCopiedLine)
        DJFreeHostsFileLineList(pCopiedLine);
    return ceError;
}

// newFdqnHostname = <shortHostname>.<dnsDomainName>
DWORD
DJReplaceNameInHostsFile(
    PCSTR filename,
    PSTR  oldShortHostname,
    PSTR  oldFqdnHostname,
    PSTR  shortHostname,
    PCSTR dnsDomainName
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PHOSTSFILELINE pHostsFileLineList = NULL;

    ceError = DJParseHostsFile(filename, &pHostsFileLineList);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJReplaceHostnameInMemory(
        pHostsFileLineList,
        oldShortHostname,
        oldFqdnHostname,
        shortHostname,
        dnsDomainName
        );
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJWriteHostsFileIfModified(filename, pHostsFileLineList);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:
    if (pHostsFileLineList)
        DJFreeHostsFileLineList(pHostsFileLineList);
    return ceError;
}
