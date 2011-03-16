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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        reader.c
 *
 * Abstract:
 *
 *        Event forwarder from eventlogd to collector service
 *
 *        Reader thread
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#include "includes.h"

#define ASSERT(x)   assert(x)

//Replaces double backslashes with single backslashes
VOID
RSysUnescapeUser(
    IN OUT PSTR pszUser
    )
{
    PSTR pszInput = pszUser;
    PSTR pszOutput = pszUser;

    while (*pszInput)
    {
        if (pszInput[0] == '\\' && pszInput[1] == '\\')
        {
            pszInput++;
        }
        *pszOutput = *pszInput;
        pszOutput++;
        pszInput++;
    }

    *pszOutput = 0;
}

DWORD
RSysSrvCheckLineMatch(
    IN PCSTR pszLine,
    IN RSYS_MESSAGE_PATTERN* pPattern,
    IN OUT HANDLE* phLsaConnection,
    OUT PSTR* ppszUser,
    OUT BOOLEAN* pbMatched
    )
{
    PSTR pszUser = NULL;
    ULONG ulUserMatchIndex = pPattern->ulUserMatchIndex;
    regmatch_t matches[10];
    BOOLEAN bAdUser = TRUE;
    DWORD dwError = 0;
    PLSA_USER_INFO_0 pUserInfo = NULL;

    ASSERT(ulUserMatchIndex < sizeof(matches)/sizeof(matches[0]));

    if (!pPattern->bCompiled)
    {
        dwError = regcomp(
                        &pPattern->compiledRegEx,
                        pPattern->pszRawMessageRegEx,
                        REG_EXTENDED);
        if (dwError)
        {
            dwError = EINVAL;
            BAIL_ON_RSYS_ERROR(dwError);
        }
        pPattern->bCompiled = TRUE;
    }

    //regexec returns 0 on match
    if (regexec(
                &pPattern->compiledRegEx,
                pszLine,
                sizeof(matches)/sizeof(matches[0]),
                matches,
                0))
    {
        goto not_matched;
    }

    dwError = LwStrndup(
                    pszLine + matches[ulUserMatchIndex].rm_so,
                    matches[ulUserMatchIndex].rm_eo -
                        matches[ulUserMatchIndex].rm_so,
                    &pszUser);
    BAIL_ON_RSYS_ERROR(dwError);

    RSysUnescapeUser(pszUser);

    if (pPattern->filter == RSYS_ANY_USER)
    {
        //matched
    }
    else
    {
        if (!*phLsaConnection)
        {
            dwError = LsaOpenServer(phLsaConnection);
            if (dwError == ERROR_FILE_NOT_FOUND ||
                    dwError == LW_ERROR_ERRNO_ECONNREFUSED)
            {
                dwError = LW_ERROR_LSA_SERVER_UNREACHABLE;
            }
            BAIL_ON_RSYS_ERROR(dwError);
        }

        /* If lsass knows about the user, it is an AD user (non-/etc/passwd at
         * least) */
        dwError = LsaFindUserByName(
                        *phLsaConnection,
                        pszUser,
                        0,
                        (PVOID*)&pUserInfo);
        if (dwError == LW_ERROR_NO_SUCH_USER)
        {
            bAdUser = FALSE;
            dwError = 0;
        }
        BAIL_ON_RSYS_ERROR(dwError);


        RSYS_LOG_VERBOSE("User [%s] is owned by %s",
                pszUser,
                bAdUser ? "lsass" : "local system");

        if ((pPattern->filter == RSYS_AD_USER) != bAdUser)
        {
            goto not_matched;
        }
    }

    *ppszUser = pszUser;
    *pbMatched = TRUE;

cleanup:
    if (pUserInfo)
    {
        LsaFreeUserInfo(
                0,
                pUserInfo);
    }

    return dwError;

error:
not_matched:
    RtlCStringFree(&pszUser);
    *ppszUser = NULL;
    *pbMatched = FALSE;

    goto cleanup;
}

// The data is returned directly into a pRecord so that the caller can build up
// an array of events.
DWORD
RSysSrvParseLine(
    IN RSYS_LINE_NODE* pLine,
    IN time_t tNow,
    OUT PEVENT_LOG_RECORD pRecord,
    OUT PBOOLEAN pbNonBlank
    )
{
    DWORD dwError = 0;
    struct tm tmParsed, tmNow;
    // Do not free
    PSTR pszPos = pLine->pszData;
    // Do not free
    PSTR pszTokenStart = NULL;
    PSTR pszIdent = NULL;
    PSTR pszNewIdent = NULL;
    PSTR pszHostname = NULL;
    PSTR pszMessage = NULL;
    BOOLEAN bPidPresent = FALSE;
    pid_t pid = (pid_t)-1;
    HANDLE hLsaConnection = NULL;
    BOOLEAN bIsMatch = FALSE;
    BOOLEAN bPatternListLocked = FALSE;
    // Do not free
    PDLINKEDLIST pPatternList = NULL;
    PDLINKEDLIST pListPos = NULL;
    RSYS_MESSAGE_PATTERN* pPattern = NULL;
    BOOLEAN bLogUnmatchedError = FALSE;
    BOOLEAN bLogUnmatchedWarning = FALSE;
    BOOLEAN bLogUnmatchedInfo = FALSE;

    dwError = RSysSrvGetLogUnmatchedEvents(
                    NULL,
                    &bLogUnmatchedError,
                    &bLogUnmatchedWarning,
                    &bLogUnmatchedInfo);
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = RSysSrvLockPatternList(
                    NULL,
                    &pPatternList);
    BAIL_ON_RSYS_ERROR(dwError);
    bPatternListLocked = TRUE;

    RSYS_LOG_VERBOSE("Removed line [%s] from escrow for event type %s", pLine->pszData,
            pLine->pSource->pszEventType);

    memset(pRecord, 0, sizeof(*pRecord));

    if (!strlen(pLine->pszData))
    {
        RSYS_LOG_INFO("Ignoring blank line");

        *pbNonBlank = FALSE;
        goto cleanup;
    }

    // <month name> <day of month> <0-23 hour>:<minute>:<second>
    pszPos = strptime(pszPos, "%b %d %H:%M:%S", &tmParsed);
    if (pszPos == NULL)
    {
        dwError = EINVAL;
        BAIL_ON_RSYS_ERROR(dwError);
    }
    // Initialize the year with the current time
    localtime_r(&tNow, &tmNow);
    tmParsed.tm_year = tmNow.tm_year;
    if (tmNow.tm_mon == 0 && tmParsed.tm_mon == 11)
    {
        tmParsed.tm_year--;
    }
    else if (tmNow.tm_mon == 11 && tmParsed.tm_mon == 0)
    {
        tmParsed.tm_year++;
    }
    tmParsed.tm_isdst = tmNow.tm_isdst;

    while (isspace((int)*pszPos))
    {
        pszPos++;
    }

    pszTokenStart = pszPos;

    while (*pszPos && !isspace((int)*pszPos))
    {
        pszPos++;
    }

    dwError = LwStrndup(
        pszTokenStart,
        pszPos - pszTokenStart,
        &pszHostname);
    BAIL_ON_RSYS_ERROR(dwError);

    while (isspace((int)*pszPos))
    {
        pszPos++;
    }

    pszTokenStart = pszPos;
    while (*pszPos != 0 && *pszPos != '[' && *pszPos != ':' &&
            !isspace((int)*pszPos))
    {
        pszPos++;
    }

    if (isspace((int)*pszPos))
    {
        // This message is directly from the syslog daemon. It does not have
        // an ident field.
        pszPos = pszTokenStart;

        dwError = LwRtlCStringDuplicate(
                        &pszIdent,
                        "syslog daemon");
        BAIL_ON_RSYS_ERROR(dwError);
    }
    else
    {
        dwError = LwStrndup(
            pszTokenStart,
            pszPos - pszTokenStart,
            &pszIdent);
        BAIL_ON_RSYS_ERROR(dwError);

        if (*pszPos == '[')
        {
            pszPos++;
            // Parse the pid
            bPidPresent = TRUE;
            pid = strtoul(pszPos, &pszPos, 10);
            if (!pszPos || *pszPos != ']')
            {
                dwError = EINVAL;
                BAIL_ON_RSYS_ERROR(dwError);
            }
            pszPos++;
        }
        if (!strncmp(pszPos, " (", 2))
        {
            // This is the daemon name on Mac. We'll append it to the ident. An example line is:
            // Oct 22 14:08:02 mac10-6 com.apple.launchd[1] (com.likewisesoftware.lwsmd): Throttling respawn: Will start in 3 seconds

            pszTokenStart = pszPos;

            pszPos += 2;
            while (*pszPos != 0 && *pszPos != ')')
            {
                pszPos++;
            }
            if (*pszPos != ')')
            {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_RSYS_ERROR(dwError);
            }
            pszPos++;

            dwError = LwAllocateStringPrintf(
                        &pszNewIdent,
                        "%s%.*s",
                        pszIdent,
                        pszPos - pszTokenStart,
                        pszTokenStart);
            BAIL_ON_RSYS_ERROR(dwError);
            
            LW_SAFE_FREE_STRING(pszIdent);
            pszIdent = pszNewIdent;
            pszNewIdent = NULL;
        }
        if (*pszPos != ':')
        {
            dwError = EINVAL;
            BAIL_ON_RSYS_ERROR(dwError);
        }
        pszPos++;
        if (*pszPos == ' ')
        {
            pszPos++;
        }
    }

    dwError = RtlCStringDuplicate(&pszMessage, pszPos);
    BAIL_ON_RSYS_ERROR(dwError);

    RSYS_LOG_VERBOSE("Parsed hostname [%s] ident [%s] pid [%d] message [%s] month %d day %d hour %d minute %d",
            pszHostname, pszIdent, pid, pszMessage,
            tmParsed.tm_mon, tmParsed.tm_mday,
            tmParsed.tm_hour, tmParsed.tm_min);

    pListPos = pPatternList;
    while (pListPos)
    {
        pPattern = (RSYS_MESSAGE_PATTERN *)pListPos->pItem;

        dwError = RSysSrvCheckLineMatch(
                        pLine->pszData,
                        pPattern,
                        &hLsaConnection,
                        &pRecord->pszUser,
                        &bIsMatch);
        BAIL_ON_RSYS_ERROR(dwError);
        if (bIsMatch)
        {
            break;
        }

        pListPos = pListPos->pNext;
    }

    if (pListPos)
    {
        pRecord->dwEventSourceId = pPattern->ulId;

        dwError = LwRtlCStringDuplicate(
                        &pRecord->pszEventType,
                        pPattern->pszEventType);
        BAIL_ON_RSYS_ERROR(dwError);
    }
    else
    {
        BOOLEAN bLogThis = FALSE;
        if (!strcmp(pLine->pSource->pszEventType, "Information") &&
                bLogUnmatchedInfo)
        {
            bLogThis = TRUE;
        }
        if (!strcmp(pLine->pSource->pszEventType, "Warning") &&
                bLogUnmatchedWarning)
        {
            bLogThis = TRUE;
        }
        if (!strcmp(pLine->pSource->pszEventType, "Error") &&
                bLogUnmatchedError)
        {
            bLogThis = TRUE;
        }

        if (!bLogThis)
        {
            RSYS_LOG_INFO("Ignoring unmatched event of type %s due to configuration setttings", pLine->pSource->pszEventType);

            *pbNonBlank = FALSE;
            goto cleanup;
        }
        pRecord->dwEventSourceId = 0;

        dwError = LwRtlCStringDuplicate(
                        &pRecord->pszEventType,
                        pLine->pSource->pszEventType);
        BAIL_ON_RSYS_ERROR(dwError);

        dwError = LwRtlCStringDuplicate(
                        &pRecord->pszUser,
                        "SYSTEM");
        BAIL_ON_RSYS_ERROR(dwError);
    }

    pRecord->dwEventRecordId = 0;

    dwError = LwRtlCStringDuplicate(
                    &pRecord->pszEventTableCategoryId,
                    "System");
    BAIL_ON_RSYS_ERROR(dwError);

    pRecord->dwEventDateTime = mktime(&tmParsed);
    if (pRecord->dwEventDateTime == (DWORD)-1)
    {
        dwError = EINVAL;
        BAIL_ON_RSYS_ERROR(dwError);
    }

    pRecord->pszEventSource = pszIdent;
    pszIdent = NULL;

    dwError = LwRtlCStringDuplicate(
                    &pRecord->pszEventCategory,
                    "System Log");
    BAIL_ON_RSYS_ERROR(dwError);

    pRecord->pszComputer = pszHostname;
    pszHostname = NULL;

    pRecord->pszDescription = pszMessage;
    pszMessage = NULL;

    dwError = LwRtlCStringDuplicate(
                    &pRecord->pszData,
                    pLine->pszData);
    BAIL_ON_RSYS_ERROR(dwError);

    *pbNonBlank = TRUE;

cleanup:
    if (bPatternListLocked)
    {
        RSysSrvUnlockPatternList(
            NULL,
            pPatternList);
    }
    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }
    RtlCStringFree(&pszHostname);
    RtlCStringFree(&pszIdent);
    LW_SAFE_FREE_STRING(pszNewIdent);
    RtlCStringFree(&pszMessage);
    return dwError;

error:
    *pbNonBlank = FALSE;

    RtlCStringFree(&pRecord->pszEventTableCategoryId);
    RtlCStringFree(&pRecord->pszEventType);
    RtlCStringFree(&pRecord->pszEventSource);
    RtlCStringFree(&pRecord->pszEventCategory);
    RtlCStringFree(&pRecord->pszUser);
    RtlCStringFree(&pRecord->pszComputer);
    RtlCStringFree(&pRecord->pszDescription);
    RtlCStringFree(&pRecord->pszData);

    goto cleanup;
}

VOID
RSysSrvUnlinkLine(
    RSYS_LINE_BUFFER *pBuffer,
    RSYS_LINE_NODE *pLine
    )
{
    if (pLine->pPrev)
    {
        pLine->pPrev->pNext = pLine->pNext;
    }
    else
    {
        pBuffer->pHead = pLine->pNext;
    }

    if (pLine->pNext)
    {
        pLine->pNext->pPrev = pLine->pPrev;
    }
    else
    {
        pBuffer->pTail = pLine->pPrev;
    }

    pLine->pNext = NULL;
    pLine->pPrev = NULL;
}

VOID
RSysSrvLinkAtTail(
    RSYS_LINE_BUFFER *pBuffer,
    RSYS_LINE_NODE *pLine
    )
{

    if (pBuffer->pTail)
    {
        pBuffer->pTail->pNext = pLine;
    }
    else
    {
        pBuffer->pHead = pLine;
    }

    pLine->pPrev = pBuffer->pTail;
    pBuffer->pTail = pLine;
}

VOID
RSysSrvFreeLine(
    RSYS_LINE_NODE* pLine
    )
{
    RtlCStringFree(&pLine->pszData);
    LW_RTL_FREE(&pLine);
}

DWORD
RSysSrvAddLine(
    IN OUT RSYS_LINE_BUFFER *pBuffer,
    // The passed in line must not be in the red black tree or linked list in
    // pBuffer
    IN OUT RSYS_LINE_NODE** ppLine
    )
{
    DWORD dwError = 0;
    // Do not free in cleanup
    RSYS_LINE_NODE* pLine = *ppLine;
    // Do not free in cleanup
    RSYS_LINE_NODE* pFoundLine = NULL;

    dwError = LwRtlRBTreeFind(
            pBuffer->pRBTree,
            pLine->pszData,
            (PVOID*)&pFoundLine);
    if (dwError == STATUS_NOT_FOUND)
    {
        dwError = 0;
    }
    else if (dwError == 0)
    {
        // Found an existing line. Update its priority and timestamp, then
        // put it in the back of the queue.
        if (pFoundLine->pSource->dwPriority == pLine->pSource->dwPriority)
        {
            pFoundLine->dwCount++;
        }
        else if (pFoundLine->pSource->dwPriority < pLine->pSource->dwPriority)
        {
            pFoundLine->pSource = pLine->pSource;
            pFoundLine->dwCount = pLine->dwCount;
        }
        RSysSrvUnlinkLine(
                pBuffer,
                pFoundLine);
        pFoundLine->qwTimeStamp = pLine->qwTimeStamp;
        RSysSrvLinkAtTail(pBuffer, pFoundLine);

        RSysSrvFreeLine(pLine);
        *ppLine = pFoundLine;
        goto cleanup;
    }
    BAIL_ON_RSYS_ERROR(dwError);

    // This is the first instance of this text
    dwError = LwRtlRBTreeAdd(
                    pBuffer->pRBTree,
                    pLine->pszData,
                    pLine);
    BAIL_ON_RSYS_ERROR(dwError);

    RSysSrvLinkAtTail(pBuffer, pLine);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
RSysSrvReadSource(
    IN OUT RSYS_SOURCE* pSource,
    IN OUT RSYS_LINE_BUFFER* pBuffer,
    uint64_t qwNow
    )
{
    DWORD dwError = 0;
    ssize_t sReadCount = 0;
    // Do not free
    PSTR pszDelimPos = NULL;
    RSYS_LINE_NODE* pLine = NULL;

    regex_t repeatRegEx;
    BOOLEAN bCompiledRegEx = FALSE;
    regmatch_t matches[2];

    dwError = regcomp(
                    &repeatRegEx,
                    "[^ ]+ +[^ ]+ [^ ]+ [^ ]+ last message repeated ([0-9]+) times?( ---)?",
                    REG_EXTENDED);
    BAIL_ON_RSYS_ERROR(dwError);
    bCompiledRegEx = TRUE;

    while (1)
    {
        sReadCount = read(
            fileno(pSource->pReadPipe),
            pSource->pszReadDataEnd,
            pSource->pszReadBufferEnd - pSource->pszReadDataEnd);
        if (sReadCount < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_RSYS_ERROR(dwError);
        }
        pSource->pszReadDataEnd += sReadCount;
        break;
    }
    ASSERT(pSource->pszReadDataEnd <= pSource->pszReadBufferEnd);

    if (pSource->pszReadDataEnd == pSource->pszReadBufferEnd)
    {
        // The buffer is too small; double it for next time
        size_t sNewLen = (pSource->pszReadBufferEnd -
            pSource->pszReadBufferStart) * 2 + 100;        
        PSTR pszNewBuffer = NULL;

        dwError = LwNtStatusToWin32Error(
                      LW_RTL_ALLOCATE(
                          &pszNewBuffer,
                          CHAR,
                          sNewLen));
        BAIL_ON_RSYS_ERROR(dwError);

        pSource->pszReadDataStart += pszNewBuffer - pSource->pszReadBufferStart;
        pSource->pszReadDataEnd += pszNewBuffer - pSource->pszReadBufferStart;

        LW_RTL_FREE(&pSource->pszReadBufferStart);

        pSource->pszReadBufferStart = pszNewBuffer;
        pSource->pszReadBufferEnd = pszNewBuffer + sNewLen;
    }

    // While there is a newline in the buffer, parse the line
    while ((pszDelimPos = memchr(pSource->pszReadDataStart,
            '\n',
            pSource->pszReadDataEnd - pSource->pszReadDataStart)) != NULL)
    {
        // null terminate the line
        *pszDelimPos = 0;

        RSYS_LOG_VERBOSE("Received line [%s] for event type %s",
                pSource->pszReadDataStart, pSource->pszEventType);

        dwError = LwNtStatusToWin32Error(
                      LW_RTL_ALLOCATE( 
                          &pLine,
                          RSYS_LINE_NODE,
                          sizeof(RSYS_LINE_NODE)));
        BAIL_ON_RSYS_ERROR(dwError);

        if (!regexec(
                    &repeatRegEx,
                    pSource->pszReadDataStart,
                    sizeof(matches)/sizeof(matches[0]),
                    matches,
                    0))
        {
            // Syslog is telling us the previous line has been repeated

            // Do not free
            LW_ANSI_STRING repeatCount = {0};

            repeatCount.Length = matches[1].rm_eo - matches[1].rm_so;
            repeatCount.MaximumLength = repeatCount.MaximumLength;
            repeatCount.Buffer = pSource->pszReadDataStart +
                                    matches[1].rm_so;

            dwError = LwRtlAnsiStringParseULONG(
                            &pLine->dwCount,
                            &repeatCount,
                            &repeatCount);
            BAIL_ON_RSYS_ERROR(dwError);

            dwError = LwRtlCStringDuplicate(
                            &pLine->pszData,
                            pSource->pszLastReadLine);
            BAIL_ON_RSYS_ERROR(dwError);
        }
        else
        {
            dwError = LwRtlCStringDuplicate(
                            &pLine->pszData,
                            pSource->pszReadDataStart);
            BAIL_ON_RSYS_ERROR(dwError);

            RtlCStringFree(&pSource->pszLastReadLine);
            dwError = LwRtlCStringDuplicate(
                            &pSource->pszLastReadLine,
                            pSource->pszReadDataStart);
            BAIL_ON_RSYS_ERROR(dwError);

            pLine->dwCount = 1;

        }

        pLine->qwTimeStamp = qwNow;
        pLine->pSource = pSource;
        dwError = RSysSrvAddLine(
                        pBuffer,
                        &pLine);
        BAIL_ON_RSYS_ERROR(dwError);
        // The line is now owned by the buffer
        pLine = NULL;

        pSource->pszReadDataStart = pszDelimPos + 1;
    }

    // If events were read, shift the buffer back
    if (pSource->pszReadDataStart != pSource->pszReadBufferStart)
    {
        memmove(pSource->pszReadBufferStart,
                pSource->pszReadDataStart,
                pSource->pszReadDataEnd - pSource->pszReadDataStart);
        pSource->pszReadDataEnd -= (pSource->pszReadDataStart -
                pSource->pszReadBufferStart);
        pSource->pszReadDataStart = pSource->pszReadBufferStart;
    }

cleanup:
    if (bCompiledRegEx)
    {
        regfree(&repeatRegEx);
    }
    if (pLine)
    {
        RSysSrvFreeLine(pLine);
    }
    return dwError;

error:
    goto cleanup;
}

VOID
RSysSrvNOPFree(
    VOID *pUnused
    )
{
}

DWORD
RSysSrvWriteEvents(
    DWORD dwCount,
    PEVENT_LOG_RECORD pList
    )
{
    DWORD dwError = 0;
    HANDLE hEventLog = NULL;

    RSYS_LOG_INFO("Sending %d events to eventlog", dwCount);

    dwError = LWIOpenEventLog(
                    NULL,
                    &hEventLog);
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = LWIWriteEventLogRecords(
                    hEventLog,
                    dwCount,
                    pList);
    BAIL_ON_RSYS_ERROR(dwError);

cleanup:
    if (hEventLog != NULL)
    {
        LWICloseEventLog(hEventLog);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RSysSrvGetSyslogPid(
    pid_t* pdwSyslogPid
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    unsigned long ulTemp = 0;
    pid_t dwSyslogPid = (pid_t)-1;
    FILE *fPidFile = NULL;
    PCSTR ppszPidPaths[] = {
        "/etc/syslog.pid",
        "/var/run/rsyslogd.pid",
        "/var/run/syslogd.pid",
        "/var/run/syslog-ng.pid",
        "/var/run/syslog.pid",
    };

    for (dwIndex = 0;
        dwIndex < sizeof(ppszPidPaths)/sizeof(ppszPidPaths[0]);
        dwIndex++)
    {
        fPidFile = fopen(ppszPidPaths[dwIndex], "r");
        if (fPidFile != NULL)
        {
            if (fscanf(fPidFile, "%lu", &ulTemp) > 0)
            {
                dwSyslogPid = (pid_t)ulTemp;
            }
            if (fclose(fPidFile) < 0)
            {
                dwError = LwMapErrnoToLwError(errno);
            }
            fPidFile = NULL;
            BAIL_ON_RSYS_ERROR(dwError);
        }

        if (dwSyslogPid != -1)
        {
            break;
        }
    }

    *pdwSyslogPid = dwSyslogPid;

cleanup:
    if (fPidFile != NULL)
    {
        fclose(fPidFile);
    }
    return dwError;

error:
    *pdwSyslogPid = -1;
    goto cleanup;
}

DWORD
RSysSrvOpenPipes(
    IN DWORD dwPipeCount,
    IN OUT RSYS_SOURCE* pPipes
    )
{
    DWORD dwError = 0;
    BOOLEAN bPipeDirExists = FALSE;
    DWORD dwIndex = 0;
    DWORD dwAttempt = 0;
    pid_t dwSyslogPid = (pid_t)-1;
    struct stat statBuf;

    dwError = RSysSrvGetSyslogPid(&dwSyslogPid);
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = LwCheckFileTypeExists(
                    RSYS_PIPE_DIR,
                    LWFILE_DIRECTORY,
                    &bPipeDirExists);
    BAIL_ON_RSYS_ERROR(dwError);

    if (!bPipeDirExists)
    {
        if (mkdir(RSYS_PIPE_DIR, S_IRWXU) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_RSYS_ERROR(dwError);
        }
    }

    for (dwIndex = 0; dwIndex < dwPipeCount; dwIndex++)
    {
        for (dwAttempt = 0; ; dwAttempt++)
        {
            // Try to create the fifo, but do not bail out if it already exists.
            if (mkfifo(pPipes[dwIndex].pszPipePath, S_IRWXU) < 0)
            {
                if (errno == EEXIST)
                {
                    dwError = 0;
                }
                else
                {
                    dwError = LwMapErrnoToLwError(errno);
                }
                BAIL_ON_RSYS_ERROR(dwError);
            }
            // In case the file already existed, make sure it is really a fifo
            if (stat(pPipes[dwIndex].pszPipePath, &statBuf) < 0)
            {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_RSYS_ERROR(dwError);
            }
            if (S_ISFIFO(statBuf.st_mode))
            {
                // We're good
                break;
            }
            if (dwSyslogPid == -1)
            {
                RSYS_LOG_ERROR("The syslog reaper pipes already exist as non-pipes. Usually this means that syslog was started before syslog reaper, but the pid for syslog cannot be determined, so cannot be restarted. Please manually clear out the existing files and restart syslog and reapsysl.");
                dwError = LW_STATUS_PIPE_NOT_AVAILABLE;
                BAIL_ON_RSYS_ERROR(dwError);
            }
            if (dwAttempt >= 3)
            {
                dwError = LW_STATUS_PIPE_NOT_AVAILABLE;
                BAIL_ON_RSYS_ERROR(dwError);
            }

            if (unlink(pPipes[dwIndex].pszPipePath) < 0)
            {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_RSYS_ERROR(dwError);
            }
        }

        // The reaper only reads from the pipe, but opening it with read and
        // write access ensures there is always one writer connected, so the
        // reaper does not have to deal with handling disconnects.
        pPipes[dwIndex].pWritePipe = fopen(pPipes[dwIndex].pszPipePath, "r+");
        if (!pPipes[dwIndex].pWritePipe)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_RSYS_ERROR(dwError);
        }

        pPipes[dwIndex].pReadPipe = fopen(pPipes[dwIndex].pszPipePath, "r");
        if (!pPipes[dwIndex].pReadPipe)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_RSYS_ERROR(dwError);
        }
    }

    if (dwSyslogPid != (pid_t)-1 && kill(dwSyslogPid, SIGHUP) < 0)
    {
        if (errno == ESRCH)
        {
            RSYS_LOG_WARNING("Pid %d was read from syslog's pid file, but that process does not exist. Syslog is most likely not running on your system.", dwSyslogPid);
        }
        else
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_RSYS_ERROR(dwError);
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
RSysSrvWaitForPipeDisconnect(
    IN DWORD dwPipeCount,
    IN RSYS_SOURCE* pPipes,
    IN uint64_t qwTimeout
    )
{
    DWORD dwError = 0;
    fd_set readFds;
    int iMaxFd = -1;
    struct timeval tNow = { 0 };
    struct timeval tTimeOut = { 0 };
    uint64_t qwNow = 0;
    uint64_t qwStartTime = 0;
    int64_t llWaitTime = 0;
    char buffer[100] = { 0 };
    DWORD dwIndex = 0;
    int iSelectResult = 0;
    ssize_t sReadCount = 0;
    pid_t dwSyslogPid = (pid_t)-1;

    RSysSrvGetSyslogPid(&dwSyslogPid);

    if (gettimeofday(&tNow, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_RSYS_ERROR(dwError);
    }
    qwStartTime = (int64_t)tNow.tv_sec * 1000000 + tNow.tv_usec;

    while (1)
    {
        FD_ZERO(&readFds);

        iMaxFd = -1;
        for (dwIndex = 0; dwIndex < dwPipeCount; dwIndex++)
        {
            if (pPipes[dwIndex].pReadPipe != NULL)
            {
                iMaxFd = LW_MAX(iMaxFd, fileno(pPipes[dwIndex].pReadPipe));
                FD_SET(fileno(pPipes[dwIndex].pReadPipe), &readFds);
            }
        }

        if (iMaxFd == -1)
        {
            // All pipes are closed
            break;
        }

        if (gettimeofday(&tNow, NULL) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_RSYS_ERROR(dwError);
        }
        qwNow = (int64_t)tNow.tv_sec * 1000000 + tNow.tv_usec;

        llWaitTime = qwStartTime + qwTimeout - qwNow;
        if (llWaitTime < 0)
        {
            llWaitTime = 0;
        }
        if (llWaitTime > 1)
        {
            llWaitTime = 1;
        }
        tTimeOut.tv_sec = llWaitTime / 1000000;
        tTimeOut.tv_usec = llWaitTime % 1000000;

        iSelectResult = select(iMaxFd + 1, &readFds, NULL, NULL, &tTimeOut);
        if (iSelectResult < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_RSYS_ERROR(dwError);
        }
        if (iSelectResult == 0)
        {
            if (gettimeofday(&tNow, NULL) < 0)
            {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_RSYS_ERROR(dwError);
            }
            qwNow = (int64_t)tNow.tv_sec * 1000000 + tNow.tv_usec;

            if (qwStartTime + qwTimeout < qwNow)
            {
                RSYS_LOG_ERROR("Timed out waiting for syslog to close reapsysld's named pipes. They will now be closed anyway");
                dwError = ERROR_TIMEOUT;
                BAIL_ON_RSYS_ERROR(dwError);
            }
            else
            {
                // Send it another HUP
                kill(dwSyslogPid, SIGHUP);
            }
        }
        for (dwIndex = 0; dwIndex < dwPipeCount; dwIndex++)
        {
            if (pPipes[dwIndex].pReadPipe &&
                    FD_ISSET(fileno(pPipes[dwIndex].pReadPipe), &readFds))
            {
                sReadCount = read(
                                fileno(pPipes[dwIndex].pReadPipe),
                                buffer,
                                sizeof(buffer));
                if (sReadCount < 0)
                {
                    dwError = LwMapErrnoToLwError(errno);
                    BAIL_ON_RSYS_ERROR(dwError);
                }
                else if (sReadCount == 0)
                {
                    // Writer closed the pipe. It is now safe to close the read
                    // side.
                    fclose(pPipes[dwIndex].pReadPipe);
                    pPipes[dwIndex].pReadPipe = NULL;
                }
            }
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

VOID
RSysSrvClosePipes(
    IN DWORD dwPipeCount,
    IN RSYS_SOURCE* pPipes
    )
{
    DWORD dwIndex = 0;
    pid_t dwSyslogPid = (pid_t)-1;
    BOOLEAN bUnlinked = FALSE;

    /* For safety, remove the named pipe when the daemon exits. If syslog tried
     * to open the named pipe while reapsysld was not running, syslog could
     * freeze waiting for reader to open the pipe. Also syslog needs to
     * transition from writing to a pipe to writing to a file or nothing.
     * Otherwise, syslog could die with SIGPIPE.
     */
    for (dwIndex = 0; dwIndex < dwPipeCount; dwIndex++)
    {
        if (pPipes[dwIndex].pReadPipe)
        {
            unlink(pPipes[dwIndex].pszPipePath);
            bUnlinked = TRUE;
        }
        if (pPipes[dwIndex].pWritePipe)
        {
            fclose(pPipes[dwIndex].pWritePipe);
            pPipes[dwIndex].pWritePipe = NULL;
        }
    }

    if (bUnlinked)
    {
        RSysSrvGetSyslogPid(&dwSyslogPid);

        if (dwSyslogPid != (pid_t)-1)
        {
            kill(dwSyslogPid, SIGHUP);

            // Wait for syslog to close its side of the pipes before closing
            // our side.
            RSysSrvWaitForPipeDisconnect(
                    dwPipeCount,
                    pPipes,
                    60 * 1000000);
        }
    }

    for (dwIndex = 0; dwIndex < dwPipeCount; dwIndex++)
    {
        RtlCStringFree(&pPipes[dwIndex].pszLastReadLine);
        LW_RTL_FREE(&pPipes[dwIndex].pszReadBufferStart);

        if (pPipes[dwIndex].pReadPipe)
        {
            fclose(pPipes[dwIndex].pReadPipe);
        }
    }
}

PVOID
RSysSrvReaderThreadRoutine(
    IN PVOID pUnused
    )
{
    DWORD dwError = 0;
    RSYS_SOURCE sources[3] = { {0}, {0}, {0} };
    DWORD dwIndex = 0;
    int iMaxFd = -1;
    fd_set readFds;
    RSYS_LINE_BUFFER lineBuffer = { 0 };
    struct timeval tNow;
    struct timeval tTimeOut;
    struct timeval* ptTimeOut = NULL;
    uint64_t qwNow;
    int64_t llWaitTime;
    PEVENT_LOG_RECORD pRecordList = NULL;
    PEVENT_LOG_RECORD pNewRecordList = NULL;
    DWORD dwListCapacity = 0;
    DWORD dwListSize = 0;
    BOOLEAN bIncrementSize = FALSE;
    DWORD dwEscrowTime = 0;
    BOOLEAN lsassUnreachable = FALSE;

    dwError = RSysSrvGetEscrowTime(
                    NULL,
                    &dwEscrowTime);
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = LwRtlRBTreeCreate(
                    (PFN_LWRTL_RB_TREE_COMPARE)strcmp,
                    RSysSrvNOPFree,
                    (PFN_LWRTL_RB_TREE_FREE_DATA)RSysSrvFreeLine,
                    &lineBuffer.pRBTree);
    BAIL_ON_RSYS_ERROR(dwError);

    sources[0].pszEventType = "Information";
    sources[0].dwPriority = 1;
    sources[0].pszPipePath = RSYS_PIPE_DIR "/information";
    sources[1].pszEventType = "Warning";
    sources[1].dwPriority = 2;
    sources[1].pszPipePath = RSYS_PIPE_DIR "/warning";
    sources[2].pszEventType = "Error";
    sources[2].dwPriority = 3;
    sources[2].pszPipePath = RSYS_PIPE_DIR "/error";

    dwError = RSysSrvOpenPipes(
                    sizeof(sources)/sizeof(sources[0]),
                    sources);
    BAIL_ON_RSYS_ERROR(dwError);

    FD_ZERO(&readFds);

    RSYS_LOG_INFO("Syslog reaper reader thread started");

    while (1)
    {
        iMaxFd = gpiSignalReaderExitPipe[0];
        FD_SET(gpiSignalReaderExitPipe[0], &readFds);

        for (dwIndex = 0;
            dwIndex < sizeof(sources)/sizeof(sources[0]);
            dwIndex++)
        {
            iMaxFd = LW_MAX(iMaxFd, fileno(sources[dwIndex].pReadPipe));
            FD_SET(fileno(sources[dwIndex].pReadPipe), &readFds);
        }

        if (gettimeofday(&tNow, NULL) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_RSYS_ERROR(dwError);
        }
        qwNow = (int64_t)tNow.tv_sec * 1000000 + tNow.tv_usec;

        if (lineBuffer.pHead)
        {
            llWaitTime = lineBuffer.pHead->qwTimeStamp + dwEscrowTime - qwNow;
            if (llWaitTime < 0)
            {
                llWaitTime = 0;
            }
            if (lsassUnreachable && llWaitTime < 10 * 1000000)
            {
                llWaitTime = 10 * 1000000;
            }
            tTimeOut.tv_sec = llWaitTime / 1000000;
            tTimeOut.tv_usec = llWaitTime % 1000000;
            ptTimeOut = &tTimeOut;
        }
        else
        {
            ptTimeOut = NULL;
        }

        if (select(iMaxFd + 1, &readFds, NULL, NULL, ptTimeOut) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_RSYS_ERROR(dwError);
        }

        if (FD_ISSET(gpiSignalReaderExitPipe[0], &readFds))
        {
            break;
        }

        if (gettimeofday(&tNow, NULL) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_RSYS_ERROR(dwError);
        }
        qwNow = (uint64_t)tNow.tv_sec * 1000000 + tNow.tv_usec;

        for (dwIndex = 0;
            dwIndex < sizeof(sources)/sizeof(sources[0]);
            dwIndex++)
        {
            RSYS_SOURCE *pSource = &sources[dwIndex];
            if (FD_ISSET(fileno(pSource->pReadPipe), &readFds))
            {
                dwError = RSysSrvReadSource(
                                pSource,
                                &lineBuffer,
                                qwNow);
                BAIL_ON_RSYS_ERROR(dwError);
            }
        }

        lsassUnreachable = FALSE;

        while (lineBuffer.pHead && lineBuffer.pHead->qwTimeStamp + dwEscrowTime < qwNow)
        {
            // Do not free this directly. Let it get freed when it is removed
            // from the red black tree.
            RSYS_LINE_NODE* pLine = lineBuffer.pHead;

            for (dwIndex = 0; dwIndex < pLine->dwCount; dwIndex++)
            {
                if (dwListSize == dwListCapacity)
                {
                    if (pRecordList)
                    {
                        dwError = RSysSrvWriteEvents(
                                        dwListSize,
                                        pRecordList);
                        if (dwError == rpc_s_connect_rejected)
                        {
                            dwError = 0;
                            RSYS_LOG_ERROR("Eventlog not running. The syslog reaper will buffer events in memory until it is ready.");
                        }
                        else
                        {
                            BAIL_ON_RSYS_ERROR(dwError);
                            LWIFreeEventRecordList(
                                dwListSize,
                                pRecordList);
                            pRecordList = NULL;
                            dwListCapacity = 0;
                            dwListSize = 0;
                        }
                    }

                    dwListCapacity = dwListSize + 500;
                    pNewRecordList = RtlMemoryRealloc(
                                    pRecordList,
                                    dwListCapacity * sizeof(pRecordList[0]));
                    if (pNewRecordList == NULL)
                    {
                        dwError = ENOMEM;
                        BAIL_ON_RSYS_ERROR(dwError);
                    }
                    pRecordList = pNewRecordList;
                }
                dwError = RSysSrvParseLine(
                                pLine,
                                tNow.tv_sec,
                                &pRecordList[dwListSize],
                                &bIncrementSize);
                if (dwError == LW_ERROR_LSA_SERVER_UNREACHABLE)
                {
                    lsassUnreachable = TRUE;
                    RSYS_LOG_ERROR("Lsass not reachable. Reapsysl will buffer events in memory.");
                    break;
                }
                BAIL_ON_RSYS_ERROR(dwError);
                
                if (bIncrementSize)
                {
                    dwListSize++;
                }
            }

            if (lsassUnreachable)
            {
                break;
            }

            RSysSrvUnlinkLine(
                &lineBuffer,
                pLine);

            dwError = LwRtlRBTreeRemove(lineBuffer.pRBTree, pLine->pszData);
            BAIL_ON_RSYS_ERROR(dwError);
        }

        if (dwListSize)
        {
            dwError = RSysSrvWriteEvents(
                            dwListSize,
                            pRecordList);
            if (dwError == rpc_s_connect_rejected)
            {
                dwError = 0;
                RSYS_LOG_ERROR("Eventlog not running. The syslog reaper will buffer events in memory until it is ready.");
            }
            else
            {
                BAIL_ON_RSYS_ERROR(dwError);
                LWIFreeEventRecordList(
                    dwListSize,
                    pRecordList);
                pRecordList = NULL;
                dwListCapacity = 0;
                dwListSize = 0;
            }
        }
    }

    RSYS_LOG_INFO("Syslog reaper reader thread stopped");

cleanup:
    RSysSrvClosePipes(
            sizeof(sources)/sizeof(sources[0]),
            sources);
    LwRtlRBTreeFree(lineBuffer.pRBTree);

    if (pRecordList)
    {
        LWIFreeEventRecordList(
            dwListSize,
            pRecordList);
    }

    if (dwError != 0)
    {
        RSYS_LOG_ERROR(
                "Syslog reaper reader thread exiting with code %d",
                dwError);
        kill(getpid(), SIGTERM);
    }
    return NULL;

error:
    goto cleanup;
}

DWORD
RSysSrvStopReaderThread(
    VOID
    )
{
    DWORD dwError = 0;

    if (gpiSignalReaderExitPipe[1] != -1)
    {
        if (close(gpiSignalReaderExitPipe[1]) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_RSYS_ERROR(dwError);
        }
        gpiSignalReaderExitPipe[1] = -1;

        dwError = pthread_join(
                        gReaderThread,
                        NULL);
        BAIL_ON_RSYS_ERROR(dwError);
    }

    if (gpiSignalReaderExitPipe[0] != -1)
    {
        if (close(gpiSignalReaderExitPipe[0]) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_RSYS_ERROR(dwError);
        }
        gpiSignalReaderExitPipe[0] = -1;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
RSysSrvStartReaderThread(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bDestroyOnError = FALSE;

    if (gpiSignalReaderExitPipe[0] != -1 || gpiSignalReaderExitPipe[1] != -1)
    {
        dwError = EEXIST;
        BAIL_ON_RSYS_ERROR(dwError);
    }

    bDestroyOnError = TRUE;

    if (pipe(gpiSignalReaderExitPipe) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_RSYS_ERROR(dwError);
    }

    dwError = pthread_create(
                    &gReaderThread,
                    NULL,
                    RSysSrvReaderThreadRoutine,
                    NULL);
    BAIL_ON_RSYS_ERROR(dwError);

cleanup:
    return dwError;

error:
    if (bDestroyOnError)
    {
        if (gpiSignalReaderExitPipe[0] != -1)
        {
            close(gpiSignalReaderExitPipe[0]);
            gpiSignalReaderExitPipe[0] = -1;
        }
        if (gpiSignalReaderExitPipe[1] != -1)
        {
            close(gpiSignalReaderExitPipe[1]);
            gpiSignalReaderExitPipe[1] = -1;
        }
    }
    goto cleanup;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
