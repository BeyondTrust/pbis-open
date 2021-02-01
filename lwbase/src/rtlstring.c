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
 *        rtlstring.c
 *
 * Abstract:
 *
 *        Base String Helpers
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#include "includes.h"
#include <lw/rtlstring.h>
#include <lw/rtlmemory.h>
#include <lw/rtlgoto.h>
#include <wc16str.h>

// XXX - HACK!!!
#define RTLP_STRING_LOG_COUNT 1000

typedef struct _RTLP_STRING_LOG_DATA {
    ULONG NextIndex;
    PSTR ppszString[RTLP_STRING_LOG_COUNT];
} RTLP_STRING_LOG_DATA, *PRTLP_STRING_LOG_DATA;

RTLP_STRING_LOG_DATA gpLwRtlStringLogData = { 0 };
#define RTLP_STRING_NULL_TEXT "<null>"

static
PCSTR
LwRtlpStringLogRotate(
    IN OPTIONAL PSTR pszString
    )
{
    // TODO-Locking or interlocked increment.
    if (gpLwRtlStringLogData.ppszString[gpLwRtlStringLogData.NextIndex])
    {
       free(gpLwRtlStringLogData.ppszString[gpLwRtlStringLogData.NextIndex]);
    }
    gpLwRtlStringLogData.ppszString[gpLwRtlStringLogData.NextIndex] = pszString;
    gpLwRtlStringLogData.NextIndex++;
    if (gpLwRtlStringLogData.NextIndex >= RTLP_STRING_LOG_COUNT)
    {
        gpLwRtlStringLogData.NextIndex = 0;
    }
    return pszString ? pszString : RTLP_STRING_NULL_TEXT;
}

PCSTR
LwRtlUnicodeStringToLog(
    IN PUNICODE_STRING pString
    )
{
    PCSTR pszOutput = NULL;

    if (LW_RTL_STRING_IS_NULL_TERMINATED(pString))
    {
        pszOutput = RtlWC16StringToLog(pString->Buffer);
    }
    else
    {
        UNICODE_STRING tempString = { 0 };
        RtlUnicodeStringDuplicate(&tempString, pString);
        pszOutput = RtlWC16StringToLog(tempString.Buffer);
        RtlUnicodeStringFree(&tempString);
    }

    return pszOutput;
}

PCSTR
LwRtlAnsiStringToLog(
    IN PANSI_STRING pString
    )
{
    PCSTR pszOutput = NULL;

    if (LW_RTL_STRING_IS_NULL_TERMINATED(pString))
    {
        pszOutput = pString->Buffer;
    }
    else
    {
        ANSI_STRING tempString = { 0 };
        RtlAnsiStringDuplicate(&tempString, pString);
        pszOutput = LwRtlpStringLogRotate(tempString.Buffer ? strdup(tempString.Buffer) : NULL);
        RtlAnsiStringFree(&tempString);
    }

    return pszOutput;
}

PCSTR
LwRtlWC16StringToLog(
    IN PCWSTR pszString
    )
{
    return LwRtlpStringLogRotate(pszString ? awc16stombs(pszString) : NULL);
}

