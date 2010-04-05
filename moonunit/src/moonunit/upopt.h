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

#ifndef __UPOPT_H__
#define __UPOPT_H__

#include <stdbool.h>
#include <stdio.h>

/* Minimalist popt-like command line option parser */

typedef struct UpoptOptionInfo
{
    /* Short option name */
    char shortname;
    /* Long option name */
    const char* longname;
    /* Description of option */
    const char *description;
    /* Description of argument (or NULL if none taken) */
    const char *argument;
    /* Enum constant */
    int constant;
} UpoptOptionInfo;

typedef struct UpoptContext UpoptContext;

#define UPOPT_ARG_NORMAL (-1)
#define UPOPT_ARG_END (-2)
#define UPOPT_END                       \
    {                                   \
        .constant = UPOPT_ARG_END       \
    }                                   \

typedef enum UpoptStatus
{
    UPOPT_STATUS_NORMAL,
    UPOPT_STATUS_DONE,
    UPOPT_STATUS_ERROR
} UpoptStatus;

UpoptContext* Upopt_CreateContext(const UpoptOptionInfo* options, int argc, char** argv);
UpoptStatus Upopt_Next(UpoptContext* context, int* constant, const char** value, char** error);
void Upopt_SetInfo(UpoptContext* context, const char* program_name, const char* normal_arguments,
                   const char* description);
void Upopt_PrintUsage(UpoptContext* context, FILE* out, int columns);
void Upopt_PrintHelp(UpoptContext* context, FILE* out, int columns);
void Upopt_DestroyContext(UpoptContext* context);

#endif
