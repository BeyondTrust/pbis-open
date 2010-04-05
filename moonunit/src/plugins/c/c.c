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

#include <moonunit/plugin.h>
#include <moonunit/loader.h>

#include <stdlib.h>

#include "c-load.h"
#include "c-run.h"

MuLoader mu_cloader =
{
    .plugin = NULL,
    .can_open = cloader_can_open,
	.open = cloader_open,
	.get_tests = cloader_get_tests,
	.free_tests = cloader_free_tests,
	.close = cloader_close,
	.library_name = cloader_library_name,
	.test_name = cloader_test_name,
	.test_suite = cloader_test_suite,
    .dispatch = cloader_dispatch,
    .free_result = cloader_free_result,
    .debug = cloader_debug,
    .construct = cloader_construct,
    .destruct = cloader_destruct,
    .options = cloader_options
};

static struct MuLoader*
get_cloader()
{
    return &mu_cloader;
}

static MuPlugin plugin =
{
    .version = MU_PLUGIN_API_1,
    .type = MU_PLUGIN_LOADER,
    .name = "c",
    .author = "Brian Koropoff",
    .description = "Loads and runs C/C++ unit tests",
    .loader = get_cloader
};

MU_PLUGIN_INIT
{
    return &plugin;
}
