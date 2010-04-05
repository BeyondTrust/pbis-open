#include "includes.h"

#define LWIGNOME_GPITEM_NAME            "Gconf Configuration Settings"

#define LWIGNOME_SETTING_TAG            "setting"
#define LWIGNOME_ENTRY_TAG              "entry"
#define LWIGNOME_LI_TAG                 "li"
#define LWIGNOME_STRING_VALUE_TAG       "stringvalue"

#define LWIGNOME_PATH_ATTR_NAME         "path"
#define LWIGNOME_ENTRY_ATTR_NAME        "name"
#define LWIGNOME_ENTRY_ATTR_MTIME       "mtime"
#define LWIGNOME_ENTRY_ATTR_TYPE        "type"
#define LWIGNOME_ENTRY_ATTR_LTYPE       "ltype"
#define LWIGNOME_ENTRY_ATTR_VALUE       "value"
#define LWIGNOME_STRING                 "string"
#define LWIGNOME_INT                    "int"

#define LWIGNOME_USR_GCONF_LWI_MAN      "xml:readwrite:$(HOME)/.gconf.xml.lwi.mandatory"

static
CENTERROR
ApplyGconfSetttings (
    PSTR pszUser,
    PGNOME_POLICY pGNomeSetting
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCSTR gconfFormat = NULL;
    PSTR gconfCommand = NULL;
    PSTR escapedValue = NULL;
    char path[PATH_MAX + 1];

    sprintf( path, 
             "%s/%s",
             pGNomeSetting->pszDirName,
             pGNomeSetting->pszEntryName);

    if (IsValidGconfPath(path)) {
        ceError = LwEscapeString(pGNomeSetting->pszValue, &escapedValue);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if( pGNomeSetting->pszLType ) {
            gconfFormat = "gconftool-2 --direct --config-source=xml:merged:%s --type %s --list-type %s --set %s [%s] >/dev/null";
            ceError = LwAllocateStringPrintf( &gconfCommand,
                                              gconfFormat,
                                              pszUser,
                                              pGNomeSetting->pszType,
                                              pGNomeSetting->pszLType,
                                              path,
                                              escapedValue
                                            );
            BAIL_ON_CENTERIS_ERROR(ceError);

        } else {
            gconfFormat = "gconftool-2 --direct --config-source=xml:merged:%s --type %s --set %s \"%s\" >/dev/null";
            ceError = LwAllocateStringPrintf( &gconfCommand, 
                                              gconfFormat,
                                              pszUser,
                                              pGNomeSetting->pszType,
                                              path,
                                              escapedValue
                                             );
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = GPARunCommand(gconfCommand);
        BAIL_ON_CENTERIS_ERROR(ceError);

        GPA_LOG_VERBOSE("Updating gconf policy with command: %s", gconfCommand);
    } else {
        GPA_LOG_VERBOSE("Warning: GConf schema does not exist for user setting with path (%s) on this computer, going to skip this setting.", path);
    }

error:

    LW_SAFE_FREE_STRING(escapedValue);
    LW_SAFE_FREE_STRING(gconfCommand);
    return ceError;
}

static
CENTERROR
ParseXMLTree(
    DWORD dwPolicyType,
    PSTR pszUser,
    xmlNodePtr root_node,
    PGNOME_POLICY pGnomeSettings
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    xmlNodePtr cur_node = NULL;

    struct _xmlAttr *pAttrNode = NULL;

    static BOOLEAN bList = FALSE;

    for ( cur_node = root_node; cur_node; cur_node = cur_node->next ) {

        if (cur_node->name) {
            if ( !strcmp( (const char*)cur_node->name,
                          (const char*)LWIGNOME_SETTING_TAG)) {

                xmlChar *pszVal = NULL;
                pszVal = xmlGetProp( cur_node, 
                                     (const xmlChar*)LWIGNOME_PATH_ATTR_NAME );
                /* Get the path */				
                strcpy( (char*)pGnomeSettings->pszDirName,(const char*)pszVal);
                xmlFree(pszVal);
            }
            else if ( !strcmp( (const char*)cur_node->name,
                               (const char*)LWIGNOME_ENTRY_TAG)) {

                if( pGnomeSettings->pszLType && bList ) {
                    ceError = ApplyGconfSetttings ( pszUser,
                                                    pGnomeSettings);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    LW_SAFE_FREE_STRING(pGnomeSettings->pszLType);
                    pGnomeSettings->pszLType = NULL;

                    bList = FALSE;
                }

                /* Parse the entry tag attributes */
                pAttrNode = cur_node->properties;

                for (; pAttrNode; pAttrNode = pAttrNode->next) {
                    if ( !strcmp( (char*)pAttrNode->name,
                                  (char*)LWIGNOME_ENTRY_ATTR_NAME)) {
                        strcpy( (char*)pGnomeSettings->pszEntryName,
                                (char*)pAttrNode->children->content);
                    }
                    else if ( !strcmp( (char*)pAttrNode->name,
                                       (char*)LWIGNOME_ENTRY_ATTR_MTIME)) {
                        strcpy( (char*)pGnomeSettings->pszMTime,
                                (char*)pAttrNode->children->content);
                    }
                    else if ( !strcmp( (char*)pAttrNode->name,
                                       (char*)LWIGNOME_ENTRY_ATTR_TYPE)) {
                        strcpy( (char*)pGnomeSettings->pszType,
                                (char*)pAttrNode->children->content);
                    }
                    else if ( !strcmp( (char*)pAttrNode->name,
                                       (char*)LWIGNOME_ENTRY_ATTR_VALUE)) {
                        strcpy( (char*)pGnomeSettings->pszValue,
                                (char*)pAttrNode->children->content);

                        if(strcmp( (char*)pGnomeSettings->pszType,
                                   (char*)LWIGNOME_STRING)!= 0){
                            ceError = ApplyGconfSetttings ( pszUser,
                                                            pGnomeSettings);
                            BAIL_ON_CENTERIS_ERROR(ceError);
                        }
                    }
                    else if ( !strcmp( (char*)pAttrNode->name,
                                       (char*)LWIGNOME_ENTRY_ATTR_LTYPE)) {

                        ceError = LwAllocateMemory( BUFFER_SIZE,
                                                    (PVOID *)&pGnomeSettings->pszLType);
                        BAIL_ON_CENTERIS_ERROR(ceError);

                        strcpy( (char*)pGnomeSettings->pszLType,
                                (char*)pAttrNode->children->content);
                    }
                }
            }
            else if ( !strcmp( (const char*)cur_node->name,
                               (const char*)LWIGNOME_STRING_VALUE_TAG)) {
               if( !bList && pGnomeSettings->pszLType ) {
                    bList = TRUE;
                    strcpy( (char*)pGnomeSettings->pszValue,
                            (char*)cur_node->children->content);
                }
                else if( bList && pGnomeSettings->pszLType ) {
                    strcat( (char*)pGnomeSettings->pszValue,
                            (char*)",");
                    strcat( (char*)pGnomeSettings->pszValue,
                            (char*)cur_node->children->content);
                }
                else {
                    strcpy( (char*)pGnomeSettings->pszValue,
                            (char*)cur_node->children->content);
                    ceError = ApplyGconfSetttings ( pszUser,
                                                    pGnomeSettings);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
            }
            else if ( !strcmp( (const char*)cur_node->name,
                               (const char*)LWIGNOME_LI_TAG)           &&
                      (pGnomeSettings->pszLType                        &&
                      !strcmp( (const char*)pGnomeSettings->pszLType,
                               (const char*)LWIGNOME_INT))){
                if( !bList && pGnomeSettings->pszLType ) {
                    bList = TRUE;
                    strcpy( (char*)pGnomeSettings->pszValue,
                            (PSTR)xmlGetProp( cur_node,
                                              (const xmlChar*)LWIGNOME_ENTRY_ATTR_VALUE));
                }
                else if( bList && pGnomeSettings->pszLType ) {
                    strcat( (char*)pGnomeSettings->pszValue,
                            (char*)",");
                    strcat( (char*)pGnomeSettings->pszValue,
                            (PSTR)xmlGetProp( cur_node,
                                              (const xmlChar*)LWIGNOME_ENTRY_ATTR_VALUE));
                }
            }
            else if ( !strcmp( (const char*)cur_node->parent->name,
                               (const char*)LWIGNOME_SETTING_TAG) &&
                      bList) {

                ceError = ApplyGconfSetttings ( pszUser,
                                                pGnomeSettings);
                BAIL_ON_CENTERIS_ERROR(ceError);

                LW_SAFE_FREE_STRING(pGnomeSettings->pszLType);
                pGnomeSettings->pszLType = NULL;

                bList = FALSE;
            }
        }

        ParseXMLTree( dwPolicyType,
                      pszUser,
                      cur_node->children,
                      pGnomeSettings);
    }
   
error:

    return ceError;
    
}

CENTERROR
ApplyGnomePolicy(
    DWORD dwPolicyType,
    PSTR pszUser,
    PGPOLWIGPITEM pGPItem,
    PGNOME_POLICY pGnomePolicy
    )
{

    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = LwAllocateMemory( BUFFER_SIZE,
                                (PVOID *)&pGnomePolicy->pszDirName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( BUFFER_SIZE,
                                (PVOID *)&pGnomePolicy->pszEntryName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( BUFFER_SIZE,
                                (PVOID *)&pGnomePolicy->pszMTime);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( BUFFER_SIZE,
                                (PVOID *)&pGnomePolicy->pszType);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( BUFFER_SIZE,
                                (PVOID *)&pGnomePolicy->pszValue);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pGnomePolicy->pszLType = NULL;

    ceError = ParseXMLTree( dwPolicyType,
                            pszUser,
                            (xmlNodePtr)pGPItem->xmlNode,
                            pGnomePolicy);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    LW_SAFE_FREE_STRING(pGnomePolicy->pszDirName);
    LW_SAFE_FREE_STRING(pGnomePolicy->pszEntryName);
    LW_SAFE_FREE_STRING(pGnomePolicy->pszMTime);
    LW_SAFE_FREE_STRING(pGnomePolicy->pszType);
    LW_SAFE_FREE_STRING(pGnomePolicy->pszValue);

    return ceError;
}


static
CENTERROR 
CreateFile (
    PSTR pszDir,
    PSTR pszFilePath,
    PSTR pszIncludeString,
    PSTR pszIncludeString2
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;
    FILE *fp = NULL;

    ceError = GPACheckDirectoryExists( pszDir,
                                      &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(!bFileExists) {
        ceError = LwCreateDirectory( pszDir,
                                     DIR_PERMS);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPACheckFileExists( pszFilePath,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(!bFileExists) {
        //Create the new file
        ceError = GPAOpenFile( pszFilePath,
                              "w",
                              &fp);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else {
        //Open the file, and append our entry
        ceError = GPAOpenFile( pszFilePath,
                              "a",
                              &fp);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fprintf( fp,
             "%s\n",
             pszIncludeString);

    if ( pszIncludeString2 ) {
        fprintf( fp,
                 "%s\n",
                 pszIncludeString2);
    }

error:

    if(fp) {
        fclose(fp);
    }

    return ceError;
}

static
CENTERROR
CheckForFileValue (
    PSTR pszFilePath,
    PSTR pszIncludeString
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szCommand[PATH_MAX + 1];
    char* output = NULL;

    sprintf( szCommand, 
             "cat %s | grep \"%s\"",
             pszFilePath,
             pszIncludeString);

    ceError = GPACaptureOutput( szCommand, &output );
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if ( output ) {
        LwFreeString( output );
    }

    return ceError;
}

static
CENTERROR
AppendGconfInfo(
    PSTR pszFilePath,
    PSTR pszPattern,
    PSTR pszAppendStr
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bPatternExists = FALSE;
    FILE *fp = NULL;

    ceError = GPACheckFileHoldsPattern( pszFilePath,
                                       pszPattern,
                                       &bPatternExists);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if(!bPatternExists) {
        ceError = GPAOpenFile( pszFilePath,
                              "a",
                              &fp);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if(!pszAppendStr){
            ceError = GPAFilePrintf( fp,
                                    "%s\n",
                                    pszPattern);
        } else {
            ceError = GPAFilePrintf( fp,
                                    "%s\n",
                                    pszAppendStr);

        }

        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPACloseFile(fp);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
    }

error:

    return ceError;
}

CENTERROR
UpdatePathFileForMachine ( PSTR szGconfDir)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;
    char szPath[PATH_MAX+1] = "";
    char szAddStringLwi[PATH_MAX+1] = "";
    char szManDir[PATH_MAX+1] = "";

    memset(szPath, 0, sizeof(szPath));
    memset(szManDir, 0, sizeof(szManDir));
    memset(szAddStringLwi, 0, sizeof(szAddStringLwi));

    //we need /etc/gconf from /etc/gconf/2/, so strip off /2/
    strncpy( szManDir,
             szGconfDir,
             (strlen(szGconfDir)-2));

    sprintf( szPath,
             "%s%s",
             szGconfDir,
             LWIGNOME_GCONF_PATH);

    sprintf( szAddStringLwi,
             "%s%s%s",
             LWIGNOME_ADD_USR_GCONF_MANPATH,
             szManDir,
             LWIGNOME_MAN_DIR );

    //Check to see if path file exists or not
    ceError = GPACheckFileExists( szPath, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    //If file exists then append the policy else create a new file
    if(bFileExists) {
        ceError = AppendGconfInfo(szPath, szAddStringLwi,NULL);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else {
        //create the /etc/opt/gnome/gconf/2/path file
        ceError = CreateFile(szGconfDir, szPath, szAddStringLwi, NULL);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    return ceError;
}

CENTERROR
InsertMandatoryPath (
    PSTR pszFileName,
    PSTR pszPath
    )

{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE *fp   = NULL;
    FILE *fpgp = NULL;
    PSTR pszLine = NULL;
    PSTR pszTmpLine = NULL;
    PSTR pszToken = NULL;
    PSTR pszLast = NULL;
    PSTR pszEnd = NULL;
    BOOLEAN bMatchFound = FALSE;
    CHAR szBackupPath[PATH_MAX+1];


    ceError = LwAllocateMemory( PATH_MAX+1,
                                (PVOID*)&pszLine);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( PATH_MAX+1,
                                (PVOID*)&pszTmpLine);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf( szBackupPath, 
             "%s_gp",
             pszFileName);

    ceError = GPAOpenFile( pszFileName, 
                          "r",
                          &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);


    ceError = GPAOpenFile( szBackupPath,
                          "w",
                          &fpgp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while( fgets( pszLine,
                  PATH_MAX,
                  fp)) {

        LwStripWhitespace(pszLine,1,1);

        if( strcspn(pszLine,"#") != 0 ) {

            strcpy(pszTmpLine,pszLine);

            pszToken = strtok_r(pszTmpLine,":",&pszLast);

            pszToken = pszLast + (strlen(pszLast) - 19);

            if( !strcmp(pszToken,"gconf.xml.mandatory") ) {
                ceError = GPAFilePrintf( fpgp,
                                        "%s\n",
                                         pszPath);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ceError = GPAFilePrintf( fpgp,
                                         "%s\n",
                                            pszLine);
                BAIL_ON_CENTERIS_ERROR(ceError);

                bMatchFound = TRUE;
            } 
            else {

                pszToken = strtok_r(pszTmpLine,"/",&pszEnd);

                pszToken = pszEnd + (strlen(pszEnd) - 20);

                if( strcmp(pszToken,"local-mandatory.path") ) {
                    ceError = GPAFilePrintf( fpgp,
                                            "%s\n",
                                            pszLine);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
           }

        }
        else {

            ceError = GPAFilePrintf( fpgp,
                                    "%s\n",
                                    pszLine);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    if( !bMatchFound) {
        ceError = GPAFilePrintf( fpgp,
                                "%s\n",
                                pszPath);
        BAIL_ON_CENTERIS_ERROR(ceError);

    }

    if(fpgp) {
        fclose(fpgp);
    }

    //Move the file
    ceError = GPAMoveFileAcrossDevices( szBackupPath,
                                       pszFileName );
    BAIL_ON_CENTERIS_ERROR(ceError);


error:

    if(fp) {
        fclose(fp);
    }

    LW_SAFE_FREE_STRING(pszLine);
    LW_SAFE_FREE_STRING(pszTmpLine);

    return ceError;
}

CENTERROR
UpdateMandatoryPathFile (
    PSTR szGconfDir,
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;
    char szPath[PATH_MAX+1];
    char szInclude[PATH_MAX+1];
    char szFileName[PATH_MAX+1];
    char szAddString[PATH_MAX+1];
    char szAddStringLwi[PATH_MAX+1];
    gid_t gid = 0;

    sprintf( szPath,
             "%s%s",
             szGconfDir,
             LWIGNOME_GCONF_PATH);

    sprintf( szInclude, 
             "include %s%s",
             szGconfDir,
             LWIGNOME_GCONF_MANDATORY_PATH);

    //Check to see if path file exists or not
    ceError = GPACheckFileExists( szPath, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    //If file exists then append the policy else create a new file
    if(bFileExists) {
        //Check whether "include /etc/opt/gnome/gconf/2/local-mandatory.path" is present in the file or not
        //parse the file
        //ceError = AppendGconfInfo(szPath, LWIGNOME_GCONF_MANDATORY_PATH,szInclude);
        //BAIL_ON_CENTERIS_ERROR(ceError);
        ceError = InsertMandatoryPath(szPath, szInclude);
        BAIL_ON_CENTERIS_ERROR(ceError);

    } else {
        //create the /etc/opt/gnome/gconf/2/path file
        ceError = CreateFile(szGconfDir, szPath, szInclude, NULL);
        BAIL_ON_CENTERIS_ERROR(ceError);

    }

    //FIX For BUG#5257
    ceError = AppendGconfInfo(szPath, LWIGNOME_IDT_LWI,LWIGNOME_USR_GCONF_LWI_MAN);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf( szPath,
             "%s%s",
             szGconfDir,
             LWIGNOME_GCONF_MANDATORY_PATH);

    //Now check to see if "/etc/opt/gnome/gconf/2/local-mandatory.path" is present
    ceError = GPACheckFileExists(szPath, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        //Check whether ".gconf.path.mandatory" is present in the file or not
        ceError = CheckForFileValue(szPath, LWIGNOME_HOME_MAN_PATH);
        if (ceError) {
            ceError = LwRemoveFile(szPath);
            BAIL_ON_CENTERIS_ERROR(ceError);
            bFileExists = FALSE;
        }
    }

    if (!bFileExists) {
        ceError = CreateFile(szGconfDir, szPath, LWIGNOME_INCLUDE_IN_MAN_PATH, NULL);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    //Check for "/home/<usr>/.gconf.path.mandatory"
    sprintf( szFileName,
             "%s/%s",
             pUser->pszHomeDir,
             LWIGNOME_HOME_MAN_PATH );

    sprintf( szAddString,
             "%s%s/%s",
             LWIGNOME_ADD_USR_GCONF_MANPATH,        
             pUser->pszHomeDir,
             LWIGNOME_HOME_IDT );

    sprintf( szAddStringLwi,
             "%s%s/%s",
             LWIGNOME_ADD_USR_GCONF_MANPATH,
             pUser->pszHomeDir,
             LWIGNOME_IDT_LWI );

    ceError = GPACheckFileExists(szFileName, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(bFileExists) {
        //Check whether "xml:readonly:/<user homedir path>/.gconf.xml.lwi.mandatory" in present in the file or not
        ceError = CheckForFileValue(szFileName, szAddStringLwi);
        if (ceError) {
            ceError = LwRemoveFile(szFileName);
            BAIL_ON_CENTERIS_ERROR(ceError);
            bFileExists = FALSE;
        }
    }

    if (!bFileExists) {
        //create the .gconf.path.mandatory file in the users homedir
        ceError = CreateFile(szGconfDir, szFileName, szAddStringLwi, szAddString);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

        
    //TODO: to find alternative 
    //ceError = CTGetUserGID( (PCSTR)pUser->pszName, &gid);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwChangeOwner( (PSTR)szFileName,
                             pUser->uid,
                             gid );
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

BOOLEAN
IsValidGconfPath(
    PCSTR path
    )
{
    static PCSTR gconfFormat = "gconftool-2 --get-schema-name %s";
    PSTR gconfCommand = NULL;
    PSTR pszOutput = NULL, pszFailed = NULL;
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = LwAllocateStringPrintf(&gconfCommand, gconfFormat, path);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACaptureOutputWithStderr(gconfCommand, TRUE, &pszOutput);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszFailed = strstr(pszOutput, "No schema");
    if (pszFailed != NULL) {
        ceError = CENTERROR_COMMAND_FAILED;
    }

error:

    LW_SAFE_FREE_STRING(gconfCommand);
    LW_SAFE_FREE_STRING(pszOutput);

    return (ceError == CENTERROR_SUCCESS);
}

