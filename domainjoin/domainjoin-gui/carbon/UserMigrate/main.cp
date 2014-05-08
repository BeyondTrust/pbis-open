//
//  main.cpp
//  UserMigrate
//
//  Created by Chuck Mount on 8/7/07.
//  Copyright Centeris Corporation 2007. All rights reserved.
//

#include "main.h"

#include "UserMigrateStatus.h"
#include "UserMigrateException.h"

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

const int UserMigrateApp::ApplicationSignature = 'CnTs';

UserMigrateApp::UserMigrateApp()
: _migrateWindow(0),
  _mainWindow(0),
  _envPath(NULL)
{
    // Create a new window. A full-fledged application would do this from an AppleEvent handler
    // for kAEOpenApplication.
	_mainWindow = new MainWindow(ApplicationSignature);
	MigrateUser();
}

UserMigrateApp::~UserMigrateApp()
{
	if (_migrateWindow)
	{
	   _migrateWindow->Close();
	   delete _migrateWindow;
	}
	if (_mainWindow)
	{
	   _mainWindow->Close();
	   delete _mainWindow;
	}
	if (_envPath)
	   delete _envPath;
}

DomainMigrateWindow&
UserMigrateApp::GetMigrateWindow()
{
    if (!_migrateWindow)
	{
	   _migrateWindow = new DomainMigrateWindow(ApplicationSignature);
	}
	
	return *_migrateWindow;
}

void
UserMigrateApp::MigrateUser()
{

	DomainMigrateWindow& migrateWindow = GetMigrateWindow();
		
	if (migrateWindow)
	{
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

//--------------------------------------------------------------------------------------------
Boolean
UserMigrateApp::HandleCommand( const HICommandExtended& inCommand )
{
    switch ( inCommand.commandID )
    {
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
UserMigrateApp::FixProcessEnvironment()
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
UserMigrateApp::Run()
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
            UserMigrateApp app;
            app.Run();
        }
        else
        {
           SInt16 outItemHit;
		   StandardAlert(kAlertStopAlert,
		                 "\pPermissions error",
					     "\pUnexpected error while setting privileges when launching Active Directory Join Application",
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
					     "\pUnexpected error when launching Active Directory Join Application",
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


