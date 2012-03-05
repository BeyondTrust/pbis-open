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
 * HAVE QUESTIONS, OR WISHTO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        svcm.h
 *
 * Abstract:
 *
 *        Service module API
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LW_SVCM_H__
#define __LW_SVCM_H__

#include <lw/attrs.h>
#include <lw/types.h>

LW_BEGIN_EXTERN_C

/**
 * @file svcm.h
 * @brief Service module API
 */

/**
 * @defgroup svcm Service modules
 * @brief Loadable service module framework
 *
 * The service module API presents a means to dynamically load,
 * start, stop, and unload services contained in dynamically
 * loadable object files.
 */

/*@{*/


/**
 * @brief Service instance
 *
 * An opaque structure that represents an instance of a service
 * loaded from a service module.
 */
typedef struct _LW_SVCM_INSTANCE LW_SVCM_INSTANCE, *PLW_SVCM_INSTANCE;

/**
 * @brief Service init function
 *
 * Initializes an instance of of the specified service.
 * Arbitrary data can be attached to the instance structure
 * with #LwRtlSvcmSetData().
 *
 * @param[in] pServiceName the name of the service
 * @param[in,out] pInstance the new service instance
 * @return an NTSTATUS code
 */
typedef LW_NTSTATUS
(*LW_SVCM_INIT_FUNCTION) (
    LW_IN LW_PCWSTR pServiceName,
    LW_IN LW_OUT PLW_SVCM_INSTANCE pInstance
    );

/**
 * @brief Generic service command function
 *
 * Generic function type for issuing a no-argument command
 * to a service.
 *
 * @param[in] pInstance the service instance
 * @return an NTSTATUS code
 */
typedef LW_NTSTATUS
(*LW_SVCM_COMMAND_FUNCTION) (
    LW_IN PLW_SVCM_INSTANCE pInstance
    );

/**
 * @brief Service start function
 *
 * Instructs the given service to start.  This function
 * may safely block until the service is completely ready.
 *
 * @param[in] pInstance the service instance
 * @param[in] ArgCount argument count
 * @param[in] ppArgv arguments
 * @param[in] FdCount file descriptor count
 * @param[in] pFds file descriptors
 * @return an NTSTATUS code
 */
typedef LW_NTSTATUS
(*LW_SVCM_START_FUNCTION) (
    LW_IN PLW_SVCM_INSTANCE pInstance,
    LW_IN LW_ULONG ArgCount,
    LW_IN LW_PWSTR* ppArgs,
    LW_IN LW_ULONG FdCount,
    LW_IN int* pFds
    );

/**
 * @brief Service destroy function
 *
 * Destroys any state associated with the given service
 * instance.
 *
 * @param[in] pInstance the service instance
 */
typedef VOID
(*LW_SVCM_DESTROY_FUNCTION) (
    LW_IN LW_OUT PLW_SVCM_INSTANCE pInstance
    );

/**
 * @brief Service module function table
 *
 * A table of functions for interacting with a service module.
 */
typedef struct _LW_SVCM_MODULE
{
    /**
     * @brief Table size
     * This field should be set to the size of the #LW_SVCM_MODULE
     * structure.  This permits additional fields to be added in
     * the future while retaining backwards compatibility.
     */
    size_t Size;
    /** @brief Init function */
    LW_SVCM_INIT_FUNCTION Init;
    /** @brief Start function */
    LW_SVCM_START_FUNCTION Start;
    /** @brief Stop function */
    LW_SVCM_COMMAND_FUNCTION Stop;
    /** @brief Configuration refresh function */
    LW_SVCM_COMMAND_FUNCTION Refresh;
    /** @brief Destroy function */
    LW_SVCM_DESTROY_FUNCTION Destroy;
} LW_SVCM_MODULE, *PLW_SVCM_MODULE;

/**
 * @brief Service module entry point
 *
 * Returns the service module function table for
 * a service module.  An function with this prototype
 * named #LW_RTL_SVCM_ENTRY_POINT_NAME should be exported
 * by the service module's object file.
 */
typedef PLW_SVCM_MODULE
(*LW_SVCM_MODULE_ENTRY_FUNCTION)(
    LW_VOID
    );

/**
 * @brief Service module entry point name
 *
 * The entry point function of a service module
 * must have a name derived from the use of this
 * macro.
 *
 * @param name the bare name of the module file (no extension)
 * @hideinitializer
 */
#define LW_RTL_SVCM_ENTRY_POINT_NAME(name) _LwSvcmEntry_##name

/**
 * @brief Service command completion function
 *
 * A callback provided by the loader of a service that
 * is invoked when a command completes.
 *
 * @param[in] pInstance the service instance
 * @param[in] Status an NTSTATUS code indicating the result of the command
 * @param[in] pContext the user context pointer
 */
typedef VOID
(*LW_SVCM_NOTIFY_FUNCTION) (
    LW_IN PLW_SVCM_INSTANCE pInstance,
    LW_IN LW_NTSTATUS Status,
    LW_IN LW_PVOID pContext
    );

/* Functions used by the module itself */

/**
 * @brief Set arbitrary data on service instance
 *
 * Sets an arbitrary data pointer on the service
 * instance that can be retrieved with #LwRtlSvcmGetData().
 * This function should only be used by the service
 * module itself and not by the loader.
 *
 * @param[in,out] pInstance the service instance
 * @param[in] pData the data pointer to set
 */
VOID
LwRtlSvcmSetData(
    LW_IN LW_OUT PLW_SVCM_INSTANCE pInstance,
    LW_IN LW_PVOID pData
    );

/**
 * @brief Get arbitrary data on service instance
 *
 * Gets the arbitrary data pointer on the service
 * previously set by #LwRtlSvcmSetData().
 * This function should only be used by the service
 * module itself and not by the loader.
 *
 * @param[in] pInstance the service instance
 * @return the data pointer
 */
PVOID
LwRtlSvcmGetData(
    PLW_SVCM_INSTANCE pInstance
    );

/* Functions used to load and interact with a module */

/**
 * @brief Load a service module
 *
 * Loads the service with the given name from the module
 * file at the specified path.
 *
 * @param[in] pServiceName the name of the service
 * @param[in] pModulePath the filesystem path of the module object file
 * @param[out] ppInstance set to the created service instance
 * @return an NTSTATUS code
 */
LW_NTSTATUS
LwRtlSvcmLoadModule(
    LW_IN LW_PCWSTR pServiceName,
    LW_IN LW_PCWSTR pModulePath,
    LW_OUT PLW_SVCM_INSTANCE* ppInstance
    );

/**
 * @brief Load an embedded service
 *
 * Loads the service with the given name directly
 * from the specified module entry point.  This allows
 * services to be compiled directly into a program
 * and then loaded.
 *
 * @param[in] pServiceName the name of the service
 * @param[in] Entry the entry point function
 * @param[out] ppInstance set to the created service instance
 * @return an NTSTATUS code
 */
LW_NTSTATUS
LwRtlSvcmLoadEmbedded(
    LW_IN LW_PCWSTR pServiceName,
    LW_IN LW_SVCM_MODULE_ENTRY_FUNCTION Entry,
    LW_OUT PLW_SVCM_INSTANCE* ppInstance
    );

/**
 * @brief Start a service
 *
 * Instructs the given service instance to start.
 * This function will return immediately -- the
 * provided notify callback will be invoked when
 * the service has completed starting.
 *
 * @param[in] pInstance the service instance
 * @param[in] ArgCount argument count
 * @param[in] ppArgs arguments
 * @param[in] FdCount file descriptor count
 * @param[in] pFds file descriptors
 * @param[in] Notify the callback to invoke when startup completes
 * @param[in] pContext a context pointer to pass to the notify function
 * @return an NTSTATUS code
 */
LW_NTSTATUS
LwRtlSvcmStart(
    LW_IN PLW_SVCM_INSTANCE pInstance,
    LW_IN LW_ULONG ArgCount,
    LW_IN LW_PWSTR* ppArgs,
    LW_IN LW_ULONG FdCount,
    LW_IN int* pFds,
    LW_IN LW_OPTIONAL LW_SVCM_NOTIFY_FUNCTION Notify,
    LW_IN LW_OPTIONAL LW_PVOID pContext
    );

/**
 * @brief Stop a service
 *
 * Instructs the given service instance to stop.
 * This function will return immediately -- the provided
 * notify callback will be invoked when the service has
 * completed stopping.
 *
 * @param[in] pInstance the service instance
 * @param[in] Notify the callback to invoke when startup completes
 * @param[in] pContext a context pointer to pass to the notify function
 * @return an NTSTATUS code
 */
LW_NTSTATUS
LwRtlSvcmStop(
    LW_IN PLW_SVCM_INSTANCE pInstance,
    LW_IN LW_OPTIONAL LW_SVCM_NOTIFY_FUNCTION Notify,
    LW_IN LW_OPTIONAL LW_PVOID pContext
    );

/**
 * @brief Refresh service configuration
 *
 * Instructs the given service instance to refresh its
 * configuration. This function will return immediately
 * -- the provided notify callback will be invoked when
 * the service has completed refreshing.
 *
 * @param[in] pInstance the service instance
 * @param[in] Notify the callback to invoke when startup completes
 * @param[in] pContext a context pointer to pass to the notify function
 * @return an NTSTATUS code
 */
LW_NTSTATUS
LwRtlSvcmRefresh(
    LW_IN PLW_SVCM_INSTANCE pInstance,
    LW_IN LW_OPTIONAL LW_SVCM_NOTIFY_FUNCTION Notify,
    LW_IN LW_OPTIONAL LW_PVOID pContext
    );

/**
 * @brief Unload a service
 *
 * Completely unloads the given service instance.
 * It is up to the caller to first ensure that the service
 * is stopped, or the behavior of this function
 * is undefined.
 *
 * @param[in,out] pInstance the service instance
 */
VOID
LwRtlSvcmUnload(
    LW_IN LW_OUT PLW_SVCM_INSTANCE pInstance
    );

VOID
LwRtlSvcmFreePool(
    VOID
    );

/*@}*/

LW_END_EXTERN_C

#endif
