/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LsaObjectType.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        LSA Object Type (Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

import java.util.EnumSet;
import java.util.HashMap;
import java.util.Map;

/**
 * Enumeration of object types defined in the Likewise authentication system.
 */
public enum LsaObjectType
{
	LSA_OBJECT_TYPE_UNDEFINED(0),
	LSA_OBJECT_TYPE_GROUP(1),
	LSA_OBJECT_TYPE_USER(2),
	LSA_OBJECT_TYPE_DOMAIN(3),
	LSA_OBJECT_TYPE_COMPUTER(4);
	
    private static final Map<Integer,LsaObjectType> lookup 
    									= new HashMap<Integer,LsaObjectType>();

	static
	{
	    for(LsaObjectType objType : EnumSet.allOf(LsaObjectType.class))
	    {
	         lookup.put(objType.getCode(), objType);
	    }
	}
	
	int _code = 0;
	
	private LsaObjectType(int code)
	{
		_code = code;
	}
	
	public int getCode()
	{
		return _code;
	}
	
	public static LsaObjectType get(int code)
	{
		return lookup.get(code);
	}
}
