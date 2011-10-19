/*
 *  DomainLeaveWindow.cpp
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/8/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "DomainLeaveWindow.h"
#include "DomainJoinInterface.h"
#include "DomainJoinException.h"

const int DomainLeaveWindow::COMPUTER_NAME_ID = 304;
const int DomainLeaveWindow::DOMAIN_NAME_ID   = 306;
const int DomainLeaveWindow::LEAVE_ID         = 308;
const int DomainLeaveWindow::CLOSE_ID         = 309;
const int DomainLeaveWindow::OU_ID            = 310;
const int DomainLeaveWindow::MIGRATE_ID       = 312;

const int DomainLeaveWindow::LEAVE_CMD_ID     = 'leav';
const int DomainLeaveWindow::MIGRATE_CMD_ID   = 'migr';
const int DomainLeaveWindow::CLOSE_CMD_ID     = 'not!';
		
//--------------------------------------------------------------------------------------------
Boolean
DomainLeaveWindow::HandleCommand( const HICommandExtended& inCommand )
{
    switch ( inCommand.commandID )
    {
        case CLOSE_CMD_ID:
			this->Close();
			break;
        			
		case LEAVE_CMD_ID:
		    HandleLeaveDomain();
			break;
        
        case MIGRATE_CMD_ID:
		    HandleMigrateUser();
			break;
			
        default:
            return false;
    }
	
	return true;
}

void
DomainLeaveWindow::SetComputerName(const std::string& name)
{
    OSStatus err = SetLabelControlString(COMPUTER_NAME_ID, name);
	if (err != noErr)
	{
	   std::string errMsg("Failed to set computer name in control");
	   throw DomainJoinException(-1, "Domain Join Error", errMsg);
	}
}

void
DomainLeaveWindow::SetDomainName(const std::string& name)
{
    OSStatus err = SetLabelControlString(DOMAIN_NAME_ID, name);
	if (err != noErr)
	{
	   std::string errMsg("Failed to set domain name in control");
	   throw DomainJoinException(-1, "Domain Join Error", errMsg);
	}
}

void
DomainLeaveWindow::SetOU(const std::string& ou)
{
    OSStatus err = SetLabelControlString(OU_ID, ou);
	if (err != noErr)
	{
	   std::string errMsg("Failed to set OU path in control");
	   throw DomainJoinException(-1, "Domain Join Error", errMsg);
	}
}

std::string
DomainLeaveWindow::GetComputerName()
{
	std::string result;
    OSStatus err = GetLabelControlString(COMPUTER_NAME_ID, result);
	if (err != noErr)
	{
	   std::string errMsg("Failed to get computer name from control");
	   throw DomainJoinException(-1, "Domain Join Error", errMsg);
	}
	
	return result;
}

std::string
DomainLeaveWindow::GetDomainName()
{
    std::string result;
    OSStatus err = GetLabelControlString(DOMAIN_NAME_ID, result);
	if (err != noErr)
	{
	   std::string errMsg("Failed to get domain name from control");
	   throw DomainJoinException(-1, "Domain Join Error", errMsg);
	}
	
	return result;
}

bool
DomainLeaveWindow::ConfirmLeave(const std::string& domainName)
{
    AlertStdCFStringAlertParamRec params;
	DialogRef dialog;
	OSStatus err = noErr;
	DialogItemIndex itemHit;
	CFStringRef msgStrRef = NULL;
	
	GetStandardAlertDefaultParams(&params, kStdCFStringAlertVersionOne);
	
	params.movable = true;
	params.defaultText = CFSTR("Yes");
	params.cancelText = CFSTR("No");
	params.otherText = NULL;
	params.defaultButton = kAlertStdAlertCancelButton;
	params.position = kWindowCenterOnParentWindow;
	
	msgStrRef = CFStringCreateWithFormat(NULL, NULL, CFSTR("Are you sure you want to leave the %s domain?"), domainName.c_str());
	
	err = CreateStandardAlert(kAlertStopAlert,
	                          CFSTR("Likewise Domain Join"),
							  msgStrRef,
							  &params,
							  &dialog);
	if (err == noErr)
	{
	   err = RunStandardAlert(dialog, NULL, &itemHit);
	   if (err != noErr)
	   {
	      throw DomainJoinException(err, "Domain Join Error", "Failed to display an alert");
	   }
	}
	else
	{
	   throw DomainJoinException(err, "Domain Join Error", "Failed to create dialog");
	}
	
	if (msgStrRef)
	{
	   CFRelease(msgStrRef);
	}
	
	return itemHit != 2;
}

void
DomainLeaveWindow::ShowLeftDomainDialog(const std::string& domainName)
{
    SInt16 outItemHit;
    char msgStr[256];
    sprintf(msgStr, "Domain leave operaton completed successfully");
    CFStringRef msgStrRef = CFStringCreateWithCString(NULL, msgStr, kCFStringEncodingASCII);
    CFStringGetPascalString(msgStrRef, (StringPtr)msgStr, 255, kCFStringEncodingASCII);
    StandardAlert(kAlertNoteAlert,
                  "\pLikewise - Active Directory",
				  (StringPtr)msgStr,
				  NULL,
				  &outItemHit);
}

void
DomainLeaveWindow::HandleLeaveDomain()
{
    try
    {
        std::string computerName = GetComputerName();
        std::string domainName = GetDomainName();
		
        if (!ConfirmLeave(domainName))
            return;
		
        setuid(0);

        DomainJoinInterface::LeaveDomain();

        ShowLeftDomainDialog(domainName);

        PostApplicationEvent(MAIN_MENU_JOIN_OR_LEAVE_ID);
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
                      "\pAn unexpected error occurred when joining the Active Directory domain. Please report this to Likewise Technical Support at support@likewisesoftware.com",
                      NULL,
                      &outItemHit);
    }
}

void
DomainLeaveWindow::HandleMigrateUser()
{
    PostApplicationEvent(MAIN_MENU_MIGRATE_ID);
}

void
DomainLeaveWindow::Close()
{
    QuitApplicationEventLoop();
    TWindow::Close();
}
