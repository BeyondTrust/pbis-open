/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LsaFindFlags.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        LSA Find Flags (Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

/**
 * Native flags used when finding objects in the Likewise Authentication System.
 */
public class LsaFindFlags
{
	private static final int LSA_FIND_FLAGS_NONE       = 0x0;
	private static final int LSA_FIND_FLAGS_NSS        = 0x00000001;
	private static final int LSA_FIND_FLAGS_LOCAL      = 0x00000002;
	private static final int LSA_FIND_FLAGS_CACHE_ONLY = 0x00000004;
	
	private int _flags = LSA_FIND_FLAGS_NONE;
	
	/**
	 * Sets the flag to lookup objects through Name Service Switch (NSS)
	 */
	public synchronized void setFindInNSS()
	{
		_flags |= LSA_FIND_FLAGS_NSS;
	}
	
	/**
	 * Resets the flag to lookup objects through Name Service Switch (NSS)
	 */
	public synchronized void unsetFindInNSS()
	{
		_flags &= ~LSA_FIND_FLAGS_NSS;
	}
	
	/**
	 * Sets the flag to lookup objects in the local authentication provider.
	 */
	public synchronized void setFindLocal()
	{
		_flags |= LSA_FIND_FLAGS_LOCAL;
	}
	
	/**
	 * Resets the flag to lookup objects in the local authentication provider.
	 */
	public synchronized void unsetFindLocal()
	{
		_flags &= ~LSA_FIND_FLAGS_LOCAL;
	}
	
	/**
	 * Sets the flag to lookup objects in the local authentication cache.
	 */
	public synchronized void setFindCacheOnly()
	{
		_flags |= LSA_FIND_FLAGS_CACHE_ONLY;
	}
	
	/**
	 * Resets the flag to lookup objects in the local authentication cache.
	 */
	public synchronized void unsetFindCacheOnly()
	{
		_flags &= ~LSA_FIND_FLAGS_CACHE_ONLY;
	}
	
	/**
	 * Retrieves the current flag set to be used when finding objects.
	 * 
	 * @return Current finder flags.
	 */
	public synchronized int getFlags()
	{
		return _flags;
	}
	
	/**
	 * Resets finder flags.
	 */
	public synchronized void reset()
	{
		_flags = LSA_FIND_FLAGS_NONE;
	}
}
