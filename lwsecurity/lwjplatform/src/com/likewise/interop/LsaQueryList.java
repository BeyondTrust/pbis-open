/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LsaQueryList.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        Query List (Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

import java.util.ArrayList;
import java.util.List;

import com.sun.jna.Structure;

/**
 * Helper class to marshal a list of strings or integers to be passed into
 * platform native APIs.
 * <p>
 * Note: Only ANSI strings are handled at this time.
 * </p>
 */
public class LsaQueryList extends Structure
{
	public LsaQueryString[] _names = null;
	public int[] _ids = null;
	
	/**
	 * Marshals an array of strings to platform native format.
	 * 
	 * @param names Array of strings to be passed to native code.
	 */
	public LsaQueryList(String[] names)
	{
		List<LsaQueryString> nameList = new ArrayList<LsaQueryString>();
		for (String name : names)
		{
			nameList.add(new LsaQueryString(name));
		}
		_names = nameList.toArray(new LsaQueryString[names.length]);
		_ids = new int[1];
		
		write();
	}
	
	/**
	 * Marshals an array of integers to platform native format.
	 * 
	 * @param names Array of integers to be passed to native code.
	 */
	public LsaQueryList(int[] ids)
	{
		_ids = ids;
		_names = new LsaQueryString[1];
		
		write();
	}
}
