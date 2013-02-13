/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LsaQueryType.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        LSA Query Type (Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

import java.util.EnumSet;
import java.util.HashMap;
import java.util.Map;

/**
 * Java Native Access Wrapper abstracting native query types defined in the
 * Likewise authentication system.
 */
public enum LsaQueryType
{
	LSA_QUERY_TYPE_UNDEFINED(0),
	LSA_QUERY_TYPE_BY_DN(1),
	LSA_QUERY_TYPE_BY_SID(2),
	LSA_QUERY_TYPE_BY_NT4(3),
	LSA_QUERY_TYPE_BY_UPN(4),
	LSA_QUERY_TYPE_BY_ALIAS(5),
	LSA_QUERY_TYPE_BY_UNIX_ID(6),
	LSA_QUERY_TYPE_BY_NAME(7);
	
    private static final Map<Integer,LsaQueryType> lookup 
    		= new HashMap<Integer,LsaQueryType>();

	static
	{
	    for(LsaQueryType queryType : EnumSet.allOf(LsaQueryType.class))
	    {
	         lookup.put(queryType.getCode(), queryType);
	    }
	}
	
	int _code;
	
	private LsaQueryType(int code)
	{
		_code = code;
	}
	
	public int getCode()
	{
		return _code;
	}
	
	public static LsaQueryType get(int code)
	{
		return lookup.get(code);
	}
}
