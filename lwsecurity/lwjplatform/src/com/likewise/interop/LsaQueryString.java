/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LsaQueryString.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        Query String (Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

import com.sun.jna.Structure;

/**
 * Helper class to marshal a string to be passed into a platform native APIs.
 * <p>
 * Note: Only ANSI strings are handled at this time.
 * </p>
 */
public class LsaQueryString extends Structure
{
	public String _query;
	
	public LsaQueryString()
	{
	}
	
	public LsaQueryString(String query)
	{
		_query = query;
		
		write();
	}
}
