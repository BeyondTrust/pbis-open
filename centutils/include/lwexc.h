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

#ifndef __LW_EXCEPTION_H__
#define __LW_EXCEPTION_H__

#include <ctsys.h>
#include <ctdef.h>

typedef struct _LWStackFrame
{
    const char* file;
    unsigned int line;
    struct _LWStackFrame *down;
} LWStackFrame;

typedef struct _LWException
{
    DWORD code;
    DWORD subcode;
    char* shortMsg;
    char* longMsg;
    LWStackFrame stack;
} LWException;

DWORD LWExceptionToString(const LWException *conv, PCSTR titlePrefix, BOOLEAN showSymbolicCode, BOOLEAN showTrace, PSTR *result);

DWORD LWPrintException(FILE *dest, const LWException *print, BOOLEAN showTrace);

void
LWRaise(
    LWException** dest, 
    DWORD code
    );

void
LWRaiseEx(
    LWException** dest,
    DWORD code,
    const char* file,
    unsigned int line,
    const char* shortMsg,
    const char* fmt,
    ...
    );

void
LWReraise(
    LWException** dest,
    LWException** src
    );

void
LWReraiseEx(
    LWException** dest,
    LWException** src,
    const char* file,
    unsigned int line
    );

void
LWHandle(
    LWException** exc
    );

#define LW_RAISE(dest, code)			\
    LWRaiseEx(dest, code, __FILE__, __LINE__,	\
	      NULL, NULL)			\

#define LW_RAISE_EX(dest, code, short_msg, __ARGS__...)	\
    LWRaiseEx(dest, code, __FILE__, __LINE__,		\
	      short_msg, ## __ARGS__)			\
    
#define LW_HANDLE(exc)				\
    LWHandle(exc)				\
    
#define LW_RERAISE(dest, src)			\
    LWReraiseEx(dest, src, __FILE__, __LINE__)	\

#define LW_IS_OK(exc)				\
    ((exc) == NULL || (exc)->code == 0)


#define LW_CLEANUP(dest, src)			\
    do						\
    {						\
	LWException** __exc = &(src);		\
	if (!LW_IS_OK(*__exc))			\
	{					\
	    LW_RERAISE(dest, __exc);		\
	    goto cleanup;			\
	}					\
    } while (0)					\

#define LW_CLEANUP_CTERR(dest, err)		\
    do						\
    {						\
	DWORD _err = (err);			\
	if (_err)				\
	{					\
	    LW_RAISE(dest, _err);		\
	    goto cleanup;			\
	}					\
    } while (0)					\

#define LW_CLEANUP_DLERROR(dest)		\
    do						\
    {						\
        LW_RAISE_EX((dest), DWORD_INCOMPATIBLE_LIBRARY, \
                "An error occurred loading/unloading a library", \
                "The following error dlerror occurred '%s'.", \
                dlerror());             	\
        goto cleanup;		        	\
    } while (0)					\

#define LW_EXC					\
    __lw_exc__					\

#define LW_TRY(dest, expr)			\
    do						\
    {						\
	LWException* LW_EXC = NULL;		\
	( expr );				\
	LW_CLEANUP(dest, LW_EXC);		\
    } while (0)					\


#endif
