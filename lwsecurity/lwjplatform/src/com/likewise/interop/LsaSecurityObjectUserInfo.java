/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LsaSecurityObjectUserInfo
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        Security Object User Information(Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

/**
 * Java Native Access abstraction of a LSA_SECURITY_OBJECT_USER_INFO
 * native structure defined in the Likewise authentication system.
 */
public class LsaSecurityObjectUserInfo extends Structure
{
	/**
	 * Security identifier of the primary group this user is a member of.
	 */
    public String  pszPrimaryGroupSid;
    /**
     * Principal name of this user object, in the format 
     * sam-account-name@REALM
     */
    public String  pszUPN;
    /**
     * Alias associated with this user object.
     */
    public String  pszAliasName;
    /**
     * Time (NT TIME) when the user's password was last set.
     */
    public long    qwPwdLastSet;
    /**
     * Maximum amount of time (NT TIME) after which the user's password expires.
     */
    public long    qwMaxPwdAge;
    /**
     * Time (NT TIME) at which the user's password will expire.
     */
    public long    qwPwdExpires;
    /**
     * Time (NT TIME) at which the user's account will expire.
     */
    public long    qwAccountExpires;

    /**
     * Whether the User Principal Name was generated (as opposed to being set
     * in Active Directory on the user's account).
     */
    public byte    bIsGeneratedUPN;
    /**
     * Whether all attributes of the user account are set (as opposed to
     * some fields being set to defaults).
     */
    public byte    bIsAccountInfoKnown;
    
    /**
     * Whether the user's password has expired.
     */
    public byte    bPasswordExpired;
    /**
     * Whether the user's password is set to never expire.
     */
    public byte    bPasswordNeverExpires;
    /**
     * Whether the user must be prompted to interactively change the password.
     */
    public byte    bPromptPasswordChange;
    /**
     * Whether the user can change his/her password.
     */
    public byte    bUserCanChangePassword;
    /**
     * Whether the user's account has been disabled.
     */
    public byte    bAccountDisabled;
    /**
     * Whether the user's account has expired.
     */
    public byte    bAccountExpired;
    /**
     * Whether the user's account has been locked.
     */
    public byte    bAccountLocked;

    /**
     * Length of the LM_HASH value of the user account.
     */
    public int     dwLmHashLen;
    /**
     * Value of the LM_HASH on the user account.
     */
    public Pointer pLmHash;
    /**
     * Length of the NT_HASH value of the user account.
     */
    public int     dwNtHashLen;
    /**
     * Value of the NT_HASH of the user account.
     */
    public Pointer pNtHash;

    /**
     * Integer user identifier associated with the user account.
     */
    public int     uid;
    /**
     * Integer group identifier associated with the primary group of the user
     * account.
     */
    public int     gid;
    /**
     * UNIX user name associated with the user account.
     * This is typically of the NT4 style.
     * Example: ABC\joe
     */
    public String  pszUnixName;
    /**
     * Plain text password associated with the user account.
     */
    public String  pszPasswd;
    /**
     * General Electric Comprehensive Operating Supervisor.
     * This field typically holds extra information about the user account
     * such as phone numbers and extensions.
     */
    public String  pszGecos;
    /**
     * Login shell
     */
    public String  pszShell;
    /**
     * Path to home directory.
     */
    public String  pszHomedir;
    
    /**
     * Builds a default user info object.
     */
    public LsaSecurityObjectUserInfo()
    {
    }
    
    /**
     * Builds a user info object based on native memory pointing to a 
     * PLSA_SECURITY_OBJECT_USER_INFO object.
     * 
     * @param p Native memory pointing to a PLSA_SECURITY_OBJECT_USER_INFO 
     *          object.
     */
    public LsaSecurityObjectUserInfo(Pointer p)
    {
    	useMemory(p);
    	read();
    }
}
