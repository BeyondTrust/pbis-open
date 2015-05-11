/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        GSSException.java
 *
 * Abstract:
 *
 *        Likewise Common Library
 *
 *        GSS Exception
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

/**
 * Exception from a GSSAPI call.
 */
public class GSSException extends RuntimeException
{
	private static final long serialVersionUID = 4634401296240444296L;
	
	private int _errCode     = 0;
	private int _majorStatus = 0;
	private int _minorStatus = 0;

	/**
	 * Builds a GSSException based on an error code.
	 * 
	 * @param errCode Likewise GSSAPI error code
	 */
	public GSSException(int errCode)
	{
		_errCode = errCode;
	}
	
	/**
	 * Builds a GSSException based on the major and minor GSS status values
	 * and associated error message.
	 * 
	 * @param errCode      Likewise GSSAPI Error code.
	 * @param majorStatus  GSSAPI major status.
	 * @param minorStatus  GSSAPI minor status.
	 * @param message      Optional error message.
	 */
	public GSSException(int errCode,
			int    majorStatus,
			int    minorStatus,
			String message
			)
	{
		super(message);
		_errCode = errCode;
		_majorStatus = majorStatus;
		_minorStatus = minorStatus;
	}
	
	/**
	 * Retrieves the Likewise error code associated with the GSSAPI call.
	 * 
	 * @return Likewise error code.
	 */
    public 
    int
	GetErrorCode()
	{
		return _errCode;
	}
    
    /**
     * Retrieves the major GSSAPI status code.
     * 
     * @return Major GSSAPI status code.
     */
    public
    int
    GetMajorStatus()
    {
    	return _majorStatus;
    }
    
    /**
     * Retrieves the minor GSSAPI status code.
     * 
     * @return Minor GSSAPI status code.
     */
    public
    int
    GetMinorStatus()
    {
    	return _minorStatus;
    }
}
