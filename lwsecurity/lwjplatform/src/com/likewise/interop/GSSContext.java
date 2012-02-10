/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        GSSContext.java
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

import java.io.Serializable;
import java.util.List;

import com.sun.jna.Pointer;

/**
 * Wrapper around native Likewise GSSAPI context.
 *
 */
public class GSSContext implements Serializable
{
	private static final long serialVersionUID = 95470472966510591L;
	
	private Pointer _nativeContext  = null;
	
	/**
	 * Builds a GSSContext object wrapping a native context.
	 */
	public GSSContext()
	{
		_nativeContext = GSSAdapter.createContext();
	}
	
	/**
	 * Accepts (and authenticates) the GSS security context.
	 * 
	 * @param securityBlob Security BLOB that must be authenticated.
	 * @return Result of the GSSAPI operation.
	 */
	public
	GSSResult
	authenticate(byte[] securityBlob)
	{
		return GSSAdapter.authenticate(_nativeContext, securityBlob);
	}
	
	/**
	 * Retrieves the client user principal associated with the authenticated
	 * GSS Context.
	 * 
	 * @return Name of the user authenticated by the GSS Context.
	 */
	public
	String
	getClientPrincipal()
	{
		return GSSAdapter.getClientPrincipal(_nativeContext);
	}
	
	/**
	 * Retrieves the list of roles the currently authenticated user is a member
	 * of.
	 * 
	 * @return List of roles the currently authenticated user is a member of.
	 */
	public
	List<String>
	getRoles()
	{
		return GSSAdapter.getRoles(_nativeContext);
	}
	
	@Override
	protected void finalize()
	{
		if (_nativeContext != null)
		{
			GSSAdapter.freeContext(_nativeContext);
			_nativeContext = null;
		}
	}
}
