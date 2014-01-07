/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        AdvapiAdapter.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        Advapi Adapter (Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;

/**
 * Java Native Access Wrapper for the Likewise library (liblwadvapi.so)
 * <p>
 * This class provides access into the ANSI C API of the Advanced API library.
 * </p>
 */
public class AdvapiAdapter extends LikewiseNativeAdapter
{
	/**
	 * Java Native Access Wrapper Library for the 
	 * Likewise library (liblwadvapi.so)
	 */
	public interface LWAdvapiLibrary extends Library
    {
		LWAdvapiLibrary INSTANCE = (LWAdvapiLibrary) Native.loadLibrary(
			                                            "lwadvapi",
			                                            LWAdvapiLibrary.class);

		/**
		 * Frees the contents of the given array of strings.
		 * 
		 * @param ppStringArray Array of strings.
		 * @param numStrings    Number of strings in the array.
		 */
		void
		LwFreeStringArray(
			Pointer ppStringArray,
			int     numStrings
			);
		
		/**
		 * Frees the memory addressed by the specified JNA pointer.
		 * 
		 * @param pMemory JNA Pointer pointing to native memory within.
		 */
		void
		LwFreeMemory(
			Pointer pMemory
			);
    }
}
