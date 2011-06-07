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
 * Module Name:
 *
 *        lwsm.h
 *
 * Abstract:
 *
 *        Primary public header file
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LWSM_H__
#define __LWSM_H__

/**
 * @file lwsm.h
 * @brief Likewise Service Manager API
 */

#include <lw/base.h>
#include <lwdef.h>

/**
 * @defgroup client Client API
 * @brief Client API
 *
 * This module contains the client-side API used to query and control services
 * in the Likewise Service Manager (lwsmd)
 */

/*@{*/

/**
 * @brief Opaque service handle
 *
 * A handle to a particular service that can be used to query or
 * perform operations upon it.
 */
typedef struct _LW_SERVICE_HANDLE *LW_SERVICE_HANDLE, **PLW_SERVICE_HANDLE;

/**
 * @brief Log level
 *
 * Describes the level of a log message in terms
 * of importance.
 */
typedef enum _LW_SM_LOG_LEVEL
{
    /**
     * @brief Indicates the default log level
     * @hideinitializer
     */
    LW_SM_LOG_LEVEL_DEFAULT = 0,
    /**
     * @brief Message that should always be logged
     * @hideinitializer
     */
    LW_SM_LOG_LEVEL_ALWAYS = LW_RTL_LOG_LEVEL_ALWAYS,
    /**
     * @brief Message that indicates an error condition
     * @hideinitializer
     */
    LW_SM_LOG_LEVEL_ERROR = LW_RTL_LOG_LEVEL_ERROR,
    /**
     * @brief Message that indicates a warning condition
     * @hideinitializer
     */
    LW_SM_LOG_LEVEL_WARNING = LW_RTL_LOG_LEVEL_WARNING,
    /**
     * @brief Message that informs the user of a normal
     * but significant condition or event
     * @hideinitializer
     */
    LW_SM_LOG_LEVEL_INFO = LW_RTL_LOG_LEVEL_INFO,
    /**
     * @brief Message that informs the user of a normal
     * and common event
     * @hideinitializer
     */
    LW_SM_LOG_LEVEL_VERBOSE = LW_RTL_LOG_LEVEL_VERBOSE,
    /**
     * @brief Message that is only of interest when attempting
     * to debug improper program behavior.
     * @hideinitializer
     */
    LW_SM_LOG_LEVEL_DEBUG = LW_RTL_LOG_LEVEL_DEBUG,
    /**
     * @brief Message that tracks low-level program execution
     * flow.
     * @hideinitializer
     */
    LW_SM_LOG_LEVEL_TRACE = LW_RTL_LOG_LEVEL_TRACE
} LW_SM_LOG_LEVEL, *PLW_SM_LOG_LEVEL;

/**
 * @brief Logger type
 *
 * Indicates the type of logging target
 */
typedef enum _LW_SM_LOGGER_TYPE
{
    /**
     * @brief Log to the default target
     * @hideinitializer
     */
    LW_SM_LOGGER_DEFAULT,
    /**
     * @brief Do not log
     * @hideinitializer
     */
    LW_SM_LOGGER_NONE,
    /**
     * @brief Log to file
     * @hideinitializer
     */
    LW_SM_LOGGER_FILE,
    /**
     * @brief Log to syslog
     * @hideinitializer
     */
    LW_SM_LOGGER_SYSLOG
} LW_SM_LOGGER_TYPE, *PLW_SM_LOGGER_TYPE;

/**
 * @brief State of a service
 *
 * Represents the state of a service (running, stopped, etc.)
 */
typedef enum _LW_SERVICE_STATE
{
    /**
     * @brief Service is running
     * @hideinitializer
     */
    LW_SERVICE_STATE_RUNNING = 0,
    /** 
     * @brief Service is stopped
     * @hideinitializer
     */
    LW_SERVICE_STATE_STOPPED = 1,
    /** 
     * @brief Service is starting
     * @hideinitializer
     */
    LW_SERVICE_STATE_STARTING = 2,
    /** 
     * @brief Service is stopping
     * @hideinitializer
     */
    LW_SERVICE_STATE_STOPPING = 3,
    /** 
     * @brief Service is paused
     * @hideinitializer
     */
    LW_SERVICE_STATE_PAUSED = 4,
    /**
     * @brief Service is pining for the fjords
     * @hideinitializer 
     */
    LW_SERVICE_STATE_DEAD = 5
} LW_SERVICE_STATE, *PLW_SERVICE_STATE;

/**
 * @brief Service type
 *
 * Represents the type of a service
 */
typedef enum _LW_SERVICE_TYPE
{
    /** 
     * @brief Service is a legacy executable
     * @hideinitializer
     */
    LW_SERVICE_TYPE_LEGACY_EXECUTABLE = 0,
    /** 
     * @brief Service is an lwsm-aware executable 
     * @hideinitializer
     */
    LW_SERVICE_TYPE_EXECUTABLE = 1,
    /** 
     * @brief Service is a module for a container
     * @hideinitializer
     */
    LW_SERVICE_TYPE_MODULE = 2,
    /** 
     * @brief Service is a driver
     * @hideinitializer
     */
    LW_SERVICE_TYPE_DRIVER = 3,
    /** 
     * @brief Service is a dummy stub
     * @hideinitializer
     */
    LW_SERVICE_TYPE_STUB = 4
} LW_SERVICE_TYPE, *PLW_SERVICE_TYPE;

/**
 * @brief Service home
 *
 * Denotes the location of a running service.
 */
typedef enum _LW_SERVICE_HOME
{
    /** 
     * @brief Service is running in a standalone process
     * @hideinitializer
     */
    LW_SERVICE_HOME_STANDALONE,
    /**
     * @brief Service is running in a service container
     * @hideinitializer
     */
    LW_SERVICE_HOME_CONTAINER,
    /**
     * @brief Service is running in the IO manager
     * @hideinitializer
     */
    LW_SERVICE_HOME_IO_MANAGER,
    /**
     * @brief Service is running directly in the service manager
     * @hideinitializer
     */
    LW_SERVICE_HOME_SERVICE_MANAGER
} LW_SERVICE_HOME, *PLW_SERVICE_HOME;

/**
 * @brief Service info mask
 *
 * A bitmask which indicates which fields to update
 * in an #LwSmUpdateServiceInfo() call.
 *
 */
typedef enum _LW_SERVICE_INFO_MASK
{
    /** 
     * @brief Update name
     * @hideinitializer
     */
    LW_SERVICE_INFO_MASK_NAME         = 0x01,
    /**
     * @brief Update description 
     * @hideinitializer
     */
    LW_SERVICE_INFO_MASK_DESCRIPTION  = 0x02,
    /**
     * @brief Update type 
     * @hideinitializer
     */
    LW_SERVICE_INFO_MASK_TYPE         = 0x04,
    /**
     * @brief Update path
     * @hideinitializer
     */
    LW_SERVICE_INFO_MASK_PATH         = 0x08,
    /**
     * @brief Update arguments
     * @hideinitializer
     */
    LW_SERVICE_INFO_MASK_ARGS         = 0x10,
    /**
     * @brief Update environment variables
     * @hideinitializer
     */
    LW_SERVICE_INFO_MASK_ENVIRONMENT  = 0x20,
    /**
     * @brief Update dependencies
     * @hideinitializer
     */
    LW_SERVICE_INFO_MASK_DEPENDENCIES = 0x40,
    /**
     * @brief Update autostart flag
     * @hideinitializer
     */
    LW_SERVICE_INFO_MASK_AUTOSTART    = 0x80,
    /**
     * @brief Update fd limit
     * @hideinitializer
     */
    LW_SERVICE_INFO_MASK_FD_LIMIT     = 0x100,
    /**
     * @brief Update service group name
     * @hideinitializer
     */
    LW_SERVICE_INFO_MASK_GROUP        = 0x200,
    /**
     * @brief Update service log settings
     */
    LW_SERVICE_INFO_MASK_LOG          = 0x400,
    /**
     * @brief Update all flags
     * @hideinitializer
     */
    LW_SERVICE_INFO_MASK_ALL          = 0x7FF
} LW_SERVICE_INFO_MASK, *PLW_SERVICE_INFO_MASK;

/**
 * @brief Service information
 *
 * Describes the basic information about a service,
 * such as its path, command line arguments, etc.
 */
typedef struct _LW_SERVICE_INFO
{
    /** @brief Service short name */
    LW_PWSTR pwszName;
    /** @brief Service description */
    LW_PWSTR pwszDescription;
    /** @brief Service type */
    LW_SERVICE_TYPE type;
    /** @brief Path to service executable or module */
    LW_PWSTR pwszPath;
    /** @brief Arguments to service when started */
    LW_PWSTR* ppwszArgs;
    /** @brief Environment variables with which to start the service  */
    LW_PWSTR* ppwszEnv;
    /** @brief Names of services on which this service depends */
    LW_PWSTR* ppwszDependencies;
    /** @brief Name of service group */
    LW_PWSTR pwszGroup;
    /** @brief Is this service automatically started? */
    LW_BOOLEAN bAutostart;
    /** @brief Desired file descriptor limit for the process */
    LW_DWORD dwFdLimit;
    /** @brief Default log type */
    LW_SM_LOGGER_TYPE DefaultLogType;
    /** @brief Default log target */
    LW_PWSTR pDefaultLogTarget;
    /** @brief Default log level */
    LW_SM_LOG_LEVEL DefaultLogLevel;
} LW_SERVICE_INFO, *PLW_SERVICE_INFO;

typedef const LW_SERVICE_INFO* PCLW_SERVICE_INFO;

/**
 * @brief Service status
 *
 * Describes the runtime status of a service
 */
typedef struct _LW_SERVICE_STATUS
{
    /** Brief Service state (stopped, running, etc.) */
    LW_SERVICE_STATE state;
    /** Brief Service home */
    LW_SERVICE_HOME home;
    /** Brief Process ID of service home */
    pid_t pid;
} LW_SERVICE_STATUS, *PLW_SERVICE_STATUS;

typedef enum _LW_SM_GLOBAL_SETTING
{
    /**
     * @brief No setting
     * @hideinitializer
     */
    LW_SM_GLOBAL_SETTING_NONE = 0,
    /**
     * @brief Control service watchdog
     *
     * (BOOLEAN) Turns service watchdog on or off globally
     * @hideinitializer
     */
    LW_SM_GLOBAL_SETTING_WATCHDOG
} LW_SM_GLOBAL_SETTING, *PLW_SM_GLOBAL_SETTING;

/**
 * @brief Acquire service handle
 *
 * Gets a handle to a known service by name
 *
 * @param[in] pwszServiceName the name of the service
 * @param[out] phHandle a service handle
 * @retval LW_ERROR_SUCCESS success
 * @retval LW_ERROR_NO_SUCH_SERVICE no service with the specified name exists
 */
DWORD
LwSmAcquireServiceHandle(
    LW_PCWSTR pwszServiceName,
    PLW_SERVICE_HANDLE phHandle
    );

/**
 * @brief Release a service handle
 *
 * Releases a handle previously acquired with #LwSmAcquireServiceHandle().
 *
 * @param[in] hHandle the service handle
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmReleaseServiceHandle(
    LW_SERVICE_HANDLE hHandle
    );

/**
 * @brief Enumerate available services
 *
 * Returns a NULL-terminated list of strings containing the names
 * of all known services.  The list should be freed with
 * #LwSmFreeServiceNameList() when done.
 *
 * @param[out] pppwszServiceNames the returned list of services
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmEnumerateServices(
    PWSTR** pppwszServiceNames
    );

/**
 * @brief Free service name list
 *
 * Frees a service name list as returned by e.g. #LwSmEnumerateServices()
 *
 * @param[in,out] ppwszServiceNames the list of services
 */
VOID
LwSmFreeServiceNameList(
    PWSTR* ppwszServiceNames
    );

/**
 * @brief Add new service
 *
 * Adds a new service to the service manager described by
 * the provided service info structure and returns a handle
 * to it.
 *
 * @param[in] pServiceInfo a service info structure describing the new service
 * @param[out] phHandle the created service
 */
DWORD
LwSmAddService(
    PCLW_SERVICE_INFO pServiceInfo,
    PLW_SERVICE_HANDLE phHandle
    );

/**
 * @brief Remove an existing service
 *
 * Removes an existing service from the service manager.
 * The service will not actually be removed until the last
 * handle to it is released with #LwSmReleaseServiceHandle().
 *
 * @param[in] hHandle the service handle
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmRemoveService(
    LW_SERVICE_HANDLE hHandle
    );

/**
 * @brief Update service information
 *
 * Updates the basic information for a service.
 *
 * @param[in] hHandle the service handle
 * @param[in] pServiceInfo service information
 * @param[in] mask a bitmask describing which information fields to update
 * @retval LW_ERROR_SUCCESS success
 * @retval LW_ERROR_ACCESS_DENIED the caller does not have permission to update the service
 */
DWORD
LwSmUpdateServiceInfo(
    LW_SERVICE_HANDLE hHandle,
    PCLW_SERVICE_INFO pServiceInfo,
    LW_SERVICE_INFO_MASK mask
    );

/**
 * @brief Start a service
 *
 * Starts the service represented by the given service handle.  If the service
 * is already started, this function trivially succeeds.  If the service is not
 * started, it will attempt to start it and wait for it finish starting.
 * If the service is in the process of starting, it will wait for it to finish
 * starting.
 *
 * @param[in] hHandle the service handle
 * @retval LW_ERROR_SUCCESS success
 * @retval LW_ERROR_SERVICE_UNRESPONSIVE the service did not respond to requests to start
 * @retval LW_ERROR_SERVICE_DEPENDENCY_UNMET the service depends on another service which is not running
 * @retval LW_ERROR_INVALID_SERVICE_TRANSITION the service cannot be started in its present state
 */
DWORD
LwSmStartService(
    LW_SERVICE_HANDLE hHandle
    );
/**
 * @brief Stop a service
 *
 * Stops the service represented by the given service handle.  If the service
 * is already stopped, this function trivially succeeds.  If the service is not
 * stopped, it will attempt to stop it and wait for it finish stopping.
 * If the service is in the process of stopping, it will wait for it to finish
 * stopping.
 *
 * @param[in] hHandle the service handle
 * @retval LW_ERROR_SUCCESS success
 * @retval LW_ERROR_SERVICE_UNRESPONSIVE the service did not respond to requests to stop
 * @retval LW_ERROR_DEPENDENT_SERVICE_STILL_RUNNING the service cannot be stopped as another running service depends on it
 * @retval LW_ERROR_INVALID_SERVICE_TRANSITION the service cannot be started in its present state
 */
DWORD
LwSmStopService(
    LW_SERVICE_HANDLE hHandle
    );
   
/**
 * @brief Get service status
 *
 * Gets the current status of the service represented by the given service handle.
 * 
 * @param[in] hHandle the service handle
 * @param[out] pStatus the status of the service
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmQueryServiceStatus(
    LW_SERVICE_HANDLE hHandle,
    PLW_SERVICE_STATUS pStatus
    );

/**
 * @brief Wait for service state change
 *
 * Waits for the given service's state to change.  The last known
 * state of the service must be specified as a parameter, and the new
 * state will be returned when a change occurs.
 *
 * @note This function requires the last known state of the service
 * to be passed in to guard against a race condition in which the service
 * changes state after its status is queried but before this function is
 * called.  Passing in the last known state guarantees that this function
 * will return immediately if the service has changed state since the last
 * query.
 *
 * @param[in] hHandle the service handle
 * @param[in] currentState the last known state of the service
 * @param[out] pNewState the new state of the service
 */
DWORD
LwSmWaitService(
    LW_SERVICE_HANDLE hHandle,
    LW_SERVICE_STATE currentState,
    PLW_SERVICE_STATE pNewState
    );

/**
 * @brief Refresh service
 *
 * Refreshes the service represented by the given service handle, which typically
 * entails it reloading its configuration.
 *
 * @param[in] hHandle the service handle
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmRefreshService(
    LW_SERVICE_HANDLE hHandle
    );


/**
 * @brief Sets log target for a service
 *
 * Sets the log target for a service.  If the service is running in a container,
 * all services in the same container will be affected.
 *
 * @param[in] hHandle the service handle, or NULL to specify lwsmd itself
 * @param[in] pFacility the facility to redirect, or NULL to set the default log target
 * @param[in] type the type of logging target
 * @param[in] pTarget the logging target, or NULL for target types that don't require it
 * @retval ERROR_SUCCESS success
 * @retval ERROR_NOT_SUPPORTED the specified service does not support setting log targets
 */
DWORD
LwSmSetServiceLogTarget(
    LW_SERVICE_HANDLE hHandle,
    LW_PCSTR pFacility,
    LW_SM_LOGGER_TYPE type,
    PCSTR pTarget
    );

/**
 * @brief Set maximum log level for a service
 *
 * Sets the maximum log level for a service.  If the service is running in
 * a container, all services in the same container will be affected.
 *
 * @param[in] hHandle the service handle, or NULL to specify lwsmd itself
 * @param[in] pFacility the facility to set the level for, or NULL to set the default log level
 * @param[in] level the maximum log level to set
 * @retval ERROR_SUCCESS success
 * @retval ERROR_NOT_SUPPORTED the specified service does not support setting log targets
 */
DWORD
LwSmSetServiceLogLevel(
    LW_SERVICE_HANDLE hHandle,
    LW_PCSTR pFacility,
    LW_SM_LOG_LEVEL level
    );

/**
 * @brief Get current logging state for a service
 *
 * Gets the current logging target and maximum log level for a service.
 * If the service is running in a container, the returned values apply
 * to all services in the same container.  The returned logging target
 * string can be freed with #LwSmFreeLogTarget().
 *
 * @param[in] hHandle the service handle, or NULL to specify lwsmd itself
 * @param[in] pFacility the facility to get state for, or NULL for the default state
 * @param[out] pType set to the logging target type
 * @param[out] ppTarget set to the logging target if applicable or NULL otherwise
 * @param[out] pLevel set to the maximum log level
 * @retval ERROR_SUCCESS
 * @retval ERROR_NOT_SUPPORTED the specified service does not support getting log state
 */
DWORD
LwSmGetServiceLogState(
    LW_SERVICE_HANDLE hHandle,
    LW_PCSTR pFacility,
    PLW_SM_LOGGER_TYPE pType,
    LW_PSTR* ppTarget,
    PLW_SM_LOG_LEVEL pLevel
    );

/**
 * @brief Free logging target
 *
 * Frees a logging target string returned by a previous call to
 * #LwSmGetServiceLogState().
 *
 * @param[in,out] pTarget
 */
VOID
LwSmFreeLogTarget(
    LW_PSTR pTarget
    );

/**
 * @brief Enumerate logging facilities for a service
 *
 * Enumerates all logging facilities for a service that have a non-default
 * configuration.
 *
 * @param[in] hHandle the service handle, or NULL to specify lwsmd itself
 * @param[out] pppFacilities set to the list of facilities
 * @retval ERROR_SUCCESS
 * @retval ERROR_NOT_SUPPORTED the specified service does not support getting log state
 */
DWORD
LwSmEnumerateServiceLogFacilities(
    LW_SERVICE_HANDLE hHandle,
    LW_PWSTR** pppFacilities
    );

/**
 * @brief Free log facility list
 *
 * Frees a list of logging facilities returned by
 * #LwSmEnumerateServiceLogFacilities().
 */
VOID
LwSmFreeLogFacilityList(
    LW_PWSTR* ppFacilities
    );

/**
 * @brief Get service info
 *
 * Gets the service info structore of the service represented by the given
 * service handle.  The structure should be freed with #LwSmFreeServiceInfo()
 * when done.
 * 
 * @param[in] hHandle the service handle
 * @param[out] ppInfo the info structure for the service
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmQueryServiceInfo(
    LW_SERVICE_HANDLE hHandle,
    PLW_SERVICE_INFO* ppInfo
    );

/**
 * @brief Get recursive dependency list
 *
 * Gets a list of all recursive dependencies of the service represented
 * by the given service handle -- that is, all services the given service
 * depends on directly or indirectly.  The entries in the list will be in the
 * order in which they would need to be started in order to start the
 * given service.  The list should be freed with #LwSmFreeServiceNameList()
 * when done.
 *
 * @param[in] hHandle the service handle
 * @param[out] pppwszServiceList the service dependency list
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmQueryServiceDependencyClosure(
    LW_SERVICE_HANDLE hHandle,
    PWSTR** pppwszServiceList
    );

/**
 * @brief Get recursive reverse dependency list
 *
 * Gets a list of all recursive reverse dependencies of the service represented
 * by the given service handle -- that is, all services which depend on the
 * given service directly or indirectly.  The entries in the list will be in the
 * order in which they would need to be stopped in order to stop the
 * given service.  The list should be freed with #LwSmFreeServiceNameList()
 * when done.
 *
 * @param[in] hHandle the service handle
 * @param[out] pppwszServiceList the service reverse dependency list
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmQueryServiceReverseDependencyClosure(
    LW_SERVICE_HANDLE hHandle,
    PWSTR** pppwszServiceList
    );

/**
 * @brief Free service info structure
 *
 * Frees a service info structure as returned by e.g.
 * #LwSmQueryServiceInfo()
 *
 * @param[in,out] pInfo the service info structure to free
 */
VOID
LwSmFreeServiceInfo(
    PLW_SERVICE_INFO pInfo
    );

/**
 * @brief Refresh service manager
 *
 * Causes the service manager to reread its own configuration.
 * To refresh the configuration of a particular service, use
 * #LwSmRefreshService().
 *
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmRefresh(
    VOID
    );

/**
 * @brief Shutdown
 *
 * Causes the service manager to shut down completely.
 *
 * @retval LW_ERROR_SUCCESS success
 */
DWORD
LwSmShutdown(
    VOID
    );

/**
 * @brief Change global setting
 *
 * Changes a global setting.  See the description of each
 * global setting for the type of parameters to pass as
 * the trailing arguments.
 *
 * @param[in] Setting the setting to change
 * @param[in] ... value or values for the given setting
 * @retval #ERROR_SUCCESS success
 * @retval #ERROR_INVALID_PARAMETER invalid setting or value
 */
DWORD
LwSmSetGlobal(
    LW_IN LW_SM_GLOBAL_SETTING Setting,
    ...
    );

/**
 * @brief Get global setting
 *
 * Gets the value of a global setting.  The
 * trailing parameters should be out pointers to
 * the appropriate types listed in the description
 * of the setting to change.
 *
 * @param[in] Setting the setting to get
 * @param[out] ... set to the value or values for the given setting
 */
DWORD
LwSmGetGlobal(
    LW_IN LW_SM_GLOBAL_SETTING Setting,
    ...
    );

/**
 * @brief Free global setting value
 *
 * Frees the value of a global setting previously
 * returned by #LwSmGetGlobal().
 *
 * @param[in] Setting the setting associated with the value to free
 * @param[in,out] ... the value or values to free
 */
VOID
LwSmFreeGlobal(
    LW_IN LW_SM_GLOBAL_SETTING Setting,
    ...
    );

/*@}*/

#endif
