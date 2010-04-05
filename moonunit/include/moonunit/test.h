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

#ifndef __MU_TEST_H__
#define __MU_TEST_H__

#include <moonunit/internal/boilerplate.h>
#include <moonunit/type.h>

/**
 * @file test.h
 * @brief Test-related types, enums, and structures
 */

/**
 * @defgroup test_bits Types, enumerations, and structures
 * @ingroup test
 */
/* @{ */

C_BEGIN_DECLS

/* Forward declarations */
struct MuTestToken;
struct MuTest;
struct MuLoader;
struct MuLibrary;

/**
 * Represents the result of a test
 */
typedef enum MuTestStatus
{
    /** Success */
    MU_STATUS_SUCCESS = 0,
    /** Generic failure */
    MU_STATUS_FAILURE,
    /** Failure due to assertion */
    MU_STATUS_ASSERTION,
    /** Failure due to crash */
    MU_STATUS_CRASH,
    /** Failure due to test exceeding time allowance */
    MU_STATUS_TIMEOUT,
    /** Failure due to uncaught exception */
    MU_STATUS_EXCEPTION,
    /** Failure due to missing resource */
    MU_STATUS_RESOURCE,
    /** Test skipped */
    MU_STATUS_SKIPPED,
} MuTestStatus;

/**
 * Represents the stage at which a failure occured
 */
typedef enum MuTestStage
{
    /** Library setup */
    MU_STAGE_LIBRARY_SETUP,
    /** Fixture setup */
    MU_STAGE_FIXTURE_SETUP,
    /** Test */
    MU_STAGE_TEST,
    /** Fixture teardown */
    MU_STAGE_FIXTURE_TEARDOWN,
    /** Library teardown */
    MU_STAGE_LIBRARY_TEARDOWN,
    /** Stage unknown */
    MU_STAGE_UNKNOWN
} MuTestStage;

#ifndef DOXYGEN
typedef struct MuBacktrace
{
    unsigned long return_addr;
    unsigned long func_addr;
    const char* file_name;
    const char* func_name;
    struct MuBacktrace* up;
    /* Reserved */
    void* reserved1;
    void* reserved2;
} MuBacktrace;

typedef struct MuTestResult
{
    /** Status of the test (pass/fail) */
    MuTestStatus status;
    /** Expected status of the test */
    MuTestStatus expected;
    /** If test failed, the stage at which the failure occured */
    MuTestStage stage;
    /** Human-readable reason why the test failed */
    const char* reason;
    /** File in which the failure occured */
    const char* file;
    /** Line on which the failure occured */
    unsigned int line;
    /** Backtrace, if available */
    MuBacktrace* backtrace;
    /* Reserved */
    void* reserved1;
    void* reserved2;
} MuTestResult;
#endif

/**
 * Indicates the level of a log event
 */
typedef enum
{
    /** Warning */
    MU_LEVEL_WARNING,
    /** Informational message */
    MU_LEVEL_INFO,
    /** Verbose message */
    MU_LEVEL_VERBOSE,
    /** Debug message */
    MU_LEVEL_DEBUG,
    /** Trace message */
    MU_LEVEL_TRACE
} MuLogLevel;

#ifndef DOXYGEN

typedef struct MuLogEvent
{
    /** Stage at which event occurred */
    MuTestStage stage;
    /** File in which event occured */
    const char* file;
    /** Line on which event occured */
    unsigned int line;
    /** Severity of event */
    MuLogLevel level;
    /** Logged message */
    const char* message;
    /* Reserved */
    void* reserved1;
    void* reserved2;
} MuLogEvent;

typedef struct MuTest
{
    struct MuLoader* loader;
    struct MuLibrary* library;
} MuTest;

typedef void (*MuThunk) (void);

const char* Mu_TestStatusToString(MuTestStatus status);
const char* Mu_TestStageToString(MuTestStage stage);
const char* Mu_Test_Name(MuTest* test);
const char* Mu_Test_Suite(MuTest* test);

#endif

C_END_DECLS

/* @} */

#endif
