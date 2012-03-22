/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        AuthenticationAdapter.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        Authentication Adapter (Java Native Access)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.interop;

import java.util.List;
import java.util.ArrayList;

import com.likewise.Win32Exception;
import com.likewise.interop.LikewiseNativeAdapter;

import com.likewise.auth.LikewiseGroup;
import com.likewise.auth.LikewiseUser;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
// import com.sun.jna.WString;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;

/**
 * Java Native Access Wrapper for the Likewise library (liblsaclient.so)
 * <p>
 * This class provides access into the ANSI C API of the LSASS client library.
 * </p>
 */
public class AuthenticationAdapter extends LikewiseNativeAdapter
{
	/**
	 * Java Native Access Wrapper Library for the 
	 * Likewise library (liblsaclient.so)
	 */
	public interface LWLsaLibrary extends Library
    {
		LWLsaLibrary INSTANCE = (LWLsaLibrary) Native.loadLibrary(
			                                            "lsaclient",
			                                            LWLsaLibrary.class);

		int
		LsaOpenServer(
			PointerByReference ppLsaHandle
			);
		
		int
		LsaFindObjects(
		    Pointer            pLsaHandle,
		    String             pszTargetProvider,
		    int                findFlags,
		    int                objectType,
		    int                queryType,
		    int                dwCount,
		    Pointer            queryList,
		    PointerByReference pppObjects
		    );
		
		int
		LsaQueryMemberOf(
			Pointer            pLsaHandle,
			String             pszTargetProvider,
			int                findFlags,
		    int                dwSidCount,
		    Pointer            ppszSidList,
		    IntByReference     pdwGroupSidCount,
		    PointerByReference pppszGroupSids
		    );

		int
		LsaAuthenticateUser(
			Pointer            pLsaHandle,
			String             pszUsername,
			String             pszPassword,
			PointerByReference ppMessage
			);
		
		int
		LsaGetGroupsForUserByName(
			Pointer            pLsaHandle,
		    String             pszUserName,
		    int                findFlags,
		    int                infoLevel,
		    IntByReference     pdwGroupsFound,
		    PointerByReference ppGroups
		    );
		
		void
		LsaFreeUserInfo(
		    int     infoLevel,
		    Pointer pUserInfo
		    );
		
		void
		LsaFreeSidList(
			int     dwNumSids,
			Pointer pSidList
			);
		
		void
		LsaFreeSecurityObjectList(
			int     dwNumObjects,
			Pointer pObjectList
			);

		void
		LsaCloseServer(
		    Pointer pLsaHandle
		    );
    }
	
	/**
	 * Retrieves the Likewise user principal associated with the 
	 * specified user name.
	 * 
	 * @param userName Name of the user to look up the principal object for.
	 * @return User principal object associated with the specified name.
	 */
	public
	static
	LikewiseUser
	findUserByName(String userName)
	{
		PointerByReference ppLsaServer = new PointerByReference();
		
		try
		{
			int winErr = 0;
			
			winErr = LWLsaLibrary.INSTANCE.LsaOpenServer(ppLsaServer);
			if (winErr != 0)
			{
				throw new Win32Exception(winErr);
			}

			return findUserByName(ppLsaServer.getValue(), userName);
		}
		finally
		{
			if (ppLsaServer.getValue() != null)
			{
				LWLsaLibrary.INSTANCE.LsaCloseServer(ppLsaServer.getValue());
				ppLsaServer.setValue(null);
			}
		}
	}
	
	/**
	 * Retrieves the Likewise group principal associated with the 
	 * specified group name.
	 * 
	 * @param groupName Name of the group to look up the principal object for.
	 * @return Group principal object associated with the specified name.
	 */
	public
	static
	LikewiseGroup
	findGroupByName(String groupName)
	{
		PointerByReference ppLsaServer = new PointerByReference();
		
		try
		{
			int winErr = 0;
			
			winErr = LWLsaLibrary.INSTANCE.LsaOpenServer(ppLsaServer);
			if (winErr != 0)
			{
				throw new Win32Exception(winErr);
			}

			return findGroupByName(ppLsaServer.getValue(), groupName);
		}
		finally
		{
			if (ppLsaServer.getValue() != null)
			{
				LWLsaLibrary.INSTANCE.LsaCloseServer(ppLsaServer.getValue());
				ppLsaServer.setValue(null);
			}
		}
	}
	
	/**
	 * Looks up the principal object associated with the specified security
	 * identifier.
	 * 
	 * @param sid Security identifier string.
	 * @return Likewise principal object associated with the given security
	 * identifier.
	 */
	public
	static
	Object
	findObjectBySID(String sid)
	{
		PointerByReference ppLsaServer = new PointerByReference();
		
		try
		{
			int winErr = 0;
			
			winErr = LWLsaLibrary.INSTANCE.LsaOpenServer(ppLsaServer);
			if (winErr != 0)
			{
				throw new Win32Exception(winErr);
			}

			return findObjectBySID(ppLsaServer.getValue(), sid);
		}
		finally
		{
			if (ppLsaServer.getValue() != null)
			{
				LWLsaLibrary.INSTANCE.LsaCloseServer(ppLsaServer.getValue());
				ppLsaServer.setValue(null);
			}
		}
	}

	/**
	 * Authenticates the given user name and password.
	 * 
	 * @param username Name of the user to authenticate.
	 * @param password Password of the user account specified.
	 * @throws Win32Exception
	 */
	public
	static
	void
	authenticate(String username, String password)
	{
		int winErr = 0;
		PointerByReference ppLsaServer = new PointerByReference();
		PointerByReference ppMessage = new PointerByReference();
		
		try
		{
			winErr = LWLsaLibrary.INSTANCE.LsaOpenServer(ppLsaServer);
			if (winErr != 0)
			{
				throw new Win32Exception(winErr);
			}
			
			winErr = LWLsaLibrary.INSTANCE.LsaAuthenticateUser(
												ppLsaServer.getValue(),
												username,
												password,
												ppMessage);
			if (winErr != 0)
			{
				throw new Win32Exception(winErr);
			}
		}
		finally
		{
			if (ppLsaServer.getValue() != null)
			{
				LWLsaLibrary.INSTANCE.LsaCloseServer(ppLsaServer.getValue());
				ppLsaServer.setValue(null);
			}
			
			if (ppMessage.getValue() != null)
			{
				AdvapiAdapter.LWAdvapiLibrary.INSTANCE.LwFreeMemory(
														ppMessage.getValue());
				ppMessage.setValue(null);
			}
		}
	}
	
	/**
	 * Retries an array of group principal objects the specified user is a
	 * member of.
	 * 
	 * @param username Name of the user whose memberships must be found.
	 * @return Array of groups the given user is a member of.
	 */
	public
	static
	LikewiseGroup[]
	getGroupsForUser(String username)
	{
		PointerByReference ppLsaServer       = new PointerByReference();
		IntByReference     pGroupSidCount = new IntByReference();
		PointerByReference pppszGroupSids = new PointerByReference();
		PointerByReference pppObjects = new PointerByReference();
		
		try
		{
			int          winErr    = 0;
			String       pszTargetProvider = null;
			LsaFindFlags findFlags = new LsaFindFlags();
			List<String> sidList   = new ArrayList<String>();
			List<LikewiseGroup> groups   = new ArrayList<LikewiseGroup>();
			
			winErr = LWLsaLibrary.INSTANCE.LsaOpenServer(ppLsaServer);
			if (winErr != 0)
			{
				throw new Win32Exception(winErr);
			}
			
			LikewiseUser user = findUserByName(ppLsaServer.getValue(), username);
			
			findFlags.setFindInNSS();
			
			sidList.add(user.getSecurityIdentifier());
			
			LsaQueryStringList sidNativeList = new LsaQueryStringList(sidList);
			
			winErr = LWLsaLibrary.INSTANCE.LsaQueryMemberOf(
												ppLsaServer.getValue(), 
												pszTargetProvider,
												findFlags.getFlags(),
												sidList.size(),
												sidNativeList.getPointer(),
												pGroupSidCount,
												pppszGroupSids);
			if (winErr != 0)
			{
				throw new Win32Exception(winErr);
			}
			
			if (pGroupSidCount.getValue() > 0)
			{
				winErr = LWLsaLibrary.INSTANCE.LsaFindObjects(
								ppLsaServer.getValue(),
								pszTargetProvider,
								findFlags.getFlags(),
								LsaObjectType.LSA_OBJECT_TYPE_GROUP.getCode(),
								LsaQueryType.LSA_QUERY_TYPE_BY_SID.getCode(),
								pGroupSidCount.getValue(),
								pppszGroupSids.getValue(),
								pppObjects);
				if (winErr != 0)
				{
					throw new Win32Exception(winErr);
				}
				
				for (Pointer pGroup : pppObjects.getValue().getPointerArray(
													0, 
													pGroupSidCount.getValue()))
				{
					LikewiseNativeObject group = new LikewiseNativeObject(pGroup);
					
					LsaObjectType objectType = 
									LsaObjectType.get(group.securityObjectType);
					
					assert(objectType.equals(LsaObjectType.LSA_OBJECT_TYPE_GROUP));
		
					groups.add(new LikewiseGroup(
							group.securityObjectInfo.groupInfo.pszUnixName,
							group.pszObjectSid,
							group.securityObjectInfo.groupInfo.gid));
				}
			}
			
			return groups.toArray(new LikewiseGroup[groups.size()]);
		}
		finally
		{
			if (ppLsaServer.getValue() != null)
			{
				LWLsaLibrary.INSTANCE.LsaCloseServer(ppLsaServer.getValue());
				ppLsaServer.setValue(null);
			}
			
			if (pppObjects.getValue() != null)
			{
				LWLsaLibrary.INSTANCE.LsaFreeSecurityObjectList(
										pGroupSidCount.getValue(),
										pppObjects.getValue());
				pppObjects.setValue(null);
			}
			
			if (pppszGroupSids.getValue() != null)
			{
				LWLsaLibrary.INSTANCE.LsaFreeSidList(
										pGroupSidCount.getValue(),
										pppszGroupSids.getValue());
				pppszGroupSids.setValue(null);
			}
		}
	}
	
	/**
	 * Finds a user principal object given the name of the user.
	 * 
	 * @param pLsaHandle Pointer (native) to the LSASS context.
	 * @param userName   Name of the user that must be looked up.
	 * @return User principal object associated with the given name.
	 */
	private
	static
	LikewiseUser
	findUserByName(Pointer pLsaHandle, String userName)
	{
		String       targetProviderName = null;
		LsaFindFlags findFlags  = new LsaFindFlags();
		List<String> userList   = new ArrayList<String>();
		List<LikewiseUser> usersFound = new ArrayList<LikewiseUser>();
		PointerByReference pppObjects = new PointerByReference();
		
		try
		{
			int winErr = 0;
			
			userList.add(userName);
			
			LsaQueryList queryList = 
				new LsaQueryList(userList.toArray(new String[userList.size()]));
			
			winErr = LWLsaLibrary.INSTANCE.LsaFindObjects(
								pLsaHandle,
								targetProviderName,
								findFlags.getFlags(),
								LsaObjectType.LSA_OBJECT_TYPE_USER.getCode(),
								LsaQueryType.LSA_QUERY_TYPE_BY_NAME.getCode(),
								userList.size(),
								queryList.getPointer(),
								pppObjects);
			if (winErr != 0)
			{
				throw new Win32Exception(winErr);
			}
			
			Pointer userObjectArray[] =
						pppObjects.getValue().getPointerArray(
												0,
												userList.size());
			
			for (Pointer pUserObject : userObjectArray)
			{
				LikewiseNativeObject user = new LikewiseNativeObject(pUserObject);
				
				LsaObjectType objectType = 
								LsaObjectType.get(user.securityObjectType);
				
				assert(objectType.equals(LsaObjectType.LSA_OBJECT_TYPE_USER));
	
				usersFound.add(new LikewiseUser(
						user.pszSamAccountName,
						user.securityObjectInfo.userInfo.uid,
						user.securityObjectInfo.userInfo.gid,
						user.pszObjectSid,
						user.securityObjectInfo.userInfo.pszHomedir,
						user.securityObjectInfo.userInfo.pszPasswd,
						user.securityObjectInfo.userInfo.pszGecos,
						user.securityObjectInfo.userInfo.pszShell,
						user.pszDN,
						user.securityObjectInfo.userInfo.pszUPN,
						Byte.valueOf(user.isLocal).intValue() == 1));
			}
			
			return usersFound.get(0);
		}
		finally
		{
			if (pppObjects.getValue() != null)
			{
				LWLsaLibrary.INSTANCE.LsaFreeSecurityObjectList(
										userList.size(),
										pppObjects.getValue());
				pppObjects.setValue(null);
			}
		}
	}
	
	/**
	 * Finds a group principal object given the name of the user.
	 * 
	 * @param  pLsaHandle  Pointer (native) to the LSASS context.
	 * @param  groupName   Name of the group that must be looked up.
	 * @return Group principal object associated with the given name.
	 */
	private
	static
	LikewiseGroup
	findGroupByName(Pointer pLsaHandle, String groupName)
	{
		String       targetProviderName = null;
		LsaFindFlags findFlags  = new LsaFindFlags();
		List<String> groupList   = new ArrayList<String>();
		List<LikewiseGroup> groupsFound = new ArrayList<LikewiseGroup>();
		PointerByReference pppObjects = new PointerByReference();
		
		try
		{
			int winErr = 0;
			
			groupList.add(groupName);
			
			LsaQueryList queryList = 
				new LsaQueryList(groupList.toArray(new String[groupList.size()]));
			
			winErr = LWLsaLibrary.INSTANCE.LsaFindObjects(
								pLsaHandle,
								targetProviderName,
								findFlags.getFlags(),
								LsaObjectType.LSA_OBJECT_TYPE_GROUP.getCode(),
								LsaQueryType.LSA_QUERY_TYPE_BY_NAME.getCode(),
								groupList.size(),
								queryList.getPointer(),
								pppObjects);
			if (winErr != 0)
			{
				throw new Win32Exception(winErr);
			}
			
			for (Pointer pGroupObject : pppObjects.getValue().getPointerArray(
															0,
															groupList.size()))
			{
				LikewiseNativeObject group = new LikewiseNativeObject(pGroupObject);
				
				LsaObjectType objectType = 
								LsaObjectType.get(group.securityObjectType);
				
				assert(objectType.equals(LsaObjectType.LSA_OBJECT_TYPE_GROUP));
	
				groupsFound.add(new LikewiseGroup(
						group.securityObjectInfo.groupInfo.pszUnixName,
						group.pszObjectSid,
						group.securityObjectInfo.groupInfo.gid));
			}
			
			return groupsFound.get(0);
		}
		finally
		{
			if (pppObjects.getValue() != null)
			{
				LWLsaLibrary.INSTANCE.LsaFreeSecurityObjectList(
										groupList.size(),
										pppObjects.getValue());
				pppObjects.setValue(null);
			}
		}
	}
	
	/**
	 * Finds a principal object given its security identifier.
	 * 
	 * @param  pLsaHandle  Pointer (native) to the LSASS context.
	 * @param  sid         Security identifier of the principal.
	 * @return Principal object associated with the given security identifier.
	 */
	private
	static
	Object
	findObjectBySID(Pointer pLsaHandle, String sid)
	{
		String       targetProviderName = null;
		LsaFindFlags findFlags  = new LsaFindFlags();
		List<String> sidList    = new ArrayList<String>();
		List<Object> objectsFound = new ArrayList<Object>();
		PointerByReference pppObjects = new PointerByReference();
		
		try
		{
			int winErr = 0;
			
			sidList.add(sid);
			
			LsaQueryList queryList = 
				new LsaQueryList(sidList.toArray(new String[sidList.size()]));
			
			winErr = LWLsaLibrary.INSTANCE.LsaFindObjects(
								pLsaHandle,
								targetProviderName,
								findFlags.getFlags(),
								LsaObjectType.LSA_OBJECT_TYPE_UNDEFINED.getCode(),
								LsaQueryType.LSA_QUERY_TYPE_BY_SID.getCode(),
								sidList.size(),
								queryList.getPointer(),
								pppObjects);
			if (winErr != 0)
			{
				throw new Win32Exception(winErr);
			}
			
			Pointer objectArray[] =
						pppObjects.getValue().getPointerArray(
												0,
												sidList.size());
			
			for (Pointer pObject : objectArray)
			{
				LikewiseNativeObject lwobj = new LikewiseNativeObject(pObject);
				
				LsaObjectType objectType = 
								LsaObjectType.get(lwobj.securityObjectType);
				
				if (objectType.equals(LsaObjectType.LSA_OBJECT_TYPE_USER))
				{
					objectsFound.add(new LikewiseUser(
							lwobj.pszSamAccountName,
							lwobj.securityObjectInfo.userInfo.uid,
							lwobj.securityObjectInfo.userInfo.gid,
							lwobj.pszObjectSid,
							lwobj.securityObjectInfo.userInfo.pszHomedir,
							lwobj.securityObjectInfo.userInfo.pszPasswd,
							lwobj.securityObjectInfo.userInfo.pszGecos,
							lwobj.securityObjectInfo.userInfo.pszShell,
							lwobj.pszDN,
							lwobj.securityObjectInfo.userInfo.pszUPN,
							Byte.valueOf(lwobj.isLocal).intValue() == 1));
				}
				else if (objectType.equals(LsaObjectType.LSA_OBJECT_TYPE_GROUP))
				{
					objectsFound.add(new LikewiseGroup(
							lwobj.securityObjectInfo.groupInfo.pszUnixName,
							lwobj.pszObjectSid,
							lwobj.securityObjectInfo.groupInfo.gid));
				}
			}
			
			return objectsFound.get(0);
		}
		finally
		{
			if (pppObjects.getValue() != null)
			{
				LWLsaLibrary.INSTANCE.LsaFreeSecurityObjectList(
										sidList.size(),
										pppObjects.getValue());
				pppObjects.setValue(null);
			}
		}
	}
}
