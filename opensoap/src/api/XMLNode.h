/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: XMLNode.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_IMPL_XMLNode_H
#define OpenSOAP_IMPL_XMLNode_H

#include "DLinkList.h"

#include <OpenSOAP/String.h>
#include <OpenSOAP/XMLNamespace.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct tagOpenSOAPXMLNode OpenSOAPXMLNode;
    typedef OpenSOAPXMLNode *OpenSOAPXMLNodePtr;
    
    struct tagOpenSOAPXMLNode {
        OpenSOAPObject          super;
        
        OpenSOAPXMLNodePtr      prev;
        OpenSOAPXMLNodePtr      next;
        OpenSOAPXMLNodePtr      parent;
        OpenSOAPXMLNodePtr      children;
        OpenSOAPStringPtr       name;
        OpenSOAPStringPtr       value;
        OpenSOAPXMLNamespacePtr thisNamespace;
    };

    int
    OpenSOAPXMLNodeInitialize(/* [out] */ OpenSOAPXMLNodePtr node,
                              /* [in]  */ OpenSOAPObjectFreeFunc free_func);

    int
    OpenSOAPXMLNodeReleaseMembers(/* [out] */ OpenSOAPXMLNodePtr node);

    int
    OpenSOAPXMLNodeRelease(/* [out] */ OpenSOAPXMLNodePtr node);

	/* Name Getter/Setter */
    int
    OpenSOAPXMLNodeGetName(/* [in]  */ OpenSOAPXMLNodePtr node,
                           /* [out] */ OpenSOAPStringPtr *name);

    int
    OpenSOAPXMLNodeSetName(/* [out] */ OpenSOAPXMLNodePtr node,
                           /* [in]  */ OpenSOAPStringPtr name,
                           /* [in]  */ int is_dup);

	int
	OpenSOAPXMLNodeIsSameNameString(/* [in]  */ OpenSOAPXMLNodePtr node,
									/* [in]  */ OpenSOAPStringPtr name,
									/* [out] */ int *isSame);
	
	/* */
	int
	OpenSOAPXMLNodeGetRoot(/* [in]  */ OpenSOAPXMLNodePtr node,
						   /* [out] */ OpenSOAPXMLNodePtr *root);
	
	/* Value Getter */
	int
	OpenSOAPXMLNodeGetValueString(/* [in]  */ OpenSOAPXMLNodePtr node,
								  /* [in]  */ OpenSOAPStringPtr typeName,
								  /* [out] */ void *value);
	int
	OpenSOAPXMLNodeGetValueMB(/* [in]  */ OpenSOAPXMLNodePtr node,
							  /* [in]  */ const char *typeName,
							  /* [out] */ void *value);	
	
	int
	OpenSOAPXMLNodeGetValueWC(/* [in]  */ OpenSOAPXMLNodePtr node,
							  /* [in]  */ const wchar_t *typeName,
							  /* [out] */ void *value);	
	
	/* Value Setter */
	int
	OpenSOAPXMLNodeSetValueAsString(/* [out] */ OpenSOAPXMLNodePtr node,
									/* [in]  */ OpenSOAPStringPtr value,
									/* [in]  */ int isDup);
	
	int
	OpenSOAPXMLNodeSetValueAsStringMB(/* [out] */ OpenSOAPXMLNodePtr node,
									  /* [in]  */ const char *value);
	
	int
	OpenSOAPXMLNodeSetValueAsStringWC(/* [out] */ OpenSOAPXMLNodePtr node,
									  /* [in]  */ const wchar_t *value);
	
	int
	OpenSOAPXMLNodeSetValueString(/* [out] */ OpenSOAPXMLNodePtr node,
								  /* [in]  */ OpenSOAPStringPtr typeName,
								  /* [in]  */ void *value);

	int
	OpenSOAPXMLNodeSetValueMB(/* [out] */ OpenSOAPXMLNodePtr node,
							  /* [in]  */ const char *typeName,
							  /* [in]  */ void *value);

	int
	OpenSOAPXMLNodeSetValueWC(/* [out] */ OpenSOAPXMLNodePtr node,
							  /* [in]  */ const wchar_t *typeName,
							  /* [in]  */ void *value);

	/* */
    int
    OpenSOAPXMLNodeInsertNext(/* [in, out] */ OpenSOAPXMLNodePtr node,
                              /* [in, out] */ OpenSOAPXMLNodePtr next);

	int
	OpenSOAPXMLNodeRemoveValue(/* [out] */ OpenSOAPXMLNodePtr node,
							   /* [in]  */ int isRelease,
							   /* [out] */ OpenSOAPStringPtr *value);
	
    int
    OpenSOAPXMLNodeRemove(/* [in, out] */ OpenSOAPXMLNodePtr node);

	/* temporary function */
    int
	OpenSOAPXMLNodeListPushBack(OpenSOAPXMLNodePtr *list,
								OpenSOAPXMLNodePtr node);

	/* */
    int
    OpenSOAPXMLNodeListRelease(/* [out] */ OpenSOAPXMLNodePtr node);

	int
	OpenSOAPXMLNodeListReleaseItem(/* [out] */ OpenSOAPXMLNodePtr *nodeList,
								   /* [in]  */ OpenSOAPXMLNodePtr node);


	int
	OpenSOAPXMLNodeListSearchNamedNodeString(/* [in]  */ OpenSOAPXMLNodePtr
											 *nodeList,
											 /* [in]  */
											 OpenSOAPStringPtr name,
											 /* [in]  */
											 OpenSOAPXMLNodePtr *node);

	int
	OpenSOAPXMLNodeListGetNextNode(/* [in]  */ OpenSOAPXMLNodePtr *nodeList,
								   /* [in]  */ OpenSOAPXMLNodePtr node,
								   /* [out] */ OpenSOAPXMLNodePtr *nextNode);
	
	int
	OpenSOAPXMLNodeListRemoveNamedNodeString(/* [in, out] */ OpenSOAPXMLNodePtr
											 *nodeList,
											 /* [in]  */
											 OpenSOAPStringPtr name,
											 /* [in]  */ int isRelease,
											 /* [out] */
											 OpenSOAPStringPtr *value);
	
	int
	OpenSOAPXMLNodeListGetNamedNodeString(/* [in, out] */ OpenSOAPXMLNodePtr
										  *nodeList,
										  /* [in]  */ OpenSOAPStringPtr name,
										  /* [in]  */ int isDup,
										  /* [in]  */
										  int (*creater)(OpenSOAPXMLNodePtr *),
										  /* [out] */
										  OpenSOAPXMLNodePtr *node);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_XMLNode_H */
