/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LikewiseSecurityObjectGroupInfo.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        Security Object Group Information(Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

/**
 * Java Native Access abstraction of a LSA_SECURITY_OBJECT_GROUP_INFO
 * native structure defined in the Likewise authentication system.
 */
public class LsaSecurityObjectGroupInfo extends Structure 
{
	/**
	 * Group identifier.
	 */
    public int    gid;
    /**
     * Alias
     */
    public String pszAliasName;
    /**
     * Name one would use to identify a group on a UNIX system.
     * This would be an NT4 style name.
     * Example: ABC\domain^users
     */
    public String pszUnixName;
    /**
     * Plain text password
     */
    public String pszPasswd;
    
    /**
     * Builds a default group info object.
     */
    public LsaSecurityObjectGroupInfo()
    {
    }
    
    /**
     * Builds a group info object based on native memory pointing to a 
     * PLSA_SECURITY_OBJECT_GROUP_INFO object.
     * 
     * @param p Native memory pointing to a PLSA_SECURITY_OBJECT_GROUP_INFO 
     *          object.
     */
    public LsaSecurityObjectGroupInfo(Pointer p)
    {
    	useMemory(p);
    	read();
    }
}
