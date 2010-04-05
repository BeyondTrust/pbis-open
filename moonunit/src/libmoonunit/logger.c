/*
 * Copyright (c) 2007, Brian Koropoff
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Moonunit project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRIAN KOROPOFF ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL BRIAN KOROPOFF BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <moonunit/loader.h>
#include <moonunit/private/util.h>
#include <moonunit/logger.h>
#include <moonunit/library.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void
Mu_Logger_SetOption(MuLogger* logger, const char *name, ...)
{
    va_list ap;

    va_start(ap, name);

    Mu_Option_Setv(logger->options, logger, name, ap);

    va_end(ap);
}

void 
Mu_Logger_SetOptionString(MuLogger* logger, const char *name, const char *value)
{
    Mu_Option_SetString(logger->options, logger, name, value);
}

MuType
Mu_Logger_OptionType(MuLogger* logger, const char *name)
{
    return Mu_Option_Type(logger->options, name);
}

void
Mu_Logger_Enter(MuLogger* logger)
{
    logger->enter(logger);
}
void
Mu_Logger_Leave(MuLogger* logger)
{
    logger->leave(logger);
}

void
Mu_Logger_LibraryEnter (struct MuLogger* logger, const char* path, MuLibrary* library)
{
    logger->library_enter(logger, path, library);
}

void
Mu_Logger_LibraryFail (struct MuLogger* logger, const char* reason)
{
    logger->library_fail(logger, reason);
}

void
Mu_Logger_LibraryLeave (struct MuLogger* logger)
{
    logger->library_leave(logger);
}

void
Mu_Logger_SuiteEnter (struct MuLogger* logger, const char* name)
{
    logger->suite_enter(logger, name);
}

void
Mu_Logger_SuiteLeave (struct MuLogger* logger)
{
    logger->suite_leave(logger);
}

void
Mu_Logger_TestEnter (struct MuLogger* logger, struct MuTest* test)
{
    logger->test_enter(logger, test);
}

void
Mu_Logger_TestLog (struct MuLogger* logger, struct MuLogEvent* event)
{
    logger->test_log(logger, event);
}

void
Mu_Logger_TestLeave (struct MuLogger* logger, 
                     struct MuTest* test, struct MuTestResult* summary)
{
    logger->test_leave(logger, test, summary);
}

void
Mu_Logger_Destroy(MuLogger* logger)
{
    logger->destroy(logger);
}
