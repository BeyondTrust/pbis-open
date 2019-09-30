/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */

#include <moonunit/moonunit.h>
#include <wc16printf.h>
#include <wc16str.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

static
void CheckCharToWchar16(const char *input)
{
    size_t inputlen = mbstrlen(input);
    if(input[0] != '\0')
        MU_ASSERT(inputlen != 0);
    wchar16_t *allocated = malloc((inputlen + 1) * sizeof(wchar16_t));
    MU_ASSERT(allocated != NULL);
    size_t converted = mbstowc16s(allocated, input, inputlen + 1);
    MU_ASSERT(converted == inputlen);
    MU_ASSERT(wc16slen(allocated) == inputlen);
    free(allocated);

    allocated = ambstowc16s(input);
    MU_ASSERT(allocated != NULL);
    MU_ASSERT(wc16slen(allocated) == inputlen);

    char *convertedback = awc16stombs(allocated);
    MU_ASSERT(convertedback != NULL);
    MU_ASSERT(!strcmp(input, convertedback));

    free(convertedback);
    free(allocated);
}

static
void CheckWcharToWchar16(const wchar_t *input)
{
    size_t inputlen = wcslen(input);
    wchar16_t *allocated = malloc((inputlen + 1) * sizeof(wchar16_t));
    MU_ASSERT(allocated != NULL);
    size_t converted = wcstowc16s(allocated, input, inputlen + 1);
    MU_ASSERT(converted == inputlen);
    MU_ASSERT(wc16slen(allocated) == inputlen);
    free(allocated);

    int free_allocated;
    allocated = awcstowc16s(input, &free_allocated);
    MU_ASSERT(allocated != NULL);
    MU_ASSERT(wc16slen(allocated) == inputlen);

    int free_convertedback;
    wchar_t *convertedback = awc16stowcs(allocated, &free_convertedback);
    MU_ASSERT(convertedback != NULL);
    MU_ASSERT(!wcscmp(input, convertedback));

    if(free_convertedback)
        free(convertedback);
    if(free_allocated)
        free(allocated);
}

MU_TEST(mbstowc16s, simple)
{
    setlocale(LC_ALL, "en_US.UTF-8");
   
    CheckCharToWchar16("a simple test string");
}

MU_TEST(mbstowc16s, japanese)
{
    setlocale(LC_ALL, "en_US.UTF-8");
    
    CheckCharToWchar16("this is a 日本語 test");
}

MU_TEST(mbstowc16s, longstr)
{
    char buffer[1024];
    int i;

    setlocale(LC_ALL, "en_US.UTF-8");
   
    for(i = 0; i < sizeof(buffer); i++)
    {
        buffer[i] = '\0';
        if(i > 0)
            buffer[i - 1] = 'a';
        CheckCharToWchar16(buffer);
    }
}

MU_TEST(wcstowc16s, simple)
{
    setlocale(LC_ALL, "en_US.UTF-8");
    CheckWcharToWchar16(L"a simple test string");
}

MU_TEST(wcstowc16s, longstr)
{
    int i;
    wchar_t wbuffer[1024];

    setlocale(LC_ALL, "en_US.UTF-8");
    for(i = 0; i < sizeof(wbuffer)/sizeof(wbuffer[0]); i++)
    {
        wbuffer[i] = '\0';
        if(i > 0)
            wbuffer[i - 1] = L'a';
        CheckWcharToWchar16(wbuffer);
    }
}

MU_TEST(wcstowc16s, japanese)
{
    setlocale(LC_ALL, "en_US.UTF-8");
    wchar_t *japanese = L"日本語";
    CheckWcharToWchar16(japanese);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
