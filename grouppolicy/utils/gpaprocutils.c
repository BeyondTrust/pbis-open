/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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

#include "includes.h"

CENTERROR
GPAMatchProgramToPID(
    PCSTR pszProgramName,
    pid_t    pid
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    FILE* pFile = NULL;

#if defined(__LWI_DARWIN__) || defined(__LWI_FREEBSD__)
    sprintf(szBuf, "ps -p %d -o command= | grep %s", pid, pszProgramName);
#elif defined(__LWI_SOLARIS__) || defined(__LWI_HP_UX__) || defined(__LWI_AIX__)
    sprintf(szBuf, "UNIX95=1 ps -p %ld -o comm= | grep %s", (long)pid, pszProgramName);
#else
    sprintf(szBuf, "UNIX95=1 ps -p %ld -o cmd= | grep %s", (long)pid, pszProgramName);
#endif

    pFile = popen(szBuf, "r");
    if (pFile == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    /* Assume that we won't find the process we are looking for */
    ceError = CENTERROR_NO_SUCH_PROCESS;

    while (TRUE) {

        if (NULL == fgets(szBuf, PATH_MAX, pFile)) {
            if (feof(pFile)) {
                break;
            } else {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_GPA_ERROR(ceError);
            }
        }

        LwStripWhitespace(szBuf,1,1);
        if (!IsNullOrEmptyString(szBuf)) {
            /* Well what do you know, it was found! */
            ceError = CENTERROR_SUCCESS;
            break;
        }

    }

error:

    if (pFile)
        pclose(pFile);

    return ceError;
}

CENTERROR
GPAIsProgramRunning(
	PCSTR pszPidFile,
	PCSTR pszProgramName,
	pid_t *pPid,
    PBOOLEAN pbRunning
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    pid_t pid = 0;
    int fd = -1;
    int result;
    char contents[20];
    BOOLEAN bFileExists = FALSE;

    if(pbRunning != NULL)
        *pbRunning = FALSE;
    if(pPid != NULL)
        *pPid = -1;

    ceError = GPACheckFileExists(pszPidFile, &bFileExists);
    BAIL_ON_GPA_ERROR(ceError);

    if (bFileExists) {

       fd = open(pszPidFile, O_RDONLY, 0644);
       if (fd < 0) {
          ceError = LwMapErrnoToLwError(errno);
          BAIL_ON_GPA_ERROR(ceError);
       }

       result = read(fd, contents, sizeof(contents)-1);
       if (result < 0) {
          ceError = LwMapErrnoToLwError(errno);
          BAIL_ON_GPA_ERROR(ceError);
       }
       else if (result > 0) {
          contents[result-1] = 0;
          result = atoi(contents);
       }

       if (result <= 0) {
          ceError = CENTERROR_NO_SUCH_PROCESS;
          BAIL_ON_GPA_ERROR(ceError);
       }

       pid = (pid_t) result;
       result = kill(pid, 0);
       if (!result)
       {
          // Verify that the peer process is the auth daemon
          ceError = GPAMatchProgramToPID(pszProgramName, pid);
          BAIL_ON_GPA_ERROR(ceError);

          if(pbRunning != NULL)
          {
              *pbRunning = TRUE;
          }
          if ( pPid ) {
            *pPid = pid;
          }
       }
       else if (errno == ESRCH) {

          ceError = CENTERROR_NO_SUCH_PROCESS;
          BAIL_ON_GPA_ERROR(ceError);

       } else {

          ceError = LwMapErrnoToLwError(errno);
          BAIL_ON_GPA_ERROR(ceError);
       }

   }

error:

    if (fd != -1) {
        close(fd);
    }

    return (ceError == CENTERROR_NO_SUCH_PROCESS ? CENTERROR_SUCCESS : ceError);
}

CENTERROR
GPASendSignal(
	pid_t pid,
	int sig 
	)
{
	int ret = 0;
	ret = kill(pid, sig);
	
	if (ret < 0) {
		return LwMapErrnoToLwError(errno);
	}
	
	return CENTERROR_SUCCESS;
}
