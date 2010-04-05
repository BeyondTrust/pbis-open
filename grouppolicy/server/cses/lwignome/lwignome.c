#include "includes.h"


CENTERROR
GPAGconfAvailable(
    PBOOLEAN Result
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    PSTR version = NULL;
    
    ceError = GPACaptureOutput("gconftool-2 -v | cut -d. -f2", &version);
    if (ceError)
    {
        *Result = FALSE;
        ceError = CENTERROR_SUCCESS;
        goto error;
    }
    
    *Result = atoi(version) >= 8;

error:

    LW_SAFE_FREE_STRING(version);

    return ceError;
}

CENTERROR
GPASignalGconfDaemon()
{
    static const char* signalCommand = "killall -HUP gconfd-2";
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    ceError = GPARunCommand(signalCommand);
    if (CENTERROR_EQUAL(ceError, CENTERROR_COMMAND_FAILED)) {
        // killall can fail if gconfd is not running,
        // but that's ok
        ceError = CENTERROR_SUCCESS;
    }
    return ceError;
}

static
CENTERROR
GetGconfDir( 
    PSTR pszGconfDir,
    PBOOLEAN pbDirExists
    )
{

    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN fDirExists = FALSE;

    strcpy(pszGconfDir, "/etc/gnome/gconf/2/");
    ceError = GPACheckDirectoryExists(pszGconfDir, &fDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( fDirExists == FALSE ) {
        strcpy(pszGconfDir, "/etc/opt/gnome/gconf/2/");
        ceError = GPACheckDirectoryExists(pszGconfDir, &fDirExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if ( fDirExists == FALSE ) {
            strcpy(pszGconfDir, "/etc/gconf/2/");
            ceError = GPACheckDirectoryExists(pszGconfDir, &fDirExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if ( fDirExists == FALSE ) {
                GPA_LOG_VERBOSE("Cannot determine gnome configuration information location, ignoring gnome policy");
                *pbDirExists = FALSE;
                goto error;
            }
        }
    }

error:
    return ceError;
}



CENTERROR
ResetGnomeGroupPolicy(
    PGPUSER pUser
    )
{
    return CENTERROR_SUCCESS;
}

static
PSTR
GetClientGUID( DWORD dwPolicyType )
{
    if(dwPolicyType == MACHINE_GROUP_POLICY)
        return LWIGNOME_MACHINE_CLIENT_GUID;
    else
        return LWIGNOME_USER_CLIENT_GUID;
}

static
PSTR
GetItemGUID( DWORD dwPolicyType )
{
    if(dwPolicyType == MACHINE_GROUP_POLICY)
        return LWIGNOME_MACHINE_ITEM_GUID;
    else
        return LWIGNOME_USER_ITEM_GUID;
}

static
void
GetGconfDirPath( 
    PGPUSER pUser,
    PSTR szGconfDir,
    PSTR pszTmpLoc,
    PSTR pszSettingDest )
{
    if(pUser) {
        //user policy
        sprintf( pszTmpLoc,
                 "%s/%s",
                 pUser->pszHomeDir,
                 LWIGNOME_IDT_LWI_NEW);

        sprintf( pszSettingDest,
                 "%s/%s",
                 pUser->pszHomeDir,
                 LWIGNOME_IDT_LWI);
    } else {
        //machine policy
        char szManDir[PATH_MAX+1] = "";

        memset(szManDir, 0, sizeof(szManDir));

        //we need /etc/gconf from /etc/gconf/2/, so strip off /2/
        strncpy( szManDir,
                 szGconfDir,
                 (strlen(szGconfDir)-2));

        sprintf( pszTmpLoc,
                 "%s%s",
                 szManDir, 
                 LWIGNOME_MAN_DIR_NEW);

        sprintf( pszSettingDest,
                 "%s%s",
                 szManDir,
                 LWIGNOME_MAN_DIR);
    }
}

static
CENTERROR
ApplySettings(
    PGPUSER pUser,
    DWORD dwPolicyType,
    PSTR pszDir,
    PSTR pszTmpDest,
    PGROUP_POLICY_OBJECT pGPO
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIGPITEM pGPItem = NULL;
    PGPOLWIDATA pLwidata = NULL;
    GNOME_POLICY gnomePolicy;

    // Look in new policy file location
    ceError = GPOInitLwiData( pUser,
                              dwPolicyType,
                              (PGPOLWIDATA*)&pLwidata,
                              pGPO->pszgPCFileSysPath,
                              pszDir,
                              GetClientGUID(dwPolicyType) );
    if ( ceError ) {
        // Look in old policy file location
        ceError = GPOInitLwiData( pUser,
                                  dwPolicyType,
                                  (PGPOLWIDATA*)&pLwidata,
                                  pGPO->pszgPCFileSysPath,
                                  NULL,
                                  NULL );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPOGetGPItem( dwPolicyType,
                            pLwidata,
                            GetItemGUID(dwPolicyType),
                            &pGPItem );

    if (!CENTERROR_IS_OK(ceError) &&
        CENTERROR_EQUAL(ceError, CENTERROR_GP_XML_GPITEM_NOT_FOUND)) {

        GPA_LOG_ALWAYS( "Gnome policy gpitem not found, returning ...");
        goto error;
    }

    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ApplyGnomePolicy( dwPolicyType, 
                                pszTmpDest,
                                pGPItem,
                                &gnomePolicy);
    BAIL_ON_CENTERIS_ERROR(ceError);


 error:

    if (pGPItem) {
       GPODestroyGPItem(pGPItem, FALSE);
    }

    if ( pLwidata ) {
        GPODestroyLwiData(pLwidata);
    }

    return ceError;
}

static
CENTERROR
ProcessMandatoryFile(
    PGPUSER pUser,
    PSTR pszGconfDir
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /*
     * Update the mandatory file "/etc/opt/gnome/gconf/2/path"
     * "local-mandatory.path" and "/home/<usr>/.gconf.path.mandatory"
     */
    if (pUser) {
        ceError = UpdateMandatoryPathFile( pszGconfDir, 
                                           pUser);
    } else {
        ceError = UpdatePathFileForMachine(pszGconfDir);
    }

    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
ProcessGnomePolicy(
    DWORD dwPolicyType,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN gconfAvailable = FALSE;

    char szTmpDest[PATH_MAX + 1];
    char szSettingDest[PATH_MAX + 1];
    char szHomeDirPolicies[PATH_MAX + 1];
    char szGconfDir[PATH_MAX + 1];
    BOOLEAN bDirExists = TRUE;
        
    ceError = GPAGconfAvailable(&gconfAvailable);
    BAIL_ON_CENTERIS_ERROR(ceError);
        
    if (!gconfAvailable) {
       GPA_LOG_VERBOSE("gconf not available or too old, ignoring gnome policy");
       goto error;
    }

    memset (szGconfDir,0,(PATH_MAX + 1));
    memset (szTmpDest,0,(PATH_MAX + 1));
    memset (szSettingDest,0,(PATH_MAX + 1));

    // Determine the gconf directory
    ceError = GetGconfDir( szGconfDir,
                           &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);
    if (!bDirExists) {
       goto error;
    }

    // Determine the .gconf.xml.lwi.mandtory path
    GetGconfDirPath( pUser,
                     szGconfDir,
                     szTmpDest,
                     szSettingDest);
    
    GPA_LOG_VERBOSE("Creating the directory %s",szTmpDest);
    //create .gconf.xml.lwi.mandatory.new directory
    ceError = LwCreateDirectory( szTmpDest,
                                 DIR_PERMS);
    BAIL_ON_CENTERIS_ERROR(ceError);

    //Get the path where policy xml to be placed, for user policy only
    memset (szHomeDirPolicies,0,(PATH_MAX + 1));
    if(dwPolicyType == USER_GROUP_POLICY){
        sprintf( szHomeDirPolicies,
                 "%s/.lwipolicies",
                 pUser->pszHomeDir );
    }


    // Process new gnome policies to add
    for (; pGPOModifiedList; pGPOModifiedList = pGPOModifiedList->pNext)  {
        BOOLEAN applicable;
        
        ceError = GPOXmlVerifyPlatformApplicable( pUser,
                                                  dwPolicyType, 
                                                  pGPOModifiedList,
                                                  szHomeDirPolicies,
                                                  &applicable);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if (!applicable)
        {
            GPA_LOG_VERBOSE("GPO(%s) disabled by platform targeting",
                            pGPOModifiedList->pszDisplayName);

            ceError = GPARemoveDirectory( szTmpDest );
            BAIL_ON_CENTERIS_ERROR(ceError);

            goto error;
        } else if (pGPOModifiedList->dwFlags & 0x00000002) {

            GPA_LOG_VERBOSE( "GPO(%s) is disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);

            ceError = GPARemoveDirectory( szTmpDest );
            BAIL_ON_CENTERIS_ERROR(ceError);

            goto error;
        } else {

            ceError = ApplySettings( pUser,
                                     dwPolicyType,
                                     szHomeDirPolicies,
                                     szTmpDest,
                                     pGPOModifiedList);

            if(CENTERROR_EQUAL(ceError, CENTERROR_GP_XML_GPITEM_NOT_FOUND)||
                CENTERROR_EQUAL(ceError, CENTERROR_GP_FILE_COPY_FAILED)) {
                GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
                ceError = GPARemoveDirectory( szTmpDest );
                BAIL_ON_CENTERIS_ERROR(ceError);
                goto error;
            }
        }
    }

    // apply Gnome settings 
    ceError = GPACheckDirectoryExists( szSettingDest,
                                      &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(bDirExists) {
        ceError = GPARemoveDirectory( szSettingDest );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    GPA_LOG_VERBOSE("Renaming the directory to %s",szSettingDest);
    if ( rename( szTmpDest,
                 szSettingDest) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (dwPolicyType == USER_GROUP_POLICY) {
        //Get GID
        gid_t gid = 0;
        
        GPA_LOG_VERBOSE("User name %s",pUser->pszName);
//TODO: to find alternative for getusergid
//        ceError = CTGetUserGID( (PCSTR)pUser->pszName, &gid);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwChangeOwner( (PSTR)szSettingDest,
                                 pUser->uid,
                                 gid);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = ProcessMandatoryFile( pUser, 
                                    (PSTR)szGconfDir);
    BAIL_ON_CENTERIS_ERROR(ceError);

    // Tell any running instances of gconfd to reload configuration
    ceError = GPASignalGconfDaemon();
    BAIL_ON_CENTERIS_ERROR(ceError);
        
error:

    GPA_LOG_VERBOSE("GP Extension exit code: %d", ceError);

    return(ceError);

}

CENTERROR
ProcessGnomeGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (pUser) {
       ceError = ProcessGnomePolicy( USER_GROUP_POLICY,
                                     pUser,
                                     pGPOModifiedList,
                                     pGPODeletedList);
    } else {
       ceError = ProcessGnomePolicy( MACHINE_GROUP_POLICY,
                                     NULL,
                                     pGPOModifiedList,
                                     pGPODeletedList);
    }

    return ceError;
}
