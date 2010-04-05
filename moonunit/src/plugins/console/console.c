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

#include <moonunit/logger.h>
#include <moonunit/plugin.h>
#include <moonunit/test.h>
#include <moonunit/library.h>
#include <moonunit/private/util.h>

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct
{
    MuLogger base;

    int fd;
    char* file;
    FILE* out;

    int align;
    bool ansi;
    bool details;
    MuLogLevel loglevel;
    char* log;
    unsigned int num_tests;
    unsigned int num_suites;
    unsigned int num_libraries;
    unsigned int num_pass;
    unsigned int num_fail;
    unsigned int num_xpass;
    unsigned int num_xfail;
    unsigned int num_skip;
    unsigned int num_lib_abort;
} ConsoleLogger;

static void
enter(MuLogger* _self)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;
    
    self->num_tests = 0;
    self->num_suites = 0;
    self->num_libraries = 0;
    self->num_pass = 0;
    self->num_fail = 0;
    self->num_xpass = 0;
    self->num_xfail = 0;
    self->num_skip = 0;
    self->num_lib_abort = 0;
}

static void
leave(MuLogger* _self)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

    if (self->ansi)
    {
        fprintf(self->out, "Summary:\n");
        fprintf(self->out, "  Libraries:         \e[1m%6u\e[0m\n", self->num_libraries);
        fprintf(self->out, "  Suites:            \e[1m%6u\e[0m\n", self->num_suites);
        fprintf(self->out, "  Tests:             \e[1m%6u\e[0m\n", self->num_tests);
        if (self->num_pass + self->num_xfail)
            fprintf(self->out, "  \e[32m\e[1mPassed\e[22m\e[0m tests:      \e[1m%6u\e[0m\n",
                    self->num_pass + self->num_xfail);
        if (self->num_fail + self->num_xpass)
            fprintf(self->out, "  \e[31m\e[1mFailed\e[22m\e[0m tests:      \e[1m%6u\e[0m\n",
                    self->num_fail + self->num_xpass);
        if (self->num_skip)
            fprintf(self->out, "  \e[33m\e[1mSkipped\e[22m\e[0m tests:     \e[1m%6u\e[0m\n",
                    self->num_skip);
        if (self->num_lib_abort)
            fprintf(self->out, "  \e[31m\e[1mAborted\e[22m\e[0m libraries: \e[1m%6u\e[0m\n\n",
                    self->num_lib_abort);
    }
    else
    {
        fprintf(self->out, "Summary:\n");
        fprintf(self->out, "  Libraries:         %6u\e\n", self->num_libraries);
        fprintf(self->out, "  Suites:            %6u\e\n", self->num_suites);
        fprintf(self->out, "  Tests:             %6u\e\n", self->num_tests);
        if (self->num_pass + self->num_xfail)
            fprintf(self->out, "  Passed tests:      %6u\n",
                    self->num_pass + self->num_xfail);
        if (self->num_fail + self->num_xpass)
            fprintf(self->out, "  Failed tests:      %6u\n",
                    self->num_fail + self->num_xpass);
        if (self->num_skip)
            fprintf(self->out, "  Skipped tests:     %6u\n",
                    self->num_skip);
        if (self->num_lib_abort)
            fprintf(self->out, "  Aborted libraries: %6u\n\n",
                    self->num_lib_abort);
    }
}

static void
library_enter(MuLogger* _self, const char* path, MuLibrary* library)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

    self->num_libraries++;

    if (library)
    {
        fprintf(self->out, "Library: %s (%s)\n", basename_pure(path), Mu_Library_Name(library));
    }
    else
    {
        fprintf(self->out, "Library: %s\n", basename_pure(path));
    }
}

static void
library_fail(MuLogger* _self, const char* reason)
{    
    ConsoleLogger* self = (ConsoleLogger*) _self;
    int i;

    if (self->ansi)
    {
        fprintf(self->out, "  \e[1mLibrary aborted\e[0m");
        for (i = 0; i < self->align - 17 - 5; i++)
        {
            fprintf(self->out, " ");
        }
        fprintf(self->out, "\e[31m\e[1mFAIL\e[22m\e[0m\n");
        fprintf(self->out, "    %s\n", reason);
    }
    else
    {
        fprintf(self->out, "  Library aborted");
        for (i = 0; i < self->align - 17 - 5; i++)
        {
            fprintf(self->out, " ");
        }
        fprintf(self->out, "FAIL\n");
        fprintf(self->out, "    %s\n", reason);
    }

    self->num_lib_abort++;
}

static void
library_leave(MuLogger* _self)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

	fprintf(self->out, "\n");
}

static void
suite_enter(MuLogger* _self, const char* name)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

    self->num_suites++;

	fprintf(self->out, "  Suite: %s\n", name);
}

static void
suite_leave(MuLogger* _self)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

	fprintf(self->out, "\n");
}

static void
test_enter(MuLogger* _self, MuTest* test)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

    self->num_tests++;
    self->log = strdup("");
}

static void
test_log(MuLogger* _self, MuLogEvent* event)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;
    char* level_str = NULL;
    int level_code = 0;
    char* old;

    if (self->loglevel == -1 || event->level > self->loglevel)
        return;

    switch (event->level)
    {
        case MU_LEVEL_WARNING:
            level_str = "warning"; level_code = 31; break;
        case MU_LEVEL_INFO:
            level_str = "info"; level_code = 33; break;
        case MU_LEVEL_VERBOSE:
            level_str = "verbose"; level_code = 34; break;
        case MU_LEVEL_DEBUG:
            level_str = "debug"; level_code = 36; break;
        case MU_LEVEL_TRACE:
            level_str = "trace"; level_code = 35; break;
    }

    old = self->log;

    if (self->ansi)
    {
        if (event->file && event->line)
        {
            self->log = format("%s      (\e[%im\e[1m%s\e[22m\e[0m) %s:%u: %s\n", old,
                               level_code, level_str, basename_pure(event->file), 
                               event->line, event->message);
        }
        else if (event->file)
        {
            self->log = format("%s      (\e[%im\e[1m%s\e[22m\e[0m) %s: %s\n", old,
                               level_code, level_str, basename_pure(event->file), 
                               event->message);
        }
        else
        {
            self->log = format("%s      (\e[%im\e[1m%s\e[22m\e[0m) %s\n", old,
                               level_code, level_str, event->message);
        }
    }
    else
    {
        if (event->file && event->line)
        {
            self->log = format("%s      (%s) %s:%u: %s\n", old,
                               level_str, basename_pure(event->file), 
                               event->line, event->message);
        }
        else if (event->file)
        {
            self->log = format("%s      (%s) %s: %s\n", old,
                               level_str, basename_pure(event->file), 
                               event->message);
        }
        else
        {
            self->log = format("%s      (%s) %s\n", old,
                               level_str, event->message);
        }
    }

    free(old);
}

static void
test_leave(MuLogger* _self, MuTest* test, MuTestResult* summary)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;
    FILE* out = self->out;
	int i;
	const char* reason, *stage, *name, *status;
    bool result = summary->status == MU_STATUS_SKIPPED || summary->status == summary->expected;
    const char* result_str = NULL;
    unsigned int result_code;

    name = Mu_Test_Name(test);

	fprintf(out, "    %s:", name);
	
    
    if (result)
    {
        result_code = 32;

        switch (summary->status)
        {
        case MU_STATUS_SUCCESS:
            result_str = "PASS ";
            self->num_pass++;
            break;
		case MU_STATUS_FAILURE:
		case MU_STATUS_ASSERTION:
		case MU_STATUS_CRASH:
        case MU_STATUS_TIMEOUT:
        case MU_STATUS_EXCEPTION:
        case MU_STATUS_RESOURCE:
            result_str = "XFAIL";
            self->num_xfail++;
            break;
        case MU_STATUS_SKIPPED:
            result_str = "SKIP ";
            result_code = 33;
            self->num_skip++;
            break;
        }
    }
    else
    {
        result_code = 31;
    
        switch (summary->status)
        {
        case MU_STATUS_SUCCESS:
            result_str = "XPASS";
            self->num_xpass++;
            break;
		case MU_STATUS_FAILURE:
		case MU_STATUS_ASSERTION:
		case MU_STATUS_CRASH:
        case MU_STATUS_TIMEOUT:
        case MU_STATUS_EXCEPTION:
        case MU_STATUS_RESOURCE:
            result_str = "FAIL ";
            self->num_fail++;
            break;
        case MU_STATUS_SKIPPED:
            result_str = "SKIP ";
            result_code = 33;
            self->num_skip++;
            break;
        }
    }

    if (summary->status == MU_STATUS_SUCCESS || (result && !self->details))
    {
        for (i = self->align - strlen(name) - 5 - strlen(result_str); i > 0; i--)
            fprintf(out, " ");
        if (self->ansi)
            fprintf(out, "\e[%um\e[1m%s\e[22m\e[0m\n", result_code, result_str);
        else
            fprintf(out, "%s\n", result_str);

        fprintf(out, "%s", self->log);
        free(self->log);
        self->log = NULL;
    }
    else
    {
        stage = Mu_TestStageToString(summary->stage);
        status = Mu_TestStatusToString(summary->status);
		
        for (i = self->align - strlen(name) - strlen(stage) - 1 - 5 - strlen(result_str); i > 0; i--)
            fprintf(out, " ");
        
        reason = summary->reason ? summary->reason : "unknown";
        if (self->ansi)
        {
            fprintf(out, "%s \e[%um\e[1m%s\e[22m\e[0m\n", stage, result_code, result_str);
        }
        else
        {
            fprintf(out, "%s %s\n", stage, result_str);
        }
        
        fprintf(out, "%s", self->log);
        free(self->log);
        self->log = NULL;

        if (self->ansi)
        {
            fprintf(out, "      (\e[%um\e[1m%s\e[22m\e[0m) ", result_code, status);
        }
        else
        {
            fprintf(out, "      (%s) ", status);
        }

        if (summary->file && summary->line)
        {
            fprintf(out, "%s:%i: %s\n", basename_pure(summary->file), summary->line, summary->reason);
        }
        else if (summary->file)
        {
            fprintf(out, "%s: %s\n", basename_pure(summary->file), summary->reason);
        }
        else
        {
            fprintf(out, "%s\n", summary->reason);
        }

        if (summary->backtrace)
        {
            MuBacktrace* frame;
            unsigned int i = 1;

            for (frame = summary->backtrace; frame; frame = frame->up)
            {
                fprintf(out, "        #%2u: ", i++);
                if (frame->func_name)
                {
                    fprintf(out, "%s ", frame->func_name);
                }
                else if (frame->file_name)
                {
                    fprintf(out, "<unknown> ");
                }

                if (frame->file_name)
                {
                    fprintf(out, "in %s", basename_pure(frame->file_name));
                }
                else if (frame->return_addr)
                {
                    fprintf(out, "[0x%lx]", frame->return_addr);
                }
                               
                fprintf(out, "\n");
            }
        }
	}
}

static int
get_fd(ConsoleLogger* self)
{
    return self->fd;
}

static void
set_fd(ConsoleLogger* self, int fd)
{
    self->fd = fd;
    if (self->out)
        fclose(self->out);
    /* Duplicate the fd to avoid ownership issues */
    self->out = fdopen(dup(fd), "w");  
}

static const char*
get_file(ConsoleLogger* self)
{
    return self->file;
}

static void
set_file(ConsoleLogger* self, const char* file)
{
    if (self->file)
        free(self->file);
    self->file = strdup(file);
    if (self->out)
        fclose(self->out);
    self->out = fopen(self->file, "w");
    self->fd = fileno(self->out);
}

static bool
get_ansi(ConsoleLogger* self)
{
    return self->ansi;
}

static void
set_ansi(ConsoleLogger* self, bool ansi)
{
    self->ansi = ansi;
}

static int
get_align(ConsoleLogger* self)
{
    return self->align;
}

static void
set_align(ConsoleLogger* self, int align)
{
    self->align = align;
}

static bool
get_details(ConsoleLogger* self)
{
    return self->details;
}

static void
set_details(ConsoleLogger* self, bool details)
{
    self->details = details;
}

static const char*
get_loglevel(ConsoleLogger* self)
{
    switch (self->loglevel)
    {
    case -1:
        return "none";
    case MU_LEVEL_WARNING:
        return "warning";
    case MU_LEVEL_INFO:
        return "info";
    case MU_LEVEL_VERBOSE:
        return "verbose";
    case MU_LEVEL_DEBUG:
        return "debug";
    case MU_LEVEL_TRACE:
        return "trace";
    default:
        return "unknown";
    }
}

static void
set_loglevel(ConsoleLogger* self, const char* level)
{
    if (!strcmp(level, "warning"))
    {
        self->loglevel = MU_LEVEL_WARNING;
    }
    else if (!strcmp(level, "info"))
    {
        self->loglevel = MU_LEVEL_INFO;
    }
    else if (!strcmp(level, "verbose"))
    {
        self->loglevel = MU_LEVEL_VERBOSE;
    }
    else if (!strcmp(level, "debug"))
    {
        self->loglevel = MU_LEVEL_DEBUG;
    }
    else if (!strcmp(level, "trace"))
    {
        self->loglevel = MU_LEVEL_TRACE;
    }
    else if (!strcmp(level, "none"))
    {
        self->loglevel = -1;
    }
}

static void
destroy(MuLogger* _self)
{
    ConsoleLogger* self = (ConsoleLogger*) _self;

    if (self->out)
        fclose(self->out);
    if (self->file)
        free(self->file);
    if (self->log)
        free(self->log);

    free(self);
}


static MuOption consolelogger_options[] =
{
    MU_OPTION("fd", MU_TYPE_INTEGER, get_fd, set_fd,
              "File descriptor to which results will be written"),
    MU_OPTION("file", MU_TYPE_STRING, get_file, set_file,
              "File to which results will be written"),
    MU_OPTION("ansi", MU_TYPE_BOOLEAN, get_ansi, set_ansi,
              "Whether to use ANSI color/fonts in output"),
    MU_OPTION("align", MU_TYPE_INTEGER, get_align, set_align,
              "Column number use for right-aligned output"),
    MU_OPTION("details", MU_TYPE_BOOLEAN, get_details, set_details,
              "Whether result details should be output for failed "
              "tests even if the failure is expected"),
    MU_OPTION("loglevel", MU_TYPE_STRING, get_loglevel, set_loglevel,
              "Maximum level of logged events which will be printed "
              "(none, warning, info, verbose, trace)"),
    MU_OPTION_END
};

static ConsoleLogger consolelogger =
{
    .base = 
    {
        .enter = enter,
        .leave = leave,
        .library_enter = library_enter,
        .library_fail = library_fail,
        .library_leave = library_leave,
        .suite_enter = suite_enter,
        .suite_leave = suite_leave,
        .test_enter = test_enter,
        .test_log = test_log,
        .test_leave = test_leave,
        .destroy = destroy,
        .options = consolelogger_options
    },
    .fd = -1,
    .out = NULL,
    .ansi = false,
    .align = 60,
    .loglevel = MU_LEVEL_INFO
};

static MuLogger*
create_consolelogger()
{
    ConsoleLogger* logger = malloc(sizeof(ConsoleLogger));

    *logger = consolelogger;

    Mu_Logger_SetOption((MuLogger*) logger, "fd", fileno(stdout));

    return (MuLogger*) logger;
}

static MuPlugin plugin =
{
    .version = MU_PLUGIN_API_1,
    .type = MU_PLUGIN_LOGGER,
    .name = "console",
    .author = "Brian Koropoff",
    .description = "Logs human-readable test results to the console or a file",
    .create_logger = create_consolelogger
};

MU_PLUGIN_INIT
{
    return &plugin;
}
