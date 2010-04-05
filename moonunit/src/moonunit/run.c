/*
 * Copyright (c) 2007-2008, Brian Koropoff
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "run.h"
#include "gdb.h"

#include <moonunit/private/util.h>
#include <moonunit/library.h>
#include <moonunit/error.h>

#include <stdlib.h>
#include <string.h>

static int
test_compare(const void* _a, const void* _b)
{
	MuTest* a = *(MuTest**) _a;
	MuTest* b = *(MuTest**) _b;
	int result;
	
	if ((result = strcmp(Mu_Test_Suite(a), Mu_Test_Suite(b))))
		return result;
	else
#ifdef SYMBOL_LAYOUT_REVERSE
		return (a == b) ? 0 : ((a < b) ? 1 : -1);
#else
		return (a == b) ? 0 : ((a < b) ? -1 : 1);
#endif
}

static unsigned int
test_count(MuTest** tests)
{
	unsigned int result;
	
	for (result = 0; tests[result]; result++);
	
	return result;
}

static bool
in_set(MuTest* test, int setc, char** set)
{
    unsigned int i;
    const char* test_name = Mu_Test_Name(test);
    const char* suite_name = Mu_Test_Suite(test);
    const char* library_name = Mu_Library_Name(test->library);
    char* test_path = format("%s/%s/%s", library_name, suite_name, test_name);
    bool result;

    for (i = 0; i < setc; i++)
    {
        if (match_path(test_path, set[i]))
        {
            result = true;
            goto done;
        }
    }

    result = false;

done:

    if (test_path)
    {
        free(test_path);
    }

    return result;
}

static void
event_proxy_cb(MuLogEvent* event, void* data)
{
    MuLogger* logger = (MuLogger*) data;

    Mu_Logger_TestLog(logger, event);
}

unsigned int
run_tests(RunSettings* settings, const char* path, int setc, char** set, MuError** _err)
{
    MuError* err = NULL;
    unsigned int failed = 0;
    MuLogger* logger = settings->logger;
    MuLoader* loader = settings->loader;
    MuLibrary* library = NULL;
    MuTest** tests = NULL;

    if (err)
    {
        MU_RERAISE_GOTO(error, _err, err);
    }

    library = Mu_Loader_Open(loader, path, &err);

    /* Even if library loading failed, log that
       we attempted to visit it */
    Mu_Logger_LibraryEnter(logger, path, library); 

    MU_CATCH(err, MU_ERROR_LOAD_LIBRARY)
    {
        Mu_Logger_LibraryFail(logger, err->message);
        failed++;
        MU_HANDLE(&err);
        goto leave;
    }

    Mu_Library_Construct(library, &err);

    MU_CATCH(err, MU_ERROR_CONSTRUCT_LIBRARY)
    {
        Mu_Logger_LibraryFail(logger, err->message);
        failed++;
        MU_HANDLE(&err);
        goto leave;
    }

    MU_CATCH_ALL(err)
    {
        MU_RERAISE_GOTO(error, _err, err);
    }

    tests = Mu_Library_GetTests(library);
    
    if (tests)
    {
        qsort(tests, test_count(tests), sizeof(*tests), test_compare);
        
        unsigned int index;
        const char* current_suite = NULL;
        
        for (index = 0; tests[index]; index++)
        {
            MuTestResult* summary = NULL;
            MuTest* test = tests[index];

            if (set != NULL && !in_set(test, setc, set))
                continue;
            
            if (current_suite == NULL || strcmp(current_suite, Mu_Test_Suite(test)))
            {
                if (current_suite)
                    Mu_Logger_SuiteLeave(logger);
                current_suite = Mu_Test_Suite(test);
                Mu_Logger_SuiteEnter(logger, Mu_Test_Suite(test));
            }
            
            Mu_Logger_TestEnter(logger, test);
            summary = loader->dispatch(loader, test, event_proxy_cb, logger);
            Mu_Logger_TestLeave(logger, test, summary);
            
            if (summary->status != MU_STATUS_SKIPPED &&
                summary->status != summary->expected &&
                settings->debug &&
                loader->debug != NULL)
            {
                void* bp;
                pid_t pid = loader->debug(loader, test, summary->stage, &bp);
                char* breakpoint = NULL;

                breakpoint = format("*0x%lx", (unsigned long) bp);

                gdb_attach_interactive(settings->self, pid, breakpoint);

                if (breakpoint)
                    free(breakpoint);
            }
            
	    if (summary->status != MU_STATUS_SKIPPED &&
            summary->status != summary->expected)
	      failed++;

            loader->free_result(loader, summary);
        }
        
        if (current_suite)
            Mu_Logger_SuiteLeave(logger);
    }

    Mu_Library_Destruct(library, &err);

    MU_CATCH(err, MU_ERROR_DESTRUCT_LIBRARY)
    {
        Mu_Logger_LibraryFail(logger, err->message);
        failed++;
        MU_HANDLE(&err);
        goto leave;
    }
    MU_CATCH_ALL(err)
    {
        MU_RERAISE_GOTO(error, _err, err);
    }    

leave:

    Mu_Logger_LibraryLeave(logger);
    
error:
    if (tests)
        Mu_Library_FreeTests(library, tests);
   
    if (library)
        Mu_Library_Close(library);

    return failed;
}

unsigned int
run_all(RunSettings* settings, const char* path, MuError** _err)
{
    return run_tests(settings, path, 0, NULL, _err);
}
