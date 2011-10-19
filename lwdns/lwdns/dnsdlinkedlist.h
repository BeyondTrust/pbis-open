#ifndef __DNSDLINKEDLIST_H__
#define __DNSDLINKEDLIST_H__

typedef struct __DNSDLINKEDLIST
{
    PVOID pItem;
    
    struct __DNSDLINKEDLIST * pNext;
    
    struct __DNSDLINKEDLIST * pPrev;
    
} DNSDLINKEDLIST, *PDNSDLINKEDLIST;

typedef VOID (*PFN_DNSDLINKEDLIST_FUNC)(
                    PVOID pData, 
                    PVOID pUserData
                    );

DWORD
DNSDLinkedListPrepend(
    PDNSDLINKEDLIST* ppList,
    PVOID        pItem
    );

DWORD
DNSDLinkedListAppend(
    PDNSDLINKEDLIST* ppList,
    PVOID        pItem
    );

BOOLEAN
DNSDLinkedListDelete(
    PDNSDLINKEDLIST* ppList,
    PVOID        pItem
    );

VOID
DNSDLinkedListForEach(
    PDNSDLINKEDLIST         pList,
    PFN_DNSDLINKEDLIST_FUNC pFunc,
    PVOID                   pUserData
    );

VOID
DNSDLinkedListFree(
    PDNSDLINKEDLIST pList
    );

#endif /* __DLINKEDLIST_H__ */

