/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "LWICRC.h"
#include <time.h>
#include <stdint.h>

LWICRC* LWICRC::_instance = NULL;

void
LWICRC::Initialize()
{
    if (_instance != NULL)
    {
        delete _instance;
        _instance = NULL;
    }
    _instance = new LWICRC();
}

void
LWICRC::Cleanup()
{
    if (_instance)
    {
       delete _instance;
    }

    _instance = NULL;
}

unsigned long
LWICRC::GetCRC(
    char* pByteArray,
    int   length)
{
    assert(_instance != NULL);

    return _instance->CalculateCRC(pByteArray, length);
}

LWICRC::LWICRC()
: _key(time(NULL))
{
    InitTable();
}

LWICRC::LWICRC(unsigned long key)
: _key (key)
{
    InitTable();
}

void
LWICRC::InitTable()
{
    assert (_key != 0);

    // for all possible byte values
    for (int i = 0; i < 256; ++i)
    {
        uint32_t reg = i << 24;
        // for all bits in a byte
        for (int j = 0; j < 8; ++j)
        {
            bool topBit = (reg & 0x80000000) != 0;
            reg <<= 1;
            if (topBit)
            {
                reg ^= _key;
            }
        }
        _table [i] = reg;
    }
}

unsigned long
LWICRC::CalculateCRC(char* pByteArray, int length) const
{
    uint32_t reg = 0;

    for (int i = 0; i < length; i++)
    {
        uint32_t top = reg >> 24;
        top ^= *(pByteArray+i);
        reg = (reg << 8) ^ _table [top];
    }

    return reg;
}
