/*
 *  DomainJoinWindow.cpp
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/8/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "DomainJoinWindow.h"
#include "DomainJoinStatus.h"
#include "DomainJoinInterface.h"
#include "DomainJoinException.h"
#include "CredentialsDialog.h"

const int DomainJoinWindow::COMPUTER_NAME_ID              = 131;
const int DomainJoinWindow::DOMAIN_NAME_ID                = 133;
const int DomainJoinWindow::DEFAULT_OU_RADIO_ID           = 135;
const int DomainJoinWindow::OU_PATH_RADIO_ID              = 136;
const int DomainJoinWindow::OU_PATH_TEXT_ID               = 137;
const int DomainJoinWindow::CANCEL_ID                     = 139;
const int DomainJoinWindow::JOIN_ID                       = 140;
const int DomainJoinWindow::USE_SHORT_NAME_FOR_LOGON_ID   = 150;
const int DomainJoinWindow::DEFAULT_USER_DOMAIN_PREFIX_ID = 152;

const int DomainJoinWindow::COMPUTER_NAME_CMD_ID          = 'cnam';
const int DomainJoinWindow::DOMAIN_NAME_CMD_ID            = 'dnam';
const int DomainJoinWindow::DEFAULT_OU_CMD_ID             = 'oudf';
const int DomainJoinWindow::OU_PATH_RADIO_CMD_ID          = 'oupt';
const int DomainJoinWindow::OU_PATH_TEXT_CMD_ID           = 'ouph';
const int DomainJoinWindow::CANCEL_CMD_ID                 = 'not!';
const int DomainJoinWindow::JOIN_CMD_ID                   = 'join';
const int DomainJoinWindow::USE_SHORT_NAME_CMD_ID         = 'shrt';

DomainJoinWindow::DomainJoinWindow(int inAppSignature)
: TWindow( inAppSignature, CFSTR("Window"), CFSTR("Join") ),
  _originalComputerName(""),
  _userName("Administrator"),
  _password(""),
  _credentialsDialog(NULL)
{
    SetRadioButton(DEFAULT_OU_RADIO_ID);
    UnsetRadioButton(OU_PATH_RADIO_ID);
    DisableLocalControl(OU_PATH_TEXT_ID);
    SetRadioButton(USE_SHORT_NAME_FOR_LOGON_ID);
    EnableLocalControl(DEFAULT_USER_DOMAIN_PREFIX_ID);
}

DomainJoinWindow::~DomainJoinWindow()
{
    if (_credentialsDialog)
	{
	   _credentialsDialog->Hide();
	   delete _credentialsDialog;
	}
}

std::string
DomainJoinWindow::GetComputerName()
{
    std::string result;
    OSStatus err = GetTextControlString(COMPUTER_NAME_ID, result);
	if (err != noErr)
	{
	   std::string errMsg("Failed to get computer name from control");
	   throw DomainJoinException(-1, "Domain Join Error", errMsg);
	}
	
	return result;
}

std::string
DomainJoinWindow::GetDomainName()
{
    std::string result;
    OSStatus err = GetTextControlString(DOMAIN_NAME_ID, result);
	if (err != noErr)
	{
	   std::string errMsg("Failed to get domain name from control");
	   throw DomainJoinException(-1, "Domain Join Error", errMsg);
	}
	
	return result;
}

std::string
DomainJoinWindow::GetOUPath()
{
    std::string result;
    if (!IsRadioButtonSet(DEFAULT_OU_RADIO_ID))
    {
        OSStatus err = GetTextControlString(OU_PATH_TEXT_ID, result);
        if (err != noErr)
        {
            std::string errMsg("Failed to get OU Path from control");
            throw DomainJoinException(-1, "Domain Join Error", errMsg);
        }
    }

    return result;
}

std::string
DomainJoinWindow::GetUserDomainPrefix()
{
    std::string result;
    if (IsRadioButtonSet(USE_SHORT_NAME_FOR_LOGON_ID))
    {
        OSStatus err = GetTextControlString(DEFAULT_USER_DOMAIN_PREFIX_ID, result);
        if (err != noErr)
        {
            std::string errMsg("Failed to get user domain prefix from control");
            throw DomainJoinException(-1, "Domain Join Error", errMsg);
        }
    }

    return result;
}

void
DomainJoinWindow::SetComputerName(const std::string& name)
{
    _originalComputerName = name;
    OSStatus err = SetTextControlString(COMPUTER_NAME_ID, name);
    if (err != noErr)
    {
        std::string errMsg("Failed to set computer name in control");
        throw DomainJoinException(-1, "Domain Join Error", errMsg);
    }
}

void
DomainJoinWindow::SetDomainName(const std::string& name)
{
    OSStatus err = SetTextControlString(DOMAIN_NAME_ID, name);
	if (err != noErr)
	{
	   std::string errMsg("Failed to set domain name in control");
	   throw DomainJoinException(-1, "Domain Join Error", errMsg);
	}
}

TWindow&
DomainJoinWindow::GetCredentialsDialog()
{
    if (_credentialsDialog == NULL)
	{
	   _credentialsDialog = new CredentialsDialog(GetAppSignature(), *this);
	}
	
	return *_credentialsDialog;
}

void
DomainJoinWindow::GetCredentials()
{
	CredentialsDialog& credsDialog = dynamic_cast<CredentialsDialog&>(GetCredentialsDialog());
	credsDialog.SetUsername(_userName);
	credsDialog.SetPassword(_password);
	credsDialog.Show();
}

void
DomainJoinWindow::ValidateOUPath(const std::string& ouPath)
{
    if (!IsRadioButtonSet(DEFAULT_OU_RADIO_ID) && !ouPath.length())
    {
        throw InvalidOUPathException();
    }
}

void
DomainJoinWindow::ValidateHostname(const std::string& hostName)
{
    // Length must be between 1 and 15
    // must not be "localhost" or "linux"
    // must not start or end with '-'
    // chars must be alphanumeric or '-'
    bool bInvalidHostname = false;
    int len = hostName.length();
    if ((len < 1) ||
        (len > 15) ||
        (hostName == "localhost") ||
		(hostName == "linux") ||
		(hostName[0] == '-') ||
		(hostName[len-1] == '-'))
	{
	    bInvalidHostname = true;
	}
	else
	{
	   for (int i = 0; i < len; i++)
	   {
	       char ch = (char)hostName[i];
		   if (!(isdigit(ch) ||
				 ((ch >= 'a') && (ch <= 'z')) || 
				 ((ch >= 'A') && (ch <= 'Z')) || 
				 (ch == '-')
				)
			  )
		   {
		      bInvalidHostname = true;
			  break;
		   }
	   }
	}

    if (bInvalidHostname)
	{
	   throw InvalidHostnameException();
	}
}

void
DomainJoinWindow::ValidateUsername(const std::string& userName)
{
    if (!userName.length())
	{
	   throw InvalidUsernameException();
	}
}

void
DomainJoinWindow::ValidateDomainname(const std::string& domainName)
{
    if (!domainName.length())
	{
	   throw InvalidDomainnameException();
	}
}

bool
DomainJoinWindow::ValidateData()
{
    bool result = false;
	std::string domainName;
	
    try
	{
            domainName = GetDomainName();
            ValidateHostname(GetComputerName());
            ValidateDomainname(domainName);
            ValidateOUPath(GetOUPath());
            result = true;
	}
	catch(InvalidDomainnameException& ide)
	{
            SInt16 outItemHit;
            char msgStr[256];
            sprintf(msgStr, "Please specify a valid domain name");
            CFStringRef msgStrRef = CFStringCreateWithCString(NULL, msgStr, kCFStringEncodingASCII);
            CFStringGetPascalString(msgStrRef, (StringPtr)msgStr, 255, kCFStringEncodingASCII);
            StandardAlert(kAlertStopAlert,
			   "\pDomain Join Error",
			   (StringPtr)msgStr,
			   NULL,
			   &outItemHit);
	}
	catch(InvalidHostnameException& ihe)
	{
	     SInt16 outItemHit;
		 char msgStr[256];
		 sprintf(msgStr, "Please specify a valid hostname");
		 CFStringRef msgStrRef = CFStringCreateWithCString(NULL, msgStr, kCFStringEncodingASCII);
		 CFStringGetPascalString(msgStrRef, (StringPtr)msgStr, 255, kCFStringEncodingASCII);
		 StandardAlert(kAlertStopAlert,
					   "\pDomain Join Error",
					   (StringPtr)msgStr,
					   NULL,
					   &outItemHit);
	}
	catch(InvalidOUPathException& ipe)
	{
		 SInt16 outItemHit;
		 char msgStr[256];
		 sprintf(msgStr, "Please specify a valid Organizational Unit");
		 CFStringRef msgStrRef = CFStringCreateWithCString(NULL, msgStr, kCFStringEncodingASCII);
		 CFStringGetPascalString(msgStrRef, (StringPtr)msgStr, 255, kCFStringEncodingASCII);
		 StandardAlert(kAlertStopAlert,
					   "\pDomain Join Error",
					   (StringPtr)msgStr,
					   NULL,
					   &outItemHit);
	}
	catch(UnresolvedDomainNameException& ude)
	{
		 SInt16 outItemHit;
		 char msgStr[256];
		 sprintf(msgStr, "The Active Directory domain %s could not be resolved.", domainName.c_str());
		 CFStringRef msgStrRef = CFStringCreateWithCString(NULL, msgStr, kCFStringEncodingASCII);
		 CFStringGetPascalString(msgStrRef, (StringPtr)msgStr, 255, kCFStringEncodingASCII);
		 StandardAlert(kAlertStopAlert,
					   "\pDomain Join Error",
					   (StringPtr)msgStr,
					   NULL,
					   &outItemHit);
	}
	
	return result;
}

void
DomainJoinWindow::ShowDomainWelcomeDialog(const std::string& domainName)
{
		 SInt16 outItemHit;
		 char msgStr[256];
		 sprintf(msgStr, "Welcome to the %s domain!", domainName.c_str());
		 CFStringRef msgStrRef = CFStringCreateWithCString(NULL, msgStr, kCFStringEncodingASCII);
		 CFStringGetPascalString(msgStrRef, (StringPtr)msgStr, 255, kCFStringEncodingASCII);
		 StandardAlert(kAlertNoteAlert,
					   "\pLikewise - Active Directory",
					   (StringPtr)msgStr,
					   NULL,
					   &outItemHit);
}

void
DomainJoinWindow::HandleJoinDomain()
{
    try
    {
        ValidateUsername(_userName);

        std::string computerName = GetComputerName();
        ValidateHostname(computerName);

        std::string domainName = GetDomainName();
        ValidateDomainname(domainName);

        std::string ouPath = GetOUPath();
        ValidateOUPath(ouPath);

        bool bUseShortNameLogons = IsRadioButtonSet(USE_SHORT_NAME_FOR_LOGON_ID);
        std::string userDomainPrefix = GetUserDomainPrefix();

        // Need to call SetComputerName since it in not always safe to
        // assume that the hostname is configured permanently. We have
        // seen cases where the GetComputerName results are from a DHCP
        // configured hostname, and not a locally set value.
        DomainJoinInterface::SetComputerName(computerName, domainName);
        _originalComputerName = computerName;

        // For launchctl to work, RUID and EUID should be 0
        setuid(0);

        DomainJoinInterface::JoinDomain(domainName,
                                        _userName,
                                        _password,
                                        ouPath,
                                        userDomainPrefix,
                                        bUseShortNameLogons,
                                        false);

        ShowDomainWelcomeDialog(domainName);

        PostApplicationEvent(MAIN_MENU_JOIN_OR_LEAVE_ID);
    }
    catch(InvalidDomainnameException& ide)
    {
        SInt16 outItemHit;
        char msgStr[256];
        sprintf(msgStr, "Please specify a valid domain name");
        CFStringRef msgStrRef = CFStringCreateWithCString(NULL, msgStr, kCFStringEncodingASCII);
        CFStringGetPascalString(msgStrRef, (StringPtr)msgStr, 255, kCFStringEncodingASCII);
        StandardAlert(kAlertStopAlert,
                    "\pDomain Join Error",
                    (StringPtr)msgStr,
                    NULL,
                    &outItemHit);
    }
    catch(InvalidHostnameException& ihe)
    {
        SInt16 outItemHit;
        char msgStr[256];
        sprintf(msgStr, "Please specify a valid hostname");
        CFStringRef msgStrRef = CFStringCreateWithCString(NULL, msgStr, kCFStringEncodingASCII);
        CFStringGetPascalString(msgStrRef, (StringPtr)msgStr, 255, kCFStringEncodingASCII);
        StandardAlert(kAlertStopAlert,
                    "\pDomain Join Error",
                    (StringPtr)msgStr,
                    NULL,
                    &outItemHit);
    }
    catch(InvalidUsernameException& iue)
    {
        SInt16 outItemHit;
        char msgStr[256];
        sprintf(msgStr, "Please specify a valid user id");
        CFStringRef msgStrRef = CFStringCreateWithCString(NULL, msgStr, kCFStringEncodingASCII);
        CFStringGetPascalString(msgStrRef, (StringPtr)msgStr, 255, kCFStringEncodingASCII);
        StandardAlert(kAlertStopAlert,
                    "\pDomain Join Error",
                    (StringPtr)msgStr,
                    NULL,
                    &outItemHit);
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
DomainJoinWindow::HandleCommand( const HICommandExtended& inCommand )
{
    switch ( inCommand.commandID )
    {
        case CANCEL_CMD_ID:
            this->Close();
            return true;

        case DEFAULT_OU_CMD_ID:
            UnsetRadioButton(OU_PATH_RADIO_ID);
            DisableLocalControl(OU_PATH_TEXT_ID);
            return true;

        case OU_PATH_RADIO_CMD_ID:
            UnsetRadioButton(DEFAULT_OU_RADIO_ID);
            EnableLocalControl(OU_PATH_TEXT_ID);
            return true;

        case USE_SHORT_NAME_CMD_ID:
            if (IsRadioButtonSet(USE_SHORT_NAME_FOR_LOGON_ID))
            {
                EnableLocalControl(DEFAULT_USER_DOMAIN_PREFIX_ID);
            }
            else
            {
                DisableLocalControl(DEFAULT_USER_DOMAIN_PREFIX_ID);
            }
            return true;

        case JOIN_CMD_ID:
            if (ValidateData())
            {
                GetCredentials();
            }
            return true;
 
        case CREDENTIALS_CMD_OK:
        {
            CredentialsDialog& credsDialog = dynamic_cast<CredentialsDialog&>(GetCredentialsDialog());
            _userName = credsDialog.GetUsername();
            _password = credsDialog.GetPassword();
            HandleJoinDomain();
        }
        return true;
			 
        case CREDENTIALS_CMD_CANCEL:
            return true;
     
        default:
            return false;
    }
}

void
DomainJoinWindow::Close()
{
    QuitApplicationEventLoop();
    TWindow::Close();
}
