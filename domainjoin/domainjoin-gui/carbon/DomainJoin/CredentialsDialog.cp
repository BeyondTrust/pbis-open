/*
 *  CredentialsDialog.cpp
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/13/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "CredentialsDialog.h"
#include "DomainJoinDefs.h"
#include "DomainJoinException.h"

const int CredentialsDialog::USERNAME_ID      = 204;
const int CredentialsDialog::PASSWORD_ID      = 206;
const int CredentialsDialog::CANCEL_BUTTON_ID = 207;
const int CredentialsDialog::OK_BUTTON_ID     = 208;
const int CredentialsDialog::CANCEL_CMD_ID    = 'not!';
const int CredentialsDialog::OK_CMD_ID        = 'ok  ';

CredentialsDialog::CredentialsDialog(int inAppSignature, TWindow& parentWindow)
: TSheet(inAppSignature, CFSTR("Window"), CFSTR("Credentials"), parentWindow)
{
}

void
CredentialsDialog::SetUsername(const std::string& userName)
{
    OSStatus err = SetTextControlString(USERNAME_ID, userName);
	if (err != noErr)
	{
	   std::string errMsg("Failed to set user name in control");
	   throw DomainJoinException(-1, "Domain Join Error", errMsg);
	}
}

std::string
CredentialsDialog::GetUsername()
{
    std::string result;
    OSStatus err = GetTextControlString(USERNAME_ID, result);
	if (err != noErr)
	{
	   std::string errMsg("Failed to get user name from control");
	   throw DomainJoinException(-1, "Domain Join Error", errMsg);
	}
	
	return result;
}

std::string
CredentialsDialog::GetPassword()
{
    std::string result;
    OSStatus err = GetPasswordControlString(PASSWORD_ID, result);
	if (err != noErr)
	{
	   std::string errMsg("Failed to get password from control");
	   throw DomainJoinException(-1, "Domain Join Error", errMsg);
	}
	
	return result;
}

void
CredentialsDialog::SetPassword(const std::string& password)
{
    OSStatus err = SetPasswordControlString(PASSWORD_ID, password);
	if (err != noErr)
	{
	   std::string errMsg("Failed to set password in control");
	   throw DomainJoinException(-1, "Domain Join Error", errMsg);
	}
}

Boolean
CredentialsDialog::HandleCommand(const HICommandExtended& inCommand)
{
    switch( inCommand.commandID )
	{
	      case OK_CMD_ID:
			   this->Close();
		       PostWindowEvent(CREDENTIALS_CMD_OK, GetParentWindowRef());
			   return true;
		  case CANCEL_CMD_ID:
			   this->Close();
		       PostWindowEvent(CREDENTIALS_CMD_CANCEL, GetParentWindowRef());
			   return true;
	      default:
		       return false;
	}
}
