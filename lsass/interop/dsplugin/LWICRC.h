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

#ifndef __LWICRC_H__
#define __LWICRC_H__

#include <assert.h>

class LWICRC
{
    public:
    
        static void Initialize();
        static void Cleanup();
        
        static unsigned long GetCRC(char* pByteArray, int length);
        
    protected:
            
        LWICRC();
        LWICRC (unsigned long key);
        ~LWICRC() {}
        
        LWICRC(const LWICRC& other);
        LWICRC& operator=(const LWICRC& other);
    
    protected:
        
        inline void SetKey(unsigned long key)
        {
            _key = key;
            InitTable();
        }
        
        inline unsigned long GetKey() const
        {
            return _key;
        }
        
        unsigned long CalculateCRC(char* pByteArray, int length) const;
        
    private:
       
       void InitTable();
        
    private:
    
        unsigned long _key;	// really 33-bit key, counting implicit 1 top-bit
        unsigned long _table[256];
        
        static LWICRC* _instance;
};

#endif /* __LWICRC_H__ */

