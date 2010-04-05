/*
 * Copyright (c) Brian Koropoff
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

#include <moonunit/test.h>
#include <moonunit/library.h>
#include <moonunit/loader.h>
#include <moonunit/private/util.h>

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "sh-exec.h"

// Determines if a library can be opened by this loader
static bool
sh_can_open (struct MuLoader* self, const char* path)
{
    return ends_with(path, ".sh");
}

// Opens a library and returns a handle
static struct MuLibrary*
sh_open (struct MuLoader* self, const char* path, MuError** err)
{
    ShLibrary* library = NULL;
    Process handle;
    array* tests = NULL;
    char* line = NULL;
    char* dot = NULL;
    unsigned int len;
    struct stat statbuf;
    
    /* As a sanity check, make sure the file is actually valid */
    if (stat(path, &statbuf) != 0)
    {
        MU_RAISE_GOTO(error, err, MU_ERROR_LOAD_LIBRARY, "%s: %s",
                      path, strerror(errno));
    }

    if (!S_ISREG(statbuf.st_mode))
    {
        MU_RAISE_GOTO(error, err, MU_ERROR_LOAD_LIBRARY, "%s: not a file",
                      path);
    }

    library = calloc(1, sizeof(ShLibrary));

    library->base.loader = self;
    library->path = strdup(path);
    library->name = strdup(basename_pure(path));

    dot = strrchr(library->name, '.');
    if (dot)
    {
        *dot = '\0';
    }

    Mu_Sh_Exec(&handle, path, "mu_enum_test_functions >& ${MU_CMD_OUT}");

    while ((len = Process_Channel_ReadLine(&handle, 4, &line)))
    {
        ShTest* test = calloc(1, sizeof(ShTest));
        char* div1, *div2;

        line[len-1] = '\0';
        div1 = strchr(line, '_');
        div2 = strchr(div1+1, '_');

        if (div1 && div2)
        {
            test->base.loader = self;
            test->base.library = (MuLibrary*) library;
            test->function = strdup(line);

            *div1 = *div2 = '\0';

            test->suite = strdup(div1+1);
            test->name = strdup(div2+1);

            tests = array_append(tests, test);
        }

        free(line);
    }

    Process_Close(&handle);

    library->tests = (ShTest**) tests;

error:

    return (MuLibrary*) library;
}

static struct MuTest**
sh_get_tests (struct MuLoader* self, struct MuLibrary* handle)
{
    ShLibrary* library = (ShLibrary*) handle;

    return (MuTest**) array_dup((array*) library->tests);
}

static void
sh_free_tests (struct MuLoader* self, struct MuLibrary* handle, struct MuTest** list)
{
    array_free((array*) list);
}

static void
sh_close (struct MuLoader* self, struct MuLibrary* handle)
{
    ShLibrary* library = (ShLibrary*) handle;

    if (library->tests)
    {
        int i;

        for (i = 0; i < array_size((array*) library->tests); i++)
        {
            ShTest* test = library->tests[i];

            free((char*) test->function);
            free((char*) test->suite);
            free((char*) test->name);
            free(test);
        }

        array_free((array*) library->tests);
    }

    free((char*) library->path);
    free(library);
}

static void
sh_construct(struct MuLoader* self, struct MuLibrary* handle, MuError** err)
{
    ShLibrary* library = (ShLibrary*) handle;

    Mu_Sh_Construct(library, err);
}

static void
sh_destruct(struct MuLoader* self, struct MuLibrary* handle, MuError** err)
{
    ShLibrary* library = (ShLibrary*) handle;

    Mu_Sh_Destruct(library, err);
}

static const char*
sh_library_name (struct MuLoader* self, struct MuLibrary* handle)
{
    ShLibrary* library = (ShLibrary*) handle;

    return library->name;
}

// Get the name of a test
static const char*
sh_test_name (struct MuLoader* self, struct MuTest* handle)
{
    ShTest* test = (ShTest*) handle;

    return test->name;
}

// Get the suite of a test
static const char*
sh_test_suite (struct MuLoader* self, struct MuTest* handle)
{
    ShTest* test = (ShTest*) handle;

    return test->suite;
}

static struct MuTestResult*
sh_dispatch (struct MuLoader* self, struct MuTest* handle, MuLogCallback lcb, void* data)
{
    ShTest* test = (ShTest*) handle;
    
    return Mu_Sh_Dispatch (test, lcb, data);
}

static void
sh_free_result (struct MuLoader* self, struct MuTestResult* result)
{
    MuBacktrace* bt, *nextbt;

    if (result->file)
        free((char*) result->file);
    if (result->reason)
        free((char*) result->reason);

    for (bt = result->backtrace; bt; bt = nextbt)
    {
        nextbt = bt->up;
        if (bt->file_name)
            free((char*) bt->file_name);
        if (bt->func_name)
            free((char*) bt->func_name);
        free(bt);
    }

    free(result);
}

static void
helper_set(MuLoader* self, const char* path)
{
    mu_sh_helper_path = strdup(path);
}

static const char*
helper_get(MuLoader* self)
{
    return mu_sh_helper_path;
}

static void
timeout_set(MuLoader* self, int timeout)
{
    mu_sh_timeout = timeout;
}

static int
timeout_get(MuLoader* self)
{
    return mu_sh_timeout;
}

static MuOption cloader_options[] =
{
    MU_OPTION("helper", MU_TYPE_STRING, helper_get, helper_set,
              "Path to the helper function script"),
    MU_OPTION("timeout", MU_TYPE_INTEGER, timeout_get, timeout_set,
              "Default time in milliseconds before tests time out"),
    MU_OPTION_END
};

static MuLoader shloader =
{
    .plugin = NULL,
    .can_open = sh_can_open,
	.open = sh_open,
	.get_tests = sh_get_tests,
	.free_tests = sh_free_tests,
	.close = sh_close,
	.library_name = sh_library_name,
	.test_name = sh_test_name,
	.test_suite = sh_test_suite,
    .dispatch = sh_dispatch,
    .free_result = sh_free_result,
    .debug = NULL,
    .construct = sh_construct,
    .destruct = sh_destruct,
    .options = cloader_options
};

static struct MuLoader*
get_shloader()
{
    return &shloader;
}

static MuPlugin plugin =
{
    .version = MU_PLUGIN_API_1,
    .type = MU_PLUGIN_LOADER,
    .name = "sh",
    .author = "Brian Koropoff",
    .description = "Loads and runs bash unit tests",
    .loader = get_shloader
};

MU_PLUGIN_INIT
{
    return &plugin;
}

