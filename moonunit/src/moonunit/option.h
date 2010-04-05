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

#ifndef __OPTION_H__
#define __OPTION_H__

#include <stdbool.h>

#include <moonunit/private/util.h>

typedef struct
{
    enum
    {
        MODE_RUN,
        MODE_LIST_PLUGINS,
        MODE_PLUGIN_INFO,
        MODE_USAGE,
        MODE_HELP
    } mode;
    bool gdb;
    bool all;
    unsigned int iterations;
    long timeout;
    char* logger;
    array* tests, *files, *loggers, *resources;
    array* loader_options;
    const char* plugin_info;
    char* errormsg;
} OptionTable;

struct MuLogger;

int Option_Parse(int argc, char** argv, OptionTable* option);
void Option_Release(OptionTable* option);
array* Option_CreateLoggers(OptionTable* option);
void Option_ConfigureLoaders(OptionTable* option);
int Option_ProcessResources(OptionTable* option);

#endif
