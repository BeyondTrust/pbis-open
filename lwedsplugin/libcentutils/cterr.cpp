/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#include "ctbase.h"
#include <errno.h>
#include "Utilities.h"
#include "PlugInShell.h"
#include <DirectoryService/DirServicesTypes.h>


long
CTMapSystemError(
    int dwError
    )
{
    long macError = eDSNoErr;

    switch(dwError)
    {
    case 0:
        macError = eDSNoErr;
        break;

    case EPERM:
        macError = eDSPermissionError;
        break;

    case EACCES:
        macError = eDSNotAuthorized;
        break;

    case ENOMEM:
        macError = eMemoryAllocError;
        break;

    case EINVAL:
        macError = eParameterError;
        break;

    default:
        LOG("Unable to map system error (%d) to macError", dwError);
        macError = dwError;
        break;
    }

    return macError;
}

