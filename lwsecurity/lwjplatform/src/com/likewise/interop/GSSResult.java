/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        GSSResult.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        GSS Authentication Adapter (Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

/**
 * Result of a GSSAPI Function Call.
 */
public class GSSResult
{
	private byte[]  _security_blob;
	private boolean _continue_needed = false;
	
	/**
	 * Builds a GSSAPI Result object.
	 * 
	 * @param security_blob   Resultant byte array of a GSSAPI call.
	 * @param continue_needed Whether the GSSAPI operation is complete.
	 */
	public
	GSSResult(
		byte[]  security_blob,
		boolean continue_needed
		)
	{
		_security_blob = security_blob;
		_continue_needed = continue_needed;
	}
	
	/**
	 * Checks whether the GSSAPI operation must be continued.
	 * 
	 * @return true if the GSSAPI operation must be continued.
	 */
	public
	boolean
	isContinueNeeded()
	{
		return _continue_needed;
	}
	
	/**
	 * Retrieves the resultant byte array of the associated GSSAPI call.
	 * 
	 * @return resultant byte array of the associated GSSAPI call
	 */
	public
	byte[]
	getSecurityBlob()
	{
		return _security_blob;
	}
}
