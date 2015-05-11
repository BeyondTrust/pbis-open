/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LikewiseSecurityObjectInfo.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        Security Object Information (Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

import com.sun.jna.Union;

/**
 * Java Native Access Wrapper around the LSA_SECURITY_OBJECT_INFO native
 * structure.
 */
public class LikewiseSecurityObjectInfo extends Union
{
	/**
	 * Group Info
	 */
	public LsaSecurityObjectGroupInfo groupInfo;
	/**
	 * User Info
	 */
	public LsaSecurityObjectUserInfo  userInfo;
}
