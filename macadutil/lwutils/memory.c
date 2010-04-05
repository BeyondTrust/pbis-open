/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#include "includes.h"

DWORD
LWAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    )
{
    DWORD dwError = 0;
    PVOID pMemory = NULL;

    /* malloc is supposed to return a non-NULL pointer when it is asked to
     * allocate 0 bytes of memory. Linux systems usually follow this rule.
     *
     * AIX does not.
     */
    if (dwSize == 0)
    {
        dwSize = 1;
    }

    pMemory = malloc(dwSize);
    if (!pMemory)
    {
        dwError = ENOMEM;
        *ppMemory = NULL;
    }
    else
    {
        memset(pMemory,0, dwSize);
        *ppMemory = pMemory;
    }
    
    return (dwError);
}

DWORD
LWReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    )
{
    DWORD dwError = 0;
    PVOID pNewMemory = NULL;

    if (pMemory == NULL)
    {
        pNewMemory = malloc(dwSize);
        memset(pNewMemory, 0, dwSize);
    }
    else
    {
        pNewMemory = realloc(pMemory, dwSize);
    }
    
    if (!pNewMemory)
    {
        dwError = ENOMEM;
        *ppNewMemory = NULL;
    }
    else
    {
        *ppNewMemory = pNewMemory;
    }

    return (dwError);
}

VOID
LWFreeMemory(
    PVOID pMemory
    )
{
    free(pMemory);
}

