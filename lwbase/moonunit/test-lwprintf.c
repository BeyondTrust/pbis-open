/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */
/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Authors: David Leimbach <dleimbach@likewise.com>
 *          (based-on work by) Kyle Stemen <kstemen@likewise.com>
 */

#include <moonunit/moonunit.h>
#include "lwprintf.h"
#include "wc16str.h"
#include <string.h>
#include "lw/rtlstring.h"

// SECTION
// UNICODE and ANSI STRING
//

MU_TEST(LwPrintf, unicode_null_w16)
{
    PUNICODE_STRING pUs = 0;
    wchar_t *wszFormat = L"%wZ";
    wchar16_t wszExpectedOutput [] = {'(','n','u','l','l',')',0};
    wchar16_t wszOutput[10] = {0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16StringW(
        wszOutput,
        sizeof(wszOutput)/sizeof(wszOutput[0]),
        &scchWrote,
        wszFormat,
        pUs);

    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(LwPrintf, ansi_null_w16)
{
    PANSI_STRING pAs = 0;
    wchar_t *wszFormat = L"%Z";
    wchar16_t wszExpectedOutput [] = {'(','n','u','l','l',')',0};
    wchar16_t wszOutput[10] = {0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16StringW(
        wszOutput,
        sizeof(wszOutput)/sizeof(wszOutput[0]),
        &scchWrote,
        wszFormat,
        pAs);

    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(LwPrintf, unicode_null)
{
    PUNICODE_STRING pUs = 0;
    char *format = "%wZ";
    char * expectedOutput = "(null)";
    char output[20];
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfString(
        output,
        sizeof(output)/sizeof(output[0]),
        &scchWrote,
        format,
        pUs);

    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}

MU_TEST(LwPrintf, ansi_null)
{
    PANSI_STRING pAs = 0;
    char *format = "%Z";
    char *expectedOutput = "(null)";
    char output[20];
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfString(
        output,
        sizeof(output)/sizeof(output[0]),
        &scchWrote,
        format,
        pAs);

    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}


MU_TEST(LwPrintf, LwPrintfW16StringW_unicode_string)
{
    wchar_t* wszFormat = L"%wZ";
    wchar16_t wszOutput[10] = {0};
    wchar16_t wszExpectedOutput[] = {'U','S', 0};
    size_t scchWrote = 0;
    UNICODE_STRING us;
    int error = 0;

    MU_ASSERT(0 == LwRtlUnicodeStringAllocateFromCString(&us, "US"));

    error = LwPrintfW16StringW(
        wszOutput,
        sizeof(wszOutput)/sizeof(wszOutput[0]),
        &scchWrote,
        wszFormat,
        &us);

    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(scchWrote == us.Length/2);
}

MU_TEST(LwPrintf, LwPrintfW16StringW_ansi_string)
{
    wchar_t* wszFormat = L"%Z";
    wchar16_t wszOutput[10] = {0};
    wchar16_t wszExpectedOutput[] = {'A','S', 0};
    size_t scchWrote = 0;
    ANSI_STRING as;
    int error = 0;

    MU_ASSERT(0 == LwRtlAnsiStringAllocateFromCString(&as, "AS"));


    error = LwPrintfW16StringW(
        wszOutput,
        sizeof(wszOutput)/sizeof(wszOutput[0]),
        &scchWrote,
        wszFormat,
        &as);

    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(scchWrote == as.Length);
}

MU_TEST(LwPrintf, LwPrintfW16String_ansi_string)
{
    char * szFormat = "%Z";
    wchar16_t wszOutput[10] = {0};
    wchar16_t wszExpectedOutput[] = {'A','S', 0};
    size_t scchWrote = 0;
    ANSI_STRING as;
    int error = 0;

    MU_ASSERT(0 == LwRtlAnsiStringAllocateFromCString(&as, "AS"));


    error = LwPrintfW16String(
        wszOutput,
        sizeof(wszOutput)/sizeof(wszOutput[0]),
        &scchWrote,
        szFormat,
        &as);
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(scchWrote == as.Length);
}

MU_TEST(LwPrintf, LwPrintfW16String_unicode_string)
{
    char * szFormat = "%wZ";
    wchar16_t wszOutput[10] = {0};
    wchar16_t wszExpectedOutput[] = {'U','S',0};
    size_t scchWrote = 0;
    UNICODE_STRING us;
    int error = 0;

    MU_ASSERT(0 == LwRtlUnicodeStringAllocateFromCString(&us, "US"));


    error = LwPrintfW16String(
        wszOutput,
        sizeof(wszOutput)/sizeof(wszOutput[0]),
        &scchWrote,
        szFormat,
        &us);
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(scchWrote == us.Length/2);
}

MU_TEST(LwPrintf, LwPrintfString_unicode_string)
{
    char * szFormat = "%wZ";
    char szOutput[10] = {0};
    char szExpectedOutput[] = {'U','S',0};
    size_t scchWrote = 0;
    UNICODE_STRING us;
    int error = 0;

    MU_ASSERT(0 == LwRtlUnicodeStringAllocateFromCString(&us, "US"));

    error = LwPrintfString(
        szOutput,
        sizeof(szOutput)/sizeof(szOutput[0]),
        &scchWrote,
        szFormat,
        &us);
    MU_ASSERT(!strcmp(szOutput, szExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(scchWrote == us.Length/2);
}

MU_TEST(LwPrintf, LwPrintfString_ansi_string)
{
    char * szFormat = "%Z";
    char szOutput[10] = {0};
    char szExpectedOutput[] = {'A','S',0};
    size_t scchWrote = 0;
    ANSI_STRING as;
    int error = 0;

    MU_ASSERT(0 == LwRtlAnsiStringAllocateFromCString(&as, "AS"));

    error = LwPrintfString(
        szOutput,
        sizeof(szOutput)/sizeof(szOutput[0]),
        &scchWrote,
        szFormat,
        &as);

    MU_ASSERT(!strcmp(szOutput, szExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(scchWrote == as.Length);
}


// SECTION
// wchar16_t * output tests
//

MU_TEST(LwPrintf, LwPrintfW16StringW_decimal)
{
    wchar_t* wszFormat = L"%d";
    wchar16_t wszOutput[10] = {0};
    wchar16_t wszExpectedOutput[] = {'1','2',0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16StringW(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                &scchWrote,
                wszFormat,
                12);

    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16StringW_precise_double)
{
    wchar_t* wszFormat = L"%.3f";
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {'0','.','1','2','3',0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16StringW(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                &scchWrote,
                wszFormat,
                0.123456);
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16StringW_parameter_width_and_precision)
{
    wchar_t* wszFormat = L"%*.*f";
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {' ','0','.','1',0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16StringW(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                &scchWrote,
                wszFormat,
                4,
                1,
                0.123456);
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16StringW_precise_long_double)
{
    wchar_t* wszFormat = L"%.3Lf";
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {'0','.','1','2','3',0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16StringW(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                &scchWrote,
                wszFormat,
                (long double)0.123456);
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16StringW_mbs)
{
    wchar_t* wszFormat = L"%*.*hhs";
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {' ', 'a', 'b', 0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16StringW(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                &scchWrote,
                wszFormat,
                3,
                2,
                "abc");
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16StringW_null)
{
    wchar_t* wszFormat = L"%hhs%ws%ls";
    wchar16_t wszOutput[30];
    wchar16_t wszExpectedOutput[] = {
        '(','n','u','l','l',')','(','n','u','l','l',')',
        '(','n','u','l','l',')',0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16StringW(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                &scchWrote,
                wszFormat,
                NULL,
                NULL,
                NULL);
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16StringW_wc16s)
{
    wchar_t* wszFormat = L"%*.*hs";
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {' ', ' ', 'a', 0};
    wchar16_t wszWrite[] = {'a', 'b', 'c'};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16StringW(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                &scchWrote,
                wszFormat,
                3,
                2,
                wszWrite);
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16StringW_wcs)
{
    wchar_t* wszFormat = L"%*.*ls";
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {' ', 'a', 'b', 0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16StringW(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                &scchWrote,
                wszFormat,
                3,
                2,
                L"abc");
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16StringW_lencheck)
{
    wchar_t* wszFormat = L"%*.*ls";
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {' ', ' ', 0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16StringW(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                &scchWrote,
                wszFormat,
                2,
                0,
                L"abc");
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
    MU_ASSERT(error == 0);

    error = LwPrintfW16StringW(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                &scchWrote,
                wszFormat,
                9,
                0,
                L"abc");
    MU_ASSERT(scchWrote == 9);
    MU_ASSERT(error == 0);

    error = LwPrintfW16StringW(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                &scchWrote,
                wszFormat,
                10,
                0,
                L"abc");
    MU_ASSERT(error);
}

MU_TEST(LwPrintf, LwPrintfW16AllocateStringW)
{
    wchar16_t wszExpectedOutput[] = {'3',' ','-',' ','a','b','c','x',0};
    wchar16_t *pwszOutput = NULL;
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16AllocateStringW(
                &pwszOutput,
                &scchWrote,
                L"%d - %lsx",
                3,
                L"abc");
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(pwszOutput, wszExpectedOutput));
    free(pwszOutput);
}


// SECTION
// UTF-8 format -> char * output tests
//
MU_TEST(LwPrintf, LwPrintfStringW_decimal)
{
    wchar_t* wszFormat = L"%d";
    char output[10] = {0};
    char expectedOutput[] = {'1','2',0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfStringW(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                12);

    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfStringW_precise_double)
{
    wchar_t* wszFormat = L"%.3f";
    char output[10];
    char expectedOutput[] = {'0','.','1','2','3',0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfStringW(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                0.123456);
    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfStringW_parameter_width_and_precision)
{
    wchar_t* wszFormat = L"%*.*f";
    char output[10];
    char expectedOutput[] = {' ','0','.','1',0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfStringW(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                4,
                1,
                0.123456);
    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfStringW_precise_long_double)
{
    wchar_t* wszFormat = L"%.3Lf";
    char output[10];
    char expectedOutput[] = {'0','.','1','2','3',0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfStringW(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                (long double)0.123456);
    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfStringW_mbs)
{
    wchar_t* wszFormat = L"%*.*hhs";
    char output[10];
    char expectedOutput[] = {' ', 'a', 'b', 0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfStringW(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                3,
                2,
                "abc");
    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfStringW_null)
{
    wchar_t* wszFormat = L"%hhs%ws%ls";
    char output[30];
    char *expectedOutput = "(null)(null)(null)";
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfStringW(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                NULL,
                NULL,
                NULL);
    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}

// short string is a wchar16_t based string on most platforms.
// there is no equivalent inttypes.h for specifying 16 bit char strings.
// the validity of this test is questionable
//
// - DL 2/11/2011
MU_TEST(LwPrintf, LwPrintfStringW_hs)
{

    wchar_t* wszFormat = L"%*.*hs";
    char output[10];
    char *expectedOutput = "  a";
    wchar16_t wszWrite[] = {'a', 'b', 'c', '\0'};
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfStringW(
                output,
                10,
                &scchWrote,
                wszFormat,
                3,
                2,
                wszWrite);

    MU_ASSERT(!strcmp(output, expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(scchWrote == strlen(expectedOutput));

}

MU_TEST(LwPrintf, LwPrintfStringW_ls)
{
    wchar_t* wszFormat = L"%*.*ls";
    char output[10];
    char *expectedOutput = " ab";
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfStringW(
                output,
                10,
                &scchWrote,
                wszFormat,
                3,
                2,
                L"abc");
    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfStringW_lencheck)
{
    wchar_t* wszFormat = L"%*.*ls";
    char output[10];
    char expectedOutput[] = {' ', ' ', 0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfStringW(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                2,
                0,
                L"abc");
    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(!strcmp(output, expectedOutput));
    MU_ASSERT(error == 0);

    error = LwPrintfStringW(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                9,
                0,
                L"abc");
    MU_ASSERT(scchWrote == 9);
    MU_ASSERT(error == 0);

    error = LwPrintfStringW(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                10,
                0,
                L"abc");
    MU_ASSERT(error != 0);
}

MU_TEST(LwPrintf, LwPrintfAllocateStringW)
{
    char expectedOutput[] = {'3',' ','-',' ','a','b','c','x',0};
    char *pOutput;
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfAllocateStringW(
                &pOutput,
                &scchWrote,
                L"%d - %lsx",
                3,
                L"abc");

    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(pOutput, expectedOutput));
    free(pOutput);
}

// SECTION
// UTF-8 format -> wchar16_t * output tests
//
MU_TEST(LwPrintf, LwPrintfW16String_decimal)
{
    char *wszFormat = "%d";
    wchar16_t output[10] = {0};
    wchar16_t expectedOutput[] = {'1','2',0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16String(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                12);

    MU_ASSERT(scchWrote == wc16slen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16String_precise_double)
{
    char *wszFormat = "%.3f";
    wchar16_t output[10];
    wchar16_t expectedOutput[] = {'0','.','1','2','3',0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16String(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                0.123456);
    MU_ASSERT(scchWrote == wc16slen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16String_parameter_width_and_precision)
{
    char *wszFormat = "%*.*f";
    wchar16_t output[10];
    wchar16_t expectedOutput[] = {' ','0','.','1',0};
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfW16String(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                4,
                1,
                0.123456);
    MU_ASSERT(scchWrote == wc16slen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16String_precise_long_double)
{
    char *wszFormat = "%.3Lf";
    wchar16_t output[10];
    wchar16_t expectedOutput[] = {'0','.','1','2','3',0};
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfW16String(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                (long double)0.123456);
    MU_ASSERT(scchWrote == wc16slen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16String_mbs)
{
    char *wszFormat = "%*.*hhs";
    wchar16_t output[10];
    wchar16_t expectedOutput[] = {' ', 'a', 'b', 0};
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfW16String(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                3,
                2,
                "abc");
    MU_ASSERT(scchWrote == wc16slen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16String_null)
{
    char *wszFormat = "%hhs%ws%ls";
    wchar16_t output[30];
    wchar16_t expectedOutput[] = {'(','n','u','l','l',')',
                                  '(','n','u','l','l',')',
                                  '(','n','u','l','l',')','\0'};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfW16String(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                NULL,
                NULL,
                NULL);
    MU_ASSERT(scchWrote == wc16slen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16String_basic_cstr_format)
{
    char *szFormat = "%s";
    wchar16_t output[10];
    wchar16_t expectedOutput[] = {'h', 'i', '\0'};
    char * write = "hi";
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfW16String(
                output,
                10,
                &scchWrote,
                szFormat,
                write);

   MU_ASSERT(!wc16scmp(output, expectedOutput));
   MU_ASSERT(error == 0);
   MU_ASSERT(scchWrote == wc16slen(expectedOutput));
}

// short string is a wchar16_t based string on most platforms.
// there is no equivalent inttypes.h for specifying 16 bit char strings.
// the validity of this test is questionable
//
// - DL 2/11/2011
MU_TEST(LwPrintf, LwPrintfW16String_hs)
{

    char *wszFormat = "%*.*hs";
    wchar16_t output[10];
    wchar16_t expectedOutput[] = {' ', ' ', 'a', '\0'};
    wchar16_t wszWrite[] = {'a', 'b', 'c', '\0'};
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfW16String(
                output,
                10,
                &scchWrote,
                wszFormat,
                3,
                2,
                wszWrite);

    MU_ASSERT(!wc16scmp(output, expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(scchWrote == wc16slen(expectedOutput));

}

MU_TEST(LwPrintf, LwPrintfW16String_ls)
{
    char *wszFormat = "%*.*ls";
    wchar16_t output[10];
    wchar16_t expectedOutput [] = {' ','a','b','\0'};
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfW16String(
                output,
                10,
                &scchWrote,
                wszFormat,
                3,
                2,
                L"abc");
    MU_ASSERT(scchWrote == wc16slen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfW16String_lencheck)
{
    char *wszFormat = "%*.*ls";
    wchar16_t output[10];
    wchar16_t expectedOutput[] = {' ', ' ', 0};
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfW16String(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                2,
                0,
                L"abc");
    MU_ASSERT(scchWrote == wc16slen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!wc16scmp(output, expectedOutput));

    error = LwPrintfW16String(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                9,
                0,
                L"abc");
    MU_ASSERT(scchWrote == 9);
    MU_ASSERT(error == 0);

    error = LwPrintfW16String(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                10,
                0,
                L"abc");
    MU_ASSERT(error);
}

MU_TEST(LwPrintf, LwPrintfW16AllocateString)
{
    wchar16_t expectedOutput[] = {'3',' ','-',' ','a','b','c','x',0};
    wchar16_t *pOutput = NULL;
    int error = 0;
    size_t scchWrote = 0;

     error = LwPrintfW16AllocateString(
                &pOutput,
                &scchWrote,
                "%d - %lsx",
                3,
                L"abc");
    MU_ASSERT(!wc16scmp(pOutput, expectedOutput));
    free(pOutput);
}

// SECTION
// UTF-8 format -> UTF-8 * output tests
//

MU_TEST(LwPrintf, LwPrintfMBS_length_vs_chars)
{
    char *szFormat = "%s";
    char output[35] = {0};
    char * expectedOutput = "λf.(λx.f (x x)) (λx.f (x x))";
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfString(
                           output,
                           sizeof(output)/sizeof(output[0]),
                           &scchWrote,
                           szFormat,
                           expectedOutput);
    MU_ASSERT(31 == scchWrote);
    MU_ASSERT(error == 0);
}

MU_TEST(LwPrintf, LwPrintfString_decimal)
{
    char *szFormat = "%d";
    char output[10] = {0};
    char expectedOutput[] = {'1','2',0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfString(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                szFormat,
                12);

    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfString_precise_double)
{
    char *wszFormat = "%.3f";
    char output[10];
    char expectedOutput[] = {'0','.','1','2','3',0};
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfString(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                0.123456);
    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfString_parameter_width_and_precision)
{
    char *wszFormat = "%*.*f";
    char output[10];
    char expectedOutput[] = {' ','0','.','1',0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfString(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                4,
                1,
                0.123456);
    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfString_precise_long_double)
{
    char *wszFormat = "%.3Lf";
    char output[10];
    char expectedOutput[] = {'0','.','1','2','3',0};
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfString(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                (long double)0.123456);
    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfString_mbs)
{
    char *wszFormat = "%*.*hhs";
    char output[10];
    char expectedOutput[] = {' ', 'a', 'b', 0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfString(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                3,
                2,
                "abc");
    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfString_null)
{
    char *wszFormat = "%hhs%ws%ls";
    char output[30];
    char expectedOutput[] = {'(','n','u','l','l',')',
                             '(','n','u','l','l',')',
                             '(','n','u','l','l',')','\0'};
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfString(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                NULL,
                NULL,
                NULL);
    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(!strcmp(output, expectedOutput));
}


// - DL 2/14/2011
MU_TEST(LwPrintf, LwPrintfString_hs)
{

    char *wszFormat = "%*.*hs";
    char output[10];
    char expectedOutput[] = {' ', ' ', 'a', '\0'};
    wchar16_t wszWrite[] = {'a', 'b', 'c', '\0'};
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfString(
                output,
                10,
                &scchWrote,
                wszFormat,
                3,
                2,
                wszWrite);

    MU_ASSERT(!strcmp(output, expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(scchWrote == strlen(expectedOutput));

}

MU_TEST(LwPrintf, LwPrintfString_ls)
{
    char *wszFormat = "%*.*ls";
    char output[10];
    char expectedOutput [] = {' ','a','b','\0'};
    int error = 0;
    size_t scchWrote = 0;

    error = LwPrintfString(
                output,
                10,
                &scchWrote,
                wszFormat,
                3,
                2,
                L"abc");

    MU_ASSERT(!strcmp(output, expectedOutput));
    MU_ASSERT(error == 0);
    MU_ASSERT(scchWrote == strlen(expectedOutput));
}

MU_TEST(LwPrintf, LwPrintfString_lencheck)
{
    char *wszFormat = "%*.*ls";
    char output[10];
    char expectedOutput[] = {' ', ' ', 0};
    size_t scchWrote = 0;
    int error = 0;

    error = LwPrintfString(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                2,
                0,
                "abc");
    MU_ASSERT(scchWrote == strlen(expectedOutput));
    MU_ASSERT(!strcmp(output, expectedOutput));
    MU_ASSERT(error == 0);

    error = LwPrintfString(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                9,
                0,
                "abc");
    MU_ASSERT(scchWrote == 9);
    MU_ASSERT(error == 0);

    error = LwPrintfString(
                output,
                sizeof(output)/sizeof(output[0]),
                &scchWrote,
                wszFormat,
                10,
                0,
                "abc");
    MU_ASSERT(error);
}


MU_TEST(LwPrintf, LwPrintf_null_precision)
{
    char *format = "%.d";
    char expectedOutput[10] = "200";
    char output[10];
    int error = 0;
    size_t written = 0;

    error = LwPrintfString(
                output,
                sizeof(output)/sizeof(output[0]),
                &written,
                format,
                200);

    MU_ASSERT(written == 3);
    MU_ASSERT(error == 0);
    MU_ASSERT(strcmp(expectedOutput, output) == 0);
}

    
