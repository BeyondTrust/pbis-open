/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog definitions
 *
 */
#ifndef __EVTSTRUCT_H__
#define __EVTSTRUCT_H__

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

typedef enum
{
    TableCategoryApplication = 0,
    TableCategoryWebBrowser, //1
    TableCategorySecurity, //2
    TableCategorySystem,  //3
    TABLE_CATEGORY_SENTINEL
} TableCategoryType;

#ifdef _DCE_IDL_
#define CONTEXT_HANDLE [context_handle]
#else
#define CONTEXT_HANDLE
#endif

#ifndef RPC_LWCOLLECTOR_HANDLE_DEFINED
#define RPC_LWCOLLECTOR_HANDLE_DEFINED
typedef CONTEXT_HANDLE struct _RPC_LWCOLLECTOR_CONNECTION *RPC_LWCOLLECTOR_HANDLE;
#endif

typedef struct _COLLECTOR_HANDLE
{
    RPC_LWCOLLECTOR_HANDLE hCollector;
    WCHAR * pszBindingString;
} COLLECTOR_HANDLE, *PCOLLECTOR_HANDLE;

#endif /* __EVTSTRUCT_H__ */
