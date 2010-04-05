/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#include "includes.h"

DWORD
LWRemoveFile(
    PCSTR pszPath
    )
{
    DWORD dwError = 0;

    while (1) {
        if (unlink(pszPath) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        } else {
            break;
        }
    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

/*
// TODO: Check access and removability before actual deletion
*/
DWORD
LWRemoveDirectory(
    PCSTR pszPath
    )
{
    DWORD dwError = 0;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[PATH_MAX+1];

    if ((pDir = opendir(pszPath)) == NULL) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0) {
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
            dwError = LWRemoveDirectory(szBuf);
            BAIL_ON_MAC_ERROR(dwError);

            if (rmdir(szBuf) < 0) {
                dwError = errno;
                BAIL_ON_MAC_ERROR(dwError);
            }

        } else {

            dwError = LWRemoveFile(szBuf);
            BAIL_ON_MAC_ERROR(dwError);

        }
    }
    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    pDir = NULL;

    if (rmdir(pszPath) < 0) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    if (pDir)
        closedir(pDir);

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWCheckLinkExists(
    PCSTR pszPath,
    PBOOLEAN pbLinkExists
    )
{
    DWORD dwError = 0;

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
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        } else {
            *pbLinkExists = (((statbuf.st_mode & S_IFMT) == S_IFLNK) ? TRUE : FALSE);
            break;
        }
    }

cleanup:

    return dwError;
    
error:
    
    *pbLinkExists = FALSE;
    
    goto cleanup;
}

DWORD
LWCheckSockExists(
    PCSTR pszPath,
    PBOOLEAN pbSockExists
    )
{
    DWORD dwError = 0;

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
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        } else {
            *pbSockExists = (((statbuf.st_mode & S_IFMT) == S_IFSOCK) ? TRUE : FALSE);
            break;
        }
    }

cleanup:

    return dwError;
    
error:

    *pbSockExists = FALSE;
    
    goto cleanup;
}

DWORD
LWCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    )
{
    DWORD dwError = 0;

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
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        } else {
            *pbFileExists = (((statbuf.st_mode & S_IFMT) == S_IFREG) ? TRUE : FALSE);
            break;
        }
    }

cleanup:

    return dwError;
    
error:

    *pbFileExists = FALSE;
    
    goto cleanup;
}

DWORD
LWCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    )
{
    DWORD dwError = 0;

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
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);

        }

        /*
           The path exists. Is it a directory?
         */

        *pbDirExists = (((statbuf.st_mode & S_IFMT) == S_IFDIR) ? TRUE : FALSE);
        break;
    }

cleanup:

    return dwError;
    
error:

    *pbDirExists = FALSE;

    goto cleanup;
}

DWORD
LWMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    )
{
    DWORD dwError = 0;

    if (rename(pszSrcPath, pszDstPath) < 0) {
        dwError = errno;
    }

    return dwError;
}

DWORD
LWCopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    )
{
    DWORD dwError = 0;
    PCSTR pszTmpSuffix = ".tmp_likewise";
    PSTR pszTmpPath = NULL;
    BOOLEAN bRemoveFile = FALSE;
    CHAR szBuf[1024+1];
    int  iFd = -1;
    int  oFd = -1;
    int  bytesRead = 0;

    if (IsNullOrEmptyString(pszSrcPath) ||
        IsNullOrEmptyString(pszDstPath)) {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LWAllocateMemory(strlen(pszDstPath)+strlen(pszTmpSuffix)+2,
                               (PVOID*)&pszTmpPath);
    BAIL_ON_MAC_ERROR(dwError);

    strcpy(pszTmpPath, pszDstPath);
    strcat(pszTmpPath, pszTmpSuffix);

    if ((iFd = open(pszSrcPath, O_RDONLY, S_IRUSR)) < 0) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    if ((oFd = open(pszTmpPath, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR)) < 0) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    bRemoveFile = TRUE;

    while (1) {

        if ((bytesRead = read(iFd, szBuf, 1024)) < 0) {

            if (errno == EINTR)
                continue;

            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }

        if (bytesRead == 0)
            break;

        if (write(oFd, szBuf, bytesRead) != bytesRead) {

            if (errno == EINTR)
                continue;

            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }

    }

    close(iFd); iFd = -1;
    close(oFd); oFd = -1;

    dwError = LWMoveFile(pszTmpPath, pszDstPath);
    BAIL_ON_MAC_ERROR(dwError);

    bRemoveFile = FALSE;

    dwError = LWChangePermissions(pszDstPath, dwPerms);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    if (iFd >= 0)
        close(iFd);

    if (oFd >= 0)
        close(oFd);

    if (bRemoveFile) {
        LWRemoveFile(pszTmpPath);
    }

    if (pszTmpPath)
        LWFreeString(pszTmpPath);

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    )
{
    DWORD dwError = 0;
    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszSrcPath, &statbuf) < 0) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    *uid = statbuf.st_uid;
    *gid = statbuf.st_gid;
    *mode = statbuf.st_mode;

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWGetOwnerUID(
    PCSTR pszFilePath,
    uid_t* pUid
    )
{
    DWORD dwError = 0;
    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszFilePath, &statbuf) < 0) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    *pUid = statbuf.st_uid;
    
cleanup:

    return dwError;
    
error:

    *pUid = 0;
    
    goto cleanup;
}

DWORD
LWCheckFileOrLinkExists(
    PCSTR pszPath,
    PBOOLEAN pbExists
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    
    dwError = LWCheckFileExists(pszPath, &bExists);
    BAIL_ON_MAC_ERROR(dwError);
    
    if (!bExists)
    {
        dwError = LWCheckLinkExists(pszPath, &bExists);
        BAIL_ON_MAC_ERROR(dwError);
    }
    
    *pbExists = bExists;

cleanup:

    return dwError;
    
error:

    *pbExists = FALSE;

    goto cleanup;
}

DWORD
LWChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;

    while (1) {
        if (chmod(pszPath, dwFileMode) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        } else {
            break;
        }
    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    )
{
    DWORD dwError = 0;

    while (1) {
        if (chown(pszPath, uid, gid) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        } else {
            break;
        }
    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;

    dwError = LWChangeOwner(pszPath, uid, gid);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWChangePermissions(pszPath, dwFileMode);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWGetCurrentDirectoryPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    CHAR szBuf[PATH_MAX+1];
    PSTR pszPath = NULL;

    if (getcwd(szBuf, PATH_MAX) == NULL) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LWAllocateString(szBuf, &pszPath);
    BAIL_ON_MAC_ERROR(dwError);

    *ppszPath = pszPath;

cleanup:

    return dwError;

error:

    if (pszPath) {
        LWFreeString(pszPath);
    }

    goto cleanup;
}

DWORD
LWChangeDirectory(
    PSTR pszPath
    )
{
    DWORD dwError = 0;
    
    if (IsNullOrEmptyString(pszPath))
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (chdir(pszPath) < 0)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWCreateDirectoryRecursive(
    PSTR  pszCurDirPath,
    PSTR  pszTmpPath,
    PSTR* ppszTmp,
    DWORD dwFileMode,
    DWORD dwWorkingFileMode,
    int   iPart
    )
{
    DWORD dwError = 0;
    PSTR    pszDirPath = NULL;
    BOOLEAN bDirCreated = FALSE;
    BOOLEAN bDirExists = FALSE;
    CHAR    szDelimiters[] = "/";

    PSTR pszToken = strtok_r(
                        (iPart ? NULL : pszTmpPath),
                        szDelimiters,
                        ppszTmp);

    if (pszToken != NULL) {

        dwError = LWAllocateMemory(
                        strlen(pszCurDirPath)+strlen(pszToken)+2,
                        (PVOID*)&pszDirPath);
        BAIL_ON_MAC_ERROR(dwError);

        sprintf(pszDirPath,
                "%s/%s",
                (!strcmp(pszCurDirPath, "/") ? "" : pszCurDirPath),
                pszToken);

        dwError = LWCheckDirectoryExists(pszDirPath, &bDirExists);
        BAIL_ON_MAC_ERROR(dwError);

        if (!bDirExists) {
            if (mkdir(pszDirPath, dwWorkingFileMode) < 0) {
                dwError = errno;
                BAIL_ON_MAC_ERROR(dwError);
            }
            bDirCreated = TRUE;
        }

        dwError = LWChangeDirectory(pszDirPath);
        BAIL_ON_MAC_ERROR(dwError);

        dwError = LWCreateDirectoryRecursive(
                        pszDirPath,
                        pszTmpPath,
                        ppszTmp,
                        dwFileMode,
                        dwWorkingFileMode,
                        iPart+1);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (bDirCreated && (dwFileMode != dwWorkingFileMode)) {
        dwError = LWChangePermissions(pszDirPath, dwFileMode);
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    if (pszDirPath) {
        LWFreeMemory(pszDirPath);
    }

    return dwError;
    
error:

    if (bDirCreated) {
        LWRemoveDirectory(pszDirPath);
    }

    goto cleanup;
}

DWORD
LWCreateTempDirectory(
    PSTR *pszPath
    )
{
    DWORD dwError = 0;
    PSTR tmpDir;
    PSTR szTemplate = NULL;

    BAIL_ON_INVALID_POINTER(pszPath);
    
    *pszPath = NULL;

    tmpDir = getenv("TMPDIR");
    if(tmpDir == NULL)
        tmpDir = "/tmp";

    dwError = LWAllocateStringPrintf(&szTemplate, "%s/centeristmpXXXXXX", tmpDir);
    BAIL_ON_MAC_ERROR(dwError);

    if (mkdtemp(szTemplate) == NULL) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    *pszPath = szTemplate;

cleanup:

    return dwError;
    
error:

    LW_SAFE_FREE_STRING(szTemplate);
    
    goto cleanup;
}

DWORD
LWCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;
    PSTR pszCurDirPath = NULL;
    PSTR pszTmpPath = NULL;
    PSTR pszTmp = NULL;
    mode_t dwWorkingFileMode;

    if (IsNullOrEmptyString(pszPath)) {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwWorkingFileMode = dwFileMode;
    if (!(dwFileMode & S_IXUSR)) {
        /*
         * This is so that we can navigate the folders
         * when we are creating the subfolders
         */
        dwWorkingFileMode |= S_IXUSR;
    }

    dwError = LWGetCurrentDirectoryPath(&pszCurDirPath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWAllocateString(pszPath, &pszTmpPath);
    BAIL_ON_MAC_ERROR(dwError);

    if (*pszPath == '/') {
        dwError = LWChangeDirectory("/");
        BAIL_ON_MAC_ERROR(dwError);

        dwError = LWCreateDirectoryRecursive(
                        "/",
                        pszTmpPath,
                        &pszTmp,
                        dwFileMode,
                        dwWorkingFileMode,
                        0);
        BAIL_ON_MAC_ERROR(dwError);

    } else {

        dwError = LWCreateDirectoryRecursive(
                        pszCurDirPath,
                        pszTmpPath,
                        &pszTmp,
                        dwFileMode,
                        dwWorkingFileMode,
                        0);
        BAIL_ON_MAC_ERROR(dwError);

    }

cleanup:

    if (pszCurDirPath) {

        LWChangeDirectory(pszCurDirPath);

        LWFreeMemory(pszCurDirPath);

    }

    LW_SAFE_FREE_STRING(pszTmpPath);

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWRemoveFiles(
    PSTR pszPath,
    BOOLEAN fDirectory,
    BOOLEAN fRecursive
    )
{
    DWORD dwError = 0;
    CHAR  szCommand[2 * PATH_MAX + 1];

    sprintf(szCommand, "/bin/rm -f %s %s %s",
            fDirectory ? "-d" : "",
            fRecursive ? "-r" : "",
            pszPath);

    if (system(szCommand) < 0) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWGetSymLinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   )
{
   DWORD dwError = 0;
   CHAR szBuf[PATH_MAX+1];
   PSTR pszTargetPath = NULL;

   memset(szBuf, 0, PATH_MAX);

   while (1) {

      if (readlink(pszPath, szBuf, PATH_MAX) < 0) {
         if (errno == EINTR)
            continue;

         dwError = errno;
         BAIL_ON_MAC_ERROR(dwError);
      }

      break;
   }

   dwError = LWAllocateString(szBuf, &pszTargetPath);
   BAIL_ON_MAC_ERROR(dwError);
   
   *ppszTargetPath = pszTargetPath;

cleanup:

    return dwError;
   
error:

    *ppszTargetPath = NULL;

    goto cleanup;
}

DWORD
LWCreateSymLink(
    PCSTR pszOldPath,
    PCSTR pszNewPath
    )
{
    DWORD dwError = 0;

    if (IsNullOrEmptyString(pszOldPath) ||
        IsNullOrEmptyString(pszNewPath)) {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (symlink(pszOldPath, pszNewPath) < 0) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

/*
// On some unix systems, you may not be allowed to
// move files across devices. So, we copy the file to a
// tmp file and then move it to the target location -
// and remove the original file
*/
DWORD
LWMoveFileAcrossDevices(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    )
{
    DWORD dwError = 0;
    CHAR szTmpPath[PATH_MAX+1] = "";
    BOOLEAN bRemoveFile = FALSE;
    uid_t uid;
    gid_t gid;
    mode_t mode;

    sprintf(szTmpPath, "%s_lwi_.tmp", pszDstPath);

    dwError = LWGetOwnerAndPermissions(pszSrcPath, &uid, &gid, &mode);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCopyFileWithPerms(pszSrcPath, szTmpPath, mode);
    BAIL_ON_MAC_ERROR(dwError);

    bRemoveFile = TRUE;

    dwError = LWMoveFile(szTmpPath, pszDstPath);
    BAIL_ON_MAC_ERROR(dwError);

    bRemoveFile = FALSE;

    dwError = LWRemoveFile(pszSrcPath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWChangeOwnerAndPermissions(pszDstPath, uid, gid, mode);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    if (bRemoveFile && !IsNullOrEmptyString(szTmpPath))
       LWRemoveFile(szTmpPath);

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWOpenFile(
    PCSTR path,
    PCSTR mode,
    FILE** handle)
{
    DWORD dwError = 0;
    FILE* pFile = NULL;
    
    pFile = fopen(path, mode);

    if (!pFile)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    
    *handle = pFile;
    
cleanup:

    return dwError;
    
error:

    if (pFile)
    {
        fclose(pFile);
    }
    
    *handle = NULL;

    goto cleanup;
}

DWORD
LWFileStreamWrite(
    FILE* handle,
    PCSTR data,
    unsigned int size)
{
    DWORD dwError = 0;
    unsigned int written = 0;

    while (written < size)
    {
        int amount = fwrite(data+written, 1, size-written, handle);
        if (amount < 0)
        {
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }
        written += amount;
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWFilePrintf(
    FILE* handle,
    PCSTR format,
    ...
    )
{
    DWORD dwError = 0;
    va_list args;
    int count;

    va_start(args, format);
    count = vfprintf(handle, format, args);
    va_end(args);

    if (count < 0)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWCloseFile(
    FILE* handle
    )
{
    DWORD dwError = 0;

    if (fclose(handle))
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWSafeCloseFile(
    FILE** handle
    )
{
    DWORD dwError = 0;
    FILE* pFile = (handle ? *handle : NULL);

    if (pFile)
    {
        if (fclose(pFile))
        {
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }
        
        *handle = NULL;
    }

cleanup:
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWReadNextLine(
    FILE* fp,
    PSTR *output,
    PBOOLEAN pbEndOfFile
    )
{
    DWORD dwError = 0;
    DynamicArray buffer;
    const char nullTerm = '\0';

    *pbEndOfFile = 0;
    *output = NULL;
    memset(&buffer, 0, sizeof(buffer));
    dwError = LWSetCapacity(&buffer, 1, 100);
    BAIL_ON_MAC_ERROR(dwError);

    while(1)
    {
        if(fgets((char*)buffer.data + buffer.size,
                buffer.capacity - buffer.size, fp) ==  NULL)
        {
            if (feof(fp)) {
                *pbEndOfFile = 1;
            } else {
                dwError = errno;
                BAIL_ON_MAC_ERROR(dwError);
            }
        }
        buffer.size += strlen((char*)buffer.data + buffer.size);

        if(*pbEndOfFile)
            break;
        if(buffer.size == buffer.capacity - 1 &&
                ((char *)buffer.data)[buffer.size - 1] != '\n')
        {
            dwError = LWSetCapacity(&buffer, 1, buffer.capacity * 2);
            BAIL_ON_MAC_ERROR(dwError);
        }
        else
            break;
    }

    dwError = LWArrayAppend(&buffer, 1, &nullTerm, 1);
    BAIL_ON_MAC_ERROR(dwError);
    
    *output = (PSTR)buffer.data;
    buffer.data = NULL;

cleanup:

    LWArrayFree(&buffer);
    
    return dwError;
    
error:

    goto cleanup;
}

//The dynamic array must be initialized (at least zeroed out) beforehand
DWORD
LWReadLines(
    FILE *file,
    DynamicArray *dest
    )
{
    DWORD   dwError = 0;
    BOOLEAN eof = FALSE;
    PSTR    readLine = NULL;

    LWArrayFree(dest);
    
    while(!eof)
    {
        dwError = LWReadNextLine(file, &readLine, &eof);
        BAIL_ON_MAC_ERROR(dwError);
        
        dwError = LWArrayAppend(dest, sizeof(readLine), &readLine, 1);
        BAIL_ON_MAC_ERROR(dwError);
        
        readLine = NULL;
    }

cleanup:

    LW_SAFE_FREE_STRING(readLine);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWWriteLines(FILE *file, const DynamicArray *lines)
{
    size_t i;
    DWORD dwError = 0;

    for(i = 0; i < lines->size; i++)
    {
        PCSTR line = *(PCSTR *)LWArrayGetItem((DynamicArray*)lines, i,
                sizeof(PCSTR));
        
        dwError = LWFileStreamWrite(file, line, strlen(line));
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

void
LWFreeLines(DynamicArray *lines)
{
    size_t i;

    for(i = 0; i < lines->size; i++)
    {
        LW_SAFE_FREE_STRING(*(PSTR *)LWArrayGetItem(
                    (DynamicArray*)lines, i, sizeof(PSTR)));
    }
    LWArrayFree(lines);
}

DWORD
LWSetCloseOnExec(
    int fd)
{
    DWORD dwError = 0;
    
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    flags |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, flags) < 0)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}
