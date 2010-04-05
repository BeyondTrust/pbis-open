#include "includes.h"

PFSTAB_GPO_OBJECT pHead = NULL;
PFSTAB_OBJECT pFSTabHead = NULL;

static
CENTERROR
MountEntries()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOG_VERBOSE("Mounting fstab entries.");

#if defined(__LWI_AIX__)
    ceError = GPARunCommand((PSTR)"nfso -o nfs_use_reserved_ports=1 > /dev/null 2>&1");
    if (ceError != CENTERROR_SUCCESS) {
        GPA_LOG_VERBOSE( "There were problems in setting nfs reserved ports variable..");
        ceError = CENTERROR_SUCCESS;
    }
#endif
    ceError = GPARunCommand((PSTR)"mount -a > /dev/null 2>&1");
    if (ceError != CENTERROR_SUCCESS) {
        GPA_LOG_VERBOSE( "There were problems in mounting fstab entries.");
        ceError = CENTERROR_SUCCESS;
    }

    return ceError;
}

static
void
GetCIFSGUID( 
    PFSTAB_OBJECT pTemp,
    PSTR *pszDisplayName,
    PSTR *pszGPOGUID
)
{
    PFSTAB_GPO_OBJECT pGPOTemp = pHead;

    while (pGPOTemp) {
        if (!strcmp( (char*)pTemp->pszMountPt, 
                     (char*)pGPOTemp->pszMountPt)) {
            *pszDisplayName = pGPOTemp->pszDisplayName;
            *pszGPOGUID = pGPOTemp->pszGPOGUID;
        }
        pGPOTemp = pGPOTemp->pNext; 
    }
}

static 
CENTERROR
InitFSTabEntries(
    PFSTAB_OBJECT *pFSTabEntry
)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
#if defined(__LWI_AIX__)
    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(*pFSTabEntry)->pszSectionName);
    BAIL_ON_CENTERIS_ERROR(ceError);
    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(*pFSTabEntry)->pszAccount);
    BAIL_ON_CENTERIS_ERROR(ceError);
    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(*pFSTabEntry)->pszBoot);
    BAIL_ON_CENTERIS_ERROR(ceError);
    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(*pFSTabEntry)->pszMount);
    BAIL_ON_CENTERIS_ERROR(ceError);
    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(*pFSTabEntry)->pszNodeName);
    BAIL_ON_CENTERIS_ERROR(ceError);
    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(*pFSTabEntry)->pszSize);
    BAIL_ON_CENTERIS_ERROR(ceError);
    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(*pFSTabEntry)->pszType);
    BAIL_ON_CENTERIS_ERROR(ceError);
    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(*pFSTabEntry)->pszLog);
    BAIL_ON_CENTERIS_ERROR(ceError);
    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(*pFSTabEntry)->pszFree);
    BAIL_ON_CENTERIS_ERROR(ceError);
#endif 
    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(*pFSTabEntry)->pszMountVol);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(*pFSTabEntry)->pszMountPt);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(*pFSTabEntry)->pszFileType);
    BAIL_ON_CENTERIS_ERROR(ceError);
#if defined(__LWI_SOLARIS__)
    ceError = LwAllocateString( (char *)"-",
                                &((*pFSTabEntry)->pszDeviceToFsck));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( (char *)"-",
                                &((*pFSTabEntry)->pszFsckPass));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( (char *)"no",
                                &((*pFSTabEntry)->pszMountAtBoot));
    BAIL_ON_CENTERIS_ERROR(ceError);
#elif defined(__LWI_AIX__)
    /*NOTE: Currently we are hardcoding the value to zero */
    ceError = LwAllocateString( (char *)"true\n", 
                                &((*pFSTabEntry)->pszPassCode));
    BAIL_ON_CENTERIS_ERROR(ceError);
#else
    /*NOTE: Currently we are hardcoding the value to zero */
    ceError = LwAllocateString( (char *)"0", 
                                &((*pFSTabEntry)->pszFrequency));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( (char *)"0", 
                                &((*pFSTabEntry)->pszPassCode));
    BAIL_ON_CENTERIS_ERROR(ceError);
#endif

error:
    return ceError;

}

static 
void
FreeFSTabEntries(
    PFSTAB_OBJECT *pFSTabEntry
)
{
#if defined(__LWI_AIX__)
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszSectionName);
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszAccount);
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszBoot);
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszMount);
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszNodeName);
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszSize);
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszType);
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszLog);
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszFree);
#endif
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszMountVol);
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszMountOptions);
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszMountPt);
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszFileType);
#if defined(__LWI_SOLARIS__)
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszDeviceToFsck);
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszFsckPass);
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszMountAtBoot);
#elif !defined(__LWI_SOLARIS__) || !defined(__LWI_AIX__)
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszFrequency);
#else
    LW_SAFE_FREE_STRING((*pFSTabEntry)->pszPassCode);
#endif
    LwFreeMemory(*pFSTabEntry);
    *pFSTabEntry = NULL;
}

static
CENTERROR
AmendFSTabList( 
    xmlNodePtr pNode,
    PFSTAB_OBJECT *pTemp,
    BOOLEAN bNewFSTabEntry
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct _xmlAttr* pTmpNode = pNode->properties;
    gid_t gid = 0;
    uid_t uid = 0;
    char szUID[PATH_MAX];
    char szGID[PATH_MAX];
    PFSTAB_OBJECT pTempNode;

#if defined (__LWI_AIX__)
    PSTR pszNodeName = NULL;
    PSTR pszNodeToken = NULL;
    PSTR pszToken = NULL;
    PSTR pszTemp = NULL;
    PSTR pszTmpCDRFS = NULL;
    BOOLEAN bNFSFileType = 0;
    BOOLEAN bCDRFSFileType = 0;
    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&pszNodeName);
    BAIL_ON_CENTERIS_ERROR(ceError);
    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&pszTmpCDRFS);
    BAIL_ON_CENTERIS_ERROR(ceError);
#endif

    if( bNewFSTabEntry ) {
        ceError = InitFSTabEntries( pTemp );
        if(ceError != CENTERROR_SUCCESS)
            goto error;
    }

    pTempNode = *pTemp;

    LW_SAFE_FREE_STRING(pTempNode->pszMountOptions);
    (*pTemp)->pszMountOptions = NULL;

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&pTempNode->pszMountOptions);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pTempNode = NULL;

    for (; pTmpNode; pTmpNode = pTmpNode->next) {

        if (IsNullOrEmptyString(pTmpNode->name)) {
            continue;
        }

#if defined(__LWI_HP_UX__)
        /* Following are the NFS mount options that aren't supported on HP-UX systems */
        if( !strcmp( (char*)pTmpNode->name,
                     (char*)"noexec")           ||
            !strcmp( (char*)pTmpNode->name,
                     (char*)"exec")             ||
            !strcmp( (char*)pTmpNode->name,
                     (char*)"sec")              ||
            !strcmp( (char*)pTmpNode->name,
                     (char*)"tcp")              ||
            !strcmp( (char*)pTmpNode->name,
                     (char*)"udp")              ||
            !strcmp( (char*)pTmpNode->name,
                     (char*)"sync")             ||
            !strcmp( (char*)pTmpNode->name,
                     (char*)"user")) {
                continue;
        }
#endif

        if (!strcmp( (char*)pTmpNode->name, (char*)LWI_FSTAB_MOUNT_VOL)) {
#if defined (__LWI_AIX__)
            pszToken = (char *)pTmpNode->children->content;

            if ( strspn( (char*)pTmpNode->children->content, 
                         (char *)"#")) {
                pszToken = (char *)pTmpNode->children->content + 1;

                strcpy( pszTmpCDRFS,
                        pszToken);

                strcpy( (char*)(*pTemp)->pszMount,
                        (char*)"false");
                strcat( (char*)(*pTemp)->pszMount,
                        "\n"); 
            }
            else {
                strcpy( (char*)(*pTemp)->pszMount,
                        (char*)"true");
                strcat( (char*)(*pTemp)->pszMount,
                        "\n");
                strcpy( pszTmpCDRFS,
                        pszToken);
            }

            if( strstr( (char*)pTmpNode->children->content, 
                        ":") ) {
                strcpy( (char *)pszNodeName,
                        (char *)pszToken);
                pszNodeToken = (char *)strtok_r( pszNodeName,
                                                 ":",
                                                 &pszTemp);

                strcpy( (char *)(*pTemp)->pszNodeName, 
                        (char *) pszNodeToken);
                strcat( (char*)(*pTemp)->pszNodeName,
                        "\n");

                pszNodeToken = (char *)strtok_r( NULL,
                                                 ":",
                                                 &pszTemp);

                strcpy( (char*)(*pTemp)->pszMountVol,
                        (char*)pszNodeToken);
                strcat( (char*)(*pTemp)->pszMountVol,
                        "\n");
                bNFSFileType = TRUE;
            }
            else {
                strcpy( (char*)(*pTemp)->pszSectionName,
                        (char*)pTmpNode->children->content);
                strcat( (char*)(*pTemp)->pszSectionName,
                        ":\n");
                sprintf( (char*)(*pTemp)->pszMountPt,
                         "%s",
                         (char*)pszTmpCDRFS);
            }
#else
            strcpy( (char*)(*pTemp)->pszMountVol,
                    (char*)pTmpNode->children->content);
#endif
        }
        else if (!strcmp( (char*)pTmpNode->name,
                          (char*)LWI_FSTAB_MOUNT_PT)) { 
#if defined (__LWI_AIX__)
            if ( bNFSFileType ) {
                sprintf( (char*)(*pTemp)->pszMountPt,
                         "\"%s\"\n",
                        (char*)pTmpNode->children->content);
                strcpy( (char*)(*pTemp)->pszSectionName,
                        (char*)pTmpNode->children->content);
                strcat( (char*)(*pTemp)->pszSectionName,
                        ":\n");
            }
            else {
                strcpy( (char*)(*pTemp)->pszMountVol,
                        (char*)pTmpNode->children->content);
                strcat( (char*)(*pTemp)->pszMountVol,
                        "\n");
            }
#else
            strcpy( (char*)(*pTemp)->pszMountPt,
                    (char*)pTmpNode->children->content);
#endif
        }
        else if (!strcmp( (char*)pTmpNode->name,
                          (char*)LWI_FSTAB_FSTYPE)) { 
            //We are supporting only 2 file types for MAC - cd9660 and nfs, and 3 for AIX - jfs, nfs and cdrfs
#if defined(__LWI_DARWIN__) || defined(__LWI_SOLARIS__) || defined(__LWI_AIX__)
            if ( !strcmp( (char*)pTmpNode->children->content, "ext2") ||
                 !strcmp( (char*)pTmpNode->children->content, "ext3") ||
                 !strcmp( (char*)pTmpNode->children->content, "nfs4") ||
                 !strcmp( (char*)pTmpNode->children->content, "cifs") ) {

                GPA_LOG_ALWAYS("Skipping lwifstab entry for the file type %s",(char*)pTmpNode->children->content);
                FreeFSTabEntries(pTemp);
                ceError = CENTERROR_SUCCESS;
                goto error;
            }
#endif

#if defined(__LWI_DARWIN__)
            if ( !strcmp( (char*)pTmpNode->children->content, "iso9660") ) {
                strcpy( (char*)(*pTemp)->pszFileType,
                        (char*)"cd9660");
            }
#elif defined(__LWI_AIX__)
            if ( !strcmp( (char*)pTmpNode->children->content, "iso9660") ) {
                strcpy( (char*)(*pTemp)->pszFileType,
                        (char*)"cdrfs\n");
                bCDRFSFileType = TRUE;
            }
#else
            strcpy( (char*)(*pTemp)->pszFileType,
                    (char*)pTmpNode->children->content);
#endif
        }
        else if( strcmp( (char*)pTmpNode->name,
                         (char*)LWI_FSTAB_MOUNT_VOL_OPT) != 0 ) {
#if defined(__LWI_AIX__)
            if ( bCDRFSFileType ) {
                strcpy( (char*)(*pTemp)->pszMountOptions,
                        "ro");

                strcpy( (char *)(*pTemp)->pszSectionName,
                        "");
                strcpy( (char *)pszNodeName,
                        (char *)(*pTemp)->pszMountVol);

                sprintf( (char *)(*pTemp)->pszSectionName,
                         "%s",
                         strtok_r(pszNodeName, "\n", &pszToken));

                strcpy( (char *)(*pTemp)->pszMountVol,
                        (char *)(*pTemp)->pszMountPt);
                strcat( (char *)(*pTemp)->pszMountVol,
                        "\n");

                sprintf( (char *)(*pTemp)->pszMountPt,
                         "\"%s\"\n",
                         (char *)(*pTemp)->pszSectionName);

                strcat( (char *)(*pTemp)->pszSectionName,
                        (char *)":\n");

                break;
            }
#endif
            if ( !strcmp( (char*)pTmpNode->name,
                          (char*)LWI_FSTAB_UID)     || 
                 !strcmp( (char*)pTmpNode->name,
                          (char*)LWI_FSTAB_GID)     ||
                 !strcmp( (char*)pTmpNode->name,
                          (char*)LWI_FSTAB_RESUID)  ||
                 !strcmp( (char*)pTmpNode->name,
                          (char*)LWI_FSTAB_RESGID)) {

                if ( !strcmp( (char*)pTmpNode->name,
                              (char*)LWI_FSTAB_UID) ||
                     !strcmp( (char*)pTmpNode->name,
                              (char*)LWI_FSTAB_RESUID)) {
       
                    ceError = GPAGetUserUID( (const char*)pTmpNode->children->content, &uid);
                    if ( ceError == CENTERROR_SUCCESS ) {

                        if( !IsNullOrEmptyString((*pTemp)->pszMountOptions) ) {
                            strcat( (char*)(*pTemp)->pszMountOptions, 
                                    (char*)","); 
                        }

                        strcat( (char*)(*pTemp)->pszMountOptions, 
                                (char*)pTmpNode->name); 
                        strcat( (char*)(*pTemp)->pszMountOptions, 
                                (char*)"="); 
                        sprintf( szUID, 
                                 "%d",
                                 (int)uid );
                        strcat( (char*)(*pTemp)->pszMountOptions, 
                                szUID); 
                    }
                    else {
                        GPA_LOG_ALWAYS("Unable to find %s for %s. Hence skipping", (char*)pTmpNode->name, (char*)pTmpNode->children->content);
                        FreeFSTabEntries(pTemp);
                        ceError = CENTERROR_SUCCESS;
                        goto error;
                    }
                }
                else if ( !strcmp( (char*)pTmpNode->name,
                                   (char*)LWI_FSTAB_GID) ||
                          !strcmp( (char*)pTmpNode->name,
                                   (char*)LWI_FSTAB_RESGID)) {

                    ceError = GPAGetUserGID( (const char*)pTmpNode->children->content, &gid);

                    if ( ceError == CENTERROR_SUCCESS ) {

                        if( !IsNullOrEmptyString((*pTemp)->pszMountOptions) ) {
                            strcat( (char*)(*pTemp)->pszMountOptions, 
                                    (char*)","); 
                        }

                        strcat( (char*)(*pTemp)->pszMountOptions, 
                                (char*)pTmpNode->name); 
                        strcat( (char*)(*pTemp)->pszMountOptions, 
                                (char*)"="); 
                        sprintf( szGID, 
                                 "%d",
                                 (int)gid );
                        strcat( (char*)(*pTemp)->pszMountOptions, 
                                szGID); 
                    }
                    else {
                        GPA_LOG_ALWAYS("Unable to find %s for %s. Hence skipping", (char*)pTmpNode->name, (char*)pTmpNode->children->content);
                        FreeFSTabEntries(pTemp);
                        ceError = CENTERROR_SUCCESS;
                        goto error;
                    }
                }
                else {

                    if( IsNullOrEmptyString((*pTemp)->pszMountOptions) ) {
                        strcat( (char*)(*pTemp)->pszMountOptions, 
                                (char*)pTmpNode->children->content); 
                    }
                    else {
                        strcat( (char*)(*pTemp)->pszMountOptions, 
                                (char*)","); 
                        strcat( (char*)(*pTemp)->pszMountOptions, 
                                (char*)pTmpNode->children->content); 
                    }
                }
            }
            else if( !strcmp( (char*)pTmpNode->children->content, 
                              (char*)"true")) {
#if defined(__LWI_SOLARIS__) || defined(__LWI_AIX__) || defined(__LWI_DARWIN__)
                if( !strcmp( (char*)pTmpNode->name,
                             "user")                    ||
                    !strcmp( (char *)pTmpNode->name,
                             "exec")                    || 
                    !strcmp( (char *)pTmpNode->name,
                             "noexec")) {
                        continue;
                }
                else if( !strcmp( (char*)pTmpNode->name,
                                  "tcp")                     ||
                         !strcmp( (char*)pTmpNode->name,
                                  "udp")) {
                    if( IsNullOrEmptyString((*pTemp)->pszMountOptions) ) {
                        strcpy( (char*)(*pTemp)->pszMountOptions,
                                "proto=");
                        strcat( (char*)(*pTemp)->pszMountOptions,
                                (char*)pTmpNode->name);
                    }
                    else {
                        strcat( (char*)(*pTemp)->pszMountOptions, 
                                (char*)","); 
                        strcat( (char*)(*pTemp)->pszMountOptions,
                                "proto=");
                        strcat( (char*)(*pTemp)->pszMountOptions,
                                (char*)pTmpNode->name);
                    }                   
                }
#else
#if defined(__LWI_HP_UX__)
                if( !strcmp( (char*)pTmpNode->name,
                             (char*)"dev")){
                    strcpy( (char*)pTmpNode->name,
                            "devs");
                }
                else if( !strcmp( (char*)pTmpNode->name,
                                  (char*)"nodev")){
                    strcpy( (char*)pTmpNode->name,
                            "nodevs");
                }
#endif
                if( IsNullOrEmptyString((*pTemp)->pszMountOptions) ) {
                    strcpy( (char*)(*pTemp)->pszMountOptions,
                            (char*)pTmpNode->name);
                }
                else {
                    strcat( (char*)(*pTemp)->pszMountOptions, 
                            (char*)","); 
                    strcat( (char*)(*pTemp)->pszMountOptions, 
                            (char*)pTmpNode->name); 
                }                
#endif
            }
            else if( strcmp( (char*)pTmpNode->children->content, 
                             (char*)"true") != 0 ) {
#if defined (__LWI_DARWIN__)
                if( !strcmp( (char*)pTmpNode->name,
                             "port")                    ||
                    !strcmp( (char*)pTmpNode->name,
                             "sec")) {
                    continue;
                }
#else
#if defined(__LWI_HP_UX__) || defined(__LWI_SOLARIS__)
                if( !strcmp( (char*)pTmpNode->name,
                             (char*)"nfsvers")){
                    strcpy( (char*)pTmpNode->name,
                            "vers"); 
                }
#endif
                if( IsNullOrEmptyString((*pTemp)->pszMountOptions) ) {
                    strcpy( (char*)(*pTemp)->pszMountOptions,
                            (char*)pTmpNode->name);
                    strcat( (char*) (*pTemp)->pszMountOptions, 
                            (char*)"="); 
                    strcat( (char*) (*pTemp)->pszMountOptions, 
                            (char*)pTmpNode->children->content); 
                }
                else {
                    strcat( (char*)(*pTemp)->pszMountOptions, 
                            (char*)",");
                    strcat( (char*)(*pTemp)->pszMountOptions,
                            (char*)pTmpNode->name);
                    strcat( (char*)(*pTemp)->pszMountOptions, 
                            (char*)"="); 
                    strcat( (char*)(*pTemp)->pszMountOptions, 
                            (char*)pTmpNode->children->content); 
                }                
#endif
            }
            else if ( !strcmp( (char*)pTmpNode->name,
                               (char*)LWI_FSTAB_USERNAME) || !strcmp( (char*)pTmpNode->name,
                                                                      (char*)LWI_FSTAB_PASSWORD)) {
                if( !IsNullOrEmptyString((*pTemp)->pszMountOptions) ) {
                    strcat( (char*)(*pTemp)->pszMountOptions, 
                            (char*)","); 
                }
                strcat( (char*)(*pTemp)->pszMountOptions, 
                        (char*)pTmpNode->name); 
                strcat( (char*)(*pTemp)->pszMountOptions, 
                        (char*)"="); 
                strcat( (char*)(*pTemp)->pszMountOptions, 
                        (char*)pTmpNode->children->content); 
            }
        }
    }
#if defined(__LWI_AIX__)
    strcat( (char*)(*pTemp)->pszMountOptions,
            "\n");
#endif

    return ceError;

error:

    return ceError;
}

static
CENTERROR
CheckMountPt(
    PSTR pszMountPt
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bDirExists = FALSE;

    ceError = GPACheckIfMountDirExists( pszMountPt,
                                        &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bDirExists) {
        ceError = LwCreateDirectory( pszMountPt,
                                     S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;
}

static
CENTERROR
FillFSTabGPOList( 
    PSTR pszMountPt,
    PSTR pszDisplayName,
    PSTR pszGPOGUID
)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFSTAB_GPO_OBJECT pNode = NULL;
    PFSTAB_GPO_OBJECT pPrev = pHead;
    PFSTAB_GPO_OBJECT pTempNode = pHead;

    while ( pHead && pTempNode ) {

        if ( !strcmp( (char*)pTempNode->pszMountPt,
                      (char*)pszMountPt)) {
            ceError = CENTERROR_SUCCESS;
            goto error;
        }
        
        pPrev = pTempNode;
        pTempNode = pTempNode->pNext;

    }

    if ( pTempNode == NULL) {
        pTempNode = pPrev;
    }

    ceError = LwAllocateMemory( sizeof(FSTAB_GPO_OBJECT),
                                (PVOID *)&pNode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(pNode)->pszMountPt);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(pNode)->pszDisplayName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&(pNode)->pszGPOGUID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    strcpy( (char*)(pNode)->pszMountPt,
            (char*)pszMountPt);

    ceError = CheckMountPt(pszMountPt);
    BAIL_ON_CENTERIS_ERROR(ceError);

    strcpy( (char*)(pNode)->pszDisplayName,
            (char*)pszDisplayName);

    strcpy( (char*)(pNode)->pszGPOGUID,
            (char*)pszGPOGUID);

    pNode->pNext = NULL;

    if ( pHead == NULL) {
        pHead = pNode;
    }
    else {
        pTempNode->pNext = pNode;
    }

    return ceError;
    
error:

    if ( pNode ) {

        if (pNode->pszMountPt) {
            LwFreeMemory(pNode->pszMountPt);
            pNode->pszMountPt = NULL;
        }

        if (pNode->pszDisplayName) {
            LwFreeMemory(pNode->pszDisplayName);
            pNode->pszDisplayName = NULL;
        }

        if (pNode->pszGPOGUID) {
            LwFreeMemory(pNode->pszGPOGUID);
            pNode->pszGPOGUID = NULL;
        }
    
        LwFreeMemory(pNode);
        pNode = NULL;
    }
 
    return ceError;
}

static
CENTERROR
ParseFSTabGPItem(
    xmlNodePtr root_node,
    PSTR pszDisplayName,
    PSTR pszGPOGUID
)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    xmlChar *nameAttrValue = NULL;

    for (cur_node = root_node; cur_node; cur_node = cur_node->next) {

        if (cur_node->name) {
            nameAttrValue = xmlGetProp( cur_node,
                                        (const xmlChar*)LWI_FSTAB_MOUNT_PT);
            if (strcmp( (char*)cur_node->name,
                        (char*)LWI_FSTAB_ENTRY) == 0 && nameAttrValue) {

                ceError = FillFSTabGPOList( (char*)nameAttrValue, 
                                            pszDisplayName,
                                            pszGPOGUID);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            if ( nameAttrValue ) {
               xmlFree( nameAttrValue );
               nameAttrValue = NULL;
            }
        }
        ParseFSTabGPItem( cur_node->children,
                          pszDisplayName,
                          pszGPOGUID);
    }

error:

    if ( nameAttrValue ) {
        xmlFree( nameAttrValue );
        nameAttrValue = NULL;
    }

    return ceError;
}

static
CENTERROR
ComputeFSTabModList(
    xmlNodePtr root_node
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    xmlChar *pszNodeText = NULL;
    xmlChar *nameAttrValue = NULL;
    PFSTAB_OBJECT pTemp = pFSTabHead;
    BOOLEAN bMatchFound = 0;

    for (cur_node = root_node; cur_node; cur_node = cur_node->next) {
        pszNodeText = NULL;

        if (cur_node->name) {
            nameAttrValue = xmlGetProp( cur_node,
                                        (const xmlChar*)LWI_FSTAB_MOUNT_PT);
            if (strcmp( (char*)cur_node->name,
                        (char*)LWI_FSTAB_ENTRY) == 0 ) {
                while( pTemp ) {
                    if( !strcmp( (char*)nameAttrValue,
                                 (char*)pTemp->pszMountPt) ) {
                        ceError = AmendFSTabList( cur_node, 
                                                  &pTemp,
                                                  FALSE);
                        BAIL_ON_CENTERIS_ERROR(ceError);
                        bMatchFound = 1;
                        break;
                    }
                    else {
                        bMatchFound = 0;
                    }
                    pTemp = pTemp->pNext;
                }
                //If match is not found
                if(!bMatchFound) {
                    PFSTAB_OBJECT pFSTabNode = NULL; 

                    ceError = LwAllocateMemory( sizeof(FSTAB_OBJECT),
                                                (PVOID)&pFSTabNode);
                        
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    ceError = AmendFSTabList( cur_node,
                                              &pFSTabNode,
                                              TRUE);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    if( pFSTabNode ) {
                        pTemp = pFSTabHead;
                        while( pTemp && pTemp->pNext ) {
                            pTemp = pTemp->pNext;
                        }
                        if (pTemp) {
                            pTemp->pNext = pFSTabNode;
                        } else {
                            pTemp = pFSTabNode;
                        }
                        pFSTabNode = NULL;
                    }
                }
                //Reinitialize the temp ptr
                if ( pFSTabHead != NULL ){
                    pTemp = pFSTabHead;
                }
                else {
                    pFSTabHead = pTemp;
                }

                if ( pszNodeText ) {
                    xmlFree( pszNodeText );
                    pszNodeText = NULL;
                }
            }

            if ( nameAttrValue ) {
               xmlFree( nameAttrValue );
               nameAttrValue = NULL;
            }
        }
        ComputeFSTabModList( cur_node->children );
    }

error:

    if ( pszNodeText ) {
        xmlFree( pszNodeText );
    }

    if ( nameAttrValue ) {
        xmlFree( nameAttrValue );
    }

    return ceError;
}

static
void
FreeGPOListMembers(
    PFSTAB_GPO_OBJECT *pTemp
)
{
    if ( (*pTemp)->pszMountPt ) {
        LwFreeString((*pTemp)->pszMountPt);
        (*pTemp)->pszMountPt = NULL;
    }

    if ((*pTemp)->pszDisplayName) {
        LwFreeString((*pTemp)->pszDisplayName);
        (*pTemp)->pszDisplayName = NULL;
    }

    if ((*pTemp)->pszGPOGUID) {
        LwFreeString((*pTemp)->pszGPOGUID);
        (*pTemp)->pszGPOGUID = NULL;
    }
}

static
CENTERROR
RemoveFSTabCredentialsFolder(
    PSTR pszDisplayName
)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bDirExists = FALSE;
    PFSTAB_GPO_OBJECT pTemp = pHead;
    PFSTAB_GPO_OBJECT pNode = NULL;
    PFSTAB_GPO_OBJECT pPrev = pTemp;
    char szFolderPath[PATH_MAX];

    while(pTemp != NULL) {
        
        if ( pTemp->pszDisplayName && !strcmp( (char*)pTemp->pszDisplayName, 
                                               (char*)pszDisplayName)) {

            sprintf( szFolderPath,
                     "%s/%s",
                     LWIFSTAB_CREDENTIALS_FILEPATH,
                     pszDisplayName);

            ceError = GPACheckDirectoryExists( szFolderPath, 
                                              &bDirExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (bDirExists) {
                
                ceError = GPARemoveDirectory(szFolderPath);
                BAIL_ON_CENTERIS_ERROR(ceError);

                FreeGPOListMembers(&pTemp);

                pNode = pTemp;

                if ( pTemp == pPrev ) {
                    pPrev = pTemp->pNext;
                    pTemp = pPrev;
                    pHead = pTemp;
                }
                else if ( pTemp->pNext != NULL ) {
                    pPrev->pNext = pTemp->pNext;
                    pTemp = pPrev->pNext;
                }
                else {
                    pPrev->pNext = NULL;
                    pTemp = NULL;
                }

                LwFreeMemory(pNode);
                pNode = NULL;
            }
        }

        if ( pTemp != NULL ) {
            pPrev = pTemp;
            pTemp = pTemp->pNext;
        }
    }

    return ceError;

error:

    return ceError;

}

static
CENTERROR
ResetFSTabGPOList()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PFSTAB_GPO_OBJECT pTmp = NULL;
    PFSTAB_GPO_OBJECT pOld = NULL;
    pTmp = pHead;

    while(pTmp != NULL) {

        /* Remove CIFS credentials folder */
        ceError = RemoveFSTabCredentialsFolder(pTmp->pszDisplayName);
        BAIL_ON_CENTERIS_ERROR(ceError);        

        FreeGPOListMembers(&pTmp);

        pOld = pTmp;  
        pTmp = pTmp->pNext;
        pOld = NULL;
    }

    pHead = NULL;

    return ceError;

error:

    return ceError;
}

CENTERROR
ResetFSTabGroupPolicy(
    PGPUSER pUser
    )
{
    return CENTERROR_SUCCESS;
}

CENTERROR
ResetFSTabPolicy()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bExists = FALSE;    
    
    ceError = GPACheckFileExists( LWIFSTAB_BACKUP_FILEPATH,
                                 &bExists );
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if ( bExists ) {
        ceError = GPACopyFileWithOriginalPerms( LWIFSTAB_BACKUP_FILEPATH,
                                               LWIFSTAB_FILEPATH);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwRemoveFile(LWIFSTAB_BACKUP_FILEPATH);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    //Clear in memory fstab gpo list
    ceError = ResetFSTabGPOList();
    BAIL_ON_CENTERIS_ERROR(ceError);
    
error:

    return ceError;
}

static
CENTERROR
ProcessCIFSFile(
    PFSTAB_OBJECT *pCIFSNode,
    DWORD dwCIFSFileCnt,
    PSTR pszCIFSFolderPath
)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszToken = NULL;
    PSTR pszMntOpt = NULL;
    PSTR pszFileName = NULL;
    PFSTAB_OBJECT pTemp =  *pCIFSNode;
    FILE *fp = NULL;

    ceError = LwAllocateString( pTemp->pszMountOptions,
                                &pszMntOpt);
    BAIL_ON_CENTERIS_ERROR(ceError);

    memset( pTemp->pszMountOptions, 
            0, 
            STATIC_PATH_BUFFER_SIZE);
    
    // Parse the mount options to tokens delimited by ,
    pszToken = strtok( pszMntOpt, 
                       ",");

    while ( pszToken ) {
        // Match for username or password 
        if( strstr( pszToken,
                    LWI_FSTAB_USERNAME)) {

            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pszFileName);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pszFileName,
                     "%s/%s%d", 
                     pszCIFSFolderPath, 
                     LWIFSTAB_CREDENTIALS_FILENAME,                      
                     dwCIFSFileCnt);
            
            // Open a fstab cifs credentials file to write username and password
            ceError = GPAOpenFile( pszFileName,
                                  "w",
                                  &fp);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if ( fprintf( fp, 
                          "%s\n", 
                          pszToken) < 0 ) {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
        else if( strstr( pszToken,
                         LWI_FSTAB_PASSWORD)) {
            if ( fprintf( fp, 
                          "%s\n", 
                          pszToken) < 0 ) {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
            /* Replace username and password in fstab file with zeros and assign the 
             * stored username password file path
             */
            if( !IsNullOrEmptyString(pTemp->pszMountOptions) ) {
                strcat( (char*)pTemp->pszMountOptions, 
                        (char*)","); 
            }

            strcat( (char*)pTemp->pszMountOptions, 
                    "credentials="); 
            strcat( (char*)pTemp->pszMountOptions, 
                    pszFileName); 
        }
        else {
            if( !IsNullOrEmptyString(pTemp->pszMountOptions) ) {
                strcat( (char*)pTemp->pszMountOptions, 
                        (char*)","); 
            }
            strcat( (char*)pTemp->pszMountOptions, 
                    pszToken); 
        }
        pszToken = strtok( NULL, 
                           ",");
    }

error:

    if ( fp ) {
        fclose( fp );
    }

    if ( pszMntOpt ) {
        LwFreeString( pszMntOpt );
        pszMntOpt = NULL;
    }

    if ( pszFileName ) {
        LwFreeString( pszFileName );
        pszFileName = NULL;
    }

    return ceError;
}

#if defined(__LWI_AIX__)
static
void
WriteAIXFSTabEntry(
    PFSTAB_OBJECT *ppAIXFSTabEntry,
    FILE *fp
)
{
    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszSectionName) ) {
        fprintf( fp,
                 "\n%s",
                 (*ppAIXFSTabEntry)->pszSectionName );
    }

    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszMountVol) ) {
        fprintf( fp,
                 "\tdev\t\t=\t%s",
                 (*ppAIXFSTabEntry)->pszMountVol );
    }

    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszMountPt) ) {
        fprintf( fp,
                 "\tvol\t\t=\t%s",
                 (*ppAIXFSTabEntry)->pszMountPt );
    }

    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszMount) ) {
        fprintf( fp,
                 "\tmount\t\t=\t%s",
                 (*ppAIXFSTabEntry)->pszMount );
    }   

    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszPassCode) ) {
        fprintf( fp,
                 "\tcheck\t\t=\t%s",
                 (*ppAIXFSTabEntry)->pszPassCode );
    }   

    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszFree) ) {
        fprintf( fp,
                 "\tfree\t\t=\t%s",
                 (*ppAIXFSTabEntry)->pszFree );
    }

    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszFileType) ) {
        fprintf( fp,
                 "\tvfs\t\t=\t%s",
                 (*ppAIXFSTabEntry)->pszFileType );
    }

    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszLog) ) {
        fprintf( fp,
                 "\tlog\t\t=\t%s",
                 (*ppAIXFSTabEntry)->pszLog );
    }   

    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszType) ) {
        fprintf( fp,
                 "\ttype\t\t=\t%s",
                 (*ppAIXFSTabEntry)->pszType );
    }   

    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszNodeName) ) {
        fprintf( fp,
                 "\tnodename        =\t%s",
                 (*ppAIXFSTabEntry)->pszNodeName );
    }   

    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszMountOptions) ) {
        fprintf( fp,
                 "\toptions\t\t=\t%s",
                 (*ppAIXFSTabEntry)->pszMountOptions );
    }   

    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszAccount) ) {
        fprintf( fp,
                 "\taccount\t\t=\t%s",
                 (*ppAIXFSTabEntry)->pszAccount );
    }
    
    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszBoot) ) {
        fprintf( fp,
                 "\tboot\t\t=\t%s",
                 (*ppAIXFSTabEntry)->pszBoot );
    }   

    if( !IsNullOrEmptyString((*ppAIXFSTabEntry)->pszSize) ) {
        fprintf( fp,
                 "\tsize\t\t=\t%s",
                 (*ppAIXFSTabEntry)->pszSize );
    }
}
#endif

static
void
WriteFSTabEntries(
    PFSTAB_OBJECT pFSTabEntries,
    FILE *fp,
    BOOLEAN bNewLine
)
{

#if defined(__LWI_AIX__) 
    WriteAIXFSTabEntry( &pFSTabEntries,
                        fp);
#elif defined(__LWI_SOLARIS__) 
    fprintf( fp, 
             "%s\t%s\t%s\t%s\t%s\t%s\t%s",
             pFSTabEntries->pszMountVol,
             pFSTabEntries->pszDeviceToFsck,    //device fsck
             pFSTabEntries->pszMountPt,
             pFSTabEntries->pszFileType,
             pFSTabEntries->pszFsckPass,        //fsck password
             pFSTabEntries->pszMountAtBoot,     //mount at boot
             pFSTabEntries->pszMountOptions);
#else
    fprintf( fp, 
             "%-20s\t%-20s\t%-15s\t%-20s\t%s\t%s",
             pFSTabEntries->pszMountVol,
             pFSTabEntries->pszMountPt,
             pFSTabEntries->pszFileType,
             pFSTabEntries->pszMountOptions,
             pFSTabEntries->pszFrequency,
             pFSTabEntries->pszPassCode);
#endif

    if ( bNewLine ) {
        fprintf( fp,
                 "%s", 
                 "\n");
    }
}

static
void
FreeFSTabEntryList(
    PFSTAB_OBJECT *pFSTabEntryList
    )
{
    PFSTAB_OBJECT pFSTabEntry = NULL;

    while(*pFSTabEntryList) {
        
        pFSTabEntry = *pFSTabEntryList;
        *pFSTabEntryList = (*pFSTabEntryList)->pNext;

#if defined(__LWI_AIX__)
        LW_SAFE_FREE_STRING(pFSTabEntry->pszSectionName);
        LW_SAFE_FREE_STRING(pFSTabEntry->pszAccount);
        LW_SAFE_FREE_STRING(pFSTabEntry->pszBoot);
        LW_SAFE_FREE_STRING(pFSTabEntry->pszMount);
        LW_SAFE_FREE_STRING(pFSTabEntry->pszNodeName);
        LW_SAFE_FREE_STRING(pFSTabEntry->pszSize);
        LW_SAFE_FREE_STRING(pFSTabEntry->pszType);
        LW_SAFE_FREE_STRING(pFSTabEntry->pszLog);
        LW_SAFE_FREE_STRING(pFSTabEntry->pszFree);
#endif
        LW_SAFE_FREE_STRING(pFSTabEntry->pszMountVol);
        LW_SAFE_FREE_STRING(pFSTabEntry->pszMountPt);
        LW_SAFE_FREE_STRING(pFSTabEntry->pszFileType);
        LW_SAFE_FREE_STRING(pFSTabEntry->pszMountOptions);
#if defined(__LWI_SOLARIS__)
        LW_SAFE_FREE_STRING(pFSTabEntry->pszDeviceToFsck);
        LW_SAFE_FREE_STRING(pFSTabEntry->pszFsckPass);
        LW_SAFE_FREE_STRING(pFSTabEntry->pszMountAtBoot);
#elif !defined(__LWI_SOLARIS__) || !defined(__LWI_AIX__)
        LW_SAFE_FREE_STRING(pFSTabEntry->pszFrequency);
#else
        LW_SAFE_FREE_STRING(pFSTabEntry->pszPassCode);
#endif
        LwFreeMemory(pFSTabEntry);
        pFSTabEntry = NULL;
    }
}

static
VOID
ReplaceSpaceWithUnderScore(
    PSTR *pszDisplayName
    )
{
    PSTR pszPos = *pszDisplayName;

    while (*pszPos != 0) {
        if (*pszPos == ' ') {
            *pszPos = '_';
        }
        pszPos++;
    }
}

static
CENTERROR
WriteFSTabFile( 
    PSTR pszFSTabFile,
    PSTR pszBaseFile
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE *fp = NULL;
    PSTR pszFileHeader = NULL;
    DWORD dwCIFSFileCnt = 0;
    PSTR pszCIFSFolderPath = NULL;
    PSTR pszCIFSGUIDFolderPath = NULL;
    PSTR pszDisplayName = NULL;
    PSTR pszGPOGUID = NULL;
    BOOLEAN bFolderExists = FALSE;
    PFSTAB_OBJECT pTemp = pFSTabHead;

    /* Open file for writing */
    ceError = GPAOpenFile( pszFSTabFile,
                          "w",
                          &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateStringPrintf( &pszFileHeader,
                                      LWIFSTAB_FILE_HEADER,
                                      pszBaseFile );
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE, 
                                (PVOID *)&pszCIFSFolderPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE, 
                                (PVOID *)&pszCIFSGUIDFolderPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pszFileHeader) {
        if ( fprintf( fp, 
                      "%s\n\n", 
                      pszFileHeader ) < 0 ) {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    /* Parse the list and write it to a file */
    while (pTemp != NULL) {
        //Check for CIFS file type
        if ( pTemp->pszFileType && !strcmp( (char*)pTemp->pszFileType, 
                                            (char*)LWI_FSTAB_FILETYPE)) { 

            GetCIFSGUID( pTemp,
                         &pszDisplayName,
                         &pszGPOGUID);

            if (!IsNullOrEmptyString(pszGPOGUID) && !IsNullOrEmptyString(pszDisplayName)) {
                ReplaceSpaceWithUnderScore(&pszDisplayName);
                sprintf( pszCIFSFolderPath, 
                         "%s/%s",
                         LWIFSTAB_CREDENTIALS_FILEPATH, 
                         pszDisplayName);

                ceError = GPACheckDirectoryExists( pszCIFSFolderPath, 
                                                  &bFolderExists);
                BAIL_ON_CENTERIS_ERROR(ceError);

                if (!bFolderExists) {
                    ceError = LwCreateDirectory( pszCIFSFolderPath,
                                                 S_IRUSR|S_IWUSR);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

                sprintf( pszCIFSGUIDFolderPath, 
                         "%s/%s/%s",
                         LWIFSTAB_CREDENTIALS_FILEPATH, 
                         pszDisplayName,
                         pszGPOGUID);

                ceError = GPACheckDirectoryExists( pszCIFSGUIDFolderPath, 
                                                  &bFolderExists);
                BAIL_ON_CENTERIS_ERROR(ceError);

                if (!bFolderExists) {
                    ceError = LwCreateDirectory( pszCIFSGUIDFolderPath,
                                                 S_IRUSR|S_IWUSR);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    dwCIFSFileCnt = 0;
                }

                ceError = ProcessCIFSFile( &pTemp,
                                           ++dwCIFSFileCnt,
                                           pszCIFSGUIDFolderPath);        
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        if (pTemp->pszFileType) {
            // Just to write neatly on fstab file
#if defined(__LWI_SOLARIS__)
            if ( pTemp->pszMountOptions &&
                 !strrchr( (char*)pTemp->pszMountOptions, 
                           '\n')) {
#else
            if( pTemp->pszPassCode &&
                !strrchr( (char*)pTemp->pszPassCode,
                          '\n')) {
#endif
                WriteFSTabEntries( pTemp,
                                   fp,
                                   TRUE);
            }
            else {
                WriteFSTabEntries( pTemp,
                                   fp,
                                   FALSE);
           }
        }
       
        pTemp = pTemp->pNext;
    }

error:

    if ( fp ) {
        fclose( fp );
    }

    if ( pszFileHeader ) {
        LwFreeString( pszFileHeader );
    }

    if ( pszCIFSFolderPath ) {
        LwFreeString( pszCIFSFolderPath );
    }

    if ( pszCIFSGUIDFolderPath ) {
        LwFreeString( pszCIFSGUIDFolderPath );
    }

    return ceError;
}

static
PFSTAB_OBJECT
ReverseFSTabEntryList(
    PFSTAB_OBJECT pFSTabEntryList
    )
{
    PFSTAB_OBJECT pPrev = NULL;
    PFSTAB_OBJECT pNext = NULL;
    PFSTAB_OBJECT pCur = pFSTabEntryList;

    while (pCur) {
        pNext = pCur->pNext;
        pCur->pNext = pPrev;
        pPrev = pCur;
        pCur = pNext;
    }

    return pPrev;
}

typedef struct __FSTABTOKEN {
    PSTR pszValue;
    struct __FSTABTOKEN * pNext;
} FSTABTOKEN, *PFSTABTOKEN;

static
void
FreeTokenList(
   PFSTABTOKEN pTokenList
   )
{
    PFSTABTOKEN pToken = NULL;

    while (pTokenList) {

        pToken = pTokenList;

        pTokenList = pTokenList->pNext;

        LW_SAFE_FREE_STRING(pToken->pszValue);
        pToken->pszValue = NULL;

        LwFreeMemory(pToken);
        pToken = NULL;
    }
}

static
PFSTABTOKEN
ReverseTokenList(
    PFSTABTOKEN pTokenList
    )
{
    PFSTABTOKEN pPrev = NULL;
    PFSTABTOKEN pNext = NULL;
    PFSTABTOKEN pCur = pTokenList;

    while (pCur) {
        pNext = pCur->pNext;
        pCur->pNext = pPrev;
        pPrev = pCur;
        pCur = pNext;
    }

    return pPrev;
}

static
BOOLEAN
IsComment(
    PCSTR pszLine
    )
{
    PCSTR pszTmp = pszLine;

    if (IsNullOrEmptyString(pszLine))
        return TRUE;

    while (*pszTmp != '\0' && isspace((int)*pszTmp))
        pszTmp++;

#if defined(__LWI_AIX__)
    return *pszTmp == '*' || *pszTmp == '\0';
#else
    return *pszTmp == '#' || *pszTmp == '\0';
#endif
}

static
CENTERROR
GetFSTabTokens(
    PSTR pszLine,
    PFSTABTOKEN* ppTokenList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFSTABTOKEN pTokenList = NULL;
    PFSTABTOKEN pToken = NULL;
    PSTR pszToken = NULL;
    PSTR pszToken_last = NULL;
    PSTR pszLineCopy = NULL;
  
    if (IsNullOrEmptyString(pszLine)) {
        *ppTokenList = NULL;
        goto done;
    }

    ceError = LwAllocateString(pszLine, &pszLineCopy);
    BAIL_ON_CENTERIS_ERROR(ceError);

#if defined(__LWI_AIX__)
    pszToken = strtok_r(pszLineCopy, "=", &pszToken_last);
#else
    pszToken = strtok_r(pszLineCopy, " \t", &pszToken_last);
#endif

    while (!IsNullOrEmptyString(pszToken)) {
        ceError = LwAllocateMemory(sizeof(FSTABTOKEN), (PVOID*)&pToken);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwAllocateString(pszToken, &pToken->pszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
    
        pToken->pNext = pTokenList;
        pTokenList = pToken;
        pToken = NULL;

#if defined(__LWI_AIX__)
        pszToken = strtok_r(NULL, "=", &pszToken_last);
#else
        pszToken = strtok_r(NULL, " \t", &pszToken_last);
#endif
    }
  
    *ppTokenList = ReverseTokenList(pTokenList);
    pTokenList = NULL;

done:
error:

    if (pTokenList)
        FreeTokenList(pTokenList);

    if (pToken)
        FreeTokenList(pToken);

    LW_SAFE_FREE_STRING(pszLineCopy);

    return ceError;
}

static
void
CheckMountEntryExists(
    PSTR pszMountPt,
    PFSTAB_OBJECT pFSTabEntry,
    PBOOLEAN pbEntryExists
    )
{
    while(pFSTabEntry) {
        if( !strcmp( (PCSTR)pFSTabEntry->pszMountPt,
                     (PCSTR)pszMountPt)){
            *pbEntryExists = TRUE;
        }
        pFSTabEntry = pFSTabEntry->pNext;
    }
}

#define TRANSFER_FSTAB_TOKEN_VALUE(src, dst)    \
  (dst) = (src); \
  (src) = NULL;

#if defined(__LWI_AIX__)
static
CENTERROR
ReadFSTabFile( 
    PSTR pszFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFSTAB_OBJECT pFSTabEntryList = NULL;
    PFSTAB_OBJECT pFSTabEntry = NULL;
    PFSTABTOKEN pTokenList = NULL;
    BOOLEAN bNextFSTabSection = FALSE;
    BOOLEAN bEntryExists = FALSE;

    FILE *fp = NULL;
    char szBuf[BUFFER_SIZE];

    /* Open a FSTAB file */
    ceError = GPAOpenFile( pszFileName, "r", &fp );
    BAIL_ON_CENTERIS_ERROR(ceError);

    while(1) {
        if (fgets (szBuf, BUFFER_SIZE, fp ) == NULL ) {

            if (feof(fp)) {
                break;
            }
            else {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
     
        if (IsComment(szBuf))
            continue;
      
        ceError = GetFSTabTokens(szBuf, &pTokenList);
        BAIL_ON_CENTERIS_ERROR(ceError);

        CheckMountEntryExists(pTokenList->pNext->pszValue, pFSTabHead, &bEntryExists);

        if(bEntryExists){
            FreeTokenList(pTokenList);
            bEntryExists = FALSE;
            continue;
        }

        PFSTABTOKEN pToken = pTokenList;

        while ( pToken ) {
            if ( strstr(pToken->pszValue, ":") ) {
                if ( bNextFSTabSection  ) {
                    pFSTabEntry->pNext = pFSTabEntryList;
                    pFSTabEntryList = pFSTabEntry;
                    pFSTabEntry = NULL;
                    bNextFSTabSection = FALSE; 
                }

           ceError = LwAllocateMemory(sizeof(FSTAB_OBJECT), (PVOID*)&pFSTabEntry);
           BAIL_ON_CENTERIS_ERROR(ceError);

           TRANSFER_FSTAB_TOKEN_VALUE(pToken->pszValue, pFSTabEntry->pszSectionName);
            }
            else if( strstr(pToken->pszValue, "dev") ) {
                TRANSFER_FSTAB_TOKEN_VALUE(pToken->pNext->pszValue, pFSTabEntry->pszMountVol);
                bNextFSTabSection = TRUE; 
                pToken = pToken->pNext;
            }   
            else if( strstr(pToken->pszValue, "vol") ) {
                TRANSFER_FSTAB_TOKEN_VALUE(pToken->pNext->pszValue, pFSTabEntry->pszMountPt);
                bNextFSTabSection = TRUE; 
                pToken = pToken->pNext;
            }
            else if( strstr(pToken->pszValue, "mount") ) {
                TRANSFER_FSTAB_TOKEN_VALUE(pToken->pNext->pszValue, pFSTabEntry->pszMount);
                bNextFSTabSection = TRUE; 
                pToken = pToken->pNext;
            }
            else if( strstr(pToken->pszValue, "nodename") ) {
                TRANSFER_FSTAB_TOKEN_VALUE(pToken->pNext->pszValue, pFSTabEntry->pszNodeName);
                bNextFSTabSection = TRUE; 
                pToken = pToken->pNext;
            }
            else if( strstr(pToken->pszValue, "options") ) {
                TRANSFER_FSTAB_TOKEN_VALUE(pToken->pNext->pszValue, pFSTabEntry->pszMountOptions);
                bNextFSTabSection = TRUE; 
                pToken = pToken->pNext;
            }
            else if( strstr(pToken->pszValue, "vfs") ) {
                TRANSFER_FSTAB_TOKEN_VALUE(pToken->pNext->pszValue, pFSTabEntry->pszFileType);
                bNextFSTabSection = TRUE; 
                pToken = pToken->pNext;
            }
            else if( strstr(pToken->pszValue, "check") ) {
                TRANSFER_FSTAB_TOKEN_VALUE(pToken->pNext->pszValue, pFSTabEntry->pszPassCode);
                bNextFSTabSection = TRUE; 
                pToken = pToken->pNext;
            }
            else if( strstr(pToken->pszValue, "log") ) {
                TRANSFER_FSTAB_TOKEN_VALUE(pToken->pNext->pszValue, pFSTabEntry->pszLog);
                bNextFSTabSection = TRUE; 
                pToken = pToken->pNext;
            }
            else if( strstr(pToken->pszValue, "account") ) {
                TRANSFER_FSTAB_TOKEN_VALUE(pToken->pNext->pszValue, pFSTabEntry->pszAccount);
                bNextFSTabSection = TRUE; 
                pToken = pToken->pNext;
            }
            else if( strstr(pToken->pszValue, "boot") ) {
                TRANSFER_FSTAB_TOKEN_VALUE(pToken->pNext->pszValue, pFSTabEntry->pszBoot);
                bNextFSTabSection = TRUE; 
                pToken = pToken->pNext;
            }
            else if( strstr(pToken->pszValue, "size") ) {
                TRANSFER_FSTAB_TOKEN_VALUE(pToken->pNext->pszValue, pFSTabEntry->pszSize);
                bNextFSTabSection = TRUE; 
                pToken = pToken->pNext;
            }
            else if( strstr(pToken->pszValue, "type") ) {
                TRANSFER_FSTAB_TOKEN_VALUE(pToken->pNext->pszValue, pFSTabEntry->pszType);
                bNextFSTabSection = TRUE; 
                pToken = pToken->pNext;
            }
            else if( strstr(pToken->pszValue, "free") ) {
                TRANSFER_FSTAB_TOKEN_VALUE(pToken->pNext->pszValue, pFSTabEntry->pszFree);
                bNextFSTabSection = TRUE; 
                pToken = pToken->pNext;
            }

            pToken = pToken->pNext;
        }

        FreeTokenList(pTokenList);
        pTokenList = NULL;
    }

    pFSTabEntry->pNext = pFSTabEntryList;
    pFSTabEntryList = pFSTabEntry;
    pFSTabEntry = NULL;

    pFSTabHead = ReverseFSTabEntryList(pFSTabEntryList);
    pFSTabEntryList = NULL;
    
error:
   
    if (pFSTabEntry) {
        FreeFSTabEntryList(&pFSTabEntry);
    }
    
    if (pTokenList)
        FreeTokenList(pTokenList);
    
    if ( fp ) {
        fclose( fp );
    }
    
    return ceError;
}
#else
static
BOOLEAN
IsValidMountEntry(PFSTABTOKEN pTokenList) {
    int nFieldCnt = 0;
    PSTR pszMountVol = NULL;
    if(pTokenList) {
       pszMountVol = pTokenList->pszValue;
    }
    while(pTokenList) {
        ++nFieldCnt;
        pTokenList = pTokenList->pNext;
    }

    if(nFieldCnt == LWIFSTAB_FIELD_CNT) {
        return TRUE;
    } else { 
        GPA_LOG_VERBOSE("Ignoring [%s] FSTab entry as it is an invalid one.", pszMountVol);
        return FALSE;
    }
}

static
CENTERROR
ReadFSTabFile( 
    PSTR pszFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PFSTAB_OBJECT pFSTabEntryList = NULL;
    PFSTAB_OBJECT pFSTabEntry = NULL;
    PFSTABTOKEN pTokenList = NULL;
    BOOLEAN bEntryExists = FALSE;

    FILE *fp = NULL;
    char szBuf[BUFFER_SIZE];

    /* Open a FSTAB file */
    ceError = GPAOpenFile( pszFileName, "r", &fp );
    BAIL_ON_CENTERIS_ERROR(ceError);

    while(1) {

        if (fgets (szBuf, BUFFER_SIZE, fp ) == NULL ) {

            if (feof(fp)) {
                break;
            }
            else {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
     
        if (IsComment(szBuf))
            continue;
      
        ceError = GetFSTabTokens(szBuf, &pTokenList);
        BAIL_ON_CENTERIS_ERROR(ceError);
      
        CheckMountEntryExists(pTokenList->pNext->pszValue, pFSTabHead, &bEntryExists);

        if(!IsValidMountEntry(pTokenList) || bEntryExists) {
            FreeTokenList(pTokenList);
            pTokenList = NULL;
            bEntryExists = FALSE;
            continue;
        }

        if (pTokenList) {
            int iColType = (int)FSTAB_MOUNTVOLUME;
            PFSTABTOKEN pToken = pTokenList;
    
            ceError = LwAllocateMemory(sizeof(FSTAB_OBJECT), (PVOID*)&pFSTabEntry);
            BAIL_ON_CENTERIS_ERROR(ceError);
    
            while (pToken) {
                switch(iColType)
                {
                    case FSTAB_MOUNTVOLUME:
                    {
                        TRANSFER_FSTAB_TOKEN_VALUE(pToken->pszValue, pFSTabEntry->pszMountVol);
                        break;
                    }
                    case FSTAB_MOUNTPOINT:
                    {
                        TRANSFER_FSTAB_TOKEN_VALUE(pToken->pszValue, pFSTabEntry->pszMountPt);
                        break;
                    }
                    case FSTAB_FILETYPE:
                    {
                        TRANSFER_FSTAB_TOKEN_VALUE(pToken->pszValue, pFSTabEntry->pszFileType);
                        break;
                    }
#if defined(__LWI_SOLARIS__)
                    case FSTAB_DEVICETOFSCK:
                    {
                        TRANSFER_FSTAB_TOKEN_VALUE(pToken->pszValue, pFSTabEntry->pszDeviceToFsck);
                        break;
                    }
                    case FSTAB_FSCKPASS:
                    {
                        TRANSFER_FSTAB_TOKEN_VALUE(pToken->pszValue, pFSTabEntry->pszFsckPass);
                        break;
                    }
                    case FSTAB_MOUNTATBOOT:
                    {
                        TRANSFER_FSTAB_TOKEN_VALUE(pToken->pszValue, pFSTabEntry->pszMountAtBoot);
                        break;
                    }
#endif
                    case FSTAB_MOUNTOPTIONS:
                    {
                        TRANSFER_FSTAB_TOKEN_VALUE(pToken->pszValue, pFSTabEntry->pszMountOptions);
                        break;
                    }
#if !defined(__LWI_SOLARIS__)
                    case FSTAB_FREQUENCY:
                    {
                        TRANSFER_FSTAB_TOKEN_VALUE(pToken->pszValue, pFSTabEntry->pszFrequency);
                        break;
                    }
                    case FSTAB_PASSCODE:
                    {
                        TRANSFER_FSTAB_TOKEN_VALUE(pToken->pszValue, pFSTabEntry->pszPassCode);
                        break;
                    }
#endif
                }
      
                iColType++;
                pToken = pToken->pNext;
      
            }

            pFSTabEntry->pNext = pFSTabEntryList;
            pFSTabEntryList = pFSTabEntry;
            pFSTabEntry = NULL;
    
            FreeTokenList(pTokenList);
            pTokenList = NULL;
        }
    }
    
    pFSTabHead = ReverseFSTabEntryList(pFSTabEntryList);
    pFSTabEntryList = NULL;
    
error:
    
    if (pFSTabEntry) {
        FreeFSTabEntryList(&pFSTabEntry);
    }
    
    if (pTokenList)
        FreeTokenList(pTokenList);
    
    if ( fp ) {
        fclose( fp );
    }
    
    return ceError;
}
#endif

static
CENTERROR
CreateFSTabFile( 
    PSTR pszFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE *fp = NULL;

    fp = fopen( pszFilePath,
                "w");
    if( !fp ) {
        GPA_LOG_VERBOSE("Failed to create file %s", pszFilePath);
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if ( fp ) {
        fclose( fp );
    }

    return ceError;
}

static
CENTERROR
ApplyLwiFSTabPolicy( 
    PGPOLWIGPITEM pGPItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIGPITEM pNewItem = NULL;
    PGPOLWIGPITEM pGPItemFromFile = NULL;
    BOOLEAN bExists = FALSE;
    FILE *fp = NULL;

    ceError = GPACheckFileExists( LWIFSTAB_BACKUP_FILEPATH,
                                 &bExists );
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if (!pGPItem) {

        ceError = ResetFSTabPolicy();
        BAIL_ON_CENTERIS_ERROR(ceError);
    } 
    else {
        if ( !bExists ) {
            ceError = GPACheckFileExists( LWIFSTAB_FILEPATH,
                                         &bExists );
            BAIL_ON_CENTERIS_ERROR(ceError);
            if ( !bExists ) {
                ceError = CreateFSTabFile( LWIFSTAB_BACKUP_FILEPATH);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
            else {
                ceError = GPACopyFileWithOriginalPerms( LWIFSTAB_FILEPATH,
                                                       LWIFSTAB_BACKUP_FILEPATH);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        GPA_LOG_VERBOSE("Applying new policy to fstab");

        /* Build fstab file list from systemfiles/fstab*/
        ceError = ReadFSTabFile(LWIFSTAB_BACKUP_FILEPATH);
        BAIL_ON_CENTERIS_ERROR(ceError);

        /* Compute modified fstab entries from RSoP XML tree */
        ceError = ComputeFSTabModList( (xmlNodePtr)pGPItem->xmlNode );
        BAIL_ON_CENTERIS_ERROR(ceError);

        /* Write the RSoP fstab file list*/
        ceError = WriteFSTabFile( LWIFSTAB_FILEPATH_GP, 
                                  LWIFSTAB_BACKUP_FILEPATH);
        BAIL_ON_CENTERIS_ERROR(ceError);

        /* copy the staging file to the real one */
        ceError = GPACopyFileWithPerms( LWIFSTAB_FILEPATH_GP, 
                                       LWIFSTAB_FILEPATH,
                                       S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH );
        BAIL_ON_CENTERIS_ERROR(ceError);

        /* remove the staging file */
        ceError = LwRemoveFile( LWIFSTAB_FILEPATH_GP );
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = MountEntries();
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    if ( fp ) {
        fclose( fp );
    }

    if (pNewItem) {
        GPODestroyGPItem( pNewItem,
                          TRUE );
    }

    if (pGPItemFromFile) {
       GPODestroyGPItem( pGPItemFromFile,
                         TRUE);
    }

    if (pFSTabHead) {
        FreeFSTabEntryList( &pFSTabHead );
    }

    return ceError;
}

CENTERROR
ProcessFSTabGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN   bSomeGPModified = FALSE;
    PGPOLWIGPITEM pGPFSTabItem = NULL;
    PGPOLWIGPITEM pRsopGPFSTabItem = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszPolicyIdentifier = NULL;
    PSTR pszSourcePath = NULL;
    PGPOLWIDATA pLwidata = NULL;
    PGPOLWIDATALIST pLwidataList = NULL;

    GPA_LOG_FUNCTION_ENTER();
    
    bSomeGPModified = ( pGPODeletedList != NULL );
    
    for (; pGPOModifiedList; pGPOModifiedList = pGPOModifiedList->pNext) {
        BOOLEAN applicable;
        
        if (pGPOModifiedList->bNewVersion) {
            bSomeGPModified = TRUE;
        }

        ceError = GPOXmlVerifyPlatformApplicable( NULL,
                                                  MACHINE_GROUP_POLICY,
                                                  pGPOModifiedList,
                                                  NULL,
                                                  &applicable);

        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if (!applicable) {
            GPA_LOG_VERBOSE("GPO(%s) disabled by platform targeting",
                            pGPOModifiedList->pszDisplayName);
            RemoveFSTabCredentialsFolder(pGPOModifiedList->pszDisplayName);

        } else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE("Ignoring GPO(%s) disabled by flags: 0x%.08x",
                            pGPOModifiedList->pszDisplayName,
                            pGPOModifiedList->dwFlags);
            RemoveFSTabCredentialsFolder(pGPOModifiedList->pszDisplayName);

        } else {

            // Look in new policy file location
            ceError = GPOInitLwiData( NULL,
                                      MACHINE_GROUP_POLICY,
                                      (PGPOLWIDATA*)&pLwidata,
                                      pGPOModifiedList->pszgPCFileSysPath,
                                      NULL,
                                      LWIFSTAB_CLIENT_GUID );
            if ( ceError ) {
                // Look in old policy file location
                ceError = GPOInitLwiData( NULL,
                                          MACHINE_GROUP_POLICY,
                                          (PGPOLWIDATA*)&pLwidata,
                                          pGPOModifiedList->pszgPCFileSysPath,
                                          NULL,
                                          NULL );
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            ceError = GPOGetGPItem( MACHINE_GROUP_POLICY,
                                    pLwidata,
                                    LWIFSTAB_ITEM_GUID,
                                    &pGPFSTabItem );
            if (!CENTERROR_IS_OK(ceError) &&
                CENTERROR_EQUAL(ceError, CENTERROR_GP_XML_GPITEM_NOT_FOUND)) {
                ceError = CENTERROR_SUCCESS;
            }
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        
        if ( pGPFSTabItem ) {

            ceError = GPACrackFileSysPath( pGPOModifiedList->pszgPCFileSysPath,
                                           &pszDomainName,
                                           &pszSourcePath,
                                           &pszPolicyIdentifier);
            BAIL_ON_CENTERIS_ERROR(ceError);

            // Parse FSTab GPItem to get mount point
            ceError = ParseFSTabGPItem( pGPFSTabItem->xmlNode,
                                        pGPOModifiedList->pszDisplayName,
                                        pszPolicyIdentifier);
            BAIL_ON_CENTERIS_ERROR(ceError);

            LW_SAFE_FREE_STRING(pszDomainName);
            LW_SAFE_FREE_STRING(pszSourcePath);
            LW_SAFE_FREE_STRING(pszPolicyIdentifier);

            if ( pRsopGPFSTabItem ) {
                /* pGPFSTabItem is modified to include items from pRsopGPFSTabItem */
                ceError = GPOCalculateRSOP( pRsopGPFSTabItem,
                                            pGPFSTabItem );                
                BAIL_ON_CENTERIS_ERROR(ceError);

                GPODestroyGPItem( pRsopGPFSTabItem, FALSE );
                pRsopGPFSTabItem = NULL;
            }
            /* the current pGPItem has all RSOP changes */
            pRsopGPFSTabItem = pGPFSTabItem;
            pGPFSTabItem = NULL;
        }

        // Hold on to each Lwidata, so that it can be reference for the life of this function
        if ( pLwidata ) {
            PGPOLWIDATALIST pTemp = pLwidataList;
            PGPOLWIDATALIST pPrev = NULL;
            PGPOLWIDATALIST pNew = NULL;

            ceError = LwAllocateMemory(sizeof(GPOLWIDATALIST), (PVOID*)&pNew);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pNew->pLwidata = pLwidata;

            // Insert the new node with its lwidata object to list (if any)
            while (pTemp) {
                pPrev = pTemp;
                pTemp = pTemp->pNext;
            }

            if (pPrev) {
                pPrev->pNext = pNew;
            } else {
                pLwidataList = pNew;
            }

            pLwidata = NULL;
        }
    }
        
    ceError = ApplyLwiFSTabPolicy( pRsopGPFSTabItem);    
    BAIL_ON_CENTERIS_ERROR(ceError);

error:
 
    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    if ( pGPFSTabItem ) {
        GPODestroyGPItem( pGPFSTabItem,
                          FALSE );
    }
    
    if ( pRsopGPFSTabItem ) {
        GPODestroyGPItem( pRsopGPFSTabItem,
                          FALSE );
    }

    // Now free any Lwidata settings we have used.
    while ( pLwidataList ) {
        PGPOLWIDATALIST pTemp = pLwidataList;
        pLwidataList = pLwidataList->pNext;

        GPODestroyLwiData(pTemp->pLwidata);
        LwFreeMemory(pTemp);
    }

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return(ceError);
}
