/*
 *  UserMigrateWindow.cpp
 *  UserMigrate
 *
 *  Created by Sriram Nambakam on 8/8/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "DomainMigrateWindow.h"
#include "UserMigrateStatus.h"
#include "UserMigrateException.h"
#include "CredentialsDialog.h"
#include <DirectoryService/DirectoryService.h>

const int DomainMigrateWindow::LOCAL_USER_PATH_ID  = 500;
const int DomainMigrateWindow::LOCAL_USER_UID_ID   = 501;
const int DomainMigrateWindow::LOCAL_USER_GID_ID   = 502;
const int DomainMigrateWindow::AD_USER_PATH_ID     = 503;
const int DomainMigrateWindow::AD_USER_UID_ID      = 504;
const int DomainMigrateWindow::AD_USER_GID_ID      = 505;
const int DomainMigrateWindow::COPY_RADIO_ID       = 506;
const int DomainMigrateWindow::MOVE_RADIO_ID       = 507;
const int DomainMigrateWindow::CANCEL_ID           = 511;
const int DomainMigrateWindow::MIGRATE_ID          = 512;
const int DomainMigrateWindow::VALIDATE_ID         = 513;
const int DomainMigrateWindow::AD_USER_EDIT_ID     = 514;
const int DomainMigrateWindow::LOCAL_USER_COMBO_ID = 515;
const int DomainMigrateWindow::MIGRATE_PROGRESS_ID = 516;
const int DomainMigrateWindow::LOCAL_USER_REAL_NAME_ID = 517;
const int DomainMigrateWindow::AD_USER_REAL_NAME_ID= 518;

const int DomainMigrateWindow::LOCAL_USER_NAME_CMD_ID = 'lcnm';
const int DomainMigrateWindow::AD_USER_NAME_CMD_ID    = 'adnm';
const int DomainMigrateWindow::COPY_RADIO_CMD_ID      = 'copy';
const int DomainMigrateWindow::MOVE_RADIO_CMD_ID      = 'move';
const int DomainMigrateWindow::VALIDATE_CMD_ID        = 'vald';
const int DomainMigrateWindow::CANCEL_CMD_ID          = 'not!';
const int DomainMigrateWindow::MIGRATE_CMD_ID         = 'mgrt';

DomainMigrateWindow::DomainMigrateWindow(int inAppSignature)
: TWindow( inAppSignature, CFSTR("Window"), CFSTR("Migrate") ),
  _localUsersFirstItem(""),
  _localUserRealName(""),
  _localUserHomeDir(""),
  _localUserUID(""),
  _localUserGID(""),
  _adUserRealName(""),
  _adUserHomeDir(""),
  _adUserUID(""),
  _adUserGID(""),
  _pLocalUsers(NULL)
{
    SetRadioButton(COPY_RADIO_ID);
    UnsetRadioButton(MOVE_RADIO_ID);
}

DomainMigrateWindow::~DomainMigrateWindow()
{
}

std::string
DomainMigrateWindow::GetLocalUserName()
{
    std::string result;
    OSStatus err = GetTextControlString(LOCAL_USER_COMBO_ID, result);
    if (err != noErr)
    {
        std::string errMsg("Failed to get local user name from control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
    
    return result;
}

std::string
DomainMigrateWindow::GetLocalUserHomeDir()
{
    std::string result;
    OSStatus err = GetTextControlString(LOCAL_USER_PATH_ID, result);
    if (err != noErr)
    {
        std::string errMsg("Failed to get local user homedir from control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
    
    return result;
}

std::string
DomainMigrateWindow::GetADUserName()
{
    std::string result;
    OSStatus err = GetTextControlString(AD_USER_EDIT_ID, result);
    if (err != noErr)
    {
        std::string errMsg("Failed to get AD user name from control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
    
    return result;
}

std::string
DomainMigrateWindow::GetADUserHomeDir()
{
    std::string result;
    OSStatus err = GetTextControlString(AD_USER_PATH_ID, result);
    if (err != noErr)
    {
        std::string errMsg("Failed to get AD user homedir from control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
    
    return result;
}

std::string
DomainMigrateWindow::GetADUserUID()
{
    std::string result;
    OSStatus err = GetTextControlString(AD_USER_UID_ID, result);
    if (err != noErr)
    {
        std::string errMsg("Failed to get AD user homedir from control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
    
    return result;
}

std::string
DomainMigrateWindow::GetADUserGID()
{
    std::string result;
    OSStatus err = GetTextControlString(AD_USER_GID_ID, result);
    if (err != noErr)
    {
        std::string errMsg("Failed to get AD user homedir from control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
    
    return result;
}

bool
DomainMigrateWindow::IsMoveOptionSelected()
{
    return IsRadioButtonSet(MOVE_RADIO_ID);
}

void
DomainMigrateWindow::SetLocalUserRealName(const std::string& value)
{
    _localUserRealName = value;
    OSStatus err = SetTextControlString(LOCAL_USER_REAL_NAME_ID, value);
    if (err != noErr)
    {
        std::string errMsg("Failed to set local user real name in control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
}

void
DomainMigrateWindow::SetLocalUserHomeDirectory(const std::string& value)
{
    _localUserHomeDir = value;
    OSStatus err = SetTextControlString(LOCAL_USER_PATH_ID, value);
    if (err != noErr)
    {
        std::string errMsg("Failed to set local user home directory in control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
}

void
DomainMigrateWindow::SetLocalUserUID(const std::string& value)
{
    _localUserUID = value;
    OSStatus err = SetTextControlString(LOCAL_USER_UID_ID, value);
    if (err != noErr)
    {
        std::string errMsg("Failed to set local user uid in control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
}

void
DomainMigrateWindow::SetLocalUserGID(const std::string& value)
{
    _localUserGID = value;
    OSStatus err = SetTextControlString(LOCAL_USER_GID_ID, value);
    if (err != noErr)
    {
        std::string errMsg("Failed to set local user gid in control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
}

void
DomainMigrateWindow::SetADUserRealName(const std::string& value)
{
    _adUserRealName = value;
    OSStatus err = SetTextControlString(AD_USER_REAL_NAME_ID, value);
    if (err != noErr)
    {
        std::string errMsg("Failed to set AD user real name in control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
}

void
DomainMigrateWindow::SetADUserHomeDirectory(const std::string& value)
{
    _adUserHomeDir = value;
    OSStatus err = SetTextControlString(AD_USER_PATH_ID, value);
    if (err != noErr)
    {
        std::string errMsg("Failed to set AD user home directory in control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
}

void
DomainMigrateWindow::SetADUserUID(const std::string& value)
{
    _adUserUID = value;
    OSStatus err = SetTextControlString(AD_USER_UID_ID, value);
    if (err != noErr)
    {
        std::string errMsg("Failed to set AD user uid in control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
}

void
DomainMigrateWindow::SetADUserGID(const std::string& value)
{
    _adUserGID = value;
    OSStatus err = SetTextControlString(AD_USER_GID_ID, value);
    if (err != noErr)
    {
        std::string errMsg("Failed to set AD user gid in control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
}

void
DomainMigrateWindow::SetADUserEdit(const std::string& value)
{
    OSStatus err = SetTextControlString(AD_USER_EDIT_ID, value);
    if (err != noErr)
    {
        std::string errMsg("Failed to set AD user gid in control");
        throw UserMigrateException(-1, "Migrate User Error", errMsg);
    }
}

void
DomainMigrateWindow::MigrateOff()
{
    OSStatus err = noErr;
    
    ControlID migrateCtrl;
    migrateCtrl.id = MIGRATE_ID;
    migrateCtrl.signature = 'CnTs';
    ControlRef migrateRef = nil;
    
    err = GetControlByID(this->GetWindowRef(), &migrateCtrl, &migrateRef);
    if (err == noErr)
    {
        DeactivateControl(migrateRef);
    }
}

void
DomainMigrateWindow::MigrateOn()
{
    OSStatus err = noErr;
    
    ControlID migrateCtrl;
    migrateCtrl.id = MIGRATE_ID;
    migrateCtrl.signature = 'CnTs';
    ControlRef migrateRef = nil;
    
    err = GetControlByID(this->GetWindowRef(), &migrateCtrl, &migrateRef);
    if (err == noErr)
    {
        ActivateControl(migrateRef);
    }
}

void
DomainMigrateWindow::SetLocalUsers()
{
    PUSER_LIST pLocalUsers = NULL;
    PUSER_LIST pTemp = NULL;
    
    ClearLocalUsersCombo();
    GetLocalUserList(&pLocalUsers);
    
    pTemp = pLocalUsers;
    
    while (pTemp)
    {
        AddUserToLocalUsersCombo(pTemp->pszUsername);
        pTemp = pTemp->pNext;
    }
    
    _pLocalUsers = pLocalUsers;
    pLocalUsers = NULL;
    
    SetTitleToLocalUsersCombo();
}

void
DomainMigrateWindow::ClearLocalUsersCombo()
{
    OSStatus err = noErr;
    
    ControlID luComboCtrl;
    luComboCtrl.id = LOCAL_USER_COMBO_ID;
    luComboCtrl.signature = 'CnTs';
    ControlRef luComboRef = nil;
    
    ItemCount numItems = 0, i = 0;
    _localUsersFirstItem = "";

    err = GetControlByID(this->GetWindowRef(), &luComboCtrl, &luComboRef);
    if (err == noErr)
    {
        numItems = HIComboBoxGetItemCount(luComboRef);
    
        for (i = 0; i < numItems; i++)
        {
            HIComboBoxRemoveItemAtIndex(luComboRef, 0); // Remove topmost item each iteration
        }
    }
    
    if (_pLocalUsers)
    {
        FreeLocalUserList(_pLocalUsers);
        _pLocalUsers = NULL;
    }
    
    SetTextControlString(LOCAL_USER_COMBO_ID, "");
}

void
DomainMigrateWindow::AddUserToLocalUsersCombo(const std::string& value)
{
    OSStatus err = noErr;
    
    ControlID luComboCtrl;
    luComboCtrl.id = LOCAL_USER_COMBO_ID;
    luComboCtrl.signature = 'CnTs';
    ControlRef luComboRef = nil;

    CFStringRef valueStrRef = CFStringCreateWithCString(NULL, value.c_str(), kCFStringEncodingASCII);
    
    err = GetControlByID(this->GetWindowRef(), &luComboCtrl, &luComboRef);
    if (err == noErr)
    {
        err = HIComboBoxAppendTextItem(luComboRef, valueStrRef, NULL);
        if (err != noErr)
        {
            std::string errMsg("Failed to add local user to combo list in control");
            throw UserMigrateException(-1, "Migrate User Error", errMsg);
        }
    }

    CFRelease(valueStrRef);
}

void
DomainMigrateWindow::SetTitleToLocalUsersCombo()
{
    OSStatus err = noErr;
    
    ControlID luComboCtrl;
    luComboCtrl.id = LOCAL_USER_COMBO_ID;
    luComboCtrl.signature = 'CnTs';
    ControlRef luComboRef = nil;
    CFStringRef firstItemRef;
    char szFirstItem[256] = {0};
    PUSER_LIST pTemp = _pLocalUsers;
    
    err = GetControlByID(this->GetWindowRef(), &luComboCtrl, &luComboRef);
    if (err == noErr)
    {
        err = HIComboBoxCopyTextItemAtIndex(luComboRef, 0, &firstItemRef);
        if (err != noErr)
        {
            _localUsersFirstItem = "";
            SetTextControlString(LOCAL_USER_COMBO_ID, _localUsersFirstItem);
            goto exit;
        }
    }

    CFStringGetCString(firstItemRef, szFirstItem, 255, kCFStringEncodingASCII);
    _localUsersFirstItem = szFirstItem;
    SetTextControlString(LOCAL_USER_COMBO_ID, _localUsersFirstItem);
    
    // Fill in the fields for the selected first item
    while (pTemp)
    {
        if (!strcmp(pTemp->pszUsername, szFirstItem))
        {
            SetLocalUserRealName(pTemp->pszUserRealName);
            SetLocalUserHomeDirectory(pTemp->pszUserHomeDir);
            SetLocalUserUID(pTemp->pszUserUID);
            SetLocalUserGID(pTemp->pszUserGID);
            break;
        }
        pTemp = pTemp->pNext;
    }
    
exit:

    if (firstItemRef)
    {
        CFRelease(firstItemRef);
    }
}

void
DomainMigrateWindow::ShowMigrateCompleteDialog(const std::string& value)
{
    SInt16 outItemHit;
    char msgStr[256];
    sprintf(msgStr,
            "Account migration complete!\n\nMigration processing logs can be found at:\n\t/tmp/lw-migrate.%s.log",
            value.c_str());
    CFStringRef msgStrRef = CFStringCreateWithCString(NULL, msgStr, kCFStringEncodingASCII);
    CFStringGetPascalString(msgStrRef, (StringPtr)msgStr, 255, kCFStringEncodingASCII);
    StandardAlert(kAlertNoteAlert,
    "\pLikewise - Active Directory",
    (StringPtr)msgStr,
    NULL,
    &outItemHit);
    CFRelease(msgStrRef);
}

void
DomainMigrateWindow::ShowMigrateCompleteErrorDialog(
    const std::string& value,
    int code,
    const std::string& resultMessage
    )
{
    SInt16 outItemHit;
    char msgStr[512] = { 0 };
    sprintf(msgStr,
            "Account migration script finished with exitcode: %d\n%sMigration processing logs can be found at:\n\t/tmp/lw-migrate.%s.log",
            code,
            resultMessage.c_str() ? resultMessage.c_str() : "",
            value.c_str());
    CFStringRef msgStrRef = CFStringCreateWithCString(NULL, msgStr, kCFStringEncodingASCII);
    CFStringGetPascalString(msgStrRef, (StringPtr)msgStr, 511, kCFStringEncodingASCII);
    StandardAlert(kAlertNoteAlert,
                  "\pLikewise - Active Directory",
                  (StringPtr)msgStr,
                  NULL,
                  &outItemHit);
                  CFRelease(msgStrRef);
}

bool
DomainMigrateWindow::ConfirmMigration(
    const std::string& localUserName,
    const std::string& localUserHomeDir,
    const std::string& adUserName,
    const std::string& adUserHomeDir,
    const std::string& adUserUID,
    const std::string& adUserGID,
    bool bMoveProfile
    )
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
    
    if (!strcmp(localUserHomeDir.c_str(), adUserHomeDir.c_str()))
    {
        msgStrRef = CFStringCreateWithFormat(NULL,
                                             NULL,
                                             CFSTR("Are you sure you want to migrate the profile?\n\tFrom local user: %s\n\tTo AD user: %s\n\nSince the two profiles share the same home directory path, the contents of '%s' will be reassigned to the new owner (UID: %s, GID: %s)"),
                                             localUserName.c_str(),
                                             adUserName.c_str(),
                                             localUserHomeDir.c_str(),
                                             adUserUID.c_str(),
                                             adUserGID.c_str());
    }
    else
    {
        msgStrRef = CFStringCreateWithFormat(NULL,
                                             NULL,
                                             CFSTR("Are you sure you want to migrate the profile?\n\tFrom local user: %s\n\tTo AD user: %s\n\nContents from '%s' will be %s to '%s'.\nOwnership will be asssigned to (UID: %s, GID: %s)"),
                                             localUserName.c_str(),
                                             adUserName.c_str(),
                                             localUserHomeDir.c_str(),
                                             bMoveProfile ? "moved" : "copied",
                                             adUserHomeDir.c_str(),
                                             adUserUID.c_str(),
                                             adUserGID.c_str());
    }

    err = CreateStandardAlert(kAlertStopAlert,
                              CFSTR("Likewise Migrate User Profile"),
                              msgStrRef,
                              &params,
                              &dialog);
    if (err == noErr)
    {
        err = RunStandardAlert(dialog, NULL, &itemHit);
        if (err != noErr)
        {
            throw UserMigrateException(err, "Domain Join Error", "Failed to display an alert");
        }
    }
    else
    {
        throw UserMigrateException(err, "Domain Join Error", "Failed to create dialog");
    }
    
    if (msgStrRef)
    {
        CFRelease(msgStrRef);
    }
    
    return itemHit != 2;
}

int
DomainMigrateWindow::CallMigrateCommand(
    const std::string& localUserHomeDir,
    const std::string& adUserName,
    const std::string& logFileName,
    bool bMoveProfile,
    char ** ppszOutput
    )
{
    long macError = eDSNoErr;
    char * pszOutput = NULL;
    int exitCode = 0;
    const char* argsCopy[] = { "/opt/likewise/bin/lw-local-user-migrate.sh",
                               localUserHomeDir.c_str(),
                               adUserName.c_str(),
                               "--log",
                               logFileName.c_str(),
                               (char *) NULL };
    const char* argsMove[] = { "/opt/likewise/bin/lw-local-user-migrate.sh",
                               localUserHomeDir.c_str(),
                               adUserName.c_str(),
                               "--move",
                               "--log",
                               logFileName.c_str(),
                               (char *) NULL };
                
    macError = CallCommandWithOutputAndErr(bMoveProfile ? argsMove[0] : argsCopy[0],
                                           bMoveProfile ? argsMove : argsCopy,
                                           true,
                                           &pszOutput,
                                           &exitCode);
    if (macError)
    {
        exitCode = -1;
    }
    
    if (ppszOutput && pszOutput)
    {
        *ppszOutput = pszOutput;
        pszOutput = NULL;
    }
    
exit:

    if (pszOutput)
    {
        free(pszOutput);
    }

    return exitCode;
}

void
DomainMigrateWindow::HideMigrateProgressBar()
{
    OSStatus err = noErr;
    
    ControlID progressBarCtrl;
    progressBarCtrl.id = MIGRATE_PROGRESS_ID;
    progressBarCtrl.signature = 'CnTs';
    ControlRef progressBarRef = nil;
    
    err = GetControlByID(this->GetWindowRef(), &progressBarCtrl, &progressBarRef);
    if (err == noErr)
    {
        HideControl(progressBarRef);
    }
}

void
DomainMigrateWindow::ShowMigrateProgressBar()
{
    OSStatus err = noErr;
    
    ControlID progressBarCtrl;
    progressBarCtrl.id = MIGRATE_PROGRESS_ID;
    progressBarCtrl.signature = 'CnTs';
    ControlRef progressBarRef = nil;
    
    err = GetControlByID(this->GetWindowRef(), &progressBarCtrl, &progressBarRef);
    if (err == noErr)
    {
        ShowControl(progressBarRef);
    }
}

void
DomainMigrateWindow::HandleMigration()
{
    try
    {
        std::string localUserName = GetLocalUserName();
        std::string localUserHomeDir = GetLocalUserHomeDir();
        std::string adUserName = GetADUserName();
        std::string adUserHomeDir = GetADUserHomeDir();
        std::string adUserUID = GetADUserUID();
        std::string adUserGID = GetADUserGID();
        bool bMoveProfile = IsMoveOptionSelected();
        
        ShowMigrateProgressBar();
        
        if (ConfirmMigration(localUserName, localUserHomeDir, adUserName, adUserHomeDir, adUserUID, adUserGID, bMoveProfile))
        {
            int ret = 0;
            char szLogFileName[256] = { 0 };
            char * pszErrorMessage = NULL;
            
            sprintf(szLogFileName, "/tmp/lw-migrate.%s.log", localUserName.c_str());
            
            // Migrate user with the parameters we have determined...
            ret = CallMigrateCommand(localUserHomeDir, adUserName, szLogFileName, bMoveProfile, &pszErrorMessage);
            
            HideMigrateProgressBar();
            
            if (ret)
            {
                ShowMigrateCompleteErrorDialog(localUserName, ret, pszErrorMessage);
                
                if (pszErrorMessage)
                {
                    free(pszErrorMessage);
                }
            }
            else
            {
                ShowMigrateCompleteDialog(localUserName);
            }
            
            // Okay to switch back to Leave dialog since the migration is completed
            PostApplicationEvent(MAIN_MENU_JOIN_OR_LEAVE_ID);
        }
        
        HideMigrateProgressBar();
    }
    catch(UserMigrateException& dje)
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
        CFRelease(msgStrRef);
    }
    catch(...)
    {
        SInt16 outItemHit;
        StandardAlert(kAlertStopAlert,
                      "\pUnexpected error",
                      "\pAn unexpected error occurred when attempting to migrate local user profile to AD profile. Please report this to Likewise Technical Support at support@likewisesoftware.com",
                      NULL,
                      &outItemHit);
    }
}

bool
DomainMigrateWindow::HandleValidateUser()
{
    long   macError = eDSNoErr;  
    char * pszRealName = NULL;
    char * pszHomeDir = NULL;
    char * pszUID = NULL;
    char * pszGID = NULL;
    
    try
    {
        std::string adUserName = GetADUserName();

        // Clear previous data fields
        SetADUserRealName("");
        SetADUserHomeDirectory("");
        SetADUserUID("");
        SetADUserGID("");
        
        macError = GetADUserInfo(adUserName.c_str(),
                                 &pszRealName,
                                 &pszHomeDir,
                                 &pszUID,
                                 &pszGID);
        //if (macError) goto exit;
        if (macError)
        {
            std::string errMsg("User name not found!");
            throw UserMigrateException(-1, "Migrate User Error", errMsg);
        }

        SetADUserRealName(pszRealName);
        SetADUserHomeDirectory(pszHomeDir);
        SetADUserUID(pszUID);
        SetADUserGID(pszGID);
    }
    catch(UserMigrateException& dje)
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
        CFRelease(msgStrRef);
    }
    catch(...)
    {
        SInt16 outItemHit;
        StandardAlert(kAlertStopAlert,
                      "\pUnexpected error",
                      "\pAn unexpected error occurred when attempting to migrate local user profile to AD profile. Please report this to Likewise Technical Support at support@likewisesoftware.com",
                      NULL,
                      &outItemHit);
    }
    
exit:

    if (pszRealName)
    {
        free(pszRealName);
    }
    
    if (pszHomeDir)
    {
        free(pszHomeDir);
    }
    
    if (pszUID)
    {
        free(pszUID);
    }
    
    if (pszGID)
    {
        free(pszGID);
    }
    
    return (macError == noErr);
}

//--------------------------------------------------------------------------------------------
Boolean
DomainMigrateWindow::HandleCommand( const HICommandExtended& inCommand )
{
    std::string localUserName = GetLocalUserName();
    std::string currentFirstItem = _localUsersFirstItem;
    PUSER_LIST pTemp = _pLocalUsers;
    
    if (localUserName != currentFirstItem)
    {
        _localUsersFirstItem = localUserName;
        while (pTemp)
        {
            if (!strcmp(pTemp->pszUsername, localUserName.c_str()))
            {
                SetLocalUserRealName(pTemp->pszUserRealName);
                SetLocalUserHomeDirectory(pTemp->pszUserHomeDir);
                SetLocalUserUID(pTemp->pszUserUID);
                SetLocalUserGID(pTemp->pszUserGID);
                break;
            }
            pTemp = pTemp->pNext;
        }
    }
    
    switch ( inCommand.commandID )
    {
        case CANCEL_CMD_ID:
            PostApplicationEvent(MAIN_MENU_JOIN_OR_LEAVE_ID);
            return true;

        case MOVE_RADIO_CMD_ID:
            UnsetRadioButton(COPY_RADIO_ID);
            return true;

        case COPY_RADIO_CMD_ID:
            UnsetRadioButton(MOVE_RADIO_ID);
            return true;
             
        case LOCAL_USER_NAME_CMD_ID:
            return true;

        case MIGRATE_CMD_ID:
            if (HandleValidateUser())
            {
                HandleMigration();
            }
            return true;
 
        case VALIDATE_CMD_ID:
            if (HandleValidateUser())
            {
                MigrateOn();
            }
            else
            {
                MigrateOff();
            }
            return true;
            
        default:
            return false;
    }
}

void
DomainMigrateWindow::Close()
{
    QuitApplicationEventLoop();
    TWindow::Close();
}


/* Directory Service helper routines to get user record data */

static
void
DoubleTheBufferSizeIfItsTooSmall(
    long *              pMacError, 
    tDirNodeReference   hDirRef, 
    tDataBufferPtr *    ppBuffer
)
    // This routine is designed to handle the case where a 
    // Open Directory routine returns eDSBufferTooSmall.  
    // If so, it doubles the size of the buffer, allowing the 
    // caller to retry the Open Directory routine with the 
    // large buffer.
    //
    // errPtr is a pointer to a Open Directory error.  
    // This routine does nothing unless that error is 
    // eDSBufferTooSmall.  In that case it frees the buffer 
    // referenced by *bufPtrPtr, replacing it with a buffer 
    // of twice the size.  It then leaves *errPtr set to 
    // eDSBufferTooSmall so that the caller retries the 
    // call with the larger buffer.
{
    long            macError = eDSNoErr;
    tDirStatus      junk;
    tDataBufferPtr  pBuffer = NULL;
    
    if (*pMacError == eDSBufferTooSmall)
    {
        // If the buffer size is already bigger than 16 MB, don't try to 
        // double it again; something has gone horribly wrong.
        if ( (*ppBuffer)->fBufferSize >= (16 * 1024 * 1024) )
        {
            macError = eDSAllocationFailed;
            if (macError) goto cleanup;
        }

        pBuffer = dsDataBufferAllocate(hDirRef, (*ppBuffer)->fBufferSize * 2);
        if (!pBuffer)
        {
            macError = eDSAllocationFailed;
            if (macError) goto cleanup;
        }
        
        junk = dsDataBufferDeAllocate(hDirRef, *ppBuffer);
        *ppBuffer = pBuffer;
    }
    
cleanup:

    // If err is eDSNoErr, the buffer expansion was successful 
    // so we leave *errPtr set to eDSBufferTooSmall.  If err 
    // is any other value, the expansion failed and we set 
    // *errPtr to that error.
        
    if (macError != eDSNoErr)
    {
        *pMacError = macError;
    }
}

static
long
dsFindDirNodes_Wrap(
    tDirReference       hDirRef,
    tDataBufferPtr *    ppDataBuffer,
    tDataListPtr        pNodeName,
    tDirPatternMatch    PatternMatchType,
    unsigned long       *pulNodeCount,
    tContextData        *inOutContinueData
    )
    // A wrapper for dsFindDirNodes that handles two special cases:
    //
    // o If the routine returns eDSBufferTooSmall, it doubles the 
    //   size of the buffer referenced by *inOutDataBufferPtrPtr 
    //   and retries.
    //
    //   Note that this change requires a change of the function 
    //   prototype; the second parameter is a pointer to a pointer 
    //   to the buffer, rather than just a pointer to the buffer. 
    //   This is so that I can modify the client's buffer pointer.
    //
    // o If the routine returns no nodes but there's valid continue data, 
    //   it retries.
    //
    // In other respects this works just like dsFindDirNodes.
{
    long macError = eDSNoErr;
    
    do {
        do {
            macError = dsFindDirNodes(
                hDirRef, 
                *ppDataBuffer, 
                pNodeName, 
                PatternMatchType, 
                pulNodeCount, 
                inOutContinueData
            );
            DoubleTheBufferSizeIfItsTooSmall(&macError, hDirRef, ppDataBuffer);
        } while (macError == eDSBufferTooSmall);
    } while ( (macError == eDSNoErr) && (*pulNodeCount == 0) && (*inOutContinueData != 0) );

    return macError;
}

enum {
    kDefaultDSBufferSize = 1024
};

static 
long
GetLocalNodePathList(
    tDirReference  hDirRef,
    tDataListPtr * ppLocalNodePath
    )
    // Returns the path to the Open Directory local node. (/NetInfo/root/ or Local/Default/)
    // dirRef is the connection to Open Directory.
    // On success, *searchNodePathListPtr is a data list that 
    // contains the search node's path components.
{
    long                macError = eDSNoErr;
    tDataBufferPtr      pDataBuffer = NULL;
    tDirPatternMatch    patternToFind = eDSLocalNodeNames;
    unsigned long       ulNodeCount = 0;
    tContextData        context = NULL;
    tDataListPtr        pLocalNodePath = NULL;
    
    // Allocate a buffer for the node find results.  We'll grow 
    // this buffer if it proves to be to small.
    
    pDataBuffer = dsDataBufferAllocate(hDirRef, kDefaultDSBufferSize);
    if (!pDataBuffer)
    {
        macError = eDSAllocationFailed;
        goto cleanup;
    }
    
    // Find the node.  Note that this is a degenerate case because 
    // we're only looking for a single node, the local node, so 
    // we don't need to loop calling dsFindDirNodes, which is the 
    // standard way of using dsFindDirNodes.
    
    macError = dsFindDirNodes_Wrap(
            hDirRef, 
            &pDataBuffer,                       // place results here
            NULL,                               // no pattern, rather...
            patternToFind,                      // ... hardwired search type
            &ulNodeCount, 
            &context
        );
    if (macError) goto cleanup;
    
    // If we didn't find any nodes, that's bad.
    
    if (ulNodeCount < 1)
    {
        macError = eDSNodeNotFound;
        goto cleanup;
    }
    
    // Grab the first node from the buffer.  Note that the inDirNodeIndex 
    // parameter to dsGetDirNodeName is one-based, so we pass in the constant 
    // 1.
    // 
    // Also, if we found more than one, that's unusual, but not enough to 
    // cause us to error.
    macError = dsGetDirNodeName(hDirRef, pDataBuffer, 1, &pLocalNodePath);
    if (macError) goto cleanup;
    
    *ppLocalNodePath = pLocalNodePath;
    pLocalNodePath = NULL;
    
cleanup:

    // Clean up.
    
    if (context != 0)
    {
        dsReleaseContinueData(hDirRef, context);
    }
    
    if (pDataBuffer)
    {
        dsDataBufferDeAllocate(hDirRef, pDataBuffer);
    }
    
    return macError;
}

static 
long
GetLikewiseNodePathList(
    tDirReference  hDirRef,
    tDataListPtr * ppLikewiseNodePath
    )
    // Returns the path to the Open Directory likewise node. (/Likewise - Active Directory/)
    // dirRef is the connection to Open Directory.
    // On success, *searchNodePathListPtr is a data list that 
    // contains the search node's path components.
{
    long                macError = eDSNoErr;
    tDataBufferPtr      pDataBuffer = NULL;
    tDataList           likewiseNodeName;
    tDirPatternMatch    patternToFind = eDSiContains;
    unsigned long       ulNodeCount = 0;
    tContextData        context = NULL;
    tDataListPtr        pLikewiseNodePath = NULL;
    
    // Create Likewise node name string list
    macError = dsBuildListFromStringsAlloc(hDirRef, &likewiseNodeName, "Likewise", NULL);
    if (macError) goto cleanup;  
    
    // Allocate a buffer for the node find results.  We'll grow 
    // this buffer if it proves to be to small.
    
    pDataBuffer = dsDataBufferAllocate(hDirRef, kDefaultDSBufferSize);
    if (!pDataBuffer)
    {
        macError = eDSAllocationFailed;
        goto cleanup;
    }
    
    // Find the node.  Note that this is a degenerate case because 
    // we're only looking for a single node, the local node, so 
    // we don't need to loop calling dsFindDirNodes, which is the 
    // standard way of using dsFindDirNodes.
    
    macError = dsFindDirNodes_Wrap(
            hDirRef, 
            &pDataBuffer,                       // place results here
            &likewiseNodeName,
            patternToFind,
            &ulNodeCount, 
            &context
        );
    if (macError) goto cleanup;
    
    // If we didn't find any nodes, that's bad.
    
    if (ulNodeCount < 1) {
        macError = eDSNodeNotFound;
        goto cleanup;
    }
    
    // Grab the first node from the buffer.  Note that the inDirNodeIndex 
    // parameter to dsGetDirNodeName is one-based, so we pass in the constant 
    // 1.
    // 
    // Also, if we found more than one, that's unusual, but not enough to 
    // cause us to error.
    macError = dsGetDirNodeName(hDirRef, pDataBuffer, 1, &pLikewiseNodePath);
    if (macError) goto cleanup;
    
    *ppLikewiseNodePath = pLikewiseNodePath;
    pLikewiseNodePath = NULL;
    
cleanup:

    dsDataListDeallocate(hDirRef, &likewiseNodeName);
    
    if (context != 0)
    {
        dsReleaseContinueData(hDirRef, context);
    }
    
    if (pDataBuffer)
    {
        dsDataBufferDeAllocate(hDirRef, pDataBuffer);
    }
    
    return macError;
}

static
long
GetUserInfo(
    tDirReference hDirRef,
    tDirNodeReference hNodeRef, 
    const char * pszUsername,
    char ** ppszRealName,
    char ** ppszUserHomeDir,
    char ** ppszUserUID,
    char ** ppszUserGID
    )
{
    long macError = eDSNoErr;
    tDirStatus          junk;
    tRecordReference refRecord = NULL;
    tDataNodePtr pRealName = NULL;
    tDataNodePtr pGeneratedID = NULL;
    tDataNodePtr pUniqueID = NULL;
    tDataNodePtr pPrimaryGroup = NULL;
    tDataNodePtr pHomeDirectory = NULL;
    tDataNodePtr pRecordName = NULL;
    tDataNodePtr pRecordTypeUser = NULL;
    tAttributeValueEntryPtr pValueEntry = NULL;
    char * pszRealName = NULL;
    char * pszHomeDir = NULL;
    char * pszUserUID = NULL;
    char * pszUserGID = NULL;

    if (!strcmp(pszUsername, "root"))
    {
        // Oops, this is root. Skip
        macError = eDSInvalidRecordName;
        goto exit;
    }
    
    pRecordName = dsDataNodeAllocateString(hDirRef, pszUsername);
    if (!pRecordName)
    {
        macError = eDSAllocationFailed;
        goto exit;
    }
    
    pRecordTypeUser = dsDataNodeAllocateString(hDirRef, kDSStdRecordTypeUsers);
    if (!pRecordTypeUser)
    {
        macError = eDSAllocationFailed;
        goto exit;
    }

    pRealName = dsDataNodeAllocateString(hDirRef, kDS1AttrDistinguishedName);
    if (!pRealName)
    {
        macError = eDSAllocationFailed;
        goto exit;
    }
    
    pGeneratedID = dsDataNodeAllocateString(hDirRef, kDS1AttrGeneratedUID);
    if (!pGeneratedID)
    {
        macError = eDSAllocationFailed;
        goto exit;
    }

    pUniqueID = dsDataNodeAllocateString(hDirRef, kDS1AttrUniqueID);
    if (!pUniqueID)
    {
        macError = eDSAllocationFailed;
        goto exit;
    }

    pPrimaryGroup = dsDataNodeAllocateString(hDirRef, kDS1AttrPrimaryGroupID);
    if (!pPrimaryGroup)
    {
        macError = eDSAllocationFailed;
        goto exit;
    }

    pHomeDirectory = dsDataNodeAllocateString(hDirRef, kDS1AttrNFSHomeDirectory);
    if (!pHomeDirectory)
    {
        macError = eDSAllocationFailed;
        goto exit;
    }
    
    // Get record by name
    macError = dsOpenRecord(hNodeRef,
                            pRecordTypeUser,
                            pRecordName,
                            &refRecord);
    if (macError) goto exit;

    // Generated ID
    macError = dsGetRecordAttributeValueByIndex(refRecord, pGeneratedID, 1, &pValueEntry);
    if (macError) goto exit;
    
    if (pValueEntry->fAttributeValueData.fBufferLength)
    {
        dsDeallocAttributeValueEntry(hDirRef, pValueEntry);
        pValueEntry = NULL;
    }
    else
    {
        macError = eDSInvalidAttributeType;
        goto exit;
    }

    // RealName
    macError = dsGetRecordAttributeValueByIndex(refRecord, pRealName, 1, &pValueEntry);
    if (macError) goto exit;
    
    if (pValueEntry->fAttributeValueData.fBufferLength > 0)
    {
        pszRealName = (char*) malloc(pValueEntry->fAttributeValueData.fBufferLength + 1);
        if (!pszRealName)
        {
            macError = eDSAllocationFailed;
            goto exit;
        }
        memset(pszRealName, 0, pValueEntry->fAttributeValueData.fBufferLength + 1);
        
        strcpy(pszRealName, pValueEntry->fAttributeValueData.fBufferData);
        dsDeallocAttributeValueEntry(hDirRef, pValueEntry);
        pValueEntry = NULL;
    }
    else
    {
        macError = eDSInvalidAttributeType;
        goto exit;
    }

    // UniqueID
    macError = dsGetRecordAttributeValueByIndex(refRecord, pUniqueID, 1, &pValueEntry);
    if (macError) goto exit;
    
    if (pValueEntry->fAttributeValueData.fBufferLength > 0)
    {
        if (!strcmp(pValueEntry->fAttributeValueData.fBufferData, "0"))
        {
            // Oops, this is root's UID. Skip
            macError = eDSInvalidRecordName;
            goto exit;
        }
        
        pszUserUID = (char*) malloc(pValueEntry->fAttributeValueData.fBufferLength + 1);
        if (!pszUserUID)
        {
            macError = eDSAllocationFailed;
            goto exit;
        }
        memset(pszUserUID, 0, pValueEntry->fAttributeValueData.fBufferLength + 1);
        
        strcpy(pszUserUID, pValueEntry->fAttributeValueData.fBufferData);
        dsDeallocAttributeValueEntry(hDirRef, pValueEntry);
        pValueEntry = NULL;
    }
    else
    {
        macError = eDSInvalidAttributeType;
        goto exit;
    }

    // PrimaryGroup
    macError = dsGetRecordAttributeValueByIndex(refRecord, pPrimaryGroup, 1, &pValueEntry);
    if (macError) goto exit;
    
    if (pValueEntry->fAttributeValueData.fBufferLength > 0)
    {
        pszUserGID = (char*) malloc(pValueEntry->fAttributeValueData.fBufferLength + 1);
        if (!pszUserGID)
        {
            macError = eDSAllocationFailed;
            goto exit;
        }
        memset(pszUserGID, 0, pValueEntry->fAttributeValueData.fBufferLength + 1);
        
        strcpy(pszUserGID, pValueEntry->fAttributeValueData.fBufferData);
        dsDeallocAttributeValueEntry(hDirRef, pValueEntry);
        pValueEntry = NULL;
    }
    else
    {
        macError = eDSInvalidAttributeType;
        goto exit;
    }   

    // HomeDirectory
    macError = dsGetRecordAttributeValueByIndex(refRecord, pHomeDirectory, 1, &pValueEntry);
    if (macError) goto exit;
    
    if (pValueEntry->fAttributeValueData.fBufferLength > 0)
    {
        pszHomeDir = (char*) malloc(pValueEntry->fAttributeValueData.fBufferLength + 1);
        if (!pszHomeDir)
        {
            macError = eDSAllocationFailed;
            goto exit;
        }
        memset(pszHomeDir, 0, pValueEntry->fAttributeValueData.fBufferLength + 1);
        
        strcpy(pszHomeDir, pValueEntry->fAttributeValueData.fBufferData);
        dsDeallocAttributeValueEntry(hDirRef, pValueEntry);
        pValueEntry = NULL;
    }
    else
    {
        macError = eDSInvalidAttributeType;
        goto exit;
    }
        
    *ppszRealName = pszRealName;
    pszRealName = NULL;
    
    *ppszUserHomeDir = pszHomeDir;
    pszHomeDir = NULL;
    
    *ppszUserUID = pszUserUID;
    pszUserUID = NULL;
    
    *ppszUserGID = pszUserGID;
    pszUserGID = NULL;
    
cleanup:
    
    if (pRealName)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pRealName);
    }
        
    if (pGeneratedID)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pGeneratedID);
    }
    
    if (pUniqueID)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pUniqueID);
    }
    
    if (pPrimaryGroup)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pPrimaryGroup);
    }
    
    if (pHomeDirectory)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pHomeDirectory);
    }
    
    if (pRecordName)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pRecordName);
    }

    if (pRecordTypeUser)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pRecordTypeUser);
    }
    
    if (pValueEntry)
    {
        junk = dsDeallocAttributeValueEntry(hDirRef, pValueEntry);
    }
    
    if (refRecord)
    {
        junk = dsCloseRecord(refRecord);
    }
    
    if (pszRealName)
    {
        free(pszRealName);
    }
        
    if (pszHomeDir)
    {
        free(pszHomeDir);
    }
    
    if (pszUserUID)
    {
        free(pszUserUID);
    }
    
    if (pszUserGID)
    {
        free(pszUserGID);
    }
    
    return macError;
    
exit:    

    *ppszRealName = NULL;
    *ppszUserHomeDir = NULL;
    *ppszUserUID = NULL;
    *ppszUserGID = NULL;
   
    goto cleanup;
}

static
bool
CreateUserNode(
    tDirReference hDirRef,
    tDirNodeReference hNodeRef,
    tDataBufferPtr pDataBuffer,
    unsigned int index,
    PUSER_LIST * ppUser
    )
{
    long macError = eDSNoErr;
    char * pszRecordName = NULL;
    tAttributeListRef refAttrList = NULL;
    tRecordEntryPtr pRecord = NULL; 
    PUSER_LIST pUser = NULL;
    
    // Create the new user node for the list
    pUser = (PUSER_LIST) malloc(sizeof(USER_LIST));
    if (!pUser)
    {
        macError = eDSAllocationFailed;
        if (macError) goto exit;
    }
    pUser->pszUsername = NULL;
    pUser->pszUserRealName = NULL;
    pUser->pszUserHomeDir = NULL;
    pUser->pszUserUID = NULL;
    pUser->pszUserGID = NULL;
    pUser->pNext = NULL;
    
    // Get record from buffer
    macError = dsGetRecordEntry(hNodeRef, 
                                pDataBuffer,
                                index, // start count at 1
                                &refAttrList,
                                &pRecord);
    if (macError) goto exit;

    // Get the record name
    macError = dsGetRecordNameFromEntry(pRecord, &pszRecordName);
    if (macError) goto exit;
    
    if (!pszRecordName)
    {
        macError = eDSAttributeValueNotFound;
        if (macError) goto exit;
    }
    
    pUser->pszUsername = pszRecordName;
    pszRecordName = NULL;

    macError = GetUserInfo(hDirRef,
                           hNodeRef,
                           pUser->pszUsername,
                           &pUser->pszUserRealName,
                           &pUser->pszUserHomeDir,
                           &pUser->pszUserUID,
                           &pUser->pszUserGID);
    if (macError) goto exit;

    *ppUser = pUser;
    pUser = NULL;

cleanup:

    if (pUser)
    {
        FreeLocalUserList(pUser);
    }

    return (*ppUser != NULL);
    
exit:

   *ppUser = NULL;
   
   goto cleanup;
}

long
GetADUserInfo(
    const char * pszUsername,
    char ** ppszRealName,
    char ** ppszUserHomeDir,
    char ** ppszUserUID,
    char ** ppszUserGID
    )
{
    long              macError = eDSNoErr;
    tDirReference     hDirRef = NULL;
    tDirNodeReference hNodeRef = NULL;
    tDataListPtr      pLikewiseNodePath = NULL;
    char *            pszRealName = NULL;
    char *            pszHomeDir = NULL;
    char *            pszUserUID = NULL;
    char *            pszUserGID = NULL;
        
    macError = dsOpenDirService(&hDirRef);
    if (macError) goto exit;

    macError = GetLikewiseNodePathList(hDirRef, &pLikewiseNodePath);
    if (macError) goto exit;

    macError = dsOpenDirNode(hDirRef, pLikewiseNodePath, &hNodeRef);
    if (macError) goto exit;
        
    macError = GetUserInfo(hDirRef,
                           hNodeRef,
                           pszUsername,
                           &pszRealName,
                           &pszHomeDir,
                           &pszUserUID,
                           &pszUserGID);
    if (macError) goto exit;
        
    *ppszRealName = pszRealName;
    pszRealName = NULL;
    
    *ppszUserHomeDir = pszHomeDir;
    pszHomeDir = NULL;
    
    *ppszUserUID = pszUserUID;
    pszUserUID = NULL;
    
    *ppszUserGID = pszUserGID;
    pszUserGID = NULL;
    
cleanup:
        
    if (pszRealName)
    {
        free(pszRealName);
    }
        
    if (pszHomeDir)
    {
        free(pszHomeDir);
    }
    
    if (pszUserUID)
    {
        free(pszUserUID);
    }
    
    if (pszUserGID)
    {
        free(pszUserGID);
    }
    
    if (pLikewiseNodePath)
    {
        dsDataListDeallocate(hDirRef, pLikewiseNodePath);
    }
    
    if (hNodeRef)
    {
        dsCloseDirNode(hNodeRef);
    }

    if (hDirRef)
    {
        dsCloseDirService(hDirRef);
    }
    
    return macError;
    
exit:    

    *ppszRealName = NULL;
    *ppszUserHomeDir = NULL;
    *ppszUserUID = NULL;
    *ppszUserGID = NULL;
   
    goto cleanup;
}

long
GetLocalUserList(
    PUSER_LIST * ppLocalUsers
    )
{
    long macError = eDSNoErr;
    
    PUSER_LIST pLocalUsers = NULL;
    PUSER_LIST pUser = NULL, pPrev = NULL;

    tDirReference       hDirRef = NULL;
    tDirNodeReference   hNodeRef = NULL;
    tRecordReference    hRecordRef = NULL;
    tDataListPtr        pLocalNodePath = NULL;
    tDataList           recordTypeUsers;
    tDataList           recordsAll;
    tDataList           attrTypeAll;
    tAttributeValueEntryPtr pValueEntry = NULL;
    long unsigned       record_count = 0;
    tDataBufferPtr      pDataBuffer = NULL;
    tContextData        pContinuationData = NULL;
    
    macError = dsBuildListFromStringsAlloc(hDirRef, &attrTypeAll, kDSAttributesAll, NULL);
    if (macError) goto error;  
    
    macError = dsBuildListFromStringsAlloc(hDirRef, &recordTypeUsers, kDSStdRecordTypeUsers, NULL);
    if (macError) goto error;

    macError = dsBuildListFromStringsAlloc(hDirRef, &recordsAll, kDSRecordsAll, NULL);
    if (macError) goto error;
    
    macError = dsOpenDirService(&hDirRef);
    if (macError) goto error;
    
    pDataBuffer = dsDataBufferAllocate(hDirRef, 1024);
    if (pDataBuffer == NULL)
    {
        macError = eDSAllocationFailed;
        if (macError) goto error;
    }

    macError = GetLocalNodePathList(hDirRef, &pLocalNodePath);
    if (macError) goto error;

    macError = dsOpenDirNode(hDirRef, pLocalNodePath, &hNodeRef);
    if (macError) goto error;
    
    do
    {
        // Reset the counter for the next read if we are continuing on
        record_count = 0;
        
        do
        {
            if (macError == eDSBufferTooSmall)
            {
                // double the buffer unless it's time to give up
                int bufferSize = pDataBuffer->fBufferSize;
                if (bufferSize > 1024*1024)
                    break;
                dsDataBufferDeAllocate(hDirRef, pDataBuffer);
                // I mean, really _dataBufferPtr = NULL;
                pDataBuffer = dsDataBufferAllocate(hDirRef, bufferSize*2);
                if(pDataBuffer == NULL)
                    break;
            }
    
            macError = dsGetRecordList(hNodeRef,
                                       pDataBuffer,
                                       &recordsAll,
                                       eDSExact,
                                       &recordTypeUsers,
                                       &attrTypeAll,  /* all attributes (NULL is documented to be legal but that's a documentation bug */
                                       false,         /* attr info and data */
                                       &record_count,
                                       &pContinuationData);

        } while (macError == eDSBufferTooSmall);

        if (record_count > 0) 
        {
            unsigned int i;
            for (i=1; i <= record_count; i++)
            {
                if (CreateUserNode(hDirRef, hNodeRef, pDataBuffer, i, &pUser))
                {
                    if (pPrev)
                    {
                        pPrev->pNext = pUser;
                    }
                    else
                    {
                        pLocalUsers = pUser;
                    }
                    
                    pPrev = pUser;
                    pUser = NULL;
                }
            }
        }
    } while (pContinuationData != NULL);

    *ppLocalUsers = pLocalUsers;
    pLocalUsers = NULL;
    
cleanup:

    // Node structure is part of the stack, each may contain an allocation
    dsDataListDeallocate(hDirRef, &recordsAll);
    dsDataListDeallocate(hDirRef, &recordTypeUsers);
    dsDataListDeallocate(hDirRef, &attrTypeAll);
    
    if (pLocalNodePath)
    {
        dsDataListDeallocate(hDirRef, pLocalNodePath);
    }
    
    if (pValueEntry)
    {
        dsDeallocAttributeValueEntry(hDirRef, pValueEntry);
    }
    
    if (pDataBuffer)
    {
        dsDataBufferDeAllocate(hDirRef, pDataBuffer);
    }
    
    if (hRecordRef)
    {
        dsCloseRecord(hRecordRef);
    }
    
    if (hNodeRef)
    {
        dsCloseDirNode(hNodeRef);
    }

    if (hDirRef)
    {
        dsCloseDirService(hDirRef);
    }

    if (pLocalUsers)
    {
        FreeLocalUserList(pLocalUsers);
    }
    
    return macError;
    
error:

    *ppLocalUsers = NULL;

    goto cleanup;
}

void
FreeLocalUserList(
    PUSER_LIST pLocalUsers
    )
{
    while (pLocalUsers)
    {
        PUSER_LIST pTemp = pLocalUsers;
        pLocalUsers = pLocalUsers->pNext;
        
        if (pTemp->pszUsername)
        {
            free(pTemp->pszUsername);
        }
        
        if (pTemp->pszUserRealName)
        {
            free(pTemp->pszUserRealName);
        }
        
        if (pTemp->pszUserUID)
        {
            free(pTemp->pszUserUID);
        }
        
        if (pTemp->pszUserGID)
        {
            free(pTemp->pszUserGID);
        }
        
        if (pTemp->pszUserHomeDir)
        {
            free(pTemp->pszUserHomeDir);
        }

        free(pTemp);
    }
}

long
CallCommandWithOutputAndErr(
    const char *  pszCommand,
    const char ** ppszArgs,
    bool          bCaptureStderr,
    char **       ppszOutput,
    int *         pExitCode
    )
{
    long         macError = eDSNoErr;  
    unsigned int buffer_size = 1024;
    unsigned int read_size, write_size;
    int out[2];
    int pid, status;
    char * pszTempOutput = NULL;

    if(ppszOutput != NULL)
        *ppszOutput = NULL;

    if (pipe(out))
    {
        macError = errno;
        if (macError) goto exit;
    }

    pid = fork();

    if (pid < 0)
    {
        macError = errno;
        if (macError) goto exit;
    }
    else if (pid == 0)
    {
        // Child process
        if (dup2(out[1], STDOUT_FILENO) < 0)
            abort();
        if (bCaptureStderr && dup2(out[1], STDERR_FILENO) < 0)
            abort();
        if (close(out[0]))
            abort();
        if (close(out[1]))
            abort();
        execvp(pszCommand, (char **)ppszArgs);
    }

    if (close(out[1]))
    {
        macError = errno;
        if (macError) goto exit;
    }

    pszTempOutput = (char *) malloc(buffer_size);
    if (!pszTempOutput)
    {
        macError = eDSAllocationFailed;
        goto exit;
    }
    memset(pszTempOutput, 0, buffer_size);

    write_size = 0;

    while ((read_size = read(out[0], pszTempOutput + write_size, buffer_size - write_size)) > 0)
    {
        write_size += read_size;
        if (write_size == buffer_size)
        {
            buffer_size *= 2;
            free(pszTempOutput);
            pszTempOutput = (char *) malloc(buffer_size);
            if (!pszTempOutput)
            {
                macError = eDSAllocationFailed;
                goto exit;
            }
            memset(pszTempOutput, 0, buffer_size);
        }
    }

    if (read_size < 0)
    {
        macError = errno;
        if (macError) goto exit;
    }

    if (close(out[0]))
    {
        macError = errno;
        if (macError) goto exit;
    }

    if (waitpid(pid, &status, 0) != pid)
    {
        macError = errno;
        if (macError) goto exit;
    }

    if(ppszOutput != NULL)
    {
        *ppszOutput = pszTempOutput;
        pszTempOutput = NULL;
    }

    if (pExitCode != NULL)
        *pExitCode = WEXITSTATUS(status);
    else if (status)
    {
        macError = eDSOperationFailed;
        goto exit;
    }

exit:

    if (pszTempOutput)
    {
        free(pszTempOutput);
    }
    
    return macError;
}

