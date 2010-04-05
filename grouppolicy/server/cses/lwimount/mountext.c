#include "includes.h"

static const PSTR SELINUX_DIRECTORY = "/etc/selinux/";

static PMOUNTPOLICYLIST g_MountPolicyHierarchyList   = NULL;
static PMOUNTPOLICYLIST g_MountPolicyRemoveList      = NULL;


static
BOOLEAN
IsSpecialMap(
    PCSTR pszPath
    )
{
    PCSTR pszTmp = pszPath;

    if (IsNullOrEmptyString(pszTmp))
        return TRUE;

    while (*pszTmp != '\0' && isspace((int)*pszTmp))
        pszTmp++;

    return (*pszTmp == '+' || *pszTmp == '\0');
}

static
BOOLEAN
IsSpecialMountToken(
    PCSTR pszPath
    )
{
    PCSTR pszTmp = pszPath;

    if (IsNullOrEmptyString(pszTmp))
        return TRUE;

    while (*pszTmp != '\0' && isspace((int)*pszTmp))
        pszTmp++;

    return (*pszTmp == '-' || *pszTmp == '\0');
}

static
CENTERROR
ReplaceFileNameChar(
    PSTR pszFileName,
    PSTR *ppszFileNameReplaced
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszCharPos = NULL;

    ceError = LwAllocateMemory( 256,
                                (PVOID *)ppszFileNameReplaced);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if( (pszCharPos = strchr( pszFileName, 
                             '.')) ) {
        strncpy( *ppszFileNameReplaced,
                 (PCSTR)pszFileName,
                 pszCharPos - pszFileName);

        strcat( *ppszFileNameReplaced,
                (PCSTR)"_");

        strcat( *ppszFileNameReplaced,
                (PCSTR)(pszCharPos + 1));
    }
    else {
        strcpy( *ppszFileNameReplaced,
                (PCSTR)pszFileName);
    }

error:

    return ceError;
}

static
CENTERROR
ProcessMapLine(
    PSTR pszFileName,
    PSTR *ppszNewPath,
    PCSTR pszConfigDirPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszFilePath = NULL;
    BOOLEAN bFileExists = FALSE;
    PSTR pszFileNameReplaced = NULL;

    ceError = ReplaceFileNameChar( pszFileName,
                                   &pszFileNameReplaced);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( 256,
                                (PVOID *)&pszFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf( pszFilePath, 
             "%s/%s", 
             pszConfigDirPath, 
             pszFileNameReplaced);

    LwStripWhitespace(pszFilePath,1,1);

    ceError = GPACheckFileExists( pszFilePath, 
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        sprintf( pszFilePath,
                 "%s/%s", 
                 GP_AM_REDIRECT_LINK, 
                 pszFileNameReplaced);

        ceError = LwAllocateString( pszFilePath, 
                                    ppszNewPath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (pszFilePath)
        LwFreeString(pszFilePath);

    if (pszFileNameReplaced)
        LwFreeString(pszFileNameReplaced);

    return ceError;
}

static
CENTERROR
GetReplacementPath(
    PSTR pszOrigPath,
    PCSTR pszConfigDirPath,
    PSTR* ppszNewPath
   )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDirPath = NULL;
    PSTR pszFileName = NULL;
    PSTR pszTmp = NULL;

    *ppszNewPath = NULL;
  
    if (!IsNullOrEmptyString(pszOrigPath)) {
        if (IsSpecialMountToken(pszOrigPath)) {
            // No changes
            ;
        } else if (!GPAIsAbsolutePath(pszOrigPath)) { 
            pszDirPath = NULL;
            ceError = LwAllocateString( pszOrigPath, 
                                        &pszFileName);
            BAIL_ON_CENTERIS_ERROR(ceError);      
        } else if (!strcmp(pszOrigPath, "/")) {
            ceError = LwAllocateString( pszOrigPath, 
                                        &pszDirPath);
            BAIL_ON_CENTERIS_ERROR(ceError);
            pszFileName = NULL;
        } else {
            pszTmp = pszOrigPath + strlen(pszOrigPath) - 1;
            while ((*pszTmp != '/' && (pszTmp != pszOrigPath))) {
                pszTmp--;
            }

            ceError = LwAllocateMemory( pszTmp - pszOrigPath + 1, 
                                        (PVOID*)&pszDirPath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            strncpy( pszDirPath, 
                     pszOrigPath, 
                     pszTmp-pszOrigPath);

            ceError = LwAllocateString( pszTmp + 1, 
                                        &pszFileName);
            BAIL_ON_CENTERIS_ERROR(ceError);

            LwStripWhitespace(pszFileName,1,1);      
        }
            
        if ( !IsNullOrEmptyString(pszFileName) &&
             (IsNullOrEmptyString(pszDirPath)  ||
             !strcmp( pszDirPath, 
                      "/etc")                  ||
             !strcmp( pszDirPath, 
                      GP_AM_REDIRECT_LINK))) {

            ceError = ProcessMapLine( pszFileName,
                                      ppszNewPath,
                                      pszConfigDirPath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

error:

    if (pszDirPath)
        LwFreeString(pszDirPath);
  
    if (pszFileName)
        LwFreeString(pszFileName);

    return ceError;
}

// If you get back NULL as the new line
// use the original line
static
CENTERROR
GetReplacementLine(
    PSTR pszOrigLine,
    PCSTR pszConfigDirPath,
    PSTR* ppszNewLine
   )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszLine = NULL;
    PSTR pszMountName = NULL;
    DWORD dwMountNameLen = 0;
    PSTR pszMountFile = NULL;
    DWORD dwMountFileLen = 0;
    PSTR pszMountOptions = NULL;
    PSTR pszReplacementPath = NULL;
    PSTR pCh = 0;
    PSTR pszTmp = NULL;

    *ppszNewLine = NULL;

    if (!GPAIsComment(pszOrigLine)) {
        ceError = LwAllocateString( pszOrigLine, 
                                    &pszLine);
        BAIL_ON_CENTERIS_ERROR(ceError);

        LwStripWhitespace(pszLine,1,1);

        // Mount Name
        pCh = pszLine;
        while (*pCh && !isspace((int)*pCh)) {
            dwMountNameLen++;
            pCh++;
        }

        ceError = LwAllocateMemory( dwMountNameLen + 1, 
                                    (PVOID*)&pszMountName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strncpy( pszMountName, 
                 pszLine, 
                 dwMountNameLen);

        if (IsSpecialMap(pszMountName))
            goto done;

        // Skip spaces
        while(*pCh && isspace((int)*pCh))
            pCh++;

        // Mount File
        pszTmp = pCh;
        while (*pCh && !isspace((int)*pCh)) {
            dwMountFileLen++;
            pCh++;
        }

        if (!dwMountFileLen) {
            GPA_LOG_WARNING( "Invalid Mount File on line [%s]", 
                             pszLine);
            goto done;
        }

        ceError = LwAllocateMemory( dwMountFileLen + 1, 
                                    (PVOID*)&pszMountFile);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strncpy( pszMountFile, 
                 pszTmp, 
                 dwMountFileLen);

        // Skip spaces
        while (*pCh && isspace((int)*pCh))
            pCh++;

        // pszMountOptions
        pszMountOptions = pCh;
       
        ceError = GetReplacementPath( pszMountFile,
                                      pszConfigDirPath,
                                      &pszReplacementPath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!IsNullOrEmptyString(pszReplacementPath)) {
            ceError = LwAllocateMemory( dwMountNameLen +
                                        strlen(pszReplacementPath) +
                                        ((pszMountOptions ?  strlen(pszMountOptions) : 0) + 4),
                                        (PVOID*)ppszNewLine);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (!IsNullOrEmptyString(pszMountOptions)) {
                sprintf( *ppszNewLine, 
                         "%s %s %s\n", 
                         pszMountName, 
                         pszReplacementPath, 
                         pszMountOptions);
            } else {
                sprintf( *ppszNewLine, 
                         "%s %s\n", 
                         pszMountName, 
                         pszReplacementPath);
            }
        }
    }

done:
error:

    if (pszLine)
        LwFreeMemory(pszLine);

    if (pszMountName)
        LwFreeString(pszMountName);
 
    if (pszMountFile)
        LwFreeString(pszMountFile);

    if (pszReplacementPath)
        LwFreeString(pszReplacementPath);

    return ceError;

}


// As part of applying the automount group policy
// we want to change the automaster file in /etc with
// the one we got from the domain controller. However,
// we don't copy any of the other associated files we
// got from AD that the automaster file references. We
// keep all other files we got from the domain controller
// in our cache folder. In order to get this to work, we
// must edit the references in the automaster file to point
// to the files in our cache.
static
CENTERROR
PreProcessAutoMasterFile(
    PCSTR pszDestFolderPath,
    PCSTR pszFileName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szOldPath[256];
    CHAR szNewPath[256];
    CHAR szBuf[1024+1];
    PSTR pszReplacementLine = NULL;
    FILE* fpNew = NULL;
    FILE* fpOld = NULL;
    BOOLEAN bRemoveFile = FALSE;
    BOOLEAN bChanged = FALSE;

    sprintf( szOldPath, 
             "%s/%s", 
             pszDestFolderPath, 
             pszFileName);

    sprintf( szNewPath, 
             "%s/%s.new", 
             pszDestFolderPath, 
             pszFileName);

    ceError = GPAOpenFile( szOldPath, 
                          "r",
                          &fpOld);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAOpenFile( szNewPath, 
                          "w",
                          &fpNew);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = TRUE;

    while (1) { 
        if (fgets(szBuf, 1024, fpOld) == NULL) {
            if (feof(fpOld)) {
                break;
            } else {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        ceError = GetReplacementLine( szBuf, 
                                      pszDestFolderPath, 
                                      &pszReplacementLine);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (IsNullOrEmptyString(pszReplacementLine)) {
            if (fputs(szBuf, fpNew) == EOF) {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        } else {
            bChanged = TRUE;
            if (fputs(pszReplacementLine, fpNew) == EOF) {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
            LwFreeString(pszReplacementLine);
            pszReplacementLine = NULL;
        }
    }

    fclose(fpOld); fpOld = NULL;
    fclose(fpNew); fpNew = NULL;

    if (bChanged) {
        ceError = LwMoveFile( szNewPath, 
                              szOldPath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        bRemoveFile = FALSE;
    }

error:

    if (fpOld) {
        fclose(fpOld);
    }

    if (fpNew) {
        fclose(fpNew);
    }

    if (bRemoveFile) {
        LwRemoveFile(szNewPath);
    }

    if (pszReplacementLine)
        LwFreeString(pszReplacementLine);

    return ceError;

}

static
CENTERROR
RestartAutomounter()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szCmd[PATH_MAX+1];

    //Run gprsrtmnt.sh with 10 second timeout
    sprintf( szCmd, 
             "( %s/bin/gprsrtmnt.sh restart & COMPID=$! ; ( sleep 10; kill $COMPID 2>/dev/null ) & TIMEOUTPID=$! ; wait $COMPID ; kill $TIMEOUTPID 2>/dev/null )", 
             PREFIXDIR);
  
    /*
     * TODO: Get better error status from the child
     */
    ceError = GPARunCommand(szCmd);
    if (ceError != CENTERROR_SUCCESS) {
        GPA_LOG_VERBOSE("There were some problems in reloading mount maps. Please recheck automount master and map files.");
        ceError = CENTERROR_SUCCESS;
    }

    return ceError;
}

static
void
FreeMountPolicyList(
    PMOUNTPOLICYLIST pPolicyList
    )
{
    PMOUNTPOLICYLIST pTemp = NULL;

    while (pPolicyList)
    {
        pTemp = pPolicyList;
        pPolicyList = pPolicyList->pNext;

        if (pTemp->pszFilepath) {
            LwFreeString(pTemp->pszFilepath);
        }
        if (pTemp->pszDirpath) {
            LwFreeString(pTemp->pszDirpath);
        }
        if (pTemp->pszPolicyIdentifier) {
            LwFreeString(pTemp->pszPolicyIdentifier);
        }
        LwFreeMemory(pTemp);
        pTemp = NULL;
    }
}

static
void
ResetPolicyFileHierarchy()
{
    FreeMountPolicyList(g_MountPolicyHierarchyList);
    g_MountPolicyHierarchyList = NULL;
}


static
CENTERROR
RemovePolicyDirFromRemoveList(
    PSTR pszDirName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMOUNTPOLICYLIST pTemp = g_MountPolicyRemoveList;
    PMOUNTPOLICYLIST pPrev = NULL;
    PMOUNTPOLICYLIST pDelete = NULL;

    while (pTemp) {
        if ( strcasecmp( pTemp->pszDirpath,
                         pszDirName ) == 0 ) {
            pDelete = pTemp;
            if ( pPrev ) {
                pPrev->pNext = pTemp->pNext;
                pTemp = pTemp->pNext;
            } else {
                pTemp = pTemp->pNext;
                g_MountPolicyRemoveList = pTemp;
            }

            pDelete->pNext = NULL; /* Don't want to delete more than just the specific node */

            FreeMountPolicyList(pDelete);     
            pDelete = NULL;

        } else {
            pPrev = pTemp;
            pTemp = pTemp->pNext;
        }
    }

    return ceError;
}

static
void
ResetPolicyRemoveList(
    )
{
    FreeMountPolicyList(g_MountPolicyRemoveList);
    g_MountPolicyRemoveList = NULL;
}

static
CENTERROR
ProcessPolicyRemoveList()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMOUNTPOLICYLIST pPolicyList = g_MountPolicyRemoveList;
    BOOLEAN bDirExists = 0;

    GPA_LOG_INFO("Processing AutoMount remove list...");

    while (pPolicyList) {
        GPA_LOG_INFO( "AutoMount Remove List Entry: %s",
                      pPolicyList->pszDirpath);

        ceError = GPACheckIfMountDirExists( pPolicyList->pszDirpath,
                                             &bDirExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bDirExists) {
            GPA_LOG_VERBOSE("Removing stale Auto Mount Directory.");
            GPARemoveDirectory(pPolicyList->pszDirpath);
        }
        pPolicyList = pPolicyList->pNext;
    }

    ResetPolicyRemoveList();

error:

    return ceError;
}


static
CENTERROR
AddPolicyDirToRemoveList(
    PSTR pszDirName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMOUNTPOLICYLIST pTemp = g_MountPolicyRemoveList;
    PMOUNTPOLICYLIST pPrev = NULL;
    PMOUNTPOLICYLIST pNew = NULL;

    ceError = LwAllocateMemory( sizeof(MOUNTPOLICYLIST),
                                (PVOID*)&pNew);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszDirName,
                                &pNew->pszDirpath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while (pTemp) {
        pPrev = pTemp;
        pTemp = pTemp->pNext;
    }

    if (pPrev) {
        pPrev->pNext = pNew;
    } else {
        g_MountPolicyRemoveList = pNew;
    }

    pNew = NULL;

error:

    if (pNew)
    { 
        FreeMountPolicyList(pNew);     
    }

    return ceError;
}



static
CENTERROR
AddPolicyFileToHierarchy(
    PSTR pszFileName,
    PSTR pszPolicyIdentifier
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMOUNTPOLICYLIST pTemp = g_MountPolicyHierarchyList;
    PMOUNTPOLICYLIST pPrev = NULL;
    PMOUNTPOLICYLIST pNew = NULL;

    ceError = LwAllocateMemory( sizeof(MOUNTPOLICYLIST), 
                                (PVOID*)&pNew);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszFileName, 
                                &pNew->pszFilepath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pszPolicyIdentifier, 
                                &pNew->pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    GPA_LOG_VERBOSE("AddPolicyFileToHierarchy: pszFileName=%s", pszFileName); 

    while (pTemp) {
        pPrev = pTemp;
        pTemp = pTemp->pNext;
    }

    if (pPrev) {
        pPrev->pNext = pNew;
    } else {
        g_MountPolicyHierarchyList = pNew;
    }

    pNew = NULL;

error:

    if (pNew)
    { 
        FreeMountPolicyList(pNew);     
    }
    
    return ceError;

}

static
void
RemovePolicyFolderFromHierarchy(
    PSTR pszFilepath
    )
{
    PMOUNTPOLICYLIST pTemp = g_MountPolicyHierarchyList;
    PMOUNTPOLICYLIST pPrev = NULL;
  
    while (pTemp) {    
        if ( !strcasecmp( pTemp->pszFilepath, 
                          pszFilepath) ) {
            if (pPrev) {
                pPrev->pNext = pTemp->pNext;
            } else {
                g_MountPolicyHierarchyList = pTemp->pNext;
            }

            pTemp->pNext = NULL; /* Don't want to delete more than just the specific node */
            FreeMountPolicyList(pTemp);     
            pTemp = NULL;
            break;
        }
    
        pPrev = pTemp;
        pTemp = pTemp->pNext;
    }
}

static
CENTERROR
RemoveMountPolicy(
    PSTR pszgPCFileSyspath,
    PSTR pszDisplayName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szNewFilePath[256];
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    BOOLEAN bDirExists = FALSE;

    ceError = GPACrackFileSysPath( pszgPCFileSyspath,
                                   &pszDomainName,
                                   &pszSourcePath,
                                   &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf( szNewFilePath,
             "%s%s_likewise_automount",
             CENTERIS_GP_DIRECTORY,
             pszPolicyIdentifier);

    GPA_LOG_VERBOSE( "Need to remove automount policy folder: %s [%s]",
                     pszDisplayName, 
                     szNewFilePath);

    ceError = GPACheckDirectoryExists( szNewFilePath, 
                                      &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bDirExists) {
        ceError = GPARemoveDirectory(szNewFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        RemovePolicyFolderFromHierarchy(szNewFilePath);
    }

error:

  return ceError;

}

static
void
FreeMountFileInfo(
    PMOUNTFILEINFO pMountFileInfo
    )
{
    if (pMountFileInfo->pszFilename)
        LwFreeString(pMountFileInfo->pszFilename);

    LwFreeMemory(pMountFileInfo);
}

static
void
FreeMountFileInfoList(
    PMOUNTFILEINFO pMountFileInfoList
    )
{
    PMOUNTFILEINFO pTemp = NULL;

    while (pMountFileInfoList) {
        pTemp = pMountFileInfoList;

        pMountFileInfoList = pMountFileInfoList->pNext;

        FreeMountFileInfo(pTemp);
    }
}

static
void
FreeMountInfoList(
    PMOUNTINFO pMountInfoList
   )
{
    PMOUNTINFO pTemp = NULL;

    while (pMountInfoList) {
        pTemp = pMountInfoList;
        pMountInfoList = pMountInfoList->pNext;

        if (pTemp->pMountFileInfoList)
            FreeMountFileInfoList(pTemp->pMountFileInfoList);

        LwFreeMemory(pTemp);
    }
}

static
void
FreeMountConfig(
    PMOUNTCONFIG pMountConfig
   )
{
    if (pMountConfig->pMountInfoList)
        FreeMountInfoList(pMountConfig->pMountInfoList);

    LwFreeMemory(pMountConfig);
}

static
CENTERROR
ParseMountInfo(
    xmlNodePtr pNodePtr,
    PMOUNTINFO* ppMountInfo
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMOUNTINFO     pMountInfo = NULL;
    PMOUNTFILEINFO pMountFileInfo_head = NULL;
    PMOUNTFILEINFO pMountFileInfo_tail = NULL;
    PMOUNTFILEINFO pMountFileInfo = NULL;
    xmlNodeSetPtr  pXmlNodeSetPtr = NULL;
    xmlNodePtr     pXmlNodePtr = NULL;
    xmlChar*       pAttrValue = NULL;
    int iNode = 0;

    ceError = LwAllocateMemory( sizeof(MOUNTINFO), 
                                (PVOID*)&pMountInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    // We are supporting only AUTOFS for now
    pMountInfo->amtype = AUTOFS;

    // Get the set of FILE elements
    ceError = GPOXmlSelectNodes( pNodePtr,
                                 "FILE",
                                 &pXmlNodeSetPtr);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pXmlNodeSetPtr) {
        for (iNode = 0; iNode < pXmlNodeSetPtr->nodeNr; iNode++) {
            pXmlNodePtr = pXmlNodeSetPtr->nodeTab[iNode];

            ceError = LwAllocateMemory( sizeof(MOUNTFILEINFO), 
                                        (PVOID*)&pMountFileInfo);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pAttrValue = xmlGetProp(pXmlNodePtr, (const xmlChar*) "Name");
            if (pAttrValue) {
                ceError = LwAllocateString( (char*)pAttrValue, 
                                            (char**)&pMountFileInfo->pszFilename);
                BAIL_ON_CENTERIS_ERROR(ceError);

                xmlFree(pAttrValue);
                pAttrValue = NULL;
            }

            pAttrValue = xmlGetProp( pXmlNodePtr, 
                                     (const xmlChar*)"Executable");
            if (pAttrValue && !strcasecmp( (const char*)pAttrValue, 
                                           "yes")) {
                pMountFileInfo->bIsExecutable = TRUE;
            }
            else {
                pMountFileInfo->bIsExecutable = FALSE;
            }

            if (pAttrValue) {
                xmlFree(pAttrValue);
                pAttrValue = NULL;
            }

            if (!pMountFileInfo_head) {
                pMountFileInfo_head = pMountFileInfo;
                pMountFileInfo_tail = pMountFileInfo;
            } 
            else {
                pMountFileInfo_tail->pNext = pMountFileInfo;
                pMountFileInfo_tail = pMountFileInfo;
            }
            pMountFileInfo = NULL;
        }
    }

    pMountInfo->pMountFileInfoList = pMountFileInfo_head;
    pMountFileInfo_head = NULL;

    *ppMountInfo = pMountInfo;
    pMountInfo = NULL;

error:

    if (pAttrValue) {
        xmlFree(pAttrValue);
        pAttrValue = NULL;
    }

    if (pMountFileInfo) {
        FreeMountFileInfo(pMountFileInfo);
    }

    if (pMountFileInfo_head) {
        FreeMountFileInfoList(pMountFileInfo_head);
    }

    if (pMountInfo) {
        FreeMountInfoList(pMountInfo);
    }

    if (pXmlNodeSetPtr) {
        xmlXPathFreeNodeSet(pXmlNodeSetPtr);
    }

    return ceError;

}

static
CENTERROR
ParseMountConfig(
    PGPOLWIGPITEM pGPItem,
    PMOUNTCONFIG* ppMountConfig
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMOUNTCONFIG pMountConfig = NULL;
    PMOUNTINFO   pMountInfo_head = NULL;
    PMOUNTINFO   pMountInfo_tail = NULL;
    PMOUNTINFO   pMountInfo = NULL;
    xmlNodeSetPtr pXmlNodeSetPtr = NULL;
    xmlNodePtr   pXmlNodePtr = NULL;
    int           iNode = 0;

    ceError = LwAllocateMemory( sizeof(MOUNTCONFIG), 
                                (PVOID*)&pMountConfig);
    BAIL_ON_CENTERIS_ERROR(ceError);

    // Get the set of AUTOMOUNTPOLICY elements
    ceError = GPOXmlSelectNodes( pGPItem->xmlNode,
                                 AUTOMOUNT_XML_QUERY,
                                 &pXmlNodeSetPtr);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pXmlNodeSetPtr) {
        for (iNode = 0; iNode < pXmlNodeSetPtr->nodeNr; iNode++) {
            pXmlNodePtr = pXmlNodeSetPtr->nodeTab[iNode];

            ceError = ParseMountInfo(pXmlNodePtr, &pMountInfo);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (!pMountInfo_head) {
                pMountInfo_head = pMountInfo;
                pMountInfo_tail = pMountInfo;
            }
            else {
                pMountInfo_tail->pNext = pMountInfo;
                pMountInfo_tail = pMountInfo;
            }
            pMountInfo = NULL;
        }
    }

    pMountConfig->pMountInfoList = pMountInfo_head;
    pMountInfo_head = NULL;

    *ppMountConfig = pMountConfig;
    pMountConfig = NULL;

error:

    if (pMountInfo) {
        FreeMountInfoList(pMountInfo);
    }

    if (pMountInfo_head) {
        FreeMountInfoList(pMountInfo_head);
    }

    if (pMountConfig) {
        FreeMountConfig(pMountConfig);
    }

    if (pXmlNodeSetPtr) {
        xmlXPathFreeNodeSet(pXmlNodeSetPtr);
    }

    return ceError;

}

static
BOOLEAN
IsSELinuxSupported()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bDirExists = FALSE;

    /* If any system has a SELinux configuration directory
     * we will consider this platform suitable for applying this policy */
    ceError = GPACheckDirectoryExists(SELINUX_DIRECTORY, &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bDirExists == FALSE) {
        ceError = CENTERROR_INVALID_OPERATION;
        goto error;
    }

error:
    return ceError == CENTERROR_SUCCESS;
}


static
CENTERROR
FixFilePermissions(
    PSTR pszDestFolderPath,
    PGPOLWIGPITEM pGPItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szPath[PATH_MAX+1];
    BOOLEAN bFileExists = FALSE;
    PMOUNTCONFIG pMountConfig = NULL;
    PMOUNTINFO pMountInfo = NULL;
    PMOUNTFILEINFO pMountFileInfo = NULL;
    DWORD dwPerms = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;

    ceError = ParseMountConfig( pGPItem, 
                                &pMountConfig);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (pMountInfo = pMountConfig->pMountInfoList; pMountInfo != NULL; pMountInfo = pMountInfo->pNext) {
        switch(pMountInfo->amtype) {
            case AUTOFS:
            {
                for(pMountFileInfo = pMountInfo->pMountFileInfoList; pMountFileInfo != NULL; pMountFileInfo = pMountFileInfo->pNext) {              
                    sprintf( szPath, 
                             "%s/%s", 
                             pszDestFolderPath, 
                             pMountFileInfo->pszFilename);

                    ceError = GPACheckFileExists( szPath, 
                                                 &bFileExists);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    if (!bFileExists) {
                        GPA_LOG_WARNING( "Automount file [%s] does not exist", 
                                         szPath);
                    } else {
                        ceError = LwChangePermissions( szPath, 
                                                       (pMountFileInfo->bIsExecutable ? dwPerms | S_IXUSR : dwPerms));
                        BAIL_ON_CENTERIS_ERROR(ceError);
                    }
                }
            }
            break;

            case AMD:
            {
                GPA_LOG_WARNING("The Berkeley Automounter Daemon (AMD) is not supported at this time");
            }
            break;
        }
    }

    /* If system is supporting SELinux, we need to change the security
       descriptor for the policy setting files to have etc_t type. */
    if (IsSELinuxSupported()) {
        sprintf( szPath, 
                 "chcon -t etc_t %s/*", 
                 pszDestFolderPath);

        ceError = GPARunCommand( szPath );
        if(!CENTERROR_IS_OK(ceError)) {
            GPA_LOG_ALWAYS( "Automount CSE unable to set security descriptor for policy files with command: %s", 
                            szPath );
            ceError = CENTERROR_SUCCESS;
        }
    }

/*  done:*/
error:

    if (pMountConfig)
        FreeMountConfig(pMountConfig);
  
    return ceError;

}

static
CENTERROR
AddMountPolicy(
    PGROUP_POLICY_OBJECT pGPO
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szSourceFolderPath[256];
    CHAR szDestFolderPath[256];
    CHAR szNewFilePath[256];
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    BOOLEAN bFileExists = FALSE;
    BOOLEAN bDirExists = FALSE;
    BOOLEAN bFolderExists = FALSE;
    PSTR pszDC = NULL;
    BOOLEAN bRemoveFolder = FALSE;
    PGPOLWIGPITEM pGPItem = NULL;
    PGPOLWIDATA pLwidata = NULL;
    CHAR  szBuf[PATH_MAX+1];
    BOOLEAN fNewPolicy = FALSE;

    if (!pGPO) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fNewPolicy = pGPO->bNewVersion;

    memset( szSourceFolderPath, 
            0, 
            sizeof(szSourceFolderPath));
    memset( szDestFolderPath, 
            0, 
            sizeof(szDestFolderPath));
    memset( szNewFilePath, 
            0, 
            sizeof(szNewFilePath));

    ceError =  GPACrackFileSysPath( pGPO->pszgPCFileSysPath,
                                    &pszDomainName,
                                    &pszSourcePath,
                                    &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetPreferredDC(&pszDC);
    if (!CENTERROR_IS_OK(ceError)) {
        if ( CENTERROR_EQUAL(ceError, CENTERROR_GP_NO_SMB_KRB5_SITE_KDC_INFO)    ||
             CENTERROR_EQUAL(ceError, CENTERROR_GP_NO_SMB_KRB5_SITE_INFO)) {
            GPA_LOG_ALWAYS( "GPAgent unable to obtain preferred server for AD site: FQDN(%s)", 
                            pszDomainName);
            ceError = CENTERROR_SUCCESS;
        } else {
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    sprintf( szSourceFolderPath, 
             "%s\\Machine\\Centeris\\Identity\\%s", 
             pszSourcePath, 
             LWIAUTOMOUNT_CLIENT_GUID);

    sprintf( szDestFolderPath,
             "%s%s_likewise_automount",
             CENTERIS_GP_DIRECTORY,
             pszPolicyIdentifier);

    sprintf( szNewFilePath, 
             "%s/%s", 
             szDestFolderPath, 
             GP_AM_MASTER_FILENAME);

    if (!fNewPolicy) {
         ceError = GPACheckDirectoryExists( szDestFolderPath,
                                          &bDirExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!bDirExists) {
            fNewPolicy = TRUE;
            GPA_LOG_ALWAYS( "Former Automount policy has been mysteriously removed from system, will attempt to recover: Domain Name [%s] Dest Path [%s]",
                            pszDomainName,
                            szDestFolderPath);
        } else {

            ceError = AddPolicyFileToHierarchy( szNewFilePath, 
                                                pszPolicyIdentifier);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = RemovePolicyDirFromRemoveList(szDestFolderPath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    if (fNewPolicy) {
        // Look in new policy file location
        ceError = GPOInitLwiData( NULL,
                                  MACHINE_GROUP_POLICY,
                                  (PGPOLWIDATA*)&pLwidata,
                                  pGPO->pszgPCFileSysPath,
                                  NULL,
                                  LWIAUTOMOUNT_CLIENT_GUID );
        if ( ceError ) {
            // Look in old policy file location
            ceError = GPOInitLwiData( NULL,
                                      MACHINE_GROUP_POLICY,
                                      (PGPOLWIDATA*)&pLwidata,
                                      pGPO->pszgPCFileSysPath,
                                      NULL,
                                      NULL );
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = GPOGetGPItem( MACHINE_GROUP_POLICY,
                                pLwidata,
                                LWIAUTOMOUNT_ITEM_GUID,
                                &pGPItem);
        if ( !CENTERROR_IS_OK(ceError) &&
             CENTERROR_EQUAL(ceError, CENTERROR_GP_XML_GPITEM_NOT_FOUND)) {
            ceError = CENTERROR_SUCCESS;
            goto done;
        }
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPACheckDirectoryExists( szDestFolderPath, 
                                          &bFolderExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFolderExists) {
            ceError = GPARemoveDirectory(szDestFolderPath);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = LwCreateDirectory( szDestFolderPath,
                                     S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
        BAIL_ON_CENTERIS_ERROR(ceError);
    
        bRemoveFolder = TRUE;

        ceError = GPOLwioCopyFileMultiple( NULL,
                                          pszDomainName,
                                          pszDC,
                                          szSourceFolderPath,
                                          szDestFolderPath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = FixFilePermissions( szDestFolderPath, 
                                      pGPItem);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPACheckFileExists( szNewFilePath, 
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            ceError = LwChangeOwnerAndPermissions( szNewFilePath, 
                                                   0, 
                                                   0, 
                                                   S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = PreProcessAutoMasterFile( szDestFolderPath, 
                                                GP_AM_MASTER_FILENAME);
            BAIL_ON_CENTERIS_ERROR(ceError);

            /* Check whether original automount file is present or not */
            ceError = GPACheckFileExists( AM_MASTER_FILEPATH, 
                                         &bFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (bFileExists) {

                bFileExists = FALSE;
                /* backup the system file for the first time */
                  sprintf( szBuf, 
                          "%s%s.lwidentity.orig", 
                          CENTERIS_GP_BACKUP_DIRECTORY, 
                          AM_MASTER_FILENAME);

                ceError = GPACheckFileExists( szBuf, 
                                             &bFileExists);
                BAIL_ON_CENTERIS_ERROR(ceError);

                if (!bFileExists) {
                    /* Back up the original so that we are able to revert to it. */
                    ceError = GPACheckFileExists( AM_MASTER_FILEPATH, 
                                                  &bFileExists);
                    BAIL_ON_CENTERIS_ERROR(ceError);
    
                    if (bFileExists) { 

                        GPA_LOG_VERBOSE("Backing up original automount system file.");
    
                        ceError = GPACopyFileWithOriginalPerms( AM_MASTER_FILEPATH,
                                                               szBuf); 
                        BAIL_ON_CENTERIS_ERROR(ceError);

                        ceError = LwChangePermissions( szBuf,
                                                       S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                        BAIL_ON_CENTERIS_ERROR(ceError);
                    }
                }
            }
            ceError = GPACopyFileWithOriginalPerms( szNewFilePath, 
                                                   AM_MASTER_FILEPATH);
            BAIL_ON_CENTERIS_ERROR(ceError);

            GPA_LOG_VERBOSE( "Received Automount policy: Domain Name [%s] Source Path [%s] Dest Path [%s]", 
                             pszDomainName, 
                             szNewFilePath, 
                             AM_MASTER_FILEPATH);

            sprintf( szNewFilePath, 
                     "%s", 
                     szDestFolderPath);

            ceError = AddPolicyFileToHierarchy( szNewFilePath, 
                                                pszPolicyIdentifier);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = RemovePolicyDirFromRemoveList(szDestFolderPath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            } else {
                GPA_LOG_ALWAYS( "Automount policy not found for: Domain Name [%s] Source Path [%s] ", 
                                pszDomainName, 
                                szNewFilePath);
                GPA_LOG_ALWAYS("Treating this as a disabled or stale policy.");
            }

            bRemoveFolder = FALSE;

    }//fNewpolicy

done:
error:

    if ( pGPItem ) {
        GPODestroyGPItem( pGPItem, 
                          FALSE );
    }

    if ( pLwidata ) {
        GPODestroyLwiData(pLwidata);
    }

    if (pszDomainName) {
        LwFreeString(pszDomainName);
    }

    if (pszSourcePath) {
        LwFreeString(pszSourcePath);
    }

    if (pszPolicyIdentifier) {
        LwFreeString(pszPolicyIdentifier);
    }

    if (pszDC) {
        LwFreeString(pszDC);
    }

    if (bRemoveFolder)
      GPARemoveDirectory(szDestFolderPath);

    return(ceError);

}

static
CENTERROR
FindActiveGPAutomountDir(
    PSTR* ppszDirPaths,
    DWORD dwNPaths,
    PSTR* ppszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bLinkExists = FALSE;
    PSTR pszPath = NULL;
    DWORD iPath = 0;

    ceError = GPACheckLinkExists( GP_AM_REDIRECT_LINK, 
                                 &bLinkExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bLinkExists) {
        /* Find out where this link is pointing to */
        ceError = GPAGetSymLinkTarget( GP_AM_REDIRECT_LINK, 
                                      &pszPath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        for (iPath = 0; iPath < dwNPaths; iPath++) {
            if (!strcmp(ppszDirPaths[iPath], pszPath)) {
                *ppszPath = pszPath;
                pszPath = NULL;
                break;
            }
        }
    }

error:

    if (pszPath)
        LwFreeString(pszPath);

   return ceError;

}

static
CENTERROR
FindAndAddPolicyDirToRemoveList(
    PGROUP_POLICY_OBJECT pGPOList,
    PSTR pszDirPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szDestFolderPath[256];
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    BOOLEAN bExists = FALSE;

    while( pGPOList ) {

        ceError =  GPACrackFileSysPath( pGPOList->pszgPCFileSysPath,
                                        &pszDomainName,
                                        &pszSourcePath,
                                        &pszPolicyIdentifier);
        BAIL_ON_CENTERIS_ERROR(ceError);

        sprintf( szDestFolderPath,
                 "%s%s_likewise_automount",
                 CENTERIS_GP_DIRECTORY,
                 pszPolicyIdentifier);
        
        if (!strcmp( pszDirPath, szDestFolderPath))
        {
            bExists = TRUE;
            break;
        }

        pGPOList = pGPOList->pNext;
    }

    if( bExists )
    {
        GPA_LOG_INFO( "Calling AddPolicyDirToRemoveList ");
        ceError = AddPolicyDirToRemoveList(pszDirPath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        GPA_LOG_INFO( "Removing dirs that are not active %s",
                         pszDirPath );

        ceError = GPARemoveDirectory(pszDirPath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    return ceError;
}

static
CENTERROR
GetCurrentListOfMountPolicies(
    PGROUP_POLICY_OBJECT pGPOList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD   dwNPaths = 0;
    PSTR* ppszDirPaths = NULL;
    DWORD   iPath = 0;
    PSTR pszActiveDirPath = NULL;

    ceError = GPAGetMatchingDirPathsInFolder( CACHEDIR,
                                             "^.*_likewise_automount$",
                                             &ppszDirPaths,
                                             &dwNPaths);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = FindActiveGPAutomountDir( ppszDirPaths, 
                                        dwNPaths, 
                                        &pszActiveDirPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (iPath = 0; iPath < dwNPaths; iPath++) {
        /* Remove the paths that are not being used */
        if (IsNullOrEmptyString(pszActiveDirPath) || 
            strcmp( ppszDirPaths[iPath], 
                    pszActiveDirPath)) {
            // If the policy is there in the modified list, dont delete directory
            // we are any how going to create the directory
            ceError = FindAndAddPolicyDirToRemoveList(pGPOList, ppszDirPaths[iPath]);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else{
            //Active directories. 
            //Store them in remove list
            GPA_LOG_INFO( "Calling AddPolicyDirToRemoveList ");
            ceError = AddPolicyDirToRemoveList(ppszDirPaths[iPath]);
            BAIL_ON_CENTERIS_ERROR(ceError);
            
        }
    }

error:
    LW_SAFE_FREE_STRING(pszActiveDirPath);
    return ceError;
}

static
CENTERROR
ProcessMountPolicyFiles()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PMOUNTPOLICYLIST pPolicyList = g_MountPolicyHierarchyList;
    CHAR szBuf[PATH_MAX+1];
    PSTR pszLinkPath = NULL;
    BOOLEAN bRemoveLinkPath = FALSE;
    BOOLEAN bLinkExists = FALSE;
    BOOLEAN bFileExists = FALSE;
  

    while (pPolicyList && pPolicyList->pNext) {    
        // Walk to last folder in our list
        pPolicyList = pPolicyList->pNext;
    }

    ceError = GPACheckLinkExists( GP_AM_REDIRECT_LINK, 
                                 &bLinkExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf( szBuf,
             "%s%s.lwidentity.orig", 
             CENTERIS_GP_BACKUP_DIRECTORY, 
             AM_MASTER_FILENAME);
    
    ceError = GPACheckFileExists( szBuf, 
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* If no pPolicyList item, then we should restore system to original setup. */
    if (!pPolicyList) {

        if (bFileExists) {      
            GPA_LOG_VERBOSE("Reverting auto mount master file to original system file");
      
            ceError = LwRemoveFile(AM_MASTER_FILEPATH);
            BAIL_ON_CENTERIS_ERROR(ceError);
      
            ceError = GPAMoveFileAcrossDevices( szBuf, 
                                               AM_MASTER_FILEPATH);
            BAIL_ON_CENTERIS_ERROR(ceError);

            bRemoveLinkPath = TRUE;
      
            ceError = RestartAutomounter();
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

    } else {

        BOOLEAN bCreateSymlink = TRUE;

        if (!bFileExists) {    
            /* Back up the original so that we are able to revert to it. */
            ceError = GPACheckFileExists( AM_MASTER_FILEPATH, 
                                         &bFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);
    
            if (bFileExists) {           
                GPA_LOG_VERBOSE("Backing up original automount system file.");

                ceError = GPACopyFileWithOriginalPerms( AM_MASTER_FILEPATH, 
                                                       szBuf);
                BAIL_ON_CENTERIS_ERROR(ceError);

            }
        }
      
        /* Now move a copy of the new automount file into it's proper place. */
        GPA_LOG_VERBOSE("Applying group policy version of autoMount to system file.");

        sprintf( szBuf, 
                 "%s%s_likewise_automount",
                 CENTERIS_GP_DIRECTORY,
                 pPolicyList->pszPolicyIdentifier);
      
        ceError = PreProcessAutoMasterFile( szBuf, 
                                            GP_AM_MASTER_FILENAME);
        BAIL_ON_CENTERIS_ERROR(ceError);
      
        strcat( szBuf, 
                "/");
        strcat( szBuf, 
                GP_AM_MASTER_FILENAME);
      
        GPA_LOG_VERBOSE( "Source Path [%s] Dest Path [%s]",
                         szBuf,
                         AM_MASTER_FILEPATH);

        ceError = GPACopyFileWithOriginalPerms( szBuf, 
                                               AM_MASTER_FILEPATH);
        BAIL_ON_CENTERIS_ERROR(ceError);

        sprintf( szBuf, 
                 "%s%s_likewise_automount",
                 CENTERIS_GP_DIRECTORY,
                 pPolicyList->pszPolicyIdentifier);

        if (bLinkExists) {
            /* Find out where this link is pointing to */
            ceError = GPAGetSymLinkTarget( GP_AM_REDIRECT_LINK, 
                                          &pszLinkPath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if ( !IsNullOrEmptyString(pszLinkPath) && 
                 strcmp(pszLinkPath, szBuf)) {
                ceError = LwRemoveFile(GP_AM_REDIRECT_LINK);
                BAIL_ON_CENTERIS_ERROR(ceError);
            } else {
                bCreateSymlink = FALSE;
            }
        }

        if (bCreateSymlink) {
            GPA_LOG_VERBOSE( "Creating automount link path at [%s][target:%s]", 
                             GP_AM_REDIRECT_LINK, 
                             szBuf);

            ceError = GPACreateSymLink( szBuf, 
                                       GP_AM_REDIRECT_LINK);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = RestartAutomounter();
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (bRemoveLinkPath && !IsNullOrEmptyString(pszLinkPath)) {
        GPARemoveDirectory(pszLinkPath);
        /* We don't need the symlink as well */
        if (bLinkExists) {
            ceError = LwRemoveFile(GP_AM_REDIRECT_LINK);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    LW_SAFE_FREE_STRING(pszLinkPath);

    return ceError;
}

static
BOOLEAN
IsAutofsSupported()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    ceError = GPACheckFileExists(AM_MASTER_FILEPATH, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists == FALSE) {
        GPA_LOG_VERBOSE("Autofs is not installed or not supported for this system. Hence skipping Auto mount group policy for this system...");
        GPA_LOG_VERBOSE("Automount group policy is not applicable for this system ");
        ceError = CENTERROR_INVALID_OPERATION;
        goto error;
    }

error:
    return ceError == CENTERROR_SUCCESS;
}

CENTERROR
ProcessAutomountGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bSomeGPModified = FALSE;
  
    GPA_LOG_FUNCTION_ENTER();

    //Check if autofs is supported
    if(!IsAutofsSupported()) {
        goto error;
    }

    ResetPolicyFileHierarchy();
    ResetPolicyRemoveList();

    ceError = GetCurrentListOfMountPolicies(pGPOModifiedList);    
    BAIL_ON_CENTERIS_ERROR(ceError);
  
    /*
     * Process old mount policies to remove
     */
    while ( pGPODeletedList ) {    
        ceError =  RemoveMountPolicy( pGPODeletedList->pszgPCFileSysPath,
                                      pGPODeletedList->pszDisplayName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    
        pGPODeletedList = pGPODeletedList->pNext;
    }

    bSomeGPModified = ( pGPODeletedList != NULL );

    /*
     * Process new mount policies to add
     */
    while ( pGPOModifiedList ) {
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
            GPA_LOG_VERBOSE( "GPO(%s) disabled by platform targeting",
                             pGPOModifiedList->pszDisplayName);
            ceError = RemoveMountPolicy( pGPOModifiedList->pszgPCFileSysPath,
                                         pGPOModifiedList->pszDisplayName );
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE( "GPO(%s) is disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);
      
            ceError =  RemoveMountPolicy( pGPOModifiedList->pszgPCFileSysPath,
                                          pGPOModifiedList->pszDisplayName);
            BAIL_ON_CENTERIS_ERROR(ceError);
            //This flag is set to revert the original file
            bSomeGPModified = TRUE;

        } else {

            ceError =  AddMountPolicy(pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    
        pGPOModifiedList = pGPOModifiedList->pNext;
    }
  
    ceError = ProcessMountPolicyFiles();
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Now remove any stale Automount type policy files left around */
    ceError = ProcessPolicyRemoveList();
    BAIL_ON_CENTERIS_ERROR(ceError);
  
error:
  
    GPA_LOG_FUNCTION_LEAVE(ceError);
  
    return(ceError);

}

CENTERROR
ResetAutomountGroupPolicy(
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* There is nothing to reset for this policy */

    return ceError;
}

