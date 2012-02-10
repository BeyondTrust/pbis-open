/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LikewiseNativeUser.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        Native User (Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

/**
 * Java Native Access Wrapper around the LSA_SECURITY_OBJECT native structure.
 */
public class LikewiseNativeObject extends Structure
{
	public LsaSecurityObjectVersionInfo secObjVerInfo;
	public String                       pszDN;
    public String                       pszObjectSid;
    public byte                         enabled;
    public byte                         isLocal;

    public String                       pszNetbiosDomainName;
    public String                       pszSamAccountName;
	public int                          securityObjectType;

    public LikewiseSecurityObjectInfo   securityObjectInfo;
	
    /**
     * Builds a Java based abstraction given a native pointer to memory 
     * associated with the PLSA_SECURITY_OBJECT type.
     * 
     * @param p Native pointer to memory associated with PLSA_SECURITY_OBJECT
     */
	public LikewiseNativeObject(Pointer p)
	{
		useMemory(p);
		read();
		
		if (LsaObjectType.get(securityObjectType).equals(
										LsaObjectType.LSA_OBJECT_TYPE_USER))
		{
			securityObjectInfo.setType(LsaSecurityObjectUserInfo.class);
		}
		else if (LsaObjectType.get(securityObjectType).equals(
										LsaObjectType.LSA_OBJECT_TYPE_GROUP))
		{
			securityObjectInfo.setType(LsaSecurityObjectGroupInfo.class);
		}
		else
		{
			throw new RuntimeException("Unexpected security object type [" +
										securityObjectType +
										"]");
		}
		
		securityObjectInfo.read();
	}
}
