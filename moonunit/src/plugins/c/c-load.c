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
#include <moonunit/test.h>
#include <moonunit/interface.h>
#include <moonunit/error.h>

#include <string.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include <config.h>

#ifdef HAVE_LIBELF
#include "elfscan.h"
#endif

#include "c-load.h"

extern MuLoader mu_cloader;

static CTest*
ctest_new(CLibrary* library, MuEntryInfo* entry)
{
    CTest* test = calloc(1, sizeof(CTest));

    if (!test)
        return NULL;

    test->base.loader = (MuLoader*) &mu_cloader;
    test->base.library = (MuLibrary*) library;
    test->entry = entry;

    return test;
}

static bool
add(MuEntryInfo* entry, CLibrary* library, MuError **_err)
{
    switch (entry->type)
    {
    case MU_ENTRY_TEST:
    {
        CTest* test = ctest_new(library, entry);

        if (!test)
        {
            MU_RAISE_RETURN(false, _err, MU_ERROR_MEMORY, "Out of memory");
        }

        library->tests = (CTest**) array_append((array*) library->tests, test);
        
        if (!library->tests)
        {
            MU_RAISE_RETURN(false, _err, MU_ERROR_MEMORY, "Out of memory");
        }
        break;
  	}
    case MU_ENTRY_FIXTURE_SETUP:
        library->fixture_setups = (MuEntryInfo**) array_append((array*) library->fixture_setups, entry);
        break;
    case MU_ENTRY_FIXTURE_TEARDOWN:
        library->fixture_teardowns = (MuEntryInfo**) array_append((array*) library->fixture_teardowns, entry);
        break;
    case MU_ENTRY_LIBRARY_SETUP:
		library->library_setup = entry;
        break;
    case MU_ENTRY_LIBRARY_TEARDOWN:
		library->library_teardown = entry;
        break;
    case MU_ENTRY_LIBRARY_CONSTRUCT:
        library->library_construct = entry;
        break;
    case MU_ENTRY_LIBRARY_DESTRUCT:
        library->library_destruct = entry;
        break;
    case MU_ENTRY_LIBRARY_INFO:
        if (!strcmp(entry->name, "name"))
        {
            library->name = safe_strdup(entry->container);
        }
        break;
    }
    

    return true;
}

#ifdef HAVE_LIBELF

static bool
entry_filter(const char* sym, void *unused)
{
	return !strncmp("__mu_e_", sym, strlen("__mu_e_"));
}

static bool
entry_add(symbol* sym, void* _library, MuError **_err)
{
    CLibrary* library = (CLibrary*) _library;	
    MuEntryInfo* entry = (MuEntryInfo*) sym->addr;

    return add(entry, library, _err);
}

static bool
cloader_scan (MuLoader* _self, CLibrary* handle, MuError ** _err)
{
    MuError* err = NULL;
    
	if (!ElfScan_GetScanner()(handle->dlhandle, entry_filter, entry_add, handle, &err))
    {
        MU_RERAISE_GOTO(error, _err, err);
    }
	
    return true;

error:
    
    return false;
}

#endif

bool
cloader_can_open(MuLoader* self, const char* path)
{
    bool result;
    void* handle = mu_dlopen(path, RTLD_LAZY);

    result = (handle != NULL) || ends_with(path, DSO_EXT);

    if (handle)
        dlclose(handle);

    return result;
}

MuLibrary*
cloader_open(MuLoader* _self, const char* path, MuError** _err)
{
	CLibrary* library = malloc(sizeof (CLibrary));
    MuError* err = NULL;
    void (*stub_hook)(MuEntryInfo*** es);
    char *last_dot;

    if (!library)
    {
        MU_RAISE_GOTO(error, _err, MU_ERROR_MEMORY, "Out of memory");
    }

    library->base.loader = _self;
	library->tests = NULL;
	library->fixture_setups = NULL;
    library->fixture_teardowns = NULL;
	library->library_setup = NULL;
    library->library_teardown = NULL;
    library->library_construct = NULL;
    library->library_destruct = NULL;
	library->path = strdup(path);
    library->name = NULL;
	library->dlhandle = mu_dlopen(library->path, RTLD_NOW);

    if (!library->dlhandle)
    {
        MU_RAISE_GOTO(error, _err, MU_ERROR_LOAD_LIBRARY, "%s", dlerror());
    }

    if ((stub_hook = dlsym(library->dlhandle, "__mu_stub_hook")))
    {
        int i;
        MuEntryInfo** entries;

        stub_hook(&entries);

        for (i = 0; entries[i]; i++)
        {
            if (!add(entries[i], library, &err))
            {
                MU_RERAISE_GOTO(error, _err, err);
            }
        }
    }
#ifdef HAVE_LIBELF
    else if (!cloader_scan(_self, library, &err))
    {
        MU_RERAISE_GOTO(error, _err, err);
    }
#else
    else
    {
        MU_RAISE_GOTO(error, _err, MU_ERROR_LOAD_LIBRARY, 
                      "Library did not contain a loading stub "
                      "and reflection is unavailable");
    }
#endif

    /* If an explicit library name was not available, create one */
    if (!library->name)
    {
        library->name = strdup(basename_pure(path));
        if (!library->name)
        {
            MU_RAISE_GOTO(error, _err, MU_ERROR_MEMORY, "Out of memory");
        }
        last_dot = strrchr(library->name, '.');
        if (last_dot)
        {
            *last_dot = '\0';
        }
    }

    return (MuLibrary*) library;

error:
    
    if (library)
    {
        cloader_close(_self, (MuLibrary*) library);
    }

    return NULL;
}

MuTest**
cloader_get_tests (MuLoader* _self, MuLibrary* _handle)
{
    CLibrary* handle = (CLibrary*) _handle;

	return (MuTest**) array_dup((array*) handle->tests);
}
    
void
cloader_free_tests (MuLoader* _self, MuLibrary* handle, MuTest** tests)
{
    array_free((array*) tests);
}

// Returns the library setup routine for handle
MuThunk
cloader_library_setup (MuLoader* _self, MuLibrary* _handle)
{
    CLibrary* handle = (CLibrary*) _handle;

    if (handle->library_setup)
    	return handle->library_setup->run;
    else
        return NULL;
}

MuThunk
cloader_library_teardown (MuLoader* _self, MuLibrary* _handle)
{
    CLibrary* handle = (CLibrary*) _handle;

    if (handle->library_teardown)
    	return handle->library_teardown->run;
    else
        return NULL;
}

MuThunk
cloader_fixture_setup (MuLoader* _self, MuTest* _test)
{
	unsigned int i;
    CTest* test = (CTest*) _test;
    CLibrary* handle = (CLibrary*) _test->library;
    const char* name = test->entry->container;


    if (handle->fixture_setups)
    {
        for (i = 0; handle->fixture_setups[i]; i++)
        {
            if (!strcmp(name, handle->fixture_setups[i]->container))
            {
                return handle->fixture_setups[i]->run;
            }
        }
    }
    
	return NULL;
}

MuThunk
cloader_fixture_teardown (MuLoader* _self, MuTest* _test)
{
	unsigned int i;
    CTest* test = (CTest*) _test;
    CLibrary* handle = (CLibrary*) _test->library;
    const char* name = test->entry->container;	

    if (handle->fixture_teardowns)
    {
        for (i = 0; handle->fixture_teardowns[i]; i++)
        {
            if (!strcmp(name, handle->fixture_teardowns[i]->container))
            {
                return handle->fixture_teardowns[i]->run;
            }
        }
    }
	
	return NULL;
}
   
void
cloader_close (MuLoader* _self, MuLibrary* _handle)
{
    CLibrary* handle = (CLibrary*) _handle;
    int i;

	if (handle->dlhandle)
        dlclose(handle->dlhandle);
    if (handle->path)
        free((void*) handle->path);
    if (handle->name)
        free((void*) handle->name);

    for (i = 0; i < array_size((array*) handle->tests); i++)
    {
        free(handle->tests[i]);
    }

    array_free((array*) handle->tests);
    array_free((array*) handle->fixture_setups);
    array_free((array*) handle->fixture_teardowns);

   	free(handle);
}

const char*
cloader_library_name (MuLoader* _self, MuLibrary* _handle)
{
    CLibrary* handle = (CLibrary*) _handle;

	return handle->name;
}

const char*
cloader_test_name (struct MuLoader* _loader, struct MuTest* _test)
{
    CTest* test = (CTest*) _test;

    return test->entry->name;
}

const char*
cloader_test_suite (struct MuLoader* _loader, struct MuTest* _test)
{
    CTest* test = (CTest*) _test;

    return test->entry->container;
}
