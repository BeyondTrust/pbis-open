/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

#include "iop.h"

#define IOP_CONFIG_TAG_DRIVER "driver:"

static
VOID
IopConfigFreeDriverConfig(
    IN OUT PIOP_DRIVER_CONFIG* ppDriverConfig
    )
{
    PIOP_DRIVER_CONFIG pDriverConfig = *ppDriverConfig;

    if (pDriverConfig)
    {
        RtlCStringFree(&pDriverConfig->pszName);
        RtlCStringFree(&pDriverConfig->pszPath);
        IoMemoryFree(pDriverConfig);
        *ppDriverConfig = NULL;
    }
}

NTSTATUS
IopConfigAddDriver(
    PIOP_CONFIG pConfig,
    PCSTR   pszName,
    PCSTR   pszPath
        )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIOP_DRIVER_CONFIG pDriverConfig = NULL;
    PLW_LIST_LINKS pLinks = NULL;

            // Check for duplicate driver config.
    for (pLinks = pConfig->DriverConfigList.Next;
         pLinks != &pConfig->DriverConfigList;
         pLinks = pLinks->Next)
    {
        PIOP_DRIVER_CONFIG pCheckDriverConfig = 
                LW_STRUCT_FROM_FIELD(pLinks, IOP_DRIVER_CONFIG, Links);
        if (!strcasecmp(pCheckDriverConfig->pszName, pszName))
        {
            LWIO_LOG_ERROR("Duplicate driver name '%s'", pszName);

            status = STATUS_UNSUCCESSFUL;
            GOTO_CLEANUP_EE(EE);
        }
    }

    status = IO_ALLOCATE(
                &pDriverConfig,
                IOP_DRIVER_CONFIG,
                sizeof(*pDriverConfig));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlCStringDuplicate(&pDriverConfig->pszName, pszName);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlCStringDuplicate(&pDriverConfig->pszPath, pszPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LwListInsertTail(&pConfig->DriverConfigList, &pDriverConfig->Links);
    pConfig->DriverCount++;

cleanup:

    if (status)
    {
        IopConfigFreeDriverConfig(&pDriverConfig);
    }

    return status;
}

NTSTATUS
IopConfigReadDriver(
     PIOP_CONFIG pConfig,
     PCSTR pszDriverKey,
     PCSTR pszDriverName
     )
{
    NTSTATUS status = 0;
    int EE = 0;
    PLWIO_CONFIG_REG pReg = NULL;
    PSTR pszDriverPath = NULL;

    status = LwIoOpenConfig(
                pszDriverKey,
                pszDriverKey,
                &pReg);
    if (status)
    {
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (!pReg)
    {
        goto cleanup;
    }

    status = LwIoReadConfigString(pReg, "Path", FALSE, &pszDriverPath);
    if (status)
    {
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    if (IsNullOrEmptyString(pszDriverPath))
    {
        LWIO_LOG_ERROR("Empty path for driver '%s'", pszDriverName);
        goto cleanup;
    }

    status = IopConfigAddDriver(pConfig, pszDriverName, pszDriverPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    LwIoCloseConfig(pReg);
    pReg = NULL;

    LwRtlCStringFree(&pszDriverPath);

    return status;

}

NTSTATUS
IopConfigAddDrivers(
     PIOP_CONFIG pConfig
     )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PLWIO_CONFIG_REG pReg = NULL;
    PSTR pszDriverKey = NULL;
    PCSTR pszDriverName = NULL;
    PSTR pszDriverNames = NULL;
    PSTR pszTokenState = NULL;

    status = LwIoOpenConfig(
                "Services\\lwio\\Parameters\\Drivers",
                "Policy\\Services\\lwio\\Parameter\\Drivers",
                &pReg);
    if (status)
    {
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
 
    if (!pReg)
    {
        goto add_default;
    }

    status = LwIoReadConfigString(pReg, "Load", FALSE, &pszDriverNames);
    if (status)
    {
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    if(IsNullOrEmptyString(pszDriverNames))
    {
        goto add_default;
    }

    pszDriverName = strtok_r(pszDriverNames, ",", &pszTokenState);
    while (!IsNullOrEmptyString(pszDriverName))
    {
        status = LwRtlCStringAllocatePrintf(
                    &pszDriverKey,
                    "Services\\lwio\\Parameters\\Drivers\\%s",
                    pszDriverName);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        status = IopConfigReadDriver(
                    pConfig,
                    pszDriverKey,
                    pszDriverName);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        LwRtlCStringFree(&pszDriverKey);

        pszDriverName = strtok_r(NULL, ",", &pszTokenState);
    }

cleanup:
        
    LwIoCloseConfig(pReg);
    pReg = NULL;

    LwRtlCStringFree(&pszDriverNames);

    LwRtlCStringFree(&pszDriverKey);

    return status;

add_default:
    status = IopConfigAddDriver(pConfig, "rdr", LIBDIR "/librdr.sys.so");
    goto cleanup;
}

VOID
IopConfigFreeConfig(
    IN OUT PIOP_CONFIG* ppConfig
    )
{
    PIOP_CONFIG pConfig = *ppConfig;
    if (pConfig)
    {
        while (!LwListIsEmpty(&pConfig->DriverConfigList))
        {
            PLW_LIST_LINKS pLinks = LwListRemoveHead(&pConfig->DriverConfigList);
            PIOP_DRIVER_CONFIG pDriverConfig = LW_STRUCT_FROM_FIELD(pLinks, IOP_DRIVER_CONFIG, Links);

            IopConfigFreeDriverConfig(&pDriverConfig);
        }
        IoMemoryFree(pConfig);
        *ppConfig = NULL;
    }
}

NTSTATUS
IopConfigReadRegistry(
    OUT PIOP_CONFIG* ppConfig
    )
{
    NTSTATUS status = 0;
    PIOP_CONFIG pConfig = NULL;

    status = IO_ALLOCATE(&pConfig, IOP_CONFIG, sizeof(*pConfig));
    GOTO_CLEANUP_ON_STATUS(status);

    LwListInit(&pConfig->DriverConfigList);

    status = IopConfigAddDrivers(pConfig);
    if (status)
    {
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:

    if (status)
    {
        IopConfigFreeConfig(&pConfig);
    }

    *ppConfig = pConfig;

    return status;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
