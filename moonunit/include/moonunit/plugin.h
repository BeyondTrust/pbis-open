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

#ifndef __MU_PLUGIN_H__
#define __MU_PLUGIN_H__

#include <stdbool.h>
#include <moonunit/internal/boilerplate.h>

struct MuLogger;
struct MuLoader;

typedef struct MuPlugin
{
    /** Plugin API version */
    enum
    {
        MU_PLUGIN_API_1
    } version;
    /** Plugin type */
    enum
    {
        MU_PLUGIN_LOADER,
        MU_PLUGIN_LOGGER
    } type;

    /** Plugin name */
    const char* name;
    /** Author of the plugin */
    const char* author;
    /** Brief description of the plugin */
    const char* description;

    struct MuLoader*  (*loader) ();
    struct MuLogger*  (*create_logger) ();
} MuPlugin;

extern MuPlugin* __mu_p_init(void);

#define MU_PLUGIN_INIT \
    MuPlugin* __mu_p_init (void)

C_BEGIN_DECLS

struct MuLoader* Mu_Plugin_GetLoaderWithName(const char *name);
struct MuLoader* Mu_Plugin_GetLoaderForFile(const char *file);
struct MuLogger* Mu_Plugin_CreateLogger(const char* name);
MuPlugin** Mu_Plugin_List(void);
MuPlugin* Mu_Plugin_GetByName(const char* name);
void Mu_Plugin_Shutdown(void);

C_END_DECLS

#endif
