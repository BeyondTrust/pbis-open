/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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

#include "includes.h"

static
DWORD
DNSGatherInterfaceInfo(
    PDNSDLINKEDLIST* ppInterfaceList
    );

#ifdef HAVE_GETIFADDRS

static
DWORD
DNSGetInfoUsingGetIfAddrs(
    PDNSDLINKEDLIST* ppInterfaceList
    );

#else

static
DWORD
DNSGetInfoUsingIoctl(
    PDNSDLINKEDLIST* ppInterfaceList
    );

#endif

#ifdef LW_SKIP_ALIASED_INTERFACES
static
BOOLEAN
DNSInterfaceIsInList(
    PCSTR pszName,
    PDNSDLINKEDLIST pInterfaceList
    );
#endif

static
DWORD
DNSBuildInterfaceArray(
    PDNSDLINKEDLIST     pInterfaceList,
    PLW_INTERFACE_INFO* ppInterfaceInfoArray,
    PDWORD              pdwNumInterfaces
    );

static
VOID
DNSFreeInterfaceLinkedList(
    PDNSDLINKEDLIST pInterfaceList
    );

static
VOID
DNSFreeInterfaceInList(
    PVOID pItem,
    PVOID pUserData
    );

DWORD
DNSGetNetworkInterfaces(
    PLW_INTERFACE_INFO* ppInterfaceInfoArray,
    PDWORD              pdwNumInterfaces
    )
{
    DWORD dwError = 0;
    PLW_INTERFACE_INFO pInterfaceInfoArray = NULL;
    DWORD dwNumInterfaces = 0;
    PDNSDLINKEDLIST pInterfaceList = NULL;
    
    dwError = DNSGatherInterfaceInfo(
                    &pInterfaceList);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    dwError = DNSBuildInterfaceArray(
                    pInterfaceList,
                    &pInterfaceInfoArray,
                    &dwNumInterfaces);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    *ppInterfaceInfoArray = pInterfaceInfoArray;
    *pdwNumInterfaces = dwNumInterfaces;
    
cleanup:

    if (pInterfaceList)
    {
        DNSFreeInterfaceLinkedList(pInterfaceList);
    }

    return dwError;
    
error:

    if (pInterfaceInfoArray)
    {
        DNSFreeNetworkInterfaces(
                pInterfaceInfoArray,
                dwNumInterfaces);
    }
    
    *ppInterfaceInfoArray = NULL;
    *pdwNumInterfaces = 0;

    goto cleanup;
}

static
DWORD
DNSGatherInterfaceInfo(
    PDNSDLINKEDLIST* ppInterfaceList
    )
{

#ifdef HAVE_GETIFADDRS
    
    return DNSGetInfoUsingGetIfAddrs(
                    ppInterfaceList);
    
#else
    
    return DNSGetInfoUsingIoctl(
                    ppInterfaceList);
#endif

}

#ifdef HAVE_GETIFADDRS

static
DWORD
DNSGetInfoUsingGetIfAddrs(
    PDNSDLINKEDLIST* ppInterfaceList
    )
{
    DWORD dwError = 0;
    PDNSDLINKEDLIST pInterfaceList = NULL;
    struct ifaddrs* pInterfaces = NULL;
    struct ifaddrs* pIter = NULL;
    PLW_INTERFACE_INFO pInterfaceInfo = NULL;
    
    if (getifaddrs(&pInterfaces) < 0)
    {
        dwError = errno;
        BAIL_ON_LWDNS_ERROR(dwError);
    }
    
    for (pIter = pInterfaces; pIter; pIter = pIter->ifa_next)
    {
        if (IsNullOrEmptyString(pIter->ifa_name))
        {
           LWDNS_LOG_VERBOSE("Skipping network interface with no name");
           continue;
        }

        LWDNS_LOG_VERBOSE("Considering network interface [%s]",
                          pIter->ifa_name);
        
        if (pIter->ifa_addr->sa_family != AF_INET)
        { 
            LWDNS_LOG_VERBOSE("Skipping network interface [%s] because it is not AF_INET family", pIter->ifa_name);
            continue;
        }
        
        if (!(pIter->ifa_flags & IFF_UP))
        {
            LWDNS_LOG_VERBOSE("Skipping in-active network interface [%s]",
                              pIter->ifa_name);
            continue;
        }
        
        if (pIter->ifa_flags & IFF_LOOPBACK)
        {
            LWDNS_LOG_VERBOSE("Skipping loopback network interface [%s]",
                              pIter->ifa_name);
            continue;
        }
        
#ifdef LW_SKIP_ALIASED_INTERFACES
        if (DNSInterfaceIsInList(pIter->ifa_name, pInterfaceList))
        {
            LWDNS_LOG_VERBOSE("Skipping aliased network interface [%s]",
                              pIter->ifa_name);
            continue;
        }
#endif
        
        dwError = DNSAllocateMemory(
                    sizeof(LW_INTERFACE_INFO),
                    (PVOID*)&pInterfaceInfo);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        dwError = DNSAllocateString(
                    pIter->ifa_name,
                    &pInterfaceInfo->pszName);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        if (pIter->ifa_addr)
        {
            memcpy(&pInterfaceInfo->ipAddr,
                   pIter->ifa_addr,
                   sizeof(*pIter->ifa_addr));
        }
        
        pInterfaceInfo->dwFlags = pIter->ifa_flags;
        
        dwError = DNSDLinkedListAppend(
                    &pInterfaceList,
                    pInterfaceInfo);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        LWDNS_LOG_VERBOSE("Added network interface [Name:%s] to list",
                          pInterfaceInfo->pszName);
        
        pInterfaceInfo = NULL;
    }
    
    *ppInterfaceList = pInterfaceList;
    
cleanup:

    if (pInterfaces)
    {
        freeifaddrs(pInterfaces);
    }

    return dwError;
    
error:

    if (pInterfaceList)
    {
        DNSFreeInterfaceLinkedList(pInterfaceList);
    }
    
    if (pInterfaceInfo)
    {
        DNSFreeNetworkInterface(pInterfaceInfo);
    }

    *ppInterfaceList = NULL;

    goto cleanup;
}

#else

static
DWORD
DNSGetInfoUsingIoctl(
    PDNSDLINKEDLIST* ppInterfaceList
    )
{
    DWORD   dwError = 0;
    SOCKET  fd = -1;
    DWORD   dwBufLen = 0;
    DWORD   dwLastBufLen = 0;
    PBYTE   pBuffer = NULL;
    PBYTE   pIter = NULL;
    struct ifconf ifc = {0};
    PDNSDLINKEDLIST pInterfaceList = NULL;
    PLW_INTERFACE_INFO pInterfaceInfo = NULL;
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        dwError = errno;
        BAIL_ON_LWDNS_ERROR(dwError);
    }
    
    dwBufLen = 64 * sizeof(struct ifreq);
    
    dwError = DNSAllocateMemory(
                dwBufLen,
                (PVOID*)&pBuffer);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    do
    {
        DWORD dwNewSize = 0;
        
        ifc.ifc_len = dwBufLen;
        ifc.ifc_buf = (caddr_t)pBuffer;
        
        // On some systems, the ioctl returns success
        // even if the buffer is insufficient. So, we
        // retry until the buffer length consolidates
        if (ioctl(fd, SIOCGIFCONF, &ifc) < 0)
        {
            if ((errno != EINVAL) || (dwLastBufLen))
            {
                dwError = errno;
                BAIL_ON_LWDNS_ERROR(dwError);
            }
        }
        else
        {
            if (dwLastBufLen == ifc.ifc_len)
            {
                break;
            }
            
            dwLastBufLen = ifc.ifc_len;
        }
        
        dwNewSize = dwLastBufLen + 32 * sizeof(struct ifreq);
        
        dwError = DNSReallocMemory(
                        pBuffer,
                        (PVOID*)&pBuffer,
                        dwNewSize);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        dwBufLen = dwNewSize;
        
    } while (TRUE);
    
    for (pIter = pBuffer; pIter < pBuffer + dwBufLen;)
    {
        CHAR   szItfName[IFNAMSIZ+1];
        PSTR   pszIpAddress = NULL;
        DWORD  dwFlags = 0;
        struct ifreq* pInterfaceRecord = NULL;
        struct ifreq  interfaceRecordCopy;
        struct sockaddr * pSA = NULL;
        DWORD dwLen = 0;
#ifdef LW_SKIP_ALIASED_INTERFACES
        PSTR   pszIndex = NULL;
#endif
        
        pInterfaceRecord = (struct ifreq*)pIter;
        
#ifdef AF_LINK
        if (pInterfaceRecord->ifr_addr.sa_family == AF_LINK)
        {
#if defined(__LWI_AIX__)
            dwLen = DNS_MAX(sizeof(struct sockaddr_dl), ((struct sockaddr_dl*)&pInterfaceRecord->ifr_addr)->sdl_len);
#else /* Solaris etc. */
            dwLen = sizeof(struct sockaddr_dl);
#endif
        }
        else
        {
#endif
            switch (pInterfaceRecord->ifr_addr.sa_family)
            {
#ifdef HAVE_SOCKADDR_SA_LEN
            dwLen = DNS_MAX(sizeof(struct sockaddr), pInterfaceRecord->ifr_addr.sa_len);
#else
        
#ifdef AF_INET6
                case AF_INET6:
                
#ifdef __LWI_HP_UX__
                     dwLen = sizeof(short) + sizeof(short) + sizeof(uint32_t) + (sizeof(uint8_t) * 16);
#else
                     dwLen = sizeof(struct sockaddr_in6);
#endif
                
                     break;
#endif


                case AF_INET:           
                default:
                
                    dwLen = sizeof(struct sockaddr);
            
                    break;
            }
#endif /* HAVE_SOCKADDR_SA_LEN */
        
#ifdef AF_LINK
        }
#endif

        pIter += sizeof(pInterfaceRecord->ifr_name) + dwLen;
        
#ifdef LW_SKIP_ALIASED_INTERFACES
        // On Solaris, the name for an aliased interface contains
        // a colon.
        if ((pszIndex = strchr(pInterfaceRecord->ifr_name, ':')))
        {
            *pszIndex = '\0';
        }
#endif

        memset(szItfName, 0, sizeof(szItfName));
        memcpy(szItfName, pInterfaceRecord->ifr_name, IFNAMSIZ);
        
        LWDNS_LOG_VERBOSE("Considering network interface [%s]", szItfName);
        
        pSA = (struct sockaddr*)&pInterfaceRecord->ifr_addr;

        if (pSA->sa_family != AF_INET)
        {
           LWDNS_LOG_VERBOSE("Skipping network interface [%s] [family:%d] because it is not AF_INET family", szItfName, pSA->sa_family);
           continue;
        }
        
        interfaceRecordCopy = *pInterfaceRecord;
        
        if (ioctl(fd, SIOCGIFFLAGS, &interfaceRecordCopy) < 0)
        {
            dwError = errno;
            BAIL_ON_LWDNS_ERROR(dwError);
        }
        
        dwFlags = interfaceRecordCopy.ifr_flags;
        
        if (dwFlags & IFF_LOOPBACK)
        {
            LWDNS_LOG_VERBOSE("Skipping loopback network interface [%s]", szItfName);
            continue;
        }
        
        if (!(dwFlags & IFF_UP))
        {
            LWDNS_LOG_VERBOSE("Skipping in-active network interface [%s]", szItfName);
            continue;
        }

#ifdef LW_SKIP_ALIASED_INTERFACES
        if (DNSInterfaceIsInList(szItfName, pInterfaceList))
        {
            LWDNS_LOG_VERBOSE("Skipping aliased network interface [%s]",
                              IsNullOrEmptyString(szItfName) ? "" : szItfName);
            continue;
        }
#endif

        dwError = DNSAllocateMemory(
                        sizeof(LW_INTERFACE_INFO),
                        (PVOID*)&pInterfaceInfo);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        dwError = DNSAllocateMemory(
                        IF_NAMESIZE,
                        (PVOID*)&pInterfaceInfo->pszName);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        strncpy(pInterfaceInfo->pszName,
                pInterfaceRecord->ifr_name,
                IF_NAMESIZE-1);
        
        pInterfaceInfo->dwFlags = dwFlags;
        
        if (ioctl(fd, SIOCGIFADDR, &interfaceRecordCopy, sizeof(struct ifreq)) < 0)
        {
            dwError = errno;
            BAIL_ON_LWDNS_ERROR(dwError);
        }
        
        // From the above logic, we consider only
        // AF_INET addresses at this point.
        memcpy(&pInterfaceInfo->ipAddr,
               &pInterfaceRecord->ifr_addr,
               sizeof(struct sockaddr_in));

        dwError = DNSDLinkedListAppend(
                        &pInterfaceList,
                        pInterfaceInfo);
        BAIL_ON_LWDNS_ERROR(dwError);

        pszIpAddress = inet_ntoa(((struct sockaddr_in*)&pInterfaceInfo->ipAddr)->sin_addr);
        
        LWDNS_LOG_VERBOSE("Added network interface [Name:%s; Address:%s] to list",
                          (IsNullOrEmptyString(pInterfaceInfo->pszName) ? "" : pInterfaceInfo->pszName),
                          (IsNullOrEmptyString(pszIpAddress) ? "" : pszIpAddress));
        
        pInterfaceInfo = NULL;
    }
    
    *ppInterfaceList = pInterfaceList;
    
cleanup:

    if (fd >= 0)
    {
        close(fd);
    }
    
    if (pBuffer)
    {
        DNSFreeMemory(pBuffer);
    }

    return dwError;
    
error:

    if (pInterfaceInfo)
    {
        DNSFreeNetworkInterface(pInterfaceInfo);
    }
    
    if (pInterfaceList)
    {
        DNSFreeInterfaceLinkedList(pInterfaceList);
    }

    goto cleanup;
}

#endif

#ifdef LW_SKIP_ALIASED_INTERFACES
static
BOOLEAN
DNSInterfaceIsInList(
    PCSTR pszName,
    PDNSDLINKEDLIST pInterfaceList
    )
{
    BOOLEAN bResult = FALSE;
    PDNSDLINKEDLIST pIter = NULL;
    
    for (pIter = pInterfaceList; pIter; pIter = pIter->pNext)
    {
        PLW_INTERFACE_INFO pInterfaceInfo = NULL;
        
        pInterfaceInfo = (PLW_INTERFACE_INFO)pIter->pItem;
        
        if (!strcasecmp(pInterfaceInfo->pszName, pszName))
        {
            bResult = TRUE;
            break;
        }
    }
    
    return bResult;
}
#endif

static
DWORD
DNSBuildInterfaceArray(
    PDNSDLINKEDLIST     pInterfaceList,
    PLW_INTERFACE_INFO* ppInterfaceInfoArray,
    PDWORD              pdwNumInterfaces
    )
{
    DWORD dwError = 0;
    DWORD dwNumInterfaces = 0;
    PLW_INTERFACE_INFO pInterfaceInfoArray = NULL;
    PDNSDLINKEDLIST pIter = NULL;
    DWORD i = 0;
    
    for (pIter = pInterfaceList; pIter; pIter = pIter->pNext)
    {
        dwNumInterfaces++;
    }
    
    if (!dwNumInterfaces)
    {
        goto done;
    }
    
    dwError = DNSAllocateMemory(
                sizeof(LW_INTERFACE_INFO) * dwNumInterfaces,
                (PVOID*)&pInterfaceInfoArray);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    for (pIter = pInterfaceList; pIter; pIter = pIter->pNext, i++)
    {
        PLW_INTERFACE_INFO pSrcInfo = NULL;
        PLW_INTERFACE_INFO pDstInfo = NULL;
        
        pSrcInfo = (PLW_INTERFACE_INFO)pIter->pItem;
        pDstInfo = &pInterfaceInfoArray[i];
        
        memcpy(pDstInfo, pSrcInfo, sizeof(LW_INTERFACE_INFO));
        memset(pSrcInfo, 0, sizeof(LW_INTERFACE_INFO));
    }
    
done:

    *ppInterfaceInfoArray = pInterfaceInfoArray;
    *pdwNumInterfaces = dwNumInterfaces;
    
cleanup:

    return dwError;
    
error:

    if (pInterfaceInfoArray)
    {
        DNSFreeNetworkInterfaces(
                pInterfaceInfoArray,
                dwNumInterfaces);
    }
    
    *ppInterfaceInfoArray = NULL;
    *pdwNumInterfaces = 0;

    goto cleanup;
}

static
VOID
DNSFreeInterfaceLinkedList(
    PDNSDLINKEDLIST pInterfaceList
    )
{
    DNSDLinkedListForEach(
            pInterfaceList,
            &DNSFreeInterfaceInList,
            NULL);
    DNSDLinkedListFree(pInterfaceList);
}

static
VOID
DNSFreeInterfaceInList(
    PVOID pItem,
    PVOID pUserData
    )
{
    if (pItem)
    {
        DNSFreeNetworkInterface((PLW_INTERFACE_INFO)pItem);
    }
}

VOID
DNSFreeNetworkInterfaces(
    PLW_INTERFACE_INFO pInterfaceInfoArray,
    DWORD              dwNumInterfaces
    )
{
    DWORD iItf = 0;
    
    for (; iItf < dwNumInterfaces; iItf++)
    {
        PLW_INTERFACE_INFO pInterfaceInfo = NULL;
        
        pInterfaceInfo = &pInterfaceInfoArray[iItf];
        
        DNSFreeNetworkInterfaceContents(pInterfaceInfo);
    }
    
    DNSFreeMemory(pInterfaceInfoArray);
}

VOID
DNSFreeNetworkInterface(
    PLW_INTERFACE_INFO pInterfaceInfo
    )
{
    DNSFreeNetworkInterfaceContents(pInterfaceInfo);
    DNSFreeMemory(pInterfaceInfo);
}

VOID
DNSFreeNetworkInterfaceContents(
    PLW_INTERFACE_INFO pInterfaceInfo
    )
{
    LWDNS_SAFE_FREE_STRING(pInterfaceInfo->pszName);
}
