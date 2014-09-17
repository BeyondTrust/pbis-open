/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright BeyondTrust 2013
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
 * license@beyondtrust.com
 */

/*
 * Copyright (C) BeyondTrust 2013
 * All rights reserved.
 *
 */

#include "includes.h"
// #include <dlfcn.h>

#define SENDTRAP_DLL_NAME      "/opt/pbis/lib/sendtrap/sendtrap.so"
#define SENDTRAP_DLL_NAME_64   "/opt/pbis/lib64/sendtrap/sendtrap.so"

typedef VOID (*PFN_SendTrapSetup) (void);
typedef VOID (*PFN_SendTrapTearDown) ();
typedef VOID (*PFN_SendTrapReadConfiguration) ();
typedef VOID (*PFN_SendTrapProcessEvents) (DWORD Count, const LW_EVENTLOG_RECORD *pRecords);

static PFN_SendTrapSetup             gpfnSendTrapSetup = NULL;
static PFN_SendTrapTearDown          gpfnSendTrapTearDown = NULL;
static PFN_SendTrapReadConfiguration gpfnSendTrapReadConfiguration = NULL;
static PFN_SendTrapProcessEvents     gpfnSendTrapProcessEvents = NULL;

static void *                    gpSendTrapLibHandle = (void*) NULL;
static BOOLEAN                   gbSendTrapLibInitialized = FALSE;


//***************************************************************************
//
//***************************************************************************
DWORD EvtSnmpSetup()
{
  DWORD dwError = ERROR_SUCCESS;

  if (gbSendTrapLibInitialized == TRUE)
     goto cleanup;

  gbSendTrapLibInitialized = TRUE;

   // open library dlopen(sendtrap.so) 
  gpSendTrapLibHandle = dlopen(SENDTRAP_DLL_NAME_64, RTLD_LAZY);
  if (gpSendTrapLibHandle == NULL)
  {
      gpSendTrapLibHandle = dlopen(SENDTRAP_DLL_NAME, RTLD_LAZY);
      if (gpSendTrapLibHandle == NULL)
      {
         dwError = LW_ERROR_LOAD_LIBRARY_FAILED;
         BAIL_ON_EVT_ERROR(dwError);
      }
  }

  gpfnSendTrapSetup = (PFN_SendTrapSetup) dlsym(gpSendTrapLibHandle, "SendTrapSnmpSetup");
  if (gpfnSendTrapSetup == NULL)
  {
      dwError = LW_ERROR_LOOKUP_SYMBOL_FAILED;
      BAIL_ON_EVT_ERROR(dwError);
  }

  gpfnSendTrapTearDown = (PFN_SendTrapTearDown) dlsym(gpSendTrapLibHandle, "SendTrapSnmpTearDown");
  if (gpfnSendTrapTearDown == NULL)
  {
      dwError = LW_ERROR_LOOKUP_SYMBOL_FAILED;
      BAIL_ON_EVT_ERROR(dwError);
  }

  gpfnSendTrapReadConfiguration = (PFN_SendTrapReadConfiguration) dlsym(gpSendTrapLibHandle, "SendTrapSnmpReadConfiguration");
  if (gpfnSendTrapReadConfiguration == NULL)
  {
      dwError = LW_ERROR_LOOKUP_SYMBOL_FAILED;
      BAIL_ON_EVT_ERROR(dwError);
  }

  gpfnSendTrapProcessEvents = (PFN_SendTrapProcessEvents) dlsym(gpSendTrapLibHandle, "SendTrapProcessEvents");
  if (gpfnSendTrapProcessEvents == NULL)
  {
      dwError = LW_ERROR_LOOKUP_SYMBOL_FAILED;
      BAIL_ON_EVT_ERROR(dwError);
  }

  if (gpSendTrapLibHandle && gpfnSendTrapSetup)
  {
     gpfnSendTrapSetup();
  }

  dwError = EvtSnmpReadConfiguration();
  BAIL_ON_EVT_ERROR(dwError);

cleanup:
    return dwError;
   
error:
    goto cleanup;
}

//***************************************************************************
//
//***************************************************************************
VOID  EvtSnmpTearDown()
{
   DWORD dwError = 0;

   if (gpSendTrapLibHandle && gpfnSendTrapTearDown)
        gpfnSendTrapTearDown();

   if (gpSendTrapLibHandle)
   {
       gpfnSendTrapProcessEvents = NULL;
       gpfnSendTrapSetup = NULL;
       gpfnSendTrapTearDown = NULL;
       
       if (dlclose(gpSendTrapLibHandle))
          dwError = LwMapErrnoToLwError(errno);

       gpSendTrapLibHandle = (void*) NULL;
       gbSendTrapLibInitialized = FALSE;
       BAIL_ON_EVT_ERROR(dwError);
   }

cleanup:
    return;

error:
    goto cleanup;

}

//***************************************************************************
//
//***************************************************************************
DWORD  EvtSnmpReadConfiguration()
{
   DWORD dwError = ERROR_SUCCESS;

  if (gpSendTrapLibHandle && gpfnSendTrapReadConfiguration)
     gpfnSendTrapReadConfiguration();

  return dwError;
}


//****************************************************************************
//
//****************************************************************************
VOID EvtSnmpProcessEvents( DWORD Count,
                           const LW_EVENTLOG_RECORD *pRecords)
{
  if (gpSendTrapLibHandle && gpfnSendTrapProcessEvents)
     gpfnSendTrapProcessEvents(Count, pRecords);

  return;
}
