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

#include <moonunit/test.h>
#include <moonunit/private/util.h>
#include <moonunit/loader.h>

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

const char*
Mu_TestStatusToString(MuTestStatus result)
{
	switch (result)
	{
    case MU_STATUS_SUCCESS:
        return "success";
    case MU_STATUS_FAILURE:
        return "failure";
    case MU_STATUS_ASSERTION:
        return "assertion";
    case MU_STATUS_CRASH:
        return "crash";
    case MU_STATUS_EXCEPTION:
        return "exception";
    case MU_STATUS_TIMEOUT:
        return "timeout";
    case MU_STATUS_SKIPPED:
        return "skipped";
    case MU_STATUS_RESOURCE:
        return "resource";
    default:
        return "unknown";
	}
}

const char*
Mu_TestStageToString(MuTestStage stage)
{
	switch (stage)
	{
		case MU_STAGE_FIXTURE_SETUP:
			return "fixture setup";
		case MU_STAGE_FIXTURE_TEARDOWN:
			return "fixture teardown";
		case MU_STAGE_LIBRARY_SETUP:
			return "library setup";
		case MU_STAGE_LIBRARY_TEARDOWN:
			return "library teardown";
		case MU_STAGE_TEST:
			return "test";
		case MU_STAGE_UNKNOWN:	
		default:
			return "unknown stage";
	}
}

const char*
Mu_Test_Name(MuTest* test)
{
    return test->loader->test_name(test->loader, test);
}

const char*
Mu_Test_Suite(MuTest* test)
{
    return test->loader->test_suite(test->loader, test);
}
