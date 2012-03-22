/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */

#include <moonunit/moonunit.h>
#include <wc16printf.h>
#include <wc16str.h>

MU_TEST(Printf, sw16printf_decimal)
{
    wchar16_t wszFormat[] = {'%', 'd', 0};
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {'1','2',0};
    ssize_t scchWrote = 0;

    scchWrote = sw16printf_new(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                wszFormat,
                12);
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));

    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(Printf, sw16printf_precise_double)
{
    wchar16_t wszFormat[] = {'%','.','3','f',0};
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {'0','.','1','2','3',0};
    ssize_t scchWrote = 0;

    scchWrote = sw16printf_new(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                wszFormat,
                0.123456);
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));

    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(Printf, sw16printf_parameter_width_and_precision)
{
    wchar16_t wszFormat[] = {'%','*','.','*','f',0};
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {' ','0','.','1',0};
    ssize_t scchWrote = 0;

    scchWrote = sw16printf_new(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                wszFormat,
                4,
                1,
                0.123456);
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));

    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(Printf, sw16printf_precise_long_double)
{
    wchar16_t wszFormat[] = {'%','.','3','L','f',0};
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {'0','.','1','2','3',0};
    ssize_t scchWrote = 0;

    scchWrote = sw16printf_new(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                wszFormat,
                (long double)0.123456);
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));

    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(Printf, sw16printf_mbs)
{
    wchar16_t wszFormat[] = {'%','*','.','*','h','h','s',0};
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {' ', 'a', 'b', 0};
    ssize_t scchWrote = 0;

    scchWrote = sw16printf_new(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                wszFormat,
                3,
                2,
                "abc");
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));

    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(Printf, sw16printf_null)
{
    wchar16_t wszFormat[] = {'%','h','h','s','%','w','s','%','l','s',0};
    wchar16_t wszOutput[30];
    wchar16_t wszExpectedOutput[] = {
        '(','n','u','l','l',')','(','n','u','l','l',')',
        '(','n','u','l','l',')',0};
    ssize_t scchWrote = 0;

    scchWrote = sw16printf_new(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                wszFormat,
                NULL,
                NULL,
                NULL);
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));

    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(Printf, sw16printf_wc16s)
{
    wchar16_t wszFormat[] = {'%','*','.','*','h','s',0};
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {' ', 'a', 'b', 0};
    wchar16_t wszWrite[] = {'a', 'b', 'c'};
    ssize_t scchWrote = 0;

    scchWrote = sw16printf_new(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                wszFormat,
                3,
                2,
                wszWrite);
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));

    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(Printf, sw16printf_wcs)
{
    wchar16_t wszFormat[] = {'%','*','.','*','l','s',0};
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {' ', 'a', 'b', 0};
    ssize_t scchWrote = 0;

    scchWrote = sw16printf_new(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                wszFormat,
                3,
                2,
                L"abc");
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));

    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));
}

MU_TEST(Printf, sw16printf_lencheck)
{
    wchar16_t wszFormat[] = {'%','*','.','*','l','s',0};
    wchar16_t wszOutput[10];
    wchar16_t wszExpectedOutput[] = {' ', ' ', 0};
    ssize_t scchWrote = 0;

    scchWrote = sw16printf_new(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                wszFormat,
                2,
                0,
                L"abc");
    MU_ASSERT(scchWrote == wc16slen(wszExpectedOutput));
    MU_ASSERT(!wc16scmp(wszOutput, wszExpectedOutput));

    scchWrote = sw16printf_new(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                wszFormat,
                9,
                0,
                L"abc");
    MU_ASSERT(scchWrote == 9);

    scchWrote = sw16printf_new(
                wszOutput,
                sizeof(wszOutput)/sizeof(wszOutput[0]),
                wszFormat,
                10,
                0,
                L"abc");
    MU_ASSERT(scchWrote == -1);
}

MU_TEST(Printf, asw16printfw)
{
    wchar16_t wszExpectedOutput[] = {'3',' ','-',' ','a','b','c','x',0};
    wchar16_t *pwszOutput = NULL;

    pwszOutput = asw16printfw(
                L"%d - %lsx",
                3,
                L"abc");
    MU_ASSERT(!wc16scmp(pwszOutput, wszExpectedOutput));
    free(pwszOutput);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
