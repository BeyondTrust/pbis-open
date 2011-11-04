/*
 * Copyright (c) Likewise Software.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Module Name:
 *
 *        ioinit.h
 *
 * Abstract:
 *
 *        IO Manager Init Header File
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

#ifndef __IOINIT_H__
#define __IOINIT_H__

#include <lwio/io-types.h>
#include <lwio/iodriver.h>

VOID
IoCleanup(
    VOID
    );

NTSTATUS
IoInitialize(
    IN OPTIONAL PIO_STATIC_DRIVER pStaticDrivers
    );

NTSTATUS
IoMgrQueryStateDriver(
    IN PWSTR pwszDriverName,
    OUT PLWIO_DRIVER_STATE pDriverState
    );

NTSTATUS
IoMgrLoadDriver(
    IN PWSTR pwszDriverName
    );

NTSTATUS
IoMgrUnloadDriver(
    IN PWSTR pwszDriverName
    );

NTSTATUS
IoMgrRefreshConfig(
    VOID
    );

#endif /* __IOINIT_H__ */
