package com.likewise.auth;

import com.likewise.interop.AuthenticationAdapter;

import junit.framework.TestCase;

public class LwTestUserLookup extends TestCase
{
	private String _userName = "ELEMENTS\\dduck";
	
	public void setUp()
	{
		// _userName = "BUILIN\\Administrator";
	}
	
	public void testUserLookup()
	{
		try
		{
			LWUser user = AuthenticationAdapter.findUserByName(_userName);
			
			System.out.println("User [SID: " +
								user.getSecurityIdentifier() + 
								"]");
		}
		catch (Exception e)
		{
			System.out.println("Failed to lookup user [" + _userName + "]. " +
								e.getMessage());
		}
	}
	
	public void testGroupsForUserLookup()
	{
		try
		{
			LWGroup[] groups = AuthenticationAdapter.getGroupsForUser(_userName);
			
			for (LWGroup group : groups)
			{
				System.out.println("Group [SID: " +
									group.getName() + 
									"]");
			}
		}
		catch (Exception e)
		{
			System.out.println("Failed to lookup groups for user [" +
								_userName + 
								"]. " +
								e.getMessage());
		}
	}
}
