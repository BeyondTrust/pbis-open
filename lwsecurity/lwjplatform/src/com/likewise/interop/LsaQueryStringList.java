/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LsaQueryStringList.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        Query String List (Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

import java.util.ArrayList;
import java.util.List;

import com.sun.jna.Structure;

/**
 * Helper class to marshal a list of strings to be passed into a platform native 
 * APIs.
 * <p>
 * Note: Only ANSI strings are handled at this time.
 * </p>
 */
public class LsaQueryStringList extends Structure
{
	public LsaQueryString[] _names = null;
	
	public LsaQueryStringList(List<String> names)
	{
		List<LsaQueryString> nameList = new ArrayList<LsaQueryString>();
		for (String name : names)
		{
			nameList.add(new LsaQueryString(name));
		}
		_names = nameList.toArray(new LsaQueryString[names.size()]);
		
		write();
	}
}
