//
//  main.cpp
//  DomainJoin
//
//  Created by Chuck Mount on 8/7/07.
//  Copyright Centeris Corporation 2007. All rights reserved.
//

#include "main.h"

#include "DomainJoinStatus.h"
#include "DomainJoinInterface.h"

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

const int DomainJoinApp::ApplicationSignature = 'CnTs';

DomainJoinApp::DomainJoinApp()
: _mainWindow(0),
  _joinWindow(0),
  _leaveWindow(0),
  _migrateWindow(0),
  _envPath(NULL)
{
    // Create a new window. A full-fledged application would do this from an AppleEvent handler
    // for kAEOpenApplication.
	_mainWindow = new MainWindow(ApplicationSignature);
	JoinOrLeaveDomain();
}

DomainJoinApp::~DomainJoinApp()
{
	if (_joinWindow)
	{
	   _joinWindow->Close();
	   delete _joinWindow;
	}
	if (_leaveWindow)
	{
	   _leaveWindow->Close();
	   delete _leaveWindow;
	}
	if (_mainWindow)
	{
	   _mainWindow->Close();
	   delete _mainWindow;
	}
	if (_envPath)
	   delete _envPath;
}

DomainJoinWindow&
DomainJoinApp::GetJoinWindow()
{
    if (!_joinWindow)
	{
	   _joinWindow = new DomainJoinWindow(ApplicationSignature);
	}
	
	return *_joinWindow;
}

DomainLeaveWindow&
DomainJoinApp::GetLeaveWindow()
{
    if (!_leaveWindow)
	{
	   _leaveWindow = new DomainLeaveWindow(ApplicationSignature);
	}
	
	return *_leaveWindow;
}

DomainMigrateWindow&
DomainJoinApp::GetMigrateWindow()
{
    if (!_migrateWindow)
	{
	   _migrateWindow = new DomainMigrateWindow(ApplicationSignature);
	}
	
	return *_migrateWindow;
}

void
DomainJoinApp::JoinOrLeaveDomain()
{
    try
    {
	    DomainJoinStatus joinStatus;
		
        DomainJoinInterface::GetDomainJoinStatus(joinStatus);
		
		DomainJoinWindow& joinWindow = GetJoinWindow();
		DomainLeaveWindow& leaveWindow = GetLeaveWindow();
        DomainMigrateWindow& migrateWindow = GetMigrateWindow();
		
		if (joinStatus.DomainName.length() > 0)
		{
           migrateWindow.Hide();
		   joinWindow.Hide();
		   leaveWindow.SetComputerName(joinStatus.Name);
		   leaveWindow.SetDomainName(joinStatus.DomainName);
                   leaveWindow.SetOU(joinStatus.OUPath);
           
		   leaveWindow.Show();
		}
		else
		{
           migrateWindow.Hide();
		   leaveWindow.Hide();
		   joinWindow.SetComputerName(joinStatus.Name);
		   joinWindow.SetDomainName(joinStatus.DomainName);
		   joinWindow.Show();
		}

    }
    catch(DomainJoinException& dje)
    {					  
		SInt16 outItemHit;
		const char* err = dje.what();	
		const char* message = dje.GetLongErrorMessage();
		DialogRef dialog;	
		CFStringRef msgStrRef = CFStringCreateWithCString(NULL, message, kCFStringEncodingASCII);
		CFStringGetPascalString(msgStrRef, (StringPtr)message, strlen(message), kCFStringEncodingASCII);
		CFStringRef errStrRef = CFStringCreateWithCString(NULL, err, kCFStringEncodingASCII);
		CFStringGetPascalString(errStrRef, (StringPtr)err, strlen(err), kCFStringEncodingASCII);
		CreateStandardAlert(kAlertStopAlert, errStrRef, msgStrRef, NULL, &dialog);
		RunStandardAlert(dialog, NULL, &outItemHit);
    }
    catch(...)
    {
        SInt16 outItemHit;
        StandardAlert(kAlertStopAlert,
                      "\pUnexpected error",
                      "\pAn unexpected error occurred when joining the Active Directory domain. Please report this to BeyondTrust Technical Support at pbis-support@beyondtrust.com",
                      NULL,
                      &outItemHit);
    }
}

void
DomainJoinApp::MigrateUser()
{
    try
    {
	    DomainJoinStatus joinStatus;
		
        DomainJoinInterface::GetDomainJoinStatus(joinStatus);
		
		DomainMigrateWindow& migrateWindow = GetMigrateWindow();
        DomainJoinWindow& joinWindow = GetJoinWindow();
		DomainLeaveWindow& leaveWindow = GetLeaveWindow();
		
		if (joinStatus.DomainName.length() > 0)
		{
		   joinWindow.Hide();
		   leaveWindow.Hide();
           
           // Clear the AD username edit control
           migrateWindow.SetADUserEdit("");
           
           // Turn off Migrate button
           migrateWindow.MigrateOff();
           
           // Clear display fields
           migrateWindow.SetLocalUserRealName("");
           migrateWindow.SetLocalUserHomeDirectory("");
           migrateWindow.SetLocalUserUID("");
           migrateWindow.SetLocalUserGID("");
           migrateWindow.SetADUserRealName("");
           migrateWindow.SetADUserHomeDirectory("");
           migrateWindow.SetADUserUID("");
           migrateWindow.SetADUserGID("");
           
           // Determine current list of local user accounts and populate the list box control         
           migrateWindow.SetLocalUsers();
           
           migrateWindow.Show();
		}
    }
    catch(DomainJoinException& dje)
    {					  
		SInt16 outItemHit;
		const char* err = dje.what();	
		const char* message = dje.GetLongErrorMessage();
		DialogRef dialog;	
		CFStringRef msgStrRef = CFStringCreateWithCString(NULL, message, kCFStringEncodingASCII);
		CFStringGetPascalString(msgStrRef, (StringPtr)message, strlen(message), kCFStringEncodingASCII);
		CFStringRef errStrRef = CFStringCreateWithCString(NULL, err, kCFStringEncodingASCII);
		CFStringGetPascalString(errStrRef, (StringPtr)err, strlen(err), kCFStringEncodingASCII);
		CreateStandardAlert(kAlertStopAlert, errStrRef, msgStrRef, NULL, &dialog);
		RunStandardAlert(dialog, NULL, &outItemHit);
    }
    catch(...)
    {
        SInt16 outItemHit;
        StandardAlert(kAlertStopAlert,
                      "\pUnexpected error",
                      "\pAn unexpected error occurred when joining the Active Directory domain. Please report this to BeyondTrust Technical Support at pbis-support@beyondtrust.com",
                      NULL,
                      &outItemHit);
    }
}

//--------------------------------------------------------------------------------------------
Boolean
DomainJoinApp::HandleCommand( const HICommandExtended& inCommand )
{
    switch ( inCommand.commandID )
    {
	
		case MAIN_MENU_JOIN_OR_LEAVE_ID:
		    JoinOrLeaveDomain();
			return true;

		case MAIN_MENU_MIGRATE_ID:
		    MigrateUser();
			return true;
            
        // Add your own command-handling cases here
        
        default:
            return false;
    }
}

//
// AuthorizationExecuteWithPrivileges strips the environment
// We need to put back some of the environment
void
DomainJoinApp::FixProcessEnvironment()
{
    std::string delim = ":";
    std::vector<std::string> subPaths;
	std::vector<std::string> essentialPaths;
	std::vector<std::string>::iterator iter;
	bool bNeedNewPath = false;
	
    std::string curPath = getenv("PATH");
	size_t idx_first = 0;
	size_t idx_next = std::string::npos;
	while (idx_first != std::string::npos)
	{
	    idx_next = curPath.find_first_of(delim, idx_first);
		if (idx_next != idx_first)
		{
		   std::string token = curPath.substr(idx_first, idx_next - idx_first);
		   subPaths.push_back(token);
		}
		idx_first = curPath.find_first_not_of(delim, idx_next);
	}
	essentialPaths.push_back("/sbin");
	essentialPaths.push_back("/bin");
	essentialPaths.push_back("/usr/bin");
	// search these paths in reverse, because we are going to add them in front (if they don't exist in the new path)
	for (iter = essentialPaths.begin(); iter != essentialPaths.end(); iter++)
	{
	    std::vector<std::string>::iterator pos = std::find(subPaths.begin(), subPaths.end(), *iter);
		if (pos == subPaths.end())
		{
		   subPaths.push_back(*iter);
		   bNeedNewPath = true;
		}
	}
	if (bNeedNewPath)
	{
	   int iPath = 0;
	   std::ostringstream newPath;
	   newPath << "PATH=";
	   for (iPath = 0, iter = subPaths.begin(); iter != subPaths.end(); iter++, iPath++)
	   {
	       if (iPath)
		   {
		      newPath << delim;
		   }
	       newPath << *iter;
	   }
	   
	   _envPath = strdup(newPath.str().c_str());
	   
	   putenv(_envPath);
	}
}

void
DomainJoinApp::Run()
{
    FixProcessEnvironment();
	TApplication::Run();
}

//--------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if (geteuid() == 0)
    {
        if (setuid(0) == 0)
        {
            DomainJoinApp app;
            app.Run();
        }
        else
        {
           SInt16 outItemHit;
		   StandardAlert(kAlertStopAlert,
		                 "\pPermissions error",
					     "\pUnexpected error while setting privileges when launching Likewise - Active Directory Join Application",
					     NULL,
					     &outItemHit);
        }
    }
    else
    {
       AuthorizationRef   authRef;
       FILE* commPipe = NULL;

       try
       {
           OSStatus status = noErr;
           AuthorizationFlags authFlags = kAuthorizationFlagDefaults;
       
           status = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, authFlags, &authRef);
           if (status != errAuthorizationSuccess)
           {
              throw FailedAdminPrivilegeException("Failed to create authorization");
           }
       
           do
           {
               AuthorizationItem authItems = {kAuthorizationRightExecute, 0, NULL, 0};
               AuthorizationRights authRights = {1, &authItems};
           
               authFlags = (kAuthorizationFlagDefaults |
                            kAuthorizationFlagInteractionAllowed |
                            kAuthorizationFlagPreAuthorize |
                            kAuthorizationFlagExtendRights);
               status = AuthorizationCopyRights(authRef, &authRights, NULL, authFlags, NULL);
        
               if (status != errAuthorizationSuccess)
               {
                  throw FailedAdminPrivilegeException("Failed to acquire admin rights");
               }
           } while (0);
           
           char msgBuf[128];
           int bytesRead = 0;
           
           authFlags = kAuthorizationFlagDefaults;
           status = AuthorizationExecuteWithPrivileges(authRef, argv[0], authFlags, argv, &commPipe);
           if (status != errAuthorizationSuccess)
           {
              throw FailedAdminPrivilegeException("Failed to launch with privileges");
           }
           
           for(bytesRead = 0; bytesRead > 0; bytesRead = read(fileno(commPipe), msgBuf, sizeof(msgBuf)));
       }
       catch(FailedAdminPrivilegeException& fape)
       {
           SInt16 outItemHit;
           char msgStr[256];
		   CFStringRef msgStrRef = CFStringCreateWithCString(NULL, fape.what(), kCFStringEncodingASCII);
		   CFStringGetPascalString(msgStrRef, (StringPtr)msgStr, 255, kCFStringEncodingASCII);
		   StandardAlert(kAlertStopAlert,
		                 "\pFailed to acquire admin privileges",
					     (StringPtr)msgStr,
					     NULL,
					     &outItemHit);
       }
       catch(...)
       {
           SInt16 outItemHit;
		   StandardAlert(kAlertStopAlert,
		                 "\pUnexpected error",
					     "\pUnexpected error when launching Likewise - Active Directory Join Application",
					     NULL,
					     &outItemHit);
       }
       
       if (authRef)
       {
           AuthorizationFree(authRef, kAuthorizationFlagDefaults);
       }
    }

    return 0;
}


