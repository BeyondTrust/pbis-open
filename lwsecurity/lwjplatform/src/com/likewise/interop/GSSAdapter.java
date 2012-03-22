/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        GSSAdapter.java
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

import java.util.ArrayList;
import java.util.List;

import com.likewise.Win32Exception;
import com.likewise.interop.LikewiseNativeAdapter;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;

/**
 * Java Native Access Wrapper for the Likewise library (liblsagsswrap.so)
 * <p>
 * This class provides access into the ANSI C API of the 
 * Likewise GSS Wrapper library.
 * </p>
 */
public class GSSAdapter extends LikewiseNativeAdapter
{
	private static Boolean _initialized = false;
	
	/**
	 * Java Native Access Wrapper Library for the 
	 * Likewise library (liblsagsswrap.so)
	 */
	public interface LWLsaGssLibrary extends Library
    {
		LWLsaGssLibrary INSTANCE = (LWLsaGssLibrary) Native.loadLibrary(
			                                            "lsagsswrap",
			                                            LWLsaGssLibrary.class);

		int
		LsaGssInitialize();
		
		int
		LsaGssCreateContext(
			PointerByReference ppContext
			);
		
		int
		LsaGssAuthenticate(
			Pointer            pContext,
		    byte[]             pInBytes,
		    int                dwInLength,
		    PointerByReference ppOutBytes,
		    IntByReference     pdwOutBytes,
		    IntByReference     pbContinueNeeded
		    );
		
		int
		LsaGssGetRoles(
			Pointer            pContext,
			PointerByReference pppRoles,
			IntByReference     pdwNumRoles
			);
		
		int
		LsaGssGetClientPrincipalName(
		    Pointer            pContext,
		    PointerByReference ppszClientName
		    );
		
		int
		LsaGssGetErrorInfo(
			Pointer            pContext,
			IntByReference     pdwMajorStatus,
			IntByReference     pdwMinorStatus,
			PointerByReference pErrorString
			);
		
		void
		LsaGssFreeContext(
			Pointer pContext
			);
		
		void
		LsaGssShutdown();
		
		void
		LsaGssFreeStringArray(
			Pointer pStringArray,
			int     dwNumStrings
			);
		
		void
		LsaGssFreeMemory(
			Pointer pMemory
			);
    }
	
	/**
	 * Retrieves the native GSS Wrapper context.
	 * 
	 * @return Likewise Native GSS Wrapper Context.
	 */
	public
	static
	Pointer
	createContext()
	{
		int winErr = 0;
		PointerByReference ppContext = new PointerByReference();
		
		initialize();
		
		winErr = LWLsaGssLibrary.INSTANCE.LsaGssCreateContext(ppContext);
		if (winErr != 0)
		{
			throw new Win32Exception(winErr);
		}
		
		return ppContext.getValue();
	}
	
	/**
	 * Authenticates the given security blob using the GSS Wrapper context.
	 * 
	 * @param pContext      Likewise Native GSS Wrapper Context
	 * @param security_blob security blob that must be accepted for 
	 *                      authentication.
	 * @return Result of the native GSSAPI call.
	 */
	public 
	static
	GSSResult
	authenticate(
			Pointer pContext,
			byte[]  security_blob
			)
	{
		PointerByReference ppOutBytes = new PointerByReference();
		IntByReference     pdwOutBytes = new IntByReference();
		IntByReference     pbContinueNeeded = new IntByReference();
		
		initialize();
		
		try
		{
			int winErr = 0;
			byte[] security_blob_out = null;
			int    security_blob_out_len = 0;
			
			winErr = LWLsaGssLibrary.INSTANCE.LsaGssAuthenticate(
												pContext,
												security_blob,
												security_blob.length,
												ppOutBytes,
												pdwOutBytes,
												pbContinueNeeded);
			if (winErr != 0)
			{
				GSSException gssExc = getGssException(pContext, winErr);
				throw gssExc;
			}
			
			Pointer pOutBytes = ppOutBytes.getValue();
			if (pOutBytes != null)
			{
				security_blob_out_len = pdwOutBytes.getValue();
				security_blob_out = new byte[security_blob_out_len];
				
				pOutBytes.read(0, security_blob_out, 0, security_blob_out_len);
			}
			
			return new GSSResult(
							security_blob_out,
							pbContinueNeeded.getValue() == 0 ? false : true);
		}
		finally
		{
			if (ppOutBytes.getValue() != null)
			{
				LWLsaGssLibrary.INSTANCE.LsaGssFreeMemory(ppOutBytes.getValue());
				ppOutBytes.setValue(null);
			}
		}
	}
	
	/**
	 * Retrieves the client principal associated with the authenticated GSS
	 * Wrapper context.
	 * 
	 * @param pContext Likewise Native GSS Wrapper Context
	 * @return Name of the authenticated user principal.
	 */
	public
	static
	String
	getClientPrincipal(Pointer pContext)
	{
		int winErr = 0;
		PointerByReference ppszClientName = new PointerByReference();
		String clientPrincipal = "";
		
		initialize();
		
		try
		{
			winErr = LWLsaGssLibrary.INSTANCE.LsaGssGetClientPrincipalName(
													pContext,
													ppszClientName);
			if (winErr != 0)
			{
				GSSException gssExc = getGssException(pContext, winErr);
				throw gssExc;
			}
			
			if (ppszClientName.getValue() != null)
			{
				clientPrincipal = ppszClientName.getValue().getString(0);
			}

			return clientPrincipal;
		}
		finally
		{
			if (ppszClientName.getValue() != null)
			{
				LWLsaGssLibrary.INSTANCE.LsaGssFreeMemory(
												ppszClientName.getValue());
			}
		}
	}
	
	/**
	 * Retrieves the list of roles the currently authenticated user is a member
	 * of.
	 * 
	 * @param pContext Likewise Native GSS Wrapper Context.
	 * @return List of roles the currently authenticated user is a member of.
	 */
	public
	static
	List<String>
	getRoles(Pointer pContext)
	{
		ArrayList<String> roles = new ArrayList<String>();
		int winErr = 0;
		PointerByReference pppszRoles = new PointerByReference();
		IntByReference pdwNumRoles = new IntByReference();
		
		initialize();
		
		try
		{
			winErr = LWLsaGssLibrary.INSTANCE.LsaGssGetRoles(
													pContext,
													pppszRoles,
													pdwNumRoles);
			if (winErr != 0)
			{
				GSSException gssExc = getGssException(pContext, winErr);
				throw gssExc;
			}
			
			if (pdwNumRoles.getValue() > 0)
			{
				Pointer groupNameArray[] =
							pppszRoles.getValue().getPointerArray(
													0,
													pdwNumRoles.getValue());
				
				for (Pointer groupName : groupNameArray)
				{
					 roles.add(groupName.getString(0));
				}
			}

			return roles;
		}
		finally
		{
			if (pppszRoles.getValue() != null)
			{
				LWLsaGssLibrary.INSTANCE.LsaGssFreeStringArray(
											pppszRoles.getValue(),
											pdwNumRoles.getValue());
			}
		}
	}
	
	/**
	 * Frees the native GSS Wrap context.
	 * 
	 * @param pContext Likewise Native GSS Wrapper Context.
	 */
	public
	static
	void
	freeContext(Pointer pContext)
	{
		if (pContext != null)
		{
			LWLsaGssLibrary.INSTANCE.LsaGssFreeContext(pContext);
		}
	}
	
	protected void finalize()
	{
		synchronized (_initialized)
		{
			if (_initialized)
			{
				LWLsaGssLibrary.INSTANCE.LsaGssShutdown();
				_initialized = false;
			}
		}
	}
	
	private
	static
	GSSException
	getGssException(Pointer pContext, int errCode)
	{
		int winErr2 = 0;
		IntByReference     pdwMajorStatus = new IntByReference();
		IntByReference     pdwMinorStatus = new IntByReference();
		PointerByReference pErrorString   = new PointerByReference();
		
		try
		{
			winErr2 = LWLsaGssLibrary.INSTANCE.LsaGssGetErrorInfo(
							pContext, 
							pdwMajorStatus, 
							pdwMinorStatus, 
							pErrorString);
			if ((winErr2 == 0))
			{
				return new GSSException(
								errCode,
								pdwMajorStatus.getValue(),
								pdwMinorStatus.getValue(),
								pErrorString.getValue().getString(0));
			}
			else
			{
				return new GSSException(errCode);
			}
		}
		finally
		{
			if (pErrorString.getValue() != null)
			{
				LWLsaGssLibrary.INSTANCE.LsaGssFreeMemory(
												pErrorString.getValue());
			}
		}
	}
	
	private
	static
	void
	initialize()
	{
		synchronized (_initialized)
		{
			if (!_initialized)
			{
				int winErr = LWLsaGssLibrary.INSTANCE.LsaGssInitialize();
				
				if (winErr > 0)
				{
					throw new Win32Exception(winErr);
				}
				
				_initialized = true;
			}
		}
	}
}
