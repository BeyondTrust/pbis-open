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
 *        Event forwarder from eventlogd to collector service
 *        File Logger Backend
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */

#ifndef __FILE_P_H__
#define __FILE_P_H__

#define EFD_ERROR_TAG   "ERROR"
#define EFD_WARN_TAG    "WARNING"
#define EFD_INFO_TAG    "INFO"
#define EFD_VERBOSE_TAG "VERBOSE"
#define EFD_DEBUG_TAG   "DEBUG"

#define EFD_LOG_TIME_FORMAT "%Y%m%d%H%M%S"

typedef struct _EFD_FILE_LOG
{
    EFD_LOG generic;
    FILE *pInfoFile;
    FILE *pErrorFile;
} EFD_FILE_LOG, *PEFD_FILE_LOG;

DWORD EfdLogToFile(
    IN PEFD_LOG pThis,
    IN EfdLogLevel level,
    IN PSTR pszMessage
    );

DWORD EfdCloseFileLog(
    IN PEFD_LOG pThis
    );

#endif /* __FILE_P_H__ */
