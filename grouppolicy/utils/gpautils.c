
#include "includes.h"

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

BOOLEAN
GPAStrEndsWith(
    PCSTR str,
    PCSTR suffix
    )
{
    size_t strLen, suffixLen;
    if(suffix == NULL)
        return TRUE;
    if(str == NULL)
        return FALSE;

    strLen = strlen(str);
    suffixLen = strlen(suffix);
    if(suffixLen > strLen)
        return FALSE;

    return strcmp(str + strLen - suffixLen, suffix) == 0;
}

BOOLEAN
GPAStrStartsWith(
    PCSTR str,
    PCSTR prefix
    )
{
    if(prefix == NULL)
        return TRUE;
    if(str == NULL)
        return FALSE;

    return strncmp(str, prefix, strlen(prefix)) == 0;
}


CENTERROR
GPACheckLinkExists(
    PCSTR pszPath,
    PBOOLEAN pbLinkExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1) {
        if (lstat(pszPath, &statbuf) < 0) {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == ENOENT || errno == ENOTDIR)
            {
                *pbLinkExists = FALSE;
                break;
            }
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_GPA_ERROR(ceError);
        } else {
            *pbLinkExists = (((statbuf.st_mode & S_IFMT) == S_IFLNK) ? TRUE : FALSE);
            break;
        }
    }

error:

    return ceError;
}

CENTERROR
GPACheckFileOrLinkExists(
    PCSTR pszPath,
    PBOOLEAN pbExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    ceError = GPACheckFileExists(pszPath, pbExists);
    BAIL_ON_GPA_ERROR(ceError);
    if(*pbExists == TRUE)
        goto error;

    ceError = GPACheckLinkExists(pszPath, pbExists);
    BAIL_ON_GPA_ERROR(ceError);

error:
    return ceError;
}

static
CENTERROR
GPAFindInPath(
    PCSTR rootPrefix,
    PCSTR filename,
    PCSTR searchPath,
    PSTR* foundPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    //Copy the search path so that strtok can be run on it
    PSTR mySearchPath = NULL;
    PSTR strtokSavePtr = NULL;
    PSTR currentDir = NULL;
    PSTR testPath = NULL;
    BOOLEAN exists = FALSE;

    if(foundPath != NULL)
        *foundPath = NULL;

    if(rootPrefix == NULL)
        rootPrefix = "";

    GCE(ceError = LwAllocateString(searchPath, &mySearchPath));

    currentDir = strtok_r(mySearchPath, ":", &strtokSavePtr);
    while(TRUE)
    {
        LW_SAFE_FREE_STRING(testPath);
        GCE(ceError = LwAllocateStringPrintf(&testPath, "%s%s/%s", rootPrefix, currentDir, filename));
        GCE(ceError = GPACheckFileOrLinkExists(testPath, &exists));
        if(exists)
        {
            if(foundPath != NULL)
            {

                GCE(ceError = LwAllocateStringPrintf(foundPath, "%s/%s", currentDir, filename));
            }
            break;
        }
        currentDir = strtok_r(NULL, ":", &strtokSavePtr);
        if(currentDir == NULL)
        {
            GCE(ceError = CENTERROR_FILE_NOT_FOUND);
        }
    }

cleanup:
    LW_SAFE_FREE_STRING(mySearchPath);
    LW_SAFE_FREE_STRING(testPath);
    return ceError;
}

CENTERROR
GPAFindFileInPath(
    PCSTR filename,
    PCSTR searchPath,
    PSTR* foundPath
    )
{
    return GPAFindInPath(NULL, filename, searchPath, foundPath);
}

CENTERROR
GPAGetSymLinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   )
{
   CENTERROR ceError = CENTERROR_SUCCESS;
   CHAR szBuf[PATH_MAX+1];

   memset(szBuf, 0, PATH_MAX);

   while (1) {

      if (readlink(pszPath, szBuf, PATH_MAX) < 0) {
         if (errno == EINTR)
            continue;

         ceError = LwMapErrnoToLwError(errno);
         BAIL_ON_GPA_ERROR(ceError);
      }

      break;
   }

   ceError = LwAllocateString(szBuf, ppszTargetPath);
   BAIL_ON_GPA_ERROR(ceError);

  error:

   return ceError;
}

CENTERROR
GPACreateSymLink(
    PCSTR pszOldPath,
    PCSTR pszNewPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (IsNullOrEmptyString(pszOldPath) ||
        IsNullOrEmptyString(pszNewPath)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_GPA_ERROR(ceError);
    }

    if (symlink(pszOldPath, pszNewPath) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

error:

    return ceError;
}

void
GPAFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    )
{
    DWORD i;

    if ( ppStringArray ) {
        for(i = 0; i < dwCount; i++)
        {
            if (ppStringArray[i]) {
                LwFreeString(ppStringArray[i]);
            }
        }

        LwFreeMemory(ppStringArray);
    }

    return;
}

static
CENTERROR
GPAGetMatchingDirEntryPathsInFolder(PCSTR pszDirPath,
                                   PCSTR pszFileNameRegExp,
                                   PSTR** pppszHostFilePaths,
                                   PDWORD pdwNPaths,
                                   DWORD dirEntryType)
{
    typedef struct __PATHNODE
    {
        PSTR pszPath;
        struct __PATHNODE *pNext;
    } PATHNODE, *PPATHNODE;

    CENTERROR ceError = CENTERROR_SUCCESS;
    DIR* pDir = NULL;
    struct dirent*  pDirEntry = NULL;
    regex_t rx;
    BOOLEAN rxAllocated = FALSE;
    regmatch_t* pResult = NULL;
    size_t nMatch = 1;
    DWORD  dwNPaths = 0;
    DWORD  iPath = 0;
    PSTR*  ppszHostFilePaths = NULL;
    CHAR   szBuf[PATH_MAX+1];
    struct stat statbuf;
    PPATHNODE pPathList = NULL;
    PPATHNODE pPathNode = NULL;
    BOOLEAN isDirPathDir;

    *pppszHostFilePaths = NULL;
    *pdwNPaths = 0;

    ceError = GPACheckDirectoryExists(pszDirPath, &isDirPathDir);
    BAIL_ON_GPA_ERROR(ceError);
    
    if(!isDirPathDir) {
        ceError = CENTERROR_INVALID_DIRECTORY;
        BAIL_ON_GPA_ERROR(ceError);
    }

    if (regcomp(&rx, pszFileNameRegExp, REG_EXTENDED) != 0) {
        ceError = CENTERROR_REGEX_COMPILE_FAILED;
        BAIL_ON_GPA_ERROR(ceError);
    }
    rxAllocated = TRUE;

    ceError = LwAllocateMemory(sizeof(regmatch_t), (PVOID*)&pResult);
    BAIL_ON_GPA_ERROR(ceError);

    pDir = opendir(pszDirPath);
    if (pDir == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        int copied = snprintf(szBuf, sizeof(szBuf), "%s/%s", pszDirPath, pDirEntry->d_name);
        if (copied >= sizeof(szBuf))
        {
            //Skip pathnames that are too long
            continue;
        }
        memset(&statbuf, 0, sizeof(struct stat));
        if (stat(szBuf, &statbuf) < 0) {
            if(errno == ENOENT)
            {
                //This occurs when there is a symbolic link pointing to a
                //location that doesn't exist, because stat looks at the final
                //file, not the link. Since this file doesn't exist anyway,
                //just skip it.
                continue;
            }
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_GPA_ERROR(ceError);
        }
        /*
         * For now, we are searching only for regular files
         * This may be enhanced in the future to support additional
         * file system entry types
         */
        if (((statbuf.st_mode & S_IFMT) == dirEntryType) &&
            (regexec(&rx, pDirEntry->d_name, nMatch, pResult, 0) == 0)) {
            dwNPaths++;

            ceError = LwAllocateMemory(sizeof(PATHNODE), (PVOID*)&pPathNode);
            BAIL_ON_GPA_ERROR(ceError);

            ceError = LwAllocateString(szBuf, &pPathNode->pszPath);
            BAIL_ON_GPA_ERROR(ceError);

            pPathNode->pNext = pPathList;
            pPathList = pPathNode;
            pPathNode = NULL;
        }
    }

    if (pPathList) {
        ceError = LwAllocateMemory(sizeof(PSTR)*dwNPaths,
                                   (PVOID*)&ppszHostFilePaths);
        BAIL_ON_GPA_ERROR(ceError);
        /*
         *  The linked list is in reverse.
         *  Assign values in reverse to get it right
         */
        iPath = dwNPaths-1;
        pPathNode = pPathList;
        while (pPathNode) {
            *(ppszHostFilePaths+iPath) = pPathNode->pszPath;
            pPathNode->pszPath = NULL;
            pPathNode = pPathNode->pNext;
            iPath--;
        }
    }

    *pppszHostFilePaths = ppszHostFilePaths;
    ppszHostFilePaths = NULL;

    *pdwNPaths = dwNPaths;

error:

    if (ppszHostFilePaths) {
        GPAFreeStringArray(ppszHostFilePaths, dwNPaths);
    }

    if (pPathNode) {
        pPathNode->pNext = pPathList;
        pPathList = pPathNode;
    }

    while(pPathList) {
        pPathNode = pPathList;
        pPathList = pPathList->pNext;
        if (pPathNode->pszPath)
            LwFreeString(pPathNode->pszPath);
        LwFreeMemory(pPathNode);
    }

    if(rxAllocated)
        regfree(&rx);

    if (pResult) {
        LwFreeMemory(pResult);
    }

    if (pDir)
        closedir(pDir);

    return ceError;
}

CENTERROR
GPAGetMatchingFilePathsInFolder(PCSTR pszDirPath,
                               PCSTR pszFileNameRegExp,
                               PSTR** pppszHostFilePaths,
                               PDWORD pdwNPaths)
{
    return GPAGetMatchingDirEntryPathsInFolder(pszDirPath,
                                              pszFileNameRegExp,
                                              pppszHostFilePaths,
                                              pdwNPaths,
                                              S_IFREG);
}

CENTERROR
GPAGetMatchingDirPathsInFolder(PCSTR pszDirPath,
                              PCSTR pszDirNameRegExp,
                              PSTR** pppszHostDirPaths,
                              PDWORD pdwNPaths
                              )
{
    return GPAGetMatchingDirEntryPathsInFolder(pszDirPath,
                                              pszDirNameRegExp,
                                              pppszHostDirPaths,
                                              pdwNPaths,
                                              S_IFDIR);
}

CENTERROR
GPAGetFileTimeStamps(
    PCSTR pszFilePath,
    time_t *patime,   /* time of last access */
    time_t *pmtime,   /* time of last modification */
    time_t *pctime )  /* time of last status change */
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszFilePath, &statbuf) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    if ( patime ) {
        *patime = statbuf.st_atime;
    }

    if ( pmtime ) {
        *pmtime = statbuf.st_mtime;
    }

    if ( pctime ) {
        *pctime = statbuf.st_ctime;
    }
error:

    return ceError;
}

/*
// Do not use this function on very large files since it reads the
// entire file into memory.
*/
CENTERROR
GPAReadFile(
    PCSTR pszFilePath,
    PSTR *ppBuffer,
    PLONG pSize
    )
{
    CENTERROR ceError = 0;
    struct stat statbuf;
    FILE *fp = NULL;

    *ppBuffer = NULL;
    if(pSize != NULL)
        *pSize = 0;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszFilePath, &statbuf) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    if (statbuf.st_size > 0) {
        /* allocate one additional byte of memory and set it to NULL to
           allow for functions like strtok() to work properly */
        ceError = LwAllocateMemory(statbuf.st_size + 1, (VOID*)ppBuffer);
        BAIL_ON_GPA_ERROR(ceError);

        fp = fopen( pszFilePath, "r" );

        if ( fp == NULL ){
            ceError = CENTERROR_GP_FILE_OPEN_FAILED;
            BAIL_ON_GPA_ERROR(ceError);
        }

        if ( fread( *ppBuffer, statbuf.st_size, 1, fp ) != 1 ) {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_GPA_ERROR(ceError);
        }

        fclose(fp);
        fp = NULL;

        if(pSize != NULL)
            *pSize = statbuf.st_size;
    }

    return ceError;

error:
    if (*ppBuffer) {
           LwFreeMemory( *ppBuffer );
           *ppBuffer = NULL;
    }

    if (fp) {
        fclose( fp );
    }

    return ceError;
}

CENTERROR
GPACheckFileHoldsPattern(
    PCSTR pszFilePath,
    PCSTR pszPattern,
    PBOOLEAN pbPatternExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE* fp = NULL;
    CHAR szBuf[1024+1];
    regex_t rx;
    regmatch_t* pResult = NULL;
    size_t nMatch = 1;
    BOOLEAN bPatternExists = FALSE;

    memset(&rx, 0, sizeof(regex_t));

    if (regcomp(&rx, pszPattern, REG_EXTENDED) != 0) {
        ceError = CENTERROR_REGEX_COMPILE_FAILED;
        BAIL_ON_GPA_ERROR(ceError);
    }

    ceError = LwAllocateMemory(sizeof(regmatch_t), (PVOID*)&pResult);
    BAIL_ON_GPA_ERROR(ceError);

    fp = fopen(pszFilePath, "r");
    if (fp == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    while (!feof(fp)) {
        if (fgets(szBuf, 1024, fp)) {
            if (regexec(&rx, szBuf, nMatch, pResult, 0) == 0) {
                bPatternExists = TRUE;
                break;
            }
        } else if (!feof(fp)) {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_GPA_ERROR(ceError);
        }
    }

    *pbPatternExists = bPatternExists;

error:

    regfree(&rx);

    if (pResult)
        LwFreeMemory(pResult);

    if (fp != NULL)
        fclose(fp);

    return ceError;
}

CENTERROR
GPAFileStreamWrite(
    FILE* handle,
    PCSTR data,
    unsigned int size
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    unsigned int written = 0;

    while (written < size)
    {
        int amount = fwrite(data+written, 1, size-written, handle);
        if (amount < 0)
        {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_GPA_ERROR(ceError);
        }
        written += amount;
    }
error:

    return ceError;
}

CENTERROR
GPAGetUserUID(
    PCSTR pszUID,
    uid_t* pUID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PVOID pUserInfo = NULL;

    if(strcmp(pszUID,"0"))
    {
        ceError = LsaOpenServer(&hLsaConnection);
        BAIL_ON_GPA_ERROR(ceError);

        ceError = LsaFindUserByName(
                     hLsaConnection,
                     pszUID,
                     0,
                     &pUserInfo);
        BAIL_ON_GPA_ERROR(ceError);

        *pUID = ((PLSA_USER_INFO_0)pUserInfo)->uid; 
    }

cleanup:
    if(pUserInfo)
    {
        LsaFreeUserInfo(0,pUserInfo);
    }
    if(hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }
    return ceError;
error:
    goto cleanup;
}

CENTERROR
GPAGetUserGID(
    PCSTR pszGID,
    gid_t* pGID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PVOID pGroupInfo = NULL;

    if(strcmp(pszGID,"0"))
    {
        ceError = LsaOpenServer(&hLsaConnection);
        BAIL_ON_GPA_ERROR(ceError);

        ceError = LsaFindGroupByName(
                     hLsaConnection,
                     pszGID,
                     0,
                     0,
                     &pGroupInfo);
        BAIL_ON_GPA_ERROR(ceError);

        *pGID = ((PLSA_GROUP_INFO_0)pGroupInfo)->gid; 
    }

cleanup:
    if(pGroupInfo)
    {
        LsaFreeGroupInfo(0,pGroupInfo);
    }
    if(hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }
    return ceError;
error:
    goto cleanup;
}


CENTERROR
GPASetCloseOnExec(
    int fd
    )
{
    long flags = fcntl(fd, F_GETFD);
    if(flags < 0)
        return LwMapErrnoToLwError(errno);

    flags |= FD_CLOEXEC;
    if(fcntl(fd, F_SETFD, flags) < 0)
        return LwMapErrnoToLwError(errno);

    return CENTERROR_SUCCESS;
}

CENTERROR
GPARemoveFiles(
    PSTR pszPath,
    BOOLEAN fDirectory,
    BOOLEAN fRecursive
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR  szCommand[2 * PATH_MAX + 1];

    sprintf(szCommand, "/bin/rm -f %s %s %s",
            fDirectory ? "-d" : "",
            fRecursive ? "-r" : "",
            pszPath);

    if (system(szCommand) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return(ceError);
}

CENTERROR
GPACheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1) {
        if (stat(pszPath, &statbuf) < 0) {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == ENOENT || errno == ENOTDIR)
            {
                *pbFileExists = FALSE;
                break;
            }
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            *pbFileExists = (((statbuf.st_mode & S_IFMT) == S_IFREG) ? TRUE : FALSE);
            break;
        }
    }

error:

    return ceError;
}

CENTERROR
GPACopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCSTR pszTmpSuffix = ".tmp_likewise";
    PSTR pszTmpPath = NULL;
    BOOLEAN bRemoveFile = FALSE;
    CHAR szBuf[1024+1];
    int  iFd = -1;
    int  oFd = -1;
    DWORD dwBytesRead = 0;

    if (IsNullOrEmptyString(pszSrcPath) ||
        IsNullOrEmptyString(pszDstPath)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwAllocateMemory(strlen(pszDstPath)+strlen(pszTmpSuffix)+2,
                               (PVOID*)&pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    strcpy(pszTmpPath, pszDstPath);
    strcat(pszTmpPath, pszTmpSuffix);

    if ((iFd = open(pszSrcPath, O_RDONLY, S_IRUSR)) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if ((oFd = open(pszTmpPath, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR)) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    bRemoveFile = TRUE;

    while (1) {

        if ((dwBytesRead = read(iFd, szBuf, 1024)) < 0) {

            if (errno == EINTR)
                continue;

            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if (dwBytesRead == 0)
            break;

        if (write(oFd, szBuf, dwBytesRead) != dwBytesRead) {

            if (errno == EINTR)
                continue;

            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

    }

    close(iFd); iFd = -1;
    close(oFd); oFd = -1;

    ceError = LwMoveFile(pszTmpPath, pszDstPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = FALSE;

    ceError = LwChangePermissions(pszDstPath, dwPerms);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if (iFd >= 0)
        close(iFd);

    if (oFd >= 0)
        close(oFd);

    if (bRemoveFile) {
        LwRemoveFile(pszTmpPath);
    }

    if (pszTmpPath)
        LwFreeString(pszTmpPath);

    return ceError;
}

CENTERROR
GPAGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszSrcPath, &statbuf) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *uid = statbuf.st_uid;
    *gid = statbuf.st_gid;
    *mode = statbuf.st_mode;

error:

    return ceError;
}

CENTERROR
GPACopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    uid_t uid;
    gid_t gid;
    mode_t mode;

    ceError = LwGetOwnerAndPermissions(pszSrcPath, &uid, &gid, &mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACopyFileWithPerms(pszSrcPath, pszDstPath, mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwChangeOwnerAndPermissions(pszDstPath, uid, gid, mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

CENTERROR
GPAFilePrintf(
    FILE* handle,
    PCSTR format,
    ...
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    va_list args;
    int count;

    va_start(args, format);
    count = vfprintf(handle, format, args);
    va_end(args);

    if (count < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    return ceError;
}

CENTERROR
GPAGetCurrentDirectoryPath(
    PSTR* ppszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    PSTR pszPath = NULL;

    if (getcwd(szBuf, PATH_MAX) == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwAllocateString(szBuf, &pszPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszPath = pszPath;

    return ceError;

error:

    if (pszPath) {
        LwFreeString(pszPath);
    }

    return ceError;
}

CENTERROR
GPARemoveDirectory(
    PCSTR pszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[PATH_MAX+1];

    if ((pDir = opendir(pszPath)) == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0) {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
            ceError = GPARemoveDirectory(szBuf);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (rmdir(szBuf) < 0) {
                ceError = LwMapErrnoToLwError(ceError);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

        } else {

            ceError = LwRemoveFile(szBuf);
            BAIL_ON_CENTERIS_ERROR(ceError);

        }
    }
    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        ceError = LwMapErrnoToLwError(ceError);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    pDir = NULL;

    if (rmdir(pszPath) < 0) {
        ceError = LwMapErrnoToLwError(ceError);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (pDir)
        closedir(pDir);

    return ceError;
}


CENTERROR
GPACheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    struct stat statbuf;

    while (1) {

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(pszPath, &statbuf) < 0) {

            if (errno == EINTR) {
                continue;
            }
            else if (errno == ENOENT || errno == ENOTDIR) {
                *pbDirExists = FALSE;
                break;
            }
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);

        }

        /*
           The path exists. Is it a directory?
         */

        *pbDirExists = (((statbuf.st_mode & S_IFMT) == S_IFDIR) ? TRUE : FALSE);
        break;
    }

error:

    return ceError;
}
/*
// On some unix systems, you may not be allowed to
// move files across devices. So, we copy the file to a
// tmp file and then move it to the target location -
// and remove the original file
*/
CENTERROR
GPAMoveFileAcrossDevices(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szTmpPath[PATH_MAX+1] = "";
    BOOLEAN bRemoveFile = FALSE;
    uid_t uid;
    gid_t gid;
    mode_t mode;

    sprintf(szTmpPath, "%s_lwi_.tmp", pszDstPath);

    ceError = LwGetOwnerAndPermissions(pszSrcPath, &uid, &gid, &mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACopyFileWithPerms(pszSrcPath, szTmpPath, mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = TRUE;

    ceError = LwMoveFile(szTmpPath, pszDstPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = FALSE;

    ceError = LwRemoveFile(pszSrcPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwChangeOwnerAndPermissions(pszDstPath, uid, gid, mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if (bRemoveFile && !IsNullOrEmptyString(szTmpPath))
       LwRemoveFile(szTmpPath);

    return ceError;
}

CENTERROR
GPAOpenFile(
    PCSTR path,
    PCSTR mode,
    FILE** handle)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    *handle = fopen(path, mode);

    if (!*handle)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
error:

    return ceError;
}

CENTERROR
GPACloseFile(
    FILE* handle
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (fclose(handle))
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;
}


CENTERROR
GPAGetOwnerUID(
    PCSTR pszFilePath,
    uid_t* pUid
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszFilePath, &statbuf) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *pUid = statbuf.st_uid;
    
error:

    return ceError;
}

CENTERROR
GPACheckSockExists(
    PCSTR pszPath,
    PBOOLEAN pbSockExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1) {
        if (stat(pszPath, &statbuf) < 0) {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == ENOENT || errno == ENOTDIR)
            {
                *pbSockExists = FALSE;
                break;
            }
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            *pbSockExists = (((statbuf.st_mode & S_IFMT) == S_IFSOCK) ? TRUE : FALSE);
            break;
        }
    }

error:

    return ceError;
}

CENTERROR
SearchIfDirExists(
    DIR* pDir,
    PCSTR pszMatchString,
    PBOOLEAN pbDirExists
    )
{
    CENTERROR  ceError = CENTERROR_SUCCESS;
    struct dirent* pDirEntry = NULL;

    *pbDirExists = FALSE;

    while( (pDirEntry = readdir(pDir)) != NULL)
    {
        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        //search for mount path if present
        if(!strcmp(pszMatchString,pDirEntry->d_name))
        {
            *pbDirExists = TRUE;
            break;
        }
    }

    return ceError;
}
    
CENTERROR
GPACheckIfMountDirExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    )
{
    CENTERROR  ceError = CENTERROR_SUCCESS;
    PSTR pszMntPath = NULL;
    PSTR pszToken = NULL;
    PSTR pszLast = NULL;
    CHAR szCatPath[PATH_MAX+1];
    DIR* pDir = NULL;
    BOOLEAN bDirExists = FALSE;

    ceError = LwAllocateString( pszPath, &pszMntPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszToken = strtok_r(pszMntPath,"/",&pszLast);

    memset(szCatPath,0,sizeof(szCatPath));

    strcat (szCatPath,"/");

    while (pszToken)
    {
        if ((pDir = opendir(szCatPath)) == NULL)
        {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = SearchIfDirExists(pDir,pszToken,&bDirExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if(!bDirExists)
        {
            *pbDirExists = FALSE;
            goto error;
        }

        if(closedir(pDir) < 0)
        {
            pDir = NULL;
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        pDir = NULL;

        strcat (szCatPath,pszToken);
        strcat (szCatPath,"/");

        pszToken = strtok_r(NULL,"/",&pszLast);
    }

    *pbDirExists = TRUE;

error:

    LW_SAFE_FREE_STRING(pszMntPath);

    if(pDir)
    {
        closedir(pDir);
        pDir = NULL;
    }

    return ceError;
}

// Remove leading whitespace and newline characters which are added by libxml2 only.
VOID
GPARemoveLeadingWhitespacesOnly(
    PSTR pszString
    )
{
    PSTR pszNew = pszString;
    PSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0' || !isspace((int)*pszString)) {
        return;
    }

    if( *pszTmp == '\n' )
        pszTmp++;

    while (pszTmp != NULL && *pszTmp != '\0' && isspace((int)*pszTmp)) {
        if ( *pszTmp == '\n' )
            break;
        pszTmp++;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        *pszNew++ = *pszTmp++;
    }

    *pszNew = '\0';
}

// Remove trailing whitespace and newline characters which are added by libxml2 only.
VOID
GPARemoveTrailingWhitespacesOnly(
    PSTR pszString
    )
{
    PSTR pszLastNewLine = NULL;
    PSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0') {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        if(*pszTmp == '\n')
            pszLastNewLine = pszTmp;
        pszTmp++;
    }

    if (pszLastNewLine != NULL) {
        if( *(pszLastNewLine-1) != '\n' ){
            *pszLastNewLine = '\n';
            pszLastNewLine++;            
        }
        *pszLastNewLine = '\0';
    }
}
