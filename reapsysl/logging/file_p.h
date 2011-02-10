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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        file_p.h
 *
 * Abstract:
 *
 *        Reaper for syslog
 *        File Logger Backend
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */

#ifndef __FILE_P_H__
#define __FILE_P_H__

#define RSYS_ERROR_TAG   "ERROR"
#define RSYS_WARN_TAG    "WARNING"
#define RSYS_INFO_TAG    "INFO"
#define RSYS_VERBOSE_TAG "VERBOSE"
#define RSYS_DEBUG_TAG   "DEBUG"

#define RSYS_LOG_TIME_FORMAT "%Y%m%d%H%M%S"

typedef struct _RSYS_FILE_LOG
{
    RSYS_LOG generic;
    FILE *pInfoFile;
    FILE *pErrorFile;
} RSYS_FILE_LOG, *PRSYS_FILE_LOG;

DWORD RSysLogToFile(
    IN PRSYS_LOG pThis,
    IN RSysLogLevel level,
    IN PSTR pszMessage
    );

DWORD RSysCloseFileLog(
    IN PRSYS_LOG pThis
    );

#endif /* __FILE_P_H__ */
