/*
 * Copyright (c) 2008, Brian Koropoff
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

#ifndef __MU_C_LOAD_H__
#define __MU_C_LOAD_H__

#include <moonunit/interface.h>
#include <moonunit/library.h>

typedef struct CTest
{
    MuTest base;
    MuEntryInfo* entry;
} CTest;

typedef struct CLibrary
{
    MuLibrary base;
	const char* path;
    const char* name;
	void* dlhandle;
	CTest** tests;
    MuEntryInfo* library_construct;
    MuEntryInfo* library_destruct;
	MuEntryInfo* library_setup;
    MuEntryInfo* library_teardown;
    MuEntryInfo** fixture_setups;
    MuEntryInfo** fixture_teardowns;
} CLibrary;

bool cloader_can_open(MuLoader* self, const char* path);
MuLibrary* cloader_open(MuLoader* _self, const char* path, MuError** _err);
MuTest** cloader_get_tests (MuLoader* _self, MuLibrary* handle);
void cloader_free_tests (MuLoader* _self, MuLibrary* handle, MuTest** tests);
void cloader_close (MuLoader* _self, MuLibrary* handle);
const char* cloader_library_name (MuLoader* _self, MuLibrary* handle);
const char* cloader_test_name (struct MuLoader* _loader, struct MuTest* _test);
const char* cloader_test_suite (struct MuLoader* _loader, struct MuTest* _test);
MuThunk cloader_library_setup (MuLoader* _self, MuLibrary* handle);
MuThunk cloader_library_teardown (MuLoader* _self, MuLibrary* handle);
MuThunk cloader_fixture_setup (MuLoader* _self, MuTest* test);
MuThunk cloader_fixture_teardown (MuLoader* _self, MuTest* test);

#endif
