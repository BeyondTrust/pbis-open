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

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#include <moonunit/logger.h>
#include <moonunit/test.h>
#include <moonunit/loader.h>
#include <moonunit/private/util.h>
#include <moonunit/plugin.h>

#include "option.h"
#include "run.h"
#include "multilog.h"

#define ALIGNMENT 60

#define die(fmt, ...)                               \
    do {                                            \
        fprintf(stderr, fmt "\n", ## __VA_ARGS__);  \
        exit(255);                                  \
    } while (0);                                    \

OptionTable option = {0};

static
int
list_plugins()
{
    MuPlugin** plugins = Mu_Plugin_List();
    unsigned int i;

    if (!plugins)
    {
        printf("No plugins found.\n");
        return 0;
    }

    printf("Loader plugins:\n  ");
    
    for (i = 0; plugins[i]; i++)
    {
        MuPlugin* plugin = plugins[i];

        if (plugin->type == MU_PLUGIN_LOADER)
            printf("%s - %s\n  ", plugin->name, plugin->description);
    }

    printf("\n");
    printf("Logger plugins:\n  ");

    for (i = 0; plugins[i]; i++)
    {
        MuPlugin* plugin = plugins[i];

        if (plugin->type == MU_PLUGIN_LOGGER)
            printf("%s - %s\n  ", plugin->name, plugin->description);
    }

    printf("\n");

    return 0;
}

static
void
print_options(void* object, MuOption* options)
{
    unsigned int i;

    for (i = 0; options[i].name; i++)
    {
        printf("  Option: %s\n", options[i].name);
        printf("    Type: %s\n", Mu_Type_ToString(options[i].type));
        printf("    Description: %s\n", options[i].description);
        printf("    Default: %s\n", Mu_Option_GetString(options, object, options[i].name));
        printf("\n");
    }
}

static
int
plugin_info(const char* name)
{
    MuPlugin* plugin = Mu_Plugin_GetByName(name);

    if (!plugin)
        die("No such plugin: %s\n", name);

    switch (plugin->type)
    {
    case MU_PLUGIN_LOADER:
    {
        MuLoader* loader = plugin->loader();
        
        printf("%s - %s\n", plugin->name, plugin->description);
        printf("  Type: loader\n");
        printf("  Author: %s\n\n", plugin->author);
        print_options(loader, loader->options);
        break;
    }
    case MU_PLUGIN_LOGGER:
    {
        MuLogger* logger = plugin->create_logger();
        printf("%s - %s\n", plugin->name, plugin->description);
        printf("  Type: logger\n");
        printf("  Author: %s\n\n", plugin->author);
        print_options(logger, logger->options);
        Mu_Logger_Destroy(logger);
        break;
    }
    }

    return 0;
}

static
int
run(char* self)
{
    MuError* err = NULL;
    unsigned int file_index;
    RunSettings settings;
    array* loggers;
    unsigned int failed = 0;

    if (Option_ProcessResources(&option))
    {
        die("Error: %s", option.errormsg);
    }

    Option_ConfigureLoaders(&option);
    loggers = Option_CreateLoggers(&option);
    
    settings.self = self;
    settings.debug = option.gdb;

    if (array_size(loggers) == 0)
    {
        /* Create default console logger */
        settings.logger = Mu_Plugin_CreateLogger("console");

        if (!settings.logger)
        {
            die("Error: Could not create logger 'console'");
        }
        
        Mu_Logger_SetOption(settings.logger, "ansi", true);
    }
    else if (array_size(loggers) == 1)
    {
        settings.logger = loggers[0];
    }
    else
    {
        settings.logger = create_multilogger(loggers);
    }

    Mu_Logger_Enter(settings.logger);

    for (file_index = 0; file_index < array_size(option.files); file_index++)
    {
        char* file = option.files[file_index];

        settings.loader = Mu_Plugin_GetLoaderForFile(file);

        if (!settings.loader)
        {
            die("Error: Could not find loader for file %s", basename_pure(file));
        }

        if (option.timeout && Mu_Loader_OptionType(settings.loader, "timeout") == MU_TYPE_INTEGER)
        {
            Mu_Loader_SetOption(settings.loader, "timeout", option.timeout);
        }
        
        if (option.iterations && Mu_Loader_OptionType(settings.loader, "iterations") == MU_TYPE_INTEGER)
        {
            Mu_Loader_SetOption(settings.loader, "iterations", option.iterations);
        }
        
        if (option.all || array_size(option.tests) == 0)
        {
            failed += run_all(&settings, file, &err);
        }
        else
        {
            failed += run_tests(&settings, file, array_size(option.tests), (char**) option.tests, &err);
        }

        MU_CATCH_ALL(err)
        {
            die("Error: %s", err->message);
        }
    }

    Mu_Logger_Leave(settings.logger);
    Mu_Logger_Destroy(settings.logger);

    Option_Release(&option);

    if (failed > 255)
        return 255;
    else
        return (int) failed;
}

int
main (int argc, char** argv)
{
    int res = 0;

    if (Option_Parse(argc, argv, &option))
    {
        die("Error: %s", option.errormsg);
    }

    switch (option.mode)
    {
    case MODE_RUN:
        res = run(argv[0]);
        break;
    case MODE_LIST_PLUGINS:
        res = list_plugins();
        break;
    case MODE_PLUGIN_INFO:
        res = plugin_info(option.plugin_info);
        break;
    case MODE_USAGE:
    case MODE_HELP:
        break;
    default:
        res = -1;
        break;
    }

    Mu_Plugin_Shutdown();

    return res;
}
