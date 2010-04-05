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

#include <string.h>
#include <ctype.h>

#include "upopt.h"
#include <moonunit/private/util.h>

struct UpoptContext
{
    bool only_normal;
    int argc, index;
    char **argv; 
    const UpoptOptionInfo* options;
    const char* program_name;
    const char* normal_arguments;
    const char* description;
};

UpoptContext*
Upopt_CreateContext(const UpoptOptionInfo* options, int argc, char** argv)
{
    UpoptContext* context = malloc(sizeof(*context));
    
    context->argc = argc;
    context->argv = argv;
    context->options = options;
    context->index = 1;
    context->only_normal = false;

    return context;
}

static const UpoptOptionInfo*
find_long(const UpoptOptionInfo* info, const char* name)
{
    int i;

    for (i = 0; info[i].constant != UPOPT_ARG_END; i++)
    {
        if (info[i].longname && !strcmp(name, info[i].longname))
            return &info[i];
    }

    return NULL;
}

static const UpoptOptionInfo*
find_short(const UpoptOptionInfo* info, const char letter)
{
    int i;

    for (i = 0; info[i].constant != UPOPT_ARG_END; i++)
    {
        if (info[i].shortname == letter)
            return &info[i];
    }

    return NULL;
}

static inline bool
is_long(const char* arg)
{
    return arg[0] && arg[1] && arg[0] == '-' && arg[1] == '-';
}

static inline bool
is_short(const char* arg)
{
    return arg[0] && arg[1] && arg[0] == '-';
}

static inline bool
is_option(const char* arg)
{
    return is_long(arg) || is_short(arg);
}

UpoptStatus
Upopt_Next(UpoptContext* context, int* constant, const char** value, char** error)
{
    const char* arg;
    const char* val = NULL;
    const UpoptOptionInfo* info = NULL;
   
    if (context->index >= context->argc)
        return UPOPT_STATUS_DONE;

    arg = context->argv[context->index++];

    if (!context->only_normal)
    {
        if (is_long(arg))
        {
            if (!arg[2])
            {
                context->only_normal = 1;
                return Upopt_Next(context, constant, value, error);
            }
            else
            {
                char* equal = strchr(arg, '=');
                
                if (equal)
                {
                    *equal = '\0';
                    val = equal + 1;
                }
                
                info = find_long(context->options, arg+2);
                
                if (equal)
                {
                    *equal = '=';
                }
                
                if (!info)
                {
                    *error = format("Unrecognized option: %s\n", arg);
                    return UPOPT_STATUS_ERROR;
                }
            }
        }
        else if (is_short(arg))
        {
            if (arg[2])
            {
                val = arg+2;
            }

            info = find_short(context->options, arg[1]);
            
            if (!info)
            {
                *error = format("Unrecognized option: %s\n", arg);
                return UPOPT_STATUS_ERROR;
            }
        }
    }
     
    if (info)
    {
        if (val && !info->argument)
        {
            if (is_long(arg))
            {
                *error = format("Did not expect an argument after --%s\n", 
                                info->longname);
            }
            else
            {
                *error = format("Did not expect an argument after -%c\n", 
                                info->shortname);
            }
            return UPOPT_STATUS_ERROR;
        }
        else if (!val && info->argument)
        {
            if ((context->index >= context->argc ||
                 (!context->only_normal &&
                  is_option(context->argv[context->index]))))
            {
                *error = format("Expected argument after %s\n", arg);
                return UPOPT_STATUS_ERROR;
            }
            val = context->argv[context->index++];
        }

        *constant = info->constant;
        *value = val;
        *error = NULL;
        return UPOPT_STATUS_NORMAL;
    }
    else
    {
        *constant = UPOPT_ARG_NORMAL;
        *value = arg;
        *error = NULL;
        return UPOPT_STATUS_NORMAL;
    }
}

void
Upopt_DestroyContext(UpoptContext* context)
{
    free(context);
}

void
Upopt_SetInfo(UpoptContext* context, const char* program_name, const char* normal_arguments, const char* description)
{
    context->program_name = program_name;
    context->normal_arguments = normal_arguments;
    context->description = description;
}

static void
print_wrapped(FILE* out, int* used, int columns, int indent, const char* format, ...)
{
    char* str;
    va_list ap;

    va_start(ap, format);
    
    str = formatv(format, ap);
    
    va_end(ap);
    
    if ((*used + strlen(str) >= columns) ||
        !strcmp(str, "\n"))
    {
        int i;
        fprintf(out, "\n");
        for (i = 0; i < indent; i++)
            fprintf(out, " ");
        *used = indent;
    }

    if (strcmp(str, "\n"))
    {
        *used += strlen(str);
        fprintf(out, "%s", str);
    }

    free(str);
}

void
Upopt_PrintUsage(UpoptContext* context, FILE* out, int columns)
{
#define PRINT(...) (print_wrapped(out, &used, columns, indent, __VA_ARGS__))
    int i, used = 0;
    int indent = 7 + strlen(context->program_name);

    fprintf(out, "%s - %s\n\n", context->program_name, context->description);
    PRINT("Usage: %s", context->program_name);

    for (i = 0; context->options[i].constant != UPOPT_ARG_END; i++)
    {
        const UpoptOptionInfo* info = &context->options[i];
        if (info->shortname && info->longname && info->argument)
        {
            PRINT(" [ -%c, --%s %s ]", info->shortname, info->longname,
                  info->argument);
        }
        else if (info->shortname && info->longname)
        {
            PRINT(" [ -%c, --%s ]", info->shortname, info->longname,
                  info->argument);
        }
        else if (info->longname && info->argument)
        {
            PRINT(" [ --%s %s ]", info->longname, info->argument);
        }
        else if (info->shortname && info->argument)
        {
            PRINT(" [ -%c %s ]", info->shortname, info->argument);
        }
        else if (info->longname)
        {
            PRINT(" [ --%s ]", info->longname);
        }
        else if (info->shortname)
        {
            PRINT(" [ -%c ]", info->shortname);
        }
    }

    if (context->normal_arguments)
    {
        PRINT(" %s", context->normal_arguments);
    }

    fprintf(out, "\n");
#undef PRINT
}

void
Upopt_PrintHelp(UpoptContext* context, FILE* out, int columns)
{
#define PRINT(...) (print_wrapped(out, &used, columns, indent + indent_newline, __VA_ARGS__))
#define PRINT_NEW(...) (print_wrapped(out, &used, columns, indent_newline, __VA_ARGS__))
    int used = 0;
    int indent = 40;
    int indent_newline = 4;
    int i;
    Upopt_PrintUsage(context, out, columns);

    fprintf(out, "\nOptions:");
    PRINT_NEW("\n");

    for (i = 0; context->options[i].constant != UPOPT_ARG_END; i++)
    {
        const UpoptOptionInfo* info = &context->options[i];

        if (info->shortname && info->longname && info->argument)
        {
            PRINT("-%c, --%s %s", info->shortname, info->longname,
                  info->argument);
        }
        else if (info->shortname && info->longname)
        {
            PRINT("-%c, --%s", info->shortname, info->longname,
                  info->argument);
        }
        else if (info->longname && info->argument)
        {
            PRINT("--%s %s", info->longname, info->argument);
        }
        else if (info->shortname && info->argument)
        {
            PRINT("-%c %s", info->shortname, info->argument);
        }
        else if (info->longname)
        {
            PRINT("--%s", info->longname);
        }
        else if (info->shortname)
        {
            PRINT("-%c", info->shortname);
        }


        for (; used < indent + indent_newline; used++)
            fprintf(out, " ");

        {
            char* desc = strdup(info->description);
            char* cur = desc, *ws;

            do
            {
                ws = strchr(cur, ' ');
                if (ws)
                    *(ws++) = 0;
                
                PRINT("%s ", cur);
                
                if (ws)
                {
                    while (isspace((int) *ws)) ws++;
                    cur = *ws ? ws : NULL;
                }
                else
                {
                    cur = NULL;
                }
            } while (cur);

            free(desc);
        }

        if (context->options[i+1].constant != UPOPT_ARG_END)
        {
            fprintf(out, "\n");
            PRINT_NEW("\n");
        }
    }

    fprintf(out, "\n");
}
