/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#ifndef __CTNETINFO_H__
#define __CTNETINFO_H__

typedef struct __NETADAPTER
{
    PSTR    pszName;
    PSTR    pszIPAddress;
    PSTR    pszENetAddress;
    BOOLEAN IsUp;
    BOOLEAN IsRunning;
    struct __NETADAPTER * pNext;
} NETADAPTERINFO, *PNETADAPTERINFO;

DWORD
LWGetNetAdapterList(
    BOOLEAN fEnXOnly,
    PNETADAPTERINFO * ppNetAdapterList
    );

VOID
LWFreeNetAdapterList(
    PNETADAPTERINFO pNetAdapterList
    );

#endif /* __CTNETINFO_H__ */
