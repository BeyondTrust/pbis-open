/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        globals.c
 *
 * Abstract:
 *
 *        Likewise Event Log
 * 
 *        Server Globals
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#include "includes.h"

pthread_t gSignalHandlerThread;
PVOID     pgSignalHandlerThread = NULL;

LWMGMTSERVERINFO gServerInfo =
{
    PTHREAD_MUTEX_INITIALIZER,  /* Lock              */
    0,                          /* Start as daemon   */
    LOG_LEVEL_ERROR,            /* Max Log Level     */
    "",                         /* Log file path     */
    "",                         /* Config file path  */
    "",                         /* Cache path        */
    "",                         /* Prefix path       */
    0                           /* Process exit flag */
};
