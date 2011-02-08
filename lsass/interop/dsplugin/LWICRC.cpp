/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
