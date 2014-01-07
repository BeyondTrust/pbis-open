package com.likewise.auth;

import com.likewise.auth.LWGroup;
import com.likewise.auth.jna.AuthenticationAdapter;

import junit.framework.TestCase;

public class AuthenticationAdapterTest extends TestCase 
{
	public void setUp()
	{
		
	}
	
	public void tearDown()
	{
		
	}
	
	public void testAuthenticate()
	{
		String username = "ELEMENTS\\dduck";
		String password = "likewise123$";
		
		try
		{
			AuthenticationAdapter.authenticate(username, password);
		}
		catch(Exception e)
		{
			System.out.println(e.getMessage());
		}
	}
	
	public void testEnumGroupsForUser()
	{
		String username = "ELEMENTS\\dduck";
		
		try
		{
			LWGroup groups[] = AuthenticationAdapter.getGroupsForUser(username);
			
			System.out.println("Number of groups : " + groups.length);
			
			for (LWGroup group : groups)
			{
				System.out.println(
						String.format("Group [Id : %d; Name : %s; SID : %s]\n",
								group.getId(),
								group.getName(),
								group.getSID()));
			}
		}
		catch(Exception e)
		{
			System.out.println(e.getMessage());
		}
	}
}
