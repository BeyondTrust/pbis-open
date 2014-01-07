/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        Win32Exception.java
 *
 * Abstract:
 *
 *        Likewise Common Library
 *
 *        Win32 Exception
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise;

/**
 * Exception based on Windows error codes.
 */
public class Win32Exception extends RuntimeException
{
	private static final long serialVersionUID = -5898365544047872196L;
	private int _winError = -1;
	
	/**
	 * Builds a Win32 exception object based on a win32 error code.
	 * @param winError Win32 error code
	 */
	public Win32Exception(int winError)
	{
		assert(winError != 0);
		_winError = winError;
	}

	/**
	 * Builds a Win32 exception object based on an error message.
	 * @param msg
	 */
    public Win32Exception(String msg) {
        super(msg);
    }

	/**
	 * Builds a Win32 exception object based on a nested exception.
	 * @param msg Nested exception
	 */
    public Win32Exception(Exception e) {
        super(e);
    }

    /**
     * Retrieves the Windows error code.
     * 
     * @return Windows error code.
     */
    public int
	GetErrorCode()
	{
		return _winError;
	}
	
    /**
     * Retrieves the error message associated with this exception.
     * 
     * @return Error message associated with this exception.
     */
	public String getMessage()
	{
		return "Win32 error code: [" + _winError + "]" + super.getMessage();
	}
}
