/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LikewiseUser.java
 *
 * Abstract:
 *
 *        Likewise Java Authentication and Authorization Service
 *
 *        User
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.auth;

import java.io.Serializable;
import java.security.Principal;

/**
 * Likewise User Principal
 * <p>
 * This is a user based on attributes assigned by the Likewise Authentication
 * stack. This user might be an identity from an Active Directory Domain or
 * from the Local Security Authority or any other authentication providers
 * configured in the Likewise Authentication Stack.
 * </p>
 */

public class LikewiseUser implements Principal, Serializable
{
	private static final long serialVersionUID = -1295269893546354525L;
	private int     _uid = -1;
	private int     _gid = -1;
	private String  _name;
	private String  _password;
	private String  _gecos;
	private String  _shell;
	private String  _homedir;
	private String  _sid;
	private String  _distinguishedName;
	private String  _userPrincipalName;
	private boolean _isLocalUser = false;
	
	/**
	 * Builds a Likewise user object using just the name.
	 * 
	 * @param name User name
	 * <p>This is typically the SAM account name of the user.</p>
	 */
	public LikewiseUser(String name)
	{
		_name = name + "";
	}
	
	/**
	 * Builds a Likewise user object using extended attributes.
	 * 
	 * @param name User name. This is the SAM account name for user from
	 *             Active directory.
	 * @param uid      User id.
	 * @param gid      Primary group id.
	 * @param sid      Security identifier.
	 * @param homedir  Path to home directory.
	 * @param password Plain text password.
	 * @param gecos    General Electric Comprehensive Operating Supervisor.
	 *                 Used to store extra information about the user such as
	 *                 phone extension etc.
	 * @param shell             Login shell.
	 * @param distinguishedName Distinguished name in LDAP format.
	 * @param userPrincipalName User principal name in the format of
	 *                          sam-acct-name@REALM
	 * @param bIsLocalUser      Whether the user is defined in the local
	 *                          security provider as opposed to a remote
	 *                          identity database such as Active Directory.
	 */
	public
	LikewiseUser(
		String  name,
		int     uid,
		int     gid,
		String  sid,
		String  homedir,
		String  password,
		String  gecos,
		String  shell,
		String  distinguishedName,
		String  userPrincipalName,
		boolean bIsLocalUser
		)
	{
		_name     = name + "";
		_uid      = uid;
		_gid      = gid;
		_sid      = sid + "";
		_homedir  = homedir + "";
		_password = password + "";
		_gecos    = gecos + "";
		_shell    = shell + "";
		_distinguishedName = distinguishedName + "";
		_userPrincipalName = userPrincipalName + "";
		_isLocalUser = bIsLocalUser;
	}

	/**
	 * Retrieves the user name.
	 * <p>
	 * Typically, this is the SAM account name assigned to users in Active
	 * Directory.
	 * </p>
	 */
	public String getName()
	{
		return _name;
	}
	
	/**
	 * Retrieves the integer user identifier assigned to the user principal.
	 * <p>
	 * On UNIX systems, this is typically the uid of the user.
	 * </p>
	 * @return integer user identifier
	 */
	public int getUserId()
	{
		return _uid;
	}
	
	/**
	 * Retrieves the identifier of the primary group the user is assigned to.
	 * 
	 * @return The identifier of the primary group the user is assigned to.
	 */
	public int getPrimaryGroupId()
	{
		return _gid;
	}
	
	/**
	 * Retrieves the path to the user's home directory.
	 * 
	 * @return Path to the user's home directory.
	 */
	public String getHomeDirectory()
	{
		return _homedir;
	}
	
	/**
	 * Retrieves the security descriptor of the current user principal.
	 * 
	 * @return Security descriptor of the current user principal.
	 */
	public String getSecurityIdentifier()
	{
		return _sid;
	}
	
	/**
	 * Retrieves the gecos field of the current user principal.
	 * <p>
	 * Gecos stands for General Electric Comprehensive Operating Supervisor.
	 * It is a field that can be used to store additional information about
	 * a user principal such as phone numbers and extensions etc.
	 * </p>
	 * @return Value of the gecos field of the current user principal.
	 */
	public String getGecos()
	{
		return _gecos;
	}
	
	/**
	 * Retrieves the plain text password of the current user principal.
	 * 
	 * @return Plain text password assigned to the current user principal.
	 */
	public String getPassword()
	{
		return _password;
	}
	
	/**
	 * Retrieves the login shell of the current user principal.
	 * 
	 * @return Login shell assigned to the current user principal. 
	 *         Example: /bin/bash
	 */
	public String getShell()
	{
		return _shell;
	}
	
	/**
	 * Retrieves the distinguished name of the current user principal in
	 * LDAP format.
	 * 
	 * @return distinguished name of the current user principal.
	 */
	public String getDistinguishedName()
	{
		return _distinguishedName;
	}
	
	/**
	 * Retrieves the principal name of the current user.
	 * <p>
	 * Typically, this is in the format of sam-account-name@REALM.
	 * </p> 
	 * @return User principal name of current user.
	 */
	public String getUserPrincipalName()
	{
		return _userPrincipalName;
	}
	
	/**
	 * Checks if the current principal is defined in the local security
	 * provider.
	 * <p>
	 * Users principals defined in Active Directory will not be considered part
	 * of the local security provider.
	 * </p>
	 * 
	 * @return true if the current user is defined in the local security 
	 *              provider.
	 */
	public boolean isLocalUser()
	{
		return _isLocalUser;
	}
	
	@Override
	public boolean equals(Object o)
	{
		if (o instanceof LikewiseUser)
		{
			LikewiseUser otherUser = (LikewiseUser)o;
			
			return _name.equals(otherUser.getName());
		}
		
		return false;
	}
}
