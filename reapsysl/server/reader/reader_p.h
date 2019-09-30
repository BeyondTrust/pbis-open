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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        poller_p.h
 *
 * Abstract:
 *
 *        Event forwarder from eventlogd to collector service
 * 
 *        Poller thread
 *
 *        Private functions for the poller thread
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 * 
 */
#ifndef __READER_P_H__
#define __READER_P_H__

typedef struct _RSYS_SOURCE
{
    // Represents the entire buffer (including uninitialized area)
    PSTR pszReadBufferStart;
    PSTR pszReadBufferEnd;
    // Represents the portion of the buffer that currently holds valid data
    PSTR pszReadDataStart;
    PSTR pszReadDataEnd;

    // This is necessary incase syslog says lines are repeated
    PSTR pszLastReadLine;

    PCSTR pszEventType;
    // Higher numbers are have higher priority
    DWORD dwPriority;
    PCSTR pszPipePath;
    FILE* pReadPipe;
    FILE* pWritePipe;
} RSYS_SOURCE;

typedef struct _RSYS_LINE_NODE RSYS_LINE_NODE;

struct _RSYS_LINE_NODE
{
    PSTR pszData;
    DWORD dwCount;
    uint64_t qwTimeStamp; //In microseconds since 1970
    RSYS_SOURCE* pSource;
    RSYS_LINE_NODE* pNext;
    RSYS_LINE_NODE* pPrev;
};

typedef struct _RSYS_LINE_BUFFER
{
    // in ascending order by timestamp
    RSYS_LINE_NODE* pHead;
    RSYS_LINE_NODE* pTail;
    // indexed by pszData and points to the same nodes
    PLWRTL_RB_TREE  pRBTree;
} RSYS_LINE_BUFFER;

VOID
RSysUnescapeUser(
    IN OUT PSTR pszUser
    );

DWORD
RSysSrvCheckLineMatch(
    IN PCSTR pszLine,
    IN RSYS_MESSAGE_PATTERN* pPattern,
    IN OUT HANDLE* phLsaConnection,
    OUT PSTR* ppszUser,
    OUT BOOLEAN* pbMatched
    );

DWORD
RSysSrvParseLine(
    IN RSYS_LINE_NODE* pLine,
    IN time_t tNow,
    OUT PEVENT_LOG_RECORD pRecord,
    OUT PBOOLEAN pbNonBlank
    );

VOID
RSysSrvUnlinkLine(
    RSYS_LINE_BUFFER *pBuffer,
    RSYS_LINE_NODE *pLine
    );

VOID
RSysSrvLinkAtTail(
    RSYS_LINE_BUFFER *pBuffer,
    RSYS_LINE_NODE *pLine
    );

VOID
RSysSrvFreeLine(
    RSYS_LINE_NODE* pLine
    );

DWORD
RSysSrvAddLine(
    IN OUT RSYS_LINE_BUFFER *pBuffer,
    // The passed in line must not be in the red black tree or linked list in
    // pBuffer
    IN OUT RSYS_LINE_NODE** ppLine
    );

DWORD
RSysSrvReadSource(
    IN OUT RSYS_SOURCE* pSource,
    IN OUT RSYS_LINE_BUFFER* pBuffer,
    uint64_t qwNow
    );

VOID
RSysSrvNOPFree(
    VOID *pUnused
    );

DWORD
RSysSrvGetSyslogPid(
    pid_t* pdwSyslogPid
    );

DWORD
RSysSrvOpenPipes(
    IN DWORD dwPipeCount,
    IN OUT RSYS_SOURCE* pPipes
    );

DWORD
RSysSrvWaitForPipeDisconnect(
    IN DWORD dwPipeCount,
    IN RSYS_SOURCE* pPipes,
    IN uint64_t qwTimeout
    );

VOID
RSysSrvClosePipes(
    IN DWORD dwPipeCount,
    IN RSYS_SOURCE* pPipes
    );

DWORD
RSysSrvWriteEvents(
    DWORD dwCount,
    PEVENT_LOG_RECORD pList
    );

PVOID
RSysSrvPollerThreadRoutine(
    IN PVOID pUnused
    );

#endif /* __READER_P_H__ */
