/*
 *  LWICRC.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 6/8/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
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

