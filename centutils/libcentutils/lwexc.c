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
#include "lwexc.h"
#include "ctstrutils.h"
#include <lw/winerror.h>
#include <lwerror.h>

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

static LWException memExc =
{
    .code = ERROR_OUTOFMEMORY,
    .shortMsg = "Out of memory",
    .longMsg = "A memory allocation failed due to insufficient system resources.",
    .stack =
    {
	NULL,
	0,
	NULL
    }
};

static LWException successExc =
{
    .code = ERROR_SUCCESS,
    .shortMsg = "Success",
    .longMsg = "The operation succeeded without error.",
    .stack =
    {
	NULL,
	0,
	NULL
    }
};

static LWException*
CreateException(
    DWORD code,
    const char* file,
    unsigned int line,
    char* shortMsg,
    char* longMsg
    )
{
    LWException* exc;

    switch (code)
    {
    case ERROR_SUCCESS:
	return &successExc;
    case ERROR_OUTOFMEMORY:
	return &memExc;
    default:
	exc = malloc(sizeof(*exc));
	if (!exc)
	    return &memExc;
	exc->code = code;
        exc->subcode = 0;
	exc->stack.file = file;
	exc->stack.line = line;
	exc->stack.down = NULL;
	exc->shortMsg = shortMsg;
	exc->longMsg = longMsg;

	return exc;
    }
}

void
LWRaise(
    LWException** dest, 
    DWORD code
    )
{
    DWORD ceError;
    char *shortMsg;
    char *longMsg;
    const char* desc = LwWin32ExtErrorToName(code);
    const char* help = LwWin32ExtErrorToDescription(code);

    if (!desc)
    {
        shortMsg = "Undocumented exception";
    }
    if ((ceError = CTAllocateString(desc, &shortMsg)))
    {
	*dest = CreateException(ceError, __FILE__, __LINE__, NULL, NULL);
	return;
    }

    if (!help)
    {
        longMsg = "An undocumented exception has occurred. Please contact Likewise technical support and use the error code to identify this exception.";
    }
    if ((ceError = CTAllocateString(help, &longMsg)))
    {
	*dest = CreateException(ceError, __FILE__, __LINE__, NULL, NULL);
	return;
    }
    
    *dest = CreateException(code, NULL, 0, shortMsg, longMsg);
}

void
LWRaiseEx(
    LWException** dest,
    DWORD code,
    const char* file,
    unsigned int line,
    const char* _shortMsg,
    const char* fmt,
    ...
    )
{
    if (dest)
    {
	DWORD ceError;
	char* shortMsg;
	char* longMsg;
	va_list ap;
	
	va_start(ap, fmt);
	
	if (!_shortMsg)
	{
	    _shortMsg = LwWin32ExtErrorToName(code);
	}
        if (!_shortMsg)
        {
            _shortMsg = "Undocumented exception";
        }

	if (!fmt)
	{
	    fmt = LwWin32ExtErrorToDescription(code);
	}
        if (!fmt)
        {
            fmt = "An undocumented exception has occurred. Please contact Likewise technical support and use the error code to identify this exception.";
        }

	if (_shortMsg)
	{
	    if ((ceError = CTAllocateString(_shortMsg, &shortMsg)))
	    {
		*dest = CreateException(ceError, __FILE__, __LINE__, NULL, NULL);
		return;
	    }
	}
	else
	{
	    shortMsg = NULL;
	}
	    
	if (fmt)
	{
	    if ((ceError = CTAllocateStringPrintfV(&longMsg, fmt, ap)))
	    {
		CTFreeString(shortMsg);
		*dest = CreateException(ceError, __FILE__, __LINE__, NULL, NULL);
		return;
	    }
	}
	else
	{
	    longMsg = NULL;
	}

	*dest = CreateException(code, file, line, shortMsg, longMsg);
    }
}

void
LWReraise(
    LWException** dest,
    LWException** src
    )
{
    if (dest)
    {
	*dest = *src;
	*src = NULL;
    }
    else
    {
	LWHandle(src);
    }
}

void
LWReraiseEx(
    LWException** dest,
    LWException** src,
    const char* file,
    unsigned int line
    )
{
    if (dest)
    {
	LWStackFrame* down = malloc(sizeof(*down));

	if (!down)
	{
	    LWHandle(src);
	    *dest = CreateException(ERROR_OUTOFMEMORY, file, line, NULL, NULL);
	}
	else
	{
	    *dest = *src;
	    *src = NULL;
	    
	    *down = (*dest)->stack;
	    (*dest)->stack.file = file;
	    (*dest)->stack.line = line;
	    (*dest)->stack.down = down;
	}
    }
    else
    {
	LWHandle(src);
    }
}

void
LWHandle(
    LWException** exc
    )
{
    if (exc && *exc)
    {
	if (*exc != &memExc && *exc != &successExc)
        {
            LWStackFrame* frame;
            LWStackFrame* nextFrame;
            
            for (frame = (*exc)->stack.down; frame; frame = nextFrame)
            {
                nextFrame = frame->down;
                free(frame);
            }

            if ((*exc)->shortMsg)
                free((*exc)->shortMsg);
            if ((*exc)->longMsg)
                free((*exc)->longMsg);
            free(*exc);
        }
	*exc = NULL;
    }
}

DWORD LWExceptionToString(const LWException *conv, PCSTR titlePrefix, BOOLEAN showSymbolicCode, BOOLEAN showTrace, PSTR *result)
{
    DWORD ceError;
    PSTR ret = NULL;
    PSTR temp = NULL;
    PCSTR codeName = NULL;

    if(titlePrefix == NULL)
        titlePrefix = "";
    
    if(showSymbolicCode)
        codeName = LwWin32ExtErrorToName(conv->code);

    if(codeName != NULL)
    {
        GCE(ceError = CTAllocateStringPrintf(
            &ret, "%s%s [%s]\n\n%s", titlePrefix, conv->shortMsg, codeName, conv->longMsg));
    }
    else
    {
        GCE(ceError = CTAllocateStringPrintf(
            &ret, "%s%s [code 0x%.8x]\n\n%s", titlePrefix, conv->shortMsg, conv->code, conv->longMsg));
    }
    if(showTrace)
    {
        const LWStackFrame *frame = &conv->stack;

        temp = ret;
        GCE(ceError = CTAllocateStringPrintf(
                &ret, "%s\n\nStack Trace:", temp));
        CT_SAFE_FREE_STRING(temp);
        while(frame != NULL)
        {
            temp = ret;
            GCE(ceError = CTAllocateStringPrintf(
                    &ret, "%s\n%s:%d", temp, frame->file, frame->line));
            CT_SAFE_FREE_STRING(temp);
            frame = frame->down;
        }
    }
    *result = ret;
    ret = NULL;

cleanup:
    CT_SAFE_FREE_STRING(temp);
    CT_SAFE_FREE_STRING(ret);
    return ceError;
}

DWORD LWPrintException(FILE *dest, const LWException *print, BOOLEAN showTrace)
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR string = NULL;
    PSTR wrapped = NULL;
    int columns;

    ceError = LWExceptionToString(print, "Error: ", FALSE, showTrace, &string);
    GCE(ceError);
    //Don't word wrap if the terminal width can't be determined
    if (CTGetTerminalWidth(fileno(dest), &columns))
        columns = -1;
    ceError = CTWordWrap(string, &wrapped, 4, columns);
    GCE(ceError);
    fprintf(dest, "%s\n", wrapped);

cleanup:
    if(ceError)
    {
        fprintf(dest, "Error %x occurred while trying to print exception\n", ceError);
    }
    CT_SAFE_FREE_STRING(string);
    CT_SAFE_FREE_STRING(wrapped);
    return ceError;
}
