/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LsaSecurityObjectVersionInfo
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        Security Object Version Information(Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

import com.sun.jna.NativeLong;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

/**
 * Java Native Access abstraction of a LSA_SECURITY_OBJECT_VERSION_INFO
 * native structure defined in the Likewise authentication system.
 */
public class LsaSecurityObjectVersionInfo extends Structure
{
    public long       qwDbId;
    public NativeLong tLastUpdated;
    public int        dwObjectSize;
    public float      fWeight;
    
    public LsaSecurityObjectVersionInfo()
    {
        qwDbId       = 0;
        tLastUpdated.setValue(0);
        dwObjectSize = 0;
        fWeight      = 0;
    }
    
	public LsaSecurityObjectVersionInfo(Pointer p)
	{
		useMemory(p);
		read();
	}
}
