/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: DLinkList.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_IMPL_DLinkList_H
#define OpenSOAP_IMPL_DLinkList_H

#include "Object.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct tagOpenSOAPDLinkListItem OpenSOAPDLinkListItem;
    typedef OpenSOAPDLinkListItem *OpenSOAPDLinkListItemPtr;
    
    struct tagOpenSOAPDLinkListItem {
        OpenSOAPObject	super;
        
        OpenSOAPDLinkListItemPtr	prev;
        OpenSOAPDLinkListItemPtr	next;
    };

    typedef struct tagOpenSOAPDLinkList OpenSOAPDLinkList;
    typedef OpenSOAPDLinkList *OpenSOAPDLinkListPtr;
    
	/* */
	int
	OpenSOAPDLinkListItemInitialize(/* [out] */ OpenSOAPDLinkListItemPtr item,
									/* [in]  */
									OpenSOAPObjectFreeFunc freeFunc);

	int
	OpenSOAPDLinkListItemReleaseMembers(/* [out] */ OpenSOAPDLinkListItemPtr
										item);

	int
	OpenSOAPDLinkListItemRelease(/* [out] */ OpenSOAPDLinkListItemPtr item);
	
	int
	OpenSOAPDLinkListItemGetNext(/* [in]  */ OpenSOAPDLinkListItemPtr item,
								 /* [out] */ OpenSOAPDLinkListItemPtr *next);

	int
	OpenSOAPDLinkListItemGetPrev(/* [in]  */ OpenSOAPDLinkListItemPtr item,
								 /* [out] */ OpenSOAPDLinkListItemPtr *prev);

	int
	OpenSOAPDLinkListItemRemoveLink(/* [out] */ OpenSOAPDLinkListItemPtr item,
									/* [out] */
									OpenSOAPDLinkListItemPtr *oldPrev,
									/* [out] */
									OpenSOAPDLinkListItemPtr *oldNext);

	int
	OpenSOAPDLinkListItemInsertPrev(/* [in]  */ OpenSOAPDLinkListItemPtr item,
									/* [out] */ OpenSOAPDLinkListItemPtr pos,
									/* [out] */	OpenSOAPDLinkListItemPtr *prev);

	int
	OpenSOAPDLinkListItemInsertNext(/* [in]  */ OpenSOAPDLinkListItemPtr item,
									/* [out] */ OpenSOAPDLinkListItemPtr pos,
									/* [out] */	OpenSOAPDLinkListItemPtr *next);
	
	int
	OpenSOAPDLinkListItemIterateProc(/* [in, out] */ OpenSOAPDLinkListItemPtr
									 item,
									 /* [in]  */ int
									 (*iterateProc)(OpenSOAPDLinkListItemPtr,
													void *),
									 /* [in, out] */ void *opt);

	/* these functions are temporary */
	int
	OpenSOAPDLinkListItemPushBack(OpenSOAPDLinkListItemPtr *list,
								  OpenSOAPDLinkListItemPtr item);
	int
	OpenSOAPDLinkListItemPopBack(OpenSOAPDLinkListItemPtr *list,
								 OpenSOAPDLinkListItemPtr *item);
	
	/* */
	int
	OpenSOAPDLinkListCreate(/* [out] */ OpenSOAPDLinkListPtr *list);
	
	int
	OpenSOAPDLinkListRelease(/* [out] */ OpenSOAPDLinkListPtr list);

	int
	OpenSOAPDLinkListInsert(/* [out] */ OpenSOAPDLinkListPtr list,
							/* [in]  */ OpenSOAPDLinkListItemPtr pos,
							/* [in]  */ OpenSOAPDLinkListItemPtr item);

	int
	OpenSOAPDLinkListRemove(/* [out] */ OpenSOAPDLinkListPtr list,
							/* [in]  */ OpenSOAPDLinkListItemPtr pos);

	int
	OpenSOAPDLinkListClear(/* [out] */ OpenSOAPDLinkListPtr list);

	int
	OpenSOAPDLinkListIterateProc(/* [in, out] */ OpenSOAPDLinkListPtr list,
								 /* [in]  */ int
								 (*iterateProc)(OpenSOAPDLinkListItemPtr,
												void *),
								 /* [in, out] */ void *opt);
	
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_DLinkList_H */
