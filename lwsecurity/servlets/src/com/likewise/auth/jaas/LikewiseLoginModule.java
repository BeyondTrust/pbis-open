/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LWLoginModule.java
 *
 * Abstract:
 *
 *        Likewise Java Authentication and Authorization Service
 *
 *        Login Module
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.auth.jaas;

import java.io.IOException;
import java.security.Principal;
import java.util.ArrayList;
import java.util.Map;
import java.util.Set;
import java.util.logging.LogManager;
import java.util.logging.Logger;

import javax.security.auth.Destroyable;
import javax.security.auth.Subject;
import javax.security.auth.callback.Callback;
import javax.security.auth.callback.CallbackHandler;
import javax.security.auth.callback.NameCallback;
import javax.security.auth.callback.PasswordCallback;
import javax.security.auth.callback.UnsupportedCallbackException;
import javax.security.auth.login.LoginException;
import javax.security.auth.spi.LoginModule;

import com.likewise.auth.LikewiseGroup;
import com.likewise.auth.LikewiseUser;
import com.likewise.interop.AuthenticationAdapter;

/**
 * Java Authentication and Authorization Service (JAAS) Module supported by
 * the Likewise Identity Services (LWIS) authentication software stack.
 *
 */
public final class LikewiseLoginModule implements LoginModule
{
	private static String NAME_KEY     = "javax.security.auth.login.name";
	private static String PASSWORD_KEY = "javax.security.auth.login.password";
	
	private static Logger   _logger          = null;
	
	private Subject         _subject         = null;
	private CallbackHandler _callbackHandler = null;
	private Map<String, ?>  _sharedState     = null;
	private Map<String, ?>  _options         = null;
	
	private NameCallback     _nameCB       = null;
	private PasswordCallback _passwdCB     = null;
	private Callback[]       _callbacks    = null;
	private boolean          _debug        = false;
	private boolean          _tryFirstPass = false;
	private boolean          _useFirstPass = false;
	private boolean          _loginResult   = false;
	private boolean          _commitResult = false;
	
	private LikewiseUser           _user  = null;
	private ArrayList<LikewiseGroup> _roles = null;
	
	static
	{
		_logger = LogManager.getLogManager().getLogger(
												LoginModule.class.getName());
	}
	
	/**
	 * Initializes the Likewise Login Module.
	 * 
	 * @param subject         Authentication context
	 * @param callbackHandler Handler to call in order to retrieve credentials
	 * @param sharedState     Map of state shared with chained login modules
	 * @param options         Map of configuration options
	 */
	public
	void
	initialize(
		Subject         subject, 
		CallbackHandler callbackHandler,
		Map<String, ?>  sharedState,
		Map<String, ?>  options
		)
	{
		_roles = new ArrayList<LikewiseGroup>();
		
		_subject         = subject;
		_callbackHandler = callbackHandler;
		_sharedState     = sharedState;
		_options         = options;
		
		_nameCB   = new NameCallback("Login");
		_passwdCB = new PasswordCallback("Password", false /* echo */);
		
		_debug = getOption("debug");	
		_tryFirstPass = getOption("try_first_pass");
		_useFirstPass = getOption("use_first_pass");
		
		if (_debug)
		{
			_logger.exiting(LoginModule.class.getName() , "initialize");
		}
	}

	/**
	 * Performs login based on the information in the current authentication
	 * context.
	 * 
	 * @return true if the current identity could login successfully.
	 */
	public
	boolean
	login() throws LoginException
	{
		if (_debug)
		{
			_logger.entering(LoginModule.class.getName() , "login");
		}
		
        try
		{
        	String name = null;
        	String password = null;

        	registerCallbacks();
        	
        	if (_sharedState.containsKey(NAME_KEY))
        	{
        		name = String.valueOf(_sharedState.get(NAME_KEY));
        	}
        	
        	if (name == null || name.length() == 0)
        	{
                 name = _nameCB.getName();
        	}
    		
    		if (_tryFirstPass)
    		{
    			if (_sharedState.containsKey(PASSWORD_KEY))
    			{
    				password = String.valueOf(_sharedState.get(PASSWORD_KEY));
    			}
    			else
    			{
        			password = String.valueOf(_passwdCB.getPassword());
        		}
    		}
    		else if (_useFirstPass)
    		{
    			if (_sharedState.containsKey(PASSWORD_KEY))
    			{
    				password = String.valueOf(_sharedState.get(PASSWORD_KEY));
    			}
    			else
    			{
    				password = String.valueOf(_passwdCB.getPassword());
    			}
    		}
    		else
    		{
    			password = new String(_passwdCB.getPassword());
    		}
    		
    		if (_debug)
    		{
    			_logger.info("Looking up user [" + name + "]");
    		}
    		
    		_user = AuthenticationAdapter.findUserByName(name);
    		
    		if (_debug)
    		{
    			_logger.info("Authenticating user [" + name + "]");
    		}
    		
    		AuthenticationAdapter.authenticate(name, password);
    		
    		if (_debug)
    		{
    			_logger.info("Querying groups for user [" + name + "]");
    		}
    		
    		LikewiseGroup groups[] = AuthenticationAdapter.getGroupsForUser(name);
    		for (LikewiseGroup group : groups)
    		{
        		if (_debug)
        		{
        			_logger.info(	"Found group [" + 
        							group.getName() + 
        							"] for user [" + 
        							name + 
        							"]");
        		}
        		
    			_roles.add(group);
    		}
    		
    		_loginResult = true;
		}
		catch(Exception e)
		{
			throw new LoginException(e.getMessage());
		}
		
		if (_debug)
		{
			_logger.exiting(LoginModule.class.getName() , "login");
		}
    	
		return _loginResult;
	}
	
	/**
	 * Aborts the current login process.
	 * 
	 * @return true according to the following matrix.
	 * <p>
	 * <table border="1">
	 * <tr>
	 * <th>Scenario</th>
	 * <th>Commit success</th>
	 * <th>Abort success</th>
	 * <th>Result</th>
	 * </tr>
	 * <tr>
	 * <td>Login succeeded</td>
	 * <td>true</td>
	 * <td>true</td>
	 * <td>true</td>
	 * </tr>
	 * <tr>
	 * <td>Login succeeded</td>
	 * <td>true</td>
	 * <td>false</td>
	 * <td>exception</td>
	 * </tr>
	 * <tr>
	 * <td>Login succeeded</td>
	 * <td>false</td>
	 * <td>true</td>
	 * <td>true</td>
	 * </tr>
	 * <tr>
	 * <td>Login succeeded</td>
	 * <td>false</td>
	 * <td>false</td>
	 * <td>exception</td>
	 * </tr>
	 * <tr>
	 * <td>Login failed</td>
	 * <td>true</td>
	 * <td>true</td>
	 * <td>false</td>
	 * </tr>
	 * <tr>
	 * <td>Login failed</td>
	 * <td>true</td>
	 * <td>false</td>
	 * <td>false</td>
	 * </tr>
	 * <tr>
	 * <td>Login failed</td>
	 * <td>false</td>
	 * <td>true</td>
	 * <td>false</td>
	 * </tr>
	 * <tr>
	 * <td>Login failed</td>
	 * <td>false</td>
	 * <td>false</td>
	 * <td>false</td>
	 * </tr>
	 * </table>
	 * </p>
	 */
	public
	boolean
	abort() throws LoginException 
	{
		// This is called when the overall authentication fails
		// case : login succeeded
		// 		commit success/abort success => return true
		// 		commit success/abort failed  => throw exception
		// 		commit failed/abort success  => return true
		// 		commit failed/abort failed   => throw exception
		// case : login failed
		// 		commit success/abort success => return false
		// 		commit success/abort failed  => return false
		// 		commit failed/abort success  => return false
		// 		commit failed/abort failed   => return false
		boolean bAborted = false;
		
		if (_debug)
		{
			_logger.entering(LoginModule.class.getName() , "abort");
		}
		
		try
		{
			Set<Principal> principals = _subject.getPrincipals();
			
			if (_commitResult)
			{
				if (_user != null)
				{
					principals.remove(_user);
				}
				if (_roles != null)
				{
					principals.remove(_roles);
				}
			}
			
			_user = null;
			_roles = null;
			
			if (_loginResult)
			{
				bAborted = true;
			}
		}
		catch(Exception e)
		{
			if (_loginResult)
			{
				throw new LoginException(e.getMessage());
			}
		}
		finally
		{
			if (_passwdCB != null)
			{
				_passwdCB.clearPassword();
			}
		}
		
		if (_debug)
		{
			_logger.exiting(LoginModule.class.getName() , "abort");
		}
		
		return bAborted;
	}
	
	/**
	 * Commits the result of the current login process to the specified subject.
	 * 
	 * @return true according to the following matrix.
	 * <p>
	 * <table border="1">
	 * <tr>
	 * <th>Login success</th>
	 * <th>Commit success</th>
	 * <th>Result</th>
	 * </tr>
	 * <tr>
	 * <td>true</td>
	 * <td>true</td>
	 * <td>true</td>
	 * </tr>
	 * <tr>
	 * <td>true</td>
	 * <td>false</td>
	 * <td>exception</td>
	 * </tr>
	 * <tr>
	 * <td>false</td>
	 * <td>true</td>
	 * <td>false</td>
	 * </tr>
	 * <tr>
	 * <td>false</td>
	 * <td>false</td>
	 * <td>false</td>
	 * </tr>
	 * </table>
	 * </p>
	 */
	public
	boolean
	commit() throws LoginException
	{
		// This is called to commit the authentication process when the
		// overall authentication succeeds.
		// case login success
		//		commit success => return true
		//      commit failed  => throw exception
		// case login failed
		//      commit success => return false
		//      commit failed  => return false
		boolean bUserAdded = false;
		
		if (_debug)
		{
			_logger.entering(LoginModule.class.getName() , "commit");
		}
		
		if (_subject.isReadOnly())
		{
			throw new LoginException("Subject cannot be read-only");
		}
		
		try
		{		
			if (_loginResult)
			{
				Set<Principal> principals = _subject.getPrincipals();
				
				principals.add(_user);
				
				bUserAdded = true;
				
				principals.addAll(_roles);
				
				_commitResult = true; // login && commit
			}
		}
		catch(Exception e)
		{
			if (bUserAdded)
			{
				_subject.getPrincipals().remove(_user);
			}
			if (_loginResult) // login && !commit
			{
				throw new LoginException(e.getMessage());
			}
		}
		
		if (_debug)
		{
			_logger.exiting(LoginModule.class.getName() , "commit");
		}
		
		return _commitResult;
	}

	/**
	 * logs the current user out of the context.
	 * 
	 * <p>
	 * Cleans up any state preserved as part of the login/commit stages.
	 * </p>
	 * 
	 * @return true if the user could be successfully logged out.
	 */
	public
	boolean
	logout() throws LoginException
	{
		boolean bLoggedOut = false;
		
		if (_debug)
		{
			_logger.entering(LoginModule.class.getName() , "logout");
		}
		
		if (_subject.isReadOnly())
		{
			if (_user != null && !(_user instanceof Destroyable))
			{
				throw new LoginException("Cannot destroy user principal");
			}
			else
			{
				_user = null;
			}
			
			if (_roles != null && !(_roles instanceof Destroyable))
			{
				throw new LoginException("Cannot destroy role principal");
			}
			else
			{
				_roles = null;
			}
			
			bLoggedOut = true;
		}
		else
		{
			try
			{
				Set<Principal> principals = _subject.getPrincipals();
				
				if (_user != null)
				{
					principals.remove(_user);
					_user = null;
				}
				if (_roles != null)
				{
					principals.removeAll(_roles);
					_roles = null;
				}
				
				bLoggedOut = true;
			}
			catch (Exception e)
			{
				throw new LoginException(e.getMessage());
			}
		}
		
		if (_passwdCB != null)
		{
			_passwdCB.clearPassword();
		}
		
		if (_debug)
		{
			_logger.exiting(LoginModule.class.getName() , "logout");
		}
		
		return bLoggedOut;
	}
	
	/**
	 * Registers call-backs to retrieve the name and password information
	 * 
	 * @throws IOException
	 * @throws UnsupportedCallbackException
	 */
	private
	void
	registerCallbacks() throws IOException, UnsupportedCallbackException
	{
		if (_callbacks == null)
		{
			Callback[] callbacks = new Callback[] { _nameCB, _passwdCB };
			
			_callbackHandler.handle(callbacks);
			
			_callbacks = callbacks;
		}
	}
	
	/**
	 * Lookup the specified option by name in the options configured for the
	 * login module.
	 * 
	 * @param name Name of the option. Example: "debug"
	 * @return The boolean value of the option if it was found, else false.
	 */
	private
	boolean
	getOption(
	    String name
	    )
	{
		boolean bResult = false;
		
		if (_options.containsKey(name))
		{
			bResult = Boolean.valueOf(_options.get(name).toString());
		}
		
		return bResult;
	}
}
