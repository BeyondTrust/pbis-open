/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwfile.c
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi) File Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "includes.h"

#if HAVE_ATTR_XATTR_H
#include <attr/xattr.h>
#define HAVE_XATTR 1
#elif HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#define HAVE_XATTR 1
#else
#define HAVE_XATTR 0
#endif

#if HAVE_XATTR
static
DWORD
LwCopyExtendedAttributes(
	PCSTR pszSrcPath,
	PCSTR pszDstPath
	);
#endif

struct __LW_SELINUX;
typedef struct __LW_SELINUX* PLW_SELINUX;

#if ENABLE_SELINUX
static
DWORD
LwSELinuxCreate(
    PLW_SELINUX *ppSELinux
    );

static
VOID
LwSELinuxFree(
    PLW_SELINUX pSELinux
    );

static
DWORD
LwSELinuxSetContext(
    PCSTR pszPath,
    mode_t mode,
    PLW_SELINUX pSELinux
    );

#endif

DWORD
LwRemoveFile(
    PCSTR pszPath
    )
{
    DWORD dwError = 0;

    while (1) {
        if (unlink(pszPath) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LW_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
LwMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    )
{
    DWORD dwError = 0;

    // POSIX says it will not return EINTR
    if (rename(pszSrcPath, pszDstPath) < 0) {
        dwError = LwMapErrnoToLwError(errno);
    }

    return dwError;
}

static
DWORD
_LwChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode,
    PLW_SELINUX pSELinux
    )
{
    DWORD dwError = 0;

    while (1) {
        if (chmod(pszPath, dwFileMode) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LW_ERROR(dwError);
        } else {

#if ENABLE_SELINUX
            // Attempt to set the file context but don't treat
            // a failure as critical
            LwSELinuxSetContext(pszPath, dwFileMode, pSELinux);
#endif

            break;
        }
    }
error:

    return dwError;
}

DWORD
LwChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    )
{
    return _LwChangePermissions(pszPath, dwFileMode, NULL);
}

DWORD
LwChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    )
{
    DWORD dwError = 0;

    while (1)
    {

        if (lchown(pszPath, uid, gid) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LW_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
LwChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;

    dwError = LwChangeOwner(pszPath, uid, gid);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwChangePermissions(pszPath, dwFileMode);
    BAIL_ON_LW_ERROR(dwError);

error:

    return dwError;
}

DWORD
LwGetCurrentDirectoryPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    CHAR szBuf[PATH_MAX+1];
    PSTR pszPath = NULL;

    if (getcwd(szBuf, PATH_MAX) == NULL) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwAllocateString(szBuf, &pszPath);
    BAIL_ON_LW_ERROR(dwError);

    *ppszPath = pszPath;

    return dwError;

error:

    if (pszPath) {
        LwFreeString(pszPath);
    }

    return dwError;
}

DWORD
LwGetOwnerAndPermissions(
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
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    *uid = statbuf.st_uid;
    *gid = statbuf.st_gid;
    *mode = statbuf.st_mode;

error:

    return dwError;
}

DWORD
LwFindFilesInPaths(
    IN PCSTR pszName,
    IN LWFILE_TYPE type,
    IN const PCSTR* ppszSearchPaths,
    OUT PDWORD pdwFoundCount,
    OUT PSTR** pppszFoundPaths
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    PSTR pszTestPath = NULL;
    DWORD dwFoundCount = 0;
    PSTR* ppszFoundPaths = NULL;
    PSTR* ppszNewFoundPaths = NULL;
    BOOLEAN bExists = FALSE;

    for (dwIndex = 0; ppszSearchPaths[dwIndex]; dwIndex++)
    {
        LW_SAFE_FREE_STRING(pszTestPath);

        dwError = LwAllocateStringPrintf(
                        &pszTestPath,
                        "%s/%s",
                        ppszSearchPaths[dwIndex],
                        pszName);
        BAIL_ON_LW_ERROR(dwError);

        dwError = LwCheckFileTypeExists(
                        pszTestPath,
                        type,
                        &bExists);
        BAIL_ON_LW_ERROR(dwError);

        if (bExists)
        {
            dwError = LwReallocMemory(
                            ppszFoundPaths,
                            OUT_PPVOID(&ppszNewFoundPaths),
                            (dwFoundCount + 1) * sizeof(ppszFoundPaths[0]));
            BAIL_ON_LW_ERROR(dwError);
            ppszFoundPaths = ppszNewFoundPaths;

            ppszFoundPaths[dwFoundCount] = pszTestPath;
            pszTestPath = NULL;
            dwFoundCount++;
        }
    }

    *pdwFoundCount = dwFoundCount;
    *pppszFoundPaths = ppszFoundPaths;
    
cleanup:
    return dwError;

error:
    *pdwFoundCount = 0;
    *pppszFoundPaths = NULL;

    if (ppszFoundPaths)
    {
        LwFreeStringArray(
                ppszFoundPaths,
                dwFoundCount);
    }
    goto cleanup;
}

DWORD
LwCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbExists
    )
{
    DWORD dwError = 0;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1) {
        if (stat(pszPath, &statbuf) < 0) {
           if (errno == EINTR) {
              continue;
           } else if (errno == ENOENT) {
             *pbExists = 0;
             break;
           }
           dwError = LwMapErrnoToLwError(errno);
           BAIL_ON_LW_ERROR(dwError);
        } else {
          *pbExists = 1;
          break;
        }
    }

error:

    return dwError;
}

DWORD
LwCheckFileTypeExists(
    PCSTR pszPath,
    LWFILE_TYPE type,
    PBOOLEAN pbExists
    )
{
    DWORD dwError = 0;
    struct stat statbuf;
    int result = 0;

    memset(&statbuf, 0, sizeof(struct stat));

    if (type == LWFILE_SYMLINK)
    {
        result = lstat(pszPath, &statbuf);
    }
    else
    {
        result = stat(pszPath, &statbuf);
    }
    if (result < 0)
    {
        if (errno == ENOENT || errno == ENOTDIR)
        {
            dwError = 0;
            *pbExists = 0;
        }
        else
        {
            dwError = LwMapErrnoToLwError(errno);
        }
        BAIL_ON_LW_ERROR(dwError);
    }
    else
    {
        switch(type)
        {
            case LWFILE_REGULAR:
                *pbExists = S_ISREG(statbuf.st_mode & S_IFMT);
                break;
            case LWFILE_DIRECTORY:
                *pbExists = S_ISDIR(statbuf.st_mode & S_IFMT);
                break;
            case LWFILE_SYMLINK:
                *pbExists = S_ISLNK(statbuf.st_mode & S_IFMT);
                break;
            case LWFILE_SOCKET:
                *pbExists = S_ISSOCK(statbuf.st_mode & S_IFMT);
                break;
            case LWFILE_PIPE:
                *pbExists = S_ISFIFO(statbuf.st_mode & S_IFMT);
                break;
            default:
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_LW_ERROR(dwError);
        }
    }

error:

    return dwError;
}

static
DWORD
_LwCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode,
    PLW_SELINUX pSELinux
    )
{
    DWORD dwError = 0;
    PSTR pszCopy = NULL;
    // Do not free
    PSTR pszSlashPos = NULL;
    // Do not free
    PSTR pszCopyEnd = NULL;
    BOOLEAN bExists = FALSE;

    if (LW_IS_NULL_OR_EMPTY_STR(pszPath))
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwAllocateString(pszPath, &pszCopy);
    BAIL_ON_LW_ERROR(dwError);
    pszCopyEnd = pszCopy + strlen(pszCopy);

    // Find the first path component. This skips the leading slash if one
    // exists.
    pszSlashPos = strchr(pszCopy + 1, '/');
    if (pszSlashPos == NULL)
    {
        pszSlashPos = pszCopyEnd;
    }
    while (1)
    {
        *pszSlashPos = 0;

        dwError = LwCheckFileTypeExists(
                        pszCopy,
                        LWFILE_DIRECTORY,
                        &bExists);
        BAIL_ON_LW_ERROR(dwError);

        if (!bExists)
        {
            // This path component does not exist. Create this path component
            // and all components underneath it.
            while (1)
            {
                *pszSlashPos = 0;

                if (mkdir(pszCopy, dwFileMode) < 0) {
                    dwError = LwMapErrnoToLwError(errno);
                    BAIL_ON_LW_ERROR(dwError);
                }

#if ENABLE_SELINUX
                // Ignore errors changing context as many files do don have a
                // context defined
                LwSELinuxSetContext(pszCopy, dwFileMode, pSELinux);
#endif
                // Find the next path component, or exit the loop if there are
                // no more.
                if (pszSlashPos == pszCopyEnd)
                {
                    break;
                }
                *pszSlashPos = '/';
                pszSlashPos = strchr(pszSlashPos + 1, '/');
                if (pszSlashPos == NULL)
                {
                    pszSlashPos = pszCopyEnd;
                }
            }
        }

        if (pszSlashPos == pszCopyEnd)
        {
            break;
        }
        *pszSlashPos = '/';
        pszSlashPos = strchr(pszSlashPos + 1, '/');
        if (pszSlashPos == NULL)
        {
            pszSlashPos = pszCopyEnd;
        }
    }

cleanup:
    LW_SAFE_FREE_STRING(pszCopy);
    return dwError;

error:
    goto cleanup;
}

DWORD
LwCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;
    PLW_SELINUX pSELinuxLocal = NULL;

#if ENABLE_SELINUX
    dwError = LwSELinuxCreate(&pSELinuxLocal);
    BAIL_ON_LW_ERROR(dwError);
#endif

    dwError = _LwCreateDirectory(pszPath, dwFileMode, pSELinuxLocal);
    BAIL_ON_LW_ERROR(dwError);

cleanup:
#if ENABLE_SELINUX
    if (pSELinuxLocal)
    {
        LwSELinuxFree(pSELinuxLocal);
        pSELinuxLocal = NULL;
    }
#endif

    return dwError;

error:
    goto cleanup;
}

DWORD
LwRemoveDuplicateInodes(
    IN OUT PDWORD pdwFoundCount,
    IN OUT PSTR* ppszFoundPaths
    )
{
    DWORD dwError = 0;
    DWORD foundCount = *pdwFoundCount;
    DWORD outputIndex = 0;
    struct stat *pStats = NULL;
    DWORD index = 0;
    DWORD index2 = 0;

    dwError = LwAllocateMemory(
                    foundCount * sizeof(pStats[0]),
                    OUT_PPVOID(&pStats));
    BAIL_ON_LW_ERROR(dwError);

    for (index = 0; index < foundCount; index++)
    {
        if (stat(ppszFoundPaths[index], &pStats[index]) < 0)
        {
            switch (errno)
            {
                case ELOOP:
                case ENOENT:
                case ENOTDIR:
                    // These errors indicate a bad symlink. Treat the path as
                    // unique from all others.
                    pStats[index].st_dev = (dev_t)-1;
                    pStats[index].st_ino = (ino_t)-1;
                    break;
                default:
                    dwError = LwMapErrnoToLwError(errno);
                    BAIL_ON_LW_ERROR(dwError);
            }
        }
    }

    if (foundCount > 0)
    {
        // Even if the first index is a duplicate, we take that value over
        // another.
        outputIndex = 1;
    }
    for (index = 1; index < foundCount; index++)
    {
        // See if ppszFoundPaths[index] is a duplicate of any paths with a
        // lower index.
        if (pStats[index].st_ino != (ino_t)-1)
        {
            for (index2 = 0; index2 < index; index2++)
            {
                if (pStats[index].st_dev == pStats[index2].st_dev &&
                    pStats[index].st_ino == pStats[index2].st_ino)
                {
                    LW_SAFE_FREE_STRING(ppszFoundPaths[index]);
                    break;
                }
            }
        }
        if (ppszFoundPaths[index] != NULL)
        {
            ppszFoundPaths[outputIndex] = ppszFoundPaths[index];
            outputIndex++;
        }
    }

    *pdwFoundCount = outputIndex;

cleanup:
    LW_SAFE_FREE_MEMORY(pStats);
    return dwError;

error:
    goto cleanup;
}

DWORD
LwCopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    )
{
    DWORD dwError = 0;
    PCSTR pszTmpSuffix = ".tmp_pbis";
    PSTR pszTmpPath = NULL;
    BOOLEAN bRemoveFile = FALSE;
    CHAR szBuf[1024+1];
    int  iFd = -1;
    int  oFd = -1;
    DWORD dwBytesRead = 0;

    if (LW_IS_NULL_OR_EMPTY_STR(pszSrcPath) ||
        LW_IS_NULL_OR_EMPTY_STR(pszDstPath)) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwAllocateMemory(strlen(pszDstPath)+strlen(pszTmpSuffix)+2,
                               (PVOID*)&pszTmpPath);
    BAIL_ON_LW_ERROR(dwError);

    strcpy(pszTmpPath, pszDstPath);
    strcat(pszTmpPath, pszTmpSuffix);

    if ((iFd = open(pszSrcPath, O_RDONLY, S_IRUSR)) < 0) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    if ((oFd = open(pszTmpPath, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR)) < 0) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    bRemoveFile = TRUE;

    while (1) {
        if ((dwBytesRead = read(iFd, szBuf, 1024)) < 0) {

            if (errno == EINTR)
                continue;

            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LW_ERROR(dwError);
        }

        if (dwBytesRead == 0)
            break;

        if (write(oFd, szBuf, dwBytesRead) != dwBytesRead) {

            if (errno == EINTR)
                continue;

            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LW_ERROR(dwError);

        }

    }

    close(iFd); iFd = -1;
    close(oFd); oFd = -1;

    dwError = LwMoveFile(pszTmpPath, pszDstPath);
    BAIL_ON_LW_ERROR(dwError);

    bRemoveFile = FALSE;

    dwError = LwChangePermissions(pszDstPath, dwPerms);
    BAIL_ON_LW_ERROR(dwError);

#if HAVE_XATTR
    dwError = LwCopyExtendedAttributes(pszSrcPath, pszDstPath);
    BAIL_ON_LW_ERROR(dwError);
#endif

error:

    if (iFd >= 0)
        close(iFd);

    if (oFd >= 0)
        close(oFd);


    if (bRemoveFile) {
        LwRemoveFile(pszTmpPath);
    }

    LW_SAFE_FREE_STRING (pszTmpPath);

    return dwError;
}

#if HAVE_XATTR
static
ssize_t _listxattr(const char* path, char* namebuf, size_t size)
{
#if defined(__LWI_DARWIN__)
	return listxattr(path, namebuf, size, XATTR_NOFOLLOW);
#else
	return listxattr(path, namebuf, size);
#endif
}

static
ssize_t _getxattr(const char* path, const char* name, void* value, size_t size)
{
#if defined(__LWI_DARWIN__)
	return getxattr(path, name, value, size, 0, XATTR_NOFOLLOW);
#else
	return getxattr(path, name, value, size);
#endif
}

static
int _setxattr(const char* path, const char* name, void* value, size_t size)
{
#if defined(__LWI_DARWIN__)
	return setxattr(path, name, value, size, 0, XATTR_NOFOLLOW);
#else
	return setxattr(path, name, value, size, 0);
#endif
}

static
DWORD
LwCopyExtendedAttributes(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
)
{
	DWORD dwError=0;
	char* list = NULL;

	ssize_t listLen = _listxattr(pszSrcPath, NULL, 0);
	list = malloc(listLen);
	listLen = _listxattr(pszSrcPath, list, listLen);

	int ns = 0;
	char* value = NULL;

	for (ns = 0; ns < listLen; ns += strlen(&list[ns]) + 1) {
		ssize_t valueLen;

		valueLen = _getxattr(pszSrcPath, &list[ns], NULL, 0);
		if (valueLen != -1) {

			value = malloc(valueLen);

			_getxattr(pszSrcPath, &list[ns], value, valueLen);
			dwError = LwMapErrnoToLwError(_setxattr(pszDstPath, &list[ns], value, valueLen));
			BAIL_ON_LW_ERROR(dwError);

			free(value);
			value = NULL;
		}
	}

cleanup:
	if(list)
	{
		free(list);
	}

	if(value)
	{
		free(value);
	}

	return dwError;

error:
	goto cleanup;
}

#endif


#if ENABLE_SELINUX

typedef char* security_context_t;
typedef struct __LW_SELINUX
{
    void *dlhandle;
    int (*is_selinux_enabled)();
    int (*matchpathcon_init)(const char *path);
    void (*matchpathcon_fini)(void);
    int (*matchpathcon)(const char *path, mode_t mode, security_context_t *con);
    int (*setfilecon)(const char *path, security_context_t con);
    void (*freecon)(security_context_t con);

    BOOLEAN bEnabled;
} LW_SELINUX;

static
DWORD
LwSELinuxCreate(
    PLW_SELINUX *ppSELinux
    )
{
    DWORD dwError = 0;
    PLW_SELINUX pSELinux = NULL;

    dwError = LwAllocateMemory(sizeof(LW_SELINUX), (PVOID*)&pSELinux);
    BAIL_ON_LW_ERROR(dwError);

    pSELinux->bEnabled = FALSE;

    pSELinux->dlhandle = dlopen(LIBSELINUX, RTLD_LAZY | RTLD_LOCAL);
    if (pSELinux->dlhandle == NULL)
    {
        LW_RTL_LOG_ERROR("Could not load " LIBSELINUX ": %s", dlerror());
        goto cleanup;
    }
    else
    {
        pSELinux->is_selinux_enabled = dlsym(pSELinux->dlhandle, "is_selinux_enabled");
        pSELinux->matchpathcon_init = dlsym(pSELinux->dlhandle, "matchpathcon_init");
        pSELinux->matchpathcon_fini = dlsym(pSELinux->dlhandle, "matchpathcon_fini");
        pSELinux->matchpathcon = dlsym(pSELinux->dlhandle, "matchpathcon");
        pSELinux->setfilecon= dlsym(pSELinux->dlhandle, "setfilecon");
        pSELinux->freecon = dlsym(pSELinux->dlhandle, "freecon");
        if (!pSELinux->is_selinux_enabled ||
            !pSELinux->matchpathcon_init ||
            !pSELinux->matchpathcon_fini ||
            !pSELinux->matchpathcon ||
            !pSELinux->setfilecon ||
            !pSELinux->freecon)
        {
            LW_RTL_LOG_ERROR("Could not find symbol in " LIBSELINUX);
            dwError = LW_ERROR_LOOKUP_SYMBOL_FAILED;
            BAIL_ON_LW_ERROR(dwError);
        }

        if (pSELinux->is_selinux_enabled() == 1)
        {
            LW_RTL_LOG_DEBUG("SELinux is enabled.");
            pSELinux->matchpathcon_init(NULL);
            pSELinux->bEnabled = TRUE;
        }
    }

    *ppSELinux = pSELinux;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pSELinux);
    goto cleanup;
}

static
VOID
LwSELinuxFree(
    PLW_SELINUX pSELinux
    )
{
    if (pSELinux)
    {
        if (pSELinux->bEnabled)
        {
            pSELinux->matchpathcon_fini();
        }

        if (pSELinux->dlhandle)
            dlclose(pSELinux->dlhandle);

        LW_SAFE_FREE_MEMORY(pSELinux);
    }
}

static
DWORD
LwSELinuxSetContext(
    PCSTR pszPath,
    mode_t mode,
    PLW_SELINUX pSELinux
    )
{
    DWORD dwError = 0;
    security_context_t context = NULL;
    PLW_SELINUX pSELinuxLocal = NULL;

    if (pSELinux == NULL)
    {
        dwError = LwSELinuxCreate(&pSELinuxLocal);
        BAIL_ON_LW_ERROR(dwError);

        pSELinux = pSELinuxLocal;
    }

    if ((pSELinux && pSELinux->bEnabled))
    {
        if (pSELinux->matchpathcon(pszPath, mode, &context))
        {
            if (errno != ENOENT) {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LW_ERROR(dwError);
            }
        }
        else
        {
            if (pSELinux->setfilecon(pszPath, context) == -1)
            {

               dwError = LwMapErrnoToLwError(errno);
            }
            BAIL_ON_LW_ERROR(dwError);
        }
    }

cleanup:
    if (context)
    {
        pSELinux->freecon(context);
        context = NULL;
    }

    if (pSELinuxLocal)
    {
        LwSELinuxFree(pSELinuxLocal);
        pSELinuxLocal = NULL;
    }

    return dwError;

error:
    goto cleanup;
}

#endif
