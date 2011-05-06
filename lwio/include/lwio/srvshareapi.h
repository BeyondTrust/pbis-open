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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        srvshareapi.h
 *
 * Abstract:
 *
 *        Client API around SRV share IOCTLs
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 */

#ifndef __LWIO_SRVSHAREAPI_H__
#define __LWIO_SRVSHAREAPI_H__

#include <lwio/io-types.h>
#include <lwio/lwshareinfo.h>

/**
 * @brief Add share
 *
 * Adds a new share with the specified share info.
 *
 * @param[in] pServer the server name
 * @param[in] Level the share info level
 * @param[in] pInfo the share info
 * @retval LW_STATUS_SUCCESS success
 * @retval LW_STATUS_INVALID_LEVEL invalid info level
 */
LW_NTSTATUS
LwIoSrvShareAdd(
    LW_IN LW_PCWSTR pServer,
    LW_IN LW_ULONG Level,
    LW_IN LW_PVOID pInfo
    );

/**
 * @brief Delete a share
 *
 * Deletes the share with the specified name.
 *
 * @param[in] pServer the server name
 * @param[in] pNetname the share name
 * @retval LW_STATUS_SUCCESS success
 * @retval LW_STATUS_NOT_FOUND the specified share was not found
 */
LW_NTSTATUS
LwIoSrvShareDel(
    LW_IN LW_PCWSTR pServer,
    LW_IN LW_PCWSTR pNetname
    );

/**
 * @brief Enumerate shares
 *
 * Enumerates all shares at the specified info level.
 * The results can be freed with #LwIoSrvShareFreeInfo().
 *
 * @param[in] pServer the server name
 * @param[in] Level the share info level to return
 * @param[out] ppInfo set to the returned share info array
 * @param[out] pCount set to the number of shares returned
 * @retval LW_STATUS_SUCCESS success
 * @retval LW_STATUS_INVALID_LEVEL invalid info level
 */
LW_NTSTATUS
LwIoSrvShareEnum(
    LW_IN LW_PCWSTR pServer,
    LW_IN LW_ULONG Level,
    LW_OUT LW_PVOID* ppInfo,
    LW_OUT LW_PULONG pCount
    );

/**
 * @brief Get share info
 *
 * Gets info about the specified share at the given info level.
 * Use #LwIoSrvShareFreeInfo() with a Count of 1 to free the result.
 *
 * @param[in] pServer the server name
 * @param[in] pNetname the share name
 * @param[in] Level the share info level to return
 * @param[out] ppInfo set to the returned share info
 * @retval LW_STATUS_SUCCESS success
 * @retval LW_STATUS_INVALID_LEVEL invalid info level
 * @retval LW_STATUS_NOT_FOUND the specified share was not found
 */
LW_NTSTATUS
LwIoSrvShareGetInfo(
    LW_IN LW_PCWSTR pServer,
    LW_IN LW_PCWSTR pNetname,
    LW_IN LW_ULONG Level,
    LW_OUT LW_PVOID* ppInfo
    );

/**
 * @brief Set share info
 *
 * Sets info about the specified share at the given info level.
 *
 * @param[in] pServer the server name
 * @param[in] pNetname the share name
 * @param[in] Level the share info level
 * @param[in] pInfo the share info to set
 * @retval LW_STATUS_SUCCESS success
 * @retval LW_STATUS_INVALID_LEVEL invalid info level
 * @retval LW_STATUS_NOT_FOUND the specified share was not found
 */
LW_NTSTATUS
LwIoSrvShareSetInfo(
    LW_IN LW_PCWSTR pServer,
    LW_IN LW_PCWSTR pNetname,
    LW_IN LW_ULONG Level,
    LW_IN LW_PVOID pInfo
    );

/**
 * @brief Free share info
 *
 * Frees a share info structure or share info structure array.
 *
 * @param[in] Level the share info level
 * @param[in] Count the number of info structure to free
 * @param[in,out] pInfo the share info to free
 */
LW_VOID
LwIoSrvShareFreeInfo(
    LW_ULONG Level,
    LW_ULONG Count,
    LW_PVOID pInfo
    );

/**
 * @brief Reload shares configuration
 * 
 * Allows reloading shares configuration in cases the sharedb 
 * content was changed. 
 */
LW_NTSTATUS
LwIoSrvShareReloadConfiguration(
    LW_VOID
    );

#endif
