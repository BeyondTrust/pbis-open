/*-----------------------------------------------------------------------------
 * $RCSfile: DLinkList.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: DLinkList.c,v 1.4 2003/09/02 10:04:46 bandou Exp $";
#endif  /* _DEBUG */

#include "DLinkList.h"

#include <stdlib.h>

/*
=begin
= OpenSOAP DLinkListItem
=end
 */
int
OpenSOAPDLinkListItemInitialize(/* [out] */ OpenSOAPDLinkListItemPtr item,
								/* [in]  */ OpenSOAPObjectFreeFunc freeFunc) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (item) {
		item->prev = NULL;
		item->next = NULL;

		ret = OpenSOAPObjectInitialize((OpenSOAPObjectPtr)item,
									   freeFunc);

	}
	
	return ret;
}

int
OpenSOAPDLinkListItemReleaseMembers(/* [out] */ OpenSOAPDLinkListItemPtr
									item) {
	int ret = OpenSOAPDLinkListItemRemoveLink(item,
											  NULL,
											  NULL);

	return ret;
}

int
OpenSOAPDLinkListItemRelease(/* [out] */ OpenSOAPDLinkListItemPtr item) {
	int ret = OpenSOAPObjectRelease((OpenSOAPObjectPtr)item);

	return ret;
}

int
OpenSOAPDLinkListItemGetNext(/* [in]  */ OpenSOAPDLinkListItemPtr item,
							 /* [out] */ OpenSOAPDLinkListItemPtr *next) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (item && next) {
		*next = item->next;
		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

int
OpenSOAPDLinkListItemGetPrev(/* [in]  */ OpenSOAPDLinkListItemPtr item,
							 /* [out] */ OpenSOAPDLinkListItemPtr *prev) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (item && prev) {
		*prev = item->prev;
		ret = OPENSOAP_NO_ERROR;
	}
	
	return ret;
}

int
OpenSOAPDLinkListItemRemoveLink(/* [out] */ OpenSOAPDLinkListItemPtr item,
								/* [out] */
								OpenSOAPDLinkListItemPtr *oldPrev,
								/* [out] */
								OpenSOAPDLinkListItemPtr *oldNext) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (item) {
		OpenSOAPDLinkListItemPtr dummyPrev = NULL;
		OpenSOAPDLinkListItemPtr dummyNext = NULL;

		if (!oldPrev) {
			oldPrev = &dummyPrev;
		}
		if (!oldNext) {
			oldNext = &dummyNext;
		}
		*oldPrev = item->prev;
		*oldNext = item->next;

		if (*oldPrev) {
			(*oldPrev)->next = *oldNext;
		}
		if (*oldNext) {
			(*oldNext)->prev = *oldPrev;
		}

		item->prev = NULL;
		item->next = NULL;

		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

int
OpenSOAPDLinkListItemInsertPrev(/* [in]  */ OpenSOAPDLinkListItemPtr item,
								/* [out] */ OpenSOAPDLinkListItemPtr pos,
								/* [out] */	OpenSOAPDLinkListItemPtr *prev) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (item && !item->prev && !item->next && pos) {
		OpenSOAPDLinkListItemPtr dummyPrev = NULL;

		if (!prev) {
			prev = &dummyPrev;
		}
		*prev = pos->prev;

		if (*prev) {
			(*prev)->next = item;
			item->prev = *prev;
		}

		pos->prev = item;
		item->next = pos;
		
		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

int
OpenSOAPDLinkListItemInsertNext(/* [in]  */ OpenSOAPDLinkListItemPtr item,
								/* [out] */ OpenSOAPDLinkListItemPtr pos,
								/* [out] */	OpenSOAPDLinkListItemPtr *next) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (item && !item->prev && !item->next && pos) {
		OpenSOAPDLinkListItemPtr dummyNext = NULL;

		if (!next) {
			next = &dummyNext;
		}
		*next = pos->next;

		if (*next) {
			(*next)->prev = item;
			item->next = *next;
		}

		pos->next = item;
		item->prev = pos;
		
		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

int
OpenSOAPDLinkListItemIterateProc(/* [in, out] */ OpenSOAPDLinkListItemPtr
								 item,
								 /* [in]  */ int
								 (*iterateProc)(OpenSOAPDLinkListItemPtr,
												void *),
								 /* [in, out] */ void *opt) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (iterateProc) {
		ret = OPENSOAP_NO_ERROR;
		while (item) {
			OpenSOAPDLinkListItemPtr next = item->next;

			ret = iterateProc(item, opt);
			if (OPENSOAP_FAILED(ret)) {
				break;
			}

			item = next;
		}
	}

	return ret;
}

/* these functions are temporary */
int
OpenSOAPDLinkListItemPushBack(/* [in, out]  */ OpenSOAPDLinkListItemPtr *list,
							  /* [in]       */ OpenSOAPDLinkListItemPtr item) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (list && item) {
		OpenSOAPDLinkListItemPtr *next = list;
		OpenSOAPDLinkListItemPtr itr = *next;
		/* set next variable pointer */
		if (*next) {
			next = &(*next)->next;
		}
		/* search list's end */
		for (; *next; itr = *next, next = &itr->next) {
		}
		/* insert list's end */
		*next = item;
		item->prev = itr;

		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

int
OpenSOAPDLinkListItemPopBack(/* [in, out]  */ OpenSOAPDLinkListItemPtr *list,
							 /* [in, out]  */ OpenSOAPDLinkListItemPtr *item) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (list) {
		OpenSOAPDLinkListItemPtr itr = *list;
		/* search list's end */
		for (; itr && itr->next; itr = itr->next) {
		}
		/* last item set to item */
		if (item) {
			*item = itr;
		}
		/* link remove */
		if (itr && itr->prev) {
			itr->prev->next = NULL;
			itr->prev = NULL;
		}
		/* last item is list's first, then list set to NULL */
		if (itr == *list) {
			*list = NULL;
		}

		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

/* OpenSOAPDLinkList */

struct tagOpenSOAPDLinkList {
	OpenSOAPObject	super;

	OpenSOAPDLinkListItemPtr	first;
	OpenSOAPDLinkListItemPtr	last;
};

static
int
OpenSOAPDLinkListFree(OpenSOAPObjectPtr obj) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (obj) {
        OpenSOAPDLinkListPtr list = (OpenSOAPDLinkListPtr)obj;
		ret = OpenSOAPDLinkListClear(list);
        if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPObjectReleaseMembers(obj);
			if (OPENSOAP_SUCCEEDED(ret)) {
				free(obj);
			}
		}
    }
    
    return ret;
}

int
OpenSOAPDLinkListCreate(/* [out] */ OpenSOAPDLinkListPtr *list) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (list) {
		ret = OPENSOAP_MEM_BADALLOC;
		*list = malloc(sizeof(OpenSOAPDLinkList));
		if (*list) {
			OpenSOAPObjectPtr obj = (OpenSOAPObjectPtr)*list;
			ret = OpenSOAPObjectInitialize(obj,
										   OpenSOAPDLinkListFree);
			if (OPENSOAP_SUCCEEDED(ret)) {
				(*list)->first = NULL;
				(*list)->last  = NULL;
			}
		}

		if (OPENSOAP_FAILED(ret)) {
			free(*list);
			*list = NULL;
		}
	}

	return ret;
}

	
int
OpenSOAPDLinkListRelease(/* [out] */ OpenSOAPDLinkListPtr list) {
	int ret = OpenSOAPObjectRelease((OpenSOAPObjectPtr)list);

	return ret;
}

int
OpenSOAPDLinkListInsert(/* [out] */ OpenSOAPDLinkListPtr list,
						/* [in]  */ OpenSOAPDLinkListItemPtr pos,
						/* [in]  */ OpenSOAPDLinkListItemPtr item) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (item) {
		if (list) {
			if (list->first) {
				if (pos) {
					ret = OpenSOAPDLinkListItemInsertPrev(item, pos, NULL);
					if (OPENSOAP_SUCCEEDED(ret) && pos == list->first) {
						/* insert as first item */
						list->first = item;
					}
				}
				else {
					/* pos == NULL then insert as last item */
					ret = OpenSOAPDLinkListItemInsertNext(item,
														  list->last,
														  NULL);
					if (OPENSOAP_SUCCEEDED(ret)) {
						list->last = item;
					}
				}
			}
			else {
				/* list is empty */
				list->first = item;
				list->last  = item;
				ret = OPENSOAP_NO_ERROR;
			}
		}
		else if (pos) {
			ret = OpenSOAPDLinkListItemInsertPrev(item, pos, NULL);
		}
	}

	return ret;
}

int
OpenSOAPDLinkListRemove(/* [out] */ OpenSOAPDLinkListPtr list,
						/* [in]  */ OpenSOAPDLinkListItemPtr pos) {
	OpenSOAPDLinkListItemPtr prev = NULL;
	OpenSOAPDLinkListItemPtr next = NULL;
	int ret = OpenSOAPDLinkListItemRemoveLink(pos, &prev, &next);

	if (OPENSOAP_SUCCEEDED(ret) && list) {
		if (list->first == pos) {
			list->first = next;
		}
		else if (list->last == pos) {
			list->last = prev;
		}
	}

	return ret;
}

int
OpenSOAPDLinkListClear(/* [out] */ OpenSOAPDLinkListPtr list) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (list) {
		OpenSOAPDLinkListItemPtr i = list->first;
		while (i) {
			OpenSOAPDLinkListItemPtr n = NULL;

			ret = OpenSOAPDLinkListItemRemoveLink(i, NULL, &n);
			if (OPENSOAP_FAILED(ret)) {
				break;
			}
			ret = OpenSOAPDLinkListItemRelease(i);
			if (OPENSOAP_FAILED(ret)) {
				break;
			}

			i = n;
		}
		list->first = i;
		if (OPENSOAP_SUCCEEDED(ret)) {
			list->last = NULL;
		}
	}

	return ret;
}

int
OpenSOAPDLinkListIterateProc(/* [in, out] */ OpenSOAPDLinkListPtr list,
							 /* [in]  */ int
							 (*iterateProc)(OpenSOAPDLinkListItemPtr,
											void *),
							 /* [in, out] */ void *opt) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (list) {
		ret = OpenSOAPDLinkListItemIterateProc(list->first, iterateProc, opt);
	}

	return ret;
}
