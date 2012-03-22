/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LikewiseHttpServletRequestWrapper.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        HTTP Servlet Request Wrapper
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.auth.filter.spnego;

import java.security.Principal;
import java.util.List;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletRequestWrapper;

/**
 * Wraps the original HTTP servlet request in such a way that the authenticated
 * remote user principal and associated roles can be provisioned.
 * <p>
 * This information is useful to subsequent chained servlet filter requests to
 * authorize the current authenticated remote user principal.
 * </p>
 */
public final class LikewiseHttpServletRequestWrapper extends
		HttpServletRequestWrapper
{
	private Principal    _remoteUser;
	private List<String> _roles;
	
	/**
	 * Builds a Likewise HTTP servlet request wrapper using the original request
	 */
	public
	LikewiseHttpServletRequestWrapper(
		HttpServletRequest request
		)
	{
		super(request);
	}
	
	/**
	 * Builds a Likewise HTTP servlet request wrapper using the original request
	 * the remote authenticated user principal and the list of roles the remote
	 * principal is a member of.
	 */
	public
	LikewiseHttpServletRequestWrapper(
		HttpServletRequest request,
		Principal          remoteUser,
		List<String>       roles
		)
	{
		super(request);
		
		_remoteUser = remoteUser;
		_roles      = roles;
	}
	
	/**
	 * Retrieves the user principal object associated with the authenticated
	 * remote user.
	 * 
	 * @return Remote user principal
	 */
	@Override
	public
	Principal
	getUserPrincipal()
	{
		if (_remoteUser != null)
		{
			return _remoteUser;
		}
		else
		{
			return ((HttpServletRequest)getRequest()).getUserPrincipal();
		}
	}
	
	/**
	 * Retrieves the name of the remote user who has been authenticated successfully.
	 * 
	 * @return SAM Account name of the authenticated user principal.
	 */
	@Override
	public
	String
	getRemoteUser()
	{
		if (_remoteUser != null)
		{
			return _remoteUser.getName(); 
		}
		else
		{
			return ((HttpServletRequest)getRequest()).getRemoteUser();
		}
	}
	
	/**
	 * Checks if the remote user is a member of the specified role.
	 * 
	 * @param  role Role the remote user might be a member of.
	 * @return true if the remote user is a member of the specified role.
	 */
	@Override
	public
	boolean
	isUserInRole(
		String role
		)
	{
		if (_roles != null)
		{
			return _roles.contains(role);
		}
		else
		{
			return ((HttpServletRequest)getRequest()).isUserInRole(role);
		}
	}
}
