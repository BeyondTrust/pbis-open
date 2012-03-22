/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LikewiseGroup.java
 *
 * Abstract:
 *
 *        Likewise Java Authentication and Authorization Service
 *
 *        Group
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.auth;

import java.security.Principal;
import java.security.acl.Group;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.lang.IllegalArgumentException;

/**
 * Likewise Group
 * <p>
 * This is a group based on attributes assigned by the Likewise Authentication
 * stack. This group may contain identities from an Active Directory Domain and
 * from the Local Security Authority and any other authentication providers
 * configured in the Likewise Authentication Stack.
 * </p>
 */
public final class LikewiseGroup implements Group
{
	private int    _id;
	private String _name;
	private String _sid;
	private HashMap<String, Principal> _members;
	
	/**
	 * Builds a Likewise Group using just the group name.
	 * 
	 * @param name UNIX Alias of the group.
	 */
	public LikewiseGroup(String name)
	{
		_name = name + "";
		_members = new HashMap<String, Principal>();
	}
	
	/**
	 * Builds a Likewise Group using the group name, security identifier and
	 * the mapped integer identifier.
	 * 
	 * @param name UNIX Alias of the group.
	 * @param sid  Security identifier assigned to the group.
	 * @param id   Integer identifier assigned to the group. This would be the
	 *             group id (gid) on UNIX systems.
	 */
	public LikewiseGroup(String name, String sid, int id)
	{
		_id   = id;
		_name = name;
		_sid  = sid;
	}
	
	/**
	 * Retrieves the group name.
	 * 
	 * @return UNIX Alias given to the group
	 */
	public String getName()
	{
		return _name;
	}
	
	/**
	 * Retrieves the security identifier assigned to the group.
	 * 
	 * @return Security identifier assigned to the group.
	 */
	public String getSID()
	{
		return _sid;
	}
	
	/**
	 * Retrieves the (integer) group id. This would be the gid on UNIX systems.
	 * 
	 * @return integer group identifier
	 */
	public int getId()
	{
		return _id;
	}

	/**
	 * Adds a user or group principal to the current group.
	 * 
	 * @param user User principal that shall be a member of the current group.
	 * @return true if the user was not in the group and was successfully added.
	 */
	public boolean addMember(Principal user)
	{
		boolean bAdded = false;
		
		if (user == null)
		{
			throw new IllegalArgumentException("Cannot add null member to group");
		}
		
		if (_members.containsKey(user.getName()))
		{
			_members.put(user.getName(), user);
			
			bAdded = true;
		}
		
		return bAdded;
	}

	/**
	 * Checks if the specified principal (user or group) is a member of the
	 * current group.
	 * 
	 * @param member Principal that must be checked for membership in the
	 *               current group.
	 * @return true if the principal was found to be a member of the current 
	 *              group.
	 */
	public boolean isMember(Principal member)
	{
		if (member == null)
		{
			throw new IllegalArgumentException("Cannot lookup null member in group");
		}
		
		return _members.containsKey(member.getName());
	}

	/**
	 * Retrieves an enumeration of all the members of the current group.
	 * 
	 * @return An enumeration of principals belonging to the current group.
	 */
	public Enumeration<? extends Principal> members()
	{
		return Collections.enumeration(_members.values());
	}

	/**
	 * Removes the specified principal from the current group.
	 * 
	 * @param user Principal that must be removed from the current group.
	 * @return true if the user was found and removed from the current group.
	 */
	public boolean removeMember(Principal user)
	{
		boolean removed = false;
		
		if ((user != null) && (null != _members.remove(user.getName())))
		{
			removed = true;
		}
		
		return removed;
	}
	
	@Override
	public boolean equals(Object o)
	{
		if (o instanceof LikewiseGroup)
		{
			LikewiseGroup otherGroup = (LikewiseGroup)o;
			
			return _name.equals(otherGroup._name);
		}
		
		return false;
	}

}
