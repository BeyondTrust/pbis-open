/*-----------------------------------------------------------------------------
 * $RCSfile: XMLNode.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: XMLNode.c,v 1.23 2002/11/06 06:41:37 bandou Exp $";
#endif  /* _DEBUG */

#include <OpenSOAP/Serializer.h>

#include "XMLNode.h"

/*
=begin
= OpenSOAP XMLNode base
=end
 */


extern
int
OpenSOAPXMLNodeRelease(/* [out] */ OpenSOAPXMLNodePtr node) {
    int ret = OpenSOAPObjectRelease((OpenSOAPObjectPtr)node);

    return ret;
}

extern
int
OpenSOAPXMLNodeReleaseMembers(/* [out] */ OpenSOAPXMLNodePtr node) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (node) {
        /* children release */
        OpenSOAPXMLNodeListRelease(node->children);
        /* */
        OpenSOAPStringRelease(node->name);
        OpenSOAPStringRelease(node->value);

		ret = OPENSOAP_NO_ERROR;
    }
    
    return ret;
}

static
int
OpenSOAPXMLNodeFree(/* [out] */ OpenSOAPObjectPtr obj) {
    int ret = OpenSOAPXMLNodeReleaseMembers((OpenSOAPXMLNodePtr)obj);

    if (OPENSOAP_SUCCEEDED(ret)) {
        free(obj);
    }

    return ret;
}
    
/*
=begin
--- function#OpenSOAPXMLNodeInitialize(node, freeFunc)
    Initialize Of Node

    :Parameters
      :[out] OpenSOAPXMLNodePtr ((|node|))
        XML Node Pointer.
      :[in]  OpenSOAPObjectFreeFunc ((|freeFunc|))
        OpenSOAPObject free function.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OpenSOAPXMLNodeInitialize(/* [out] */ OpenSOAPXMLNodePtr node,
                          /* [in]  */ OpenSOAPObjectFreeFunc freeFunc) {

    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (node) {
        node->prev = NULL;
        node->next = NULL;
        node->parent = NULL;
        node->children = NULL;

        node->name = NULL;
        node->value = NULL;
        node->thisNamespace = NULL;

        ret = OpenSOAPObjectInitialize((OpenSOAPObjectPtr)node,
                                       freeFunc ? freeFunc
                                       : OpenSOAPXMLNodeFree);
    }
    
    return ret;
}
      
/*
=begin
--- function#OpenSOAPXMLNodeSetName(node, name, isDup)
    Setting Name of Node

    :Parameters
      :OpenSOAPXMLNodePtr [in, out] ((|node|))
        XML Node Pointer.
      :OpenSOAPStringPtr [in] ((|name|))
        Setting Node Name
      :int [in] ((|isDup|))
        duplicate flag.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OpenSOAPXMLNodeSetName(/* [out] */ OpenSOAPXMLNodePtr node,
                       /* [in]  */ OpenSOAPStringPtr name,
                       /* [in]  */ int isDup) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (node) {
        if (isDup) {
            ret = OpenSOAPStringDuplicate(name, &node->name);
        }
        else {
            if (name) {
                OpenSOAPStringRelease(node->name);
                node->name = name;
                ret = OPENSOAP_NO_ERROR;
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeGetName(node, Name)
    Getting of Node Name

    :Parameters
      :[in]  OpenSOAPXMLNodePtr ((|node|))
        XML Node Pointer.
      :[out] OpenSOAPStringPtr ((|name|))
        Result Name of Node.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OpenSOAPXMLNodeGetName(/* [in]  */ OpenSOAPXMLNodePtr node,
                       /* [out] */ OpenSOAPStringPtr *name) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (node) {
        ret = OpenSOAPStringDuplicate(node->name, name);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeGetName(node, name, isSame)
    Is same node name.

    :Parameters
      :[in]  OpenSOAPXMLNodePtr ((|node|))
        XML node Pointer.
      :[in]  OpenSOAPStringPtr ((|name|))
        Compare name.
	  :[out] int * ((|isSame|))
	    Judge result stored buffer.
    :Return Value
      :int
	    Error code.
=end
 */
extern
int
OpenSOAPXMLNodeIsSameNameString(/* [in]  */ OpenSOAPXMLNodePtr node,
								/* [in]  */ OpenSOAPStringPtr name,
								/* [out] */ int *isSame) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (node && isSame) {
		OpenSOAPStringPtr nodeName = node->name;
		*isSame = 1;
		ret = OPENSOAP_NO_ERROR;
		if (name || nodeName) {
			*isSame = 0;
			if (name && nodeName) {
				int cmpResult = 0;
				ret = OpenSOAPStringCompare(name,
											nodeName,
											&cmpResult);
				*isSame = (cmpResult == 0);
			}
		}
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeGetRoot(node, root)
    Get root node.

    :Parameters
      :[in]  OpenSOAPXMLNodePtr ((|node|))
        XML Node Pointer.
      :[out] OpenSOAPXMLNodePtr * ((|root|))
        root node Pointer.
    :Return Value
      :int
	    Error code.
=end
 */
extern
int
OpenSOAPXMLNodeGetRoot(/* [in]  */ OpenSOAPXMLNodePtr node,
					   /* [out] */ OpenSOAPXMLNodePtr *root) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (node && root) {
		ret = OPENSOAP_NO_ERROR;
		for (; node->parent; node = node->parent) {
		}
		*root = node;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeGetValueString(node, typeName, value)
    Get Value of XML Node

    :Parameters
      :[in]  OpenSOAPXMLNodePtr ((|node|))
        OpenSOAP XML Element.
      :[in]  OpenSOAPString ((|typeName|))
        Type Name
      :[out] void * ((|value|))
        Setting Buffer Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
/* OPENSOAP_API */
OpenSOAPXMLNodeGetValueString(/* [in]  */ OpenSOAPXMLNodePtr node,
							  /* [in]  */ OpenSOAPStringPtr typeName,
							  /* [out] */ void *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	
	if (node && typeName && value) {
		OpenSOAPDeserializerFunc deserializer = NULL;
		ret = OpenSOAPGetDeserializer(typeName, &deserializer);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = deserializer(node->value, value);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeGetValueMB(node, typeName, value)
    Get Value of XML Node

    :Parameters
      :[in]  OpenSOAPXMLNodePtr ((|node|))
        OpenSOAP XML Element.
      :[in]  const char * ((|typeName|))
        Type Name
      :[out] void * ((|value|))
        Setting Buffer Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
/* OPENSOAP_API */
OpenSOAPXMLNodeGetValueMB(/* [in]  */ OpenSOAPXMLNodePtr node,
						  /* [in]  */ const char *typeName,
						  /* [out] */ void *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	
	if (node && typeName && *typeName && value) {
		OpenSOAPStringPtr typeNameStr = NULL;
		ret = OpenSOAPStringCreateWithMB(typeName, &typeNameStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLNodeGetValueString(node, typeNameStr, value);
			
			OpenSOAPStringRelease(typeNameStr);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeGetValueMB(node, typeName, value)
    Get Value of XML Node

    :Parameters
      :[in]  OpenSOAPXMLNodePtr ((|node|))
        OpenSOAP XML Element.
      :[in]  const wchar_t * ((|typeName|))
        Type Name
      :[out] void * ((|value|))
        Setting Buffer Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
/* OPENSOAP_API */
OpenSOAPXMLNodeGetValueWC(/* [in]  */ OpenSOAPXMLNodePtr node,
						  /* [in]  */ const wchar_t *typeName,
						  /* [out] */ void *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	
	if (node && typeName && *typeName && value) {
		OpenSOAPStringPtr typeNameStr = NULL;
		ret = OpenSOAPStringCreateWithWC(typeName, &typeNameStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLNodeGetValueString(node, typeNameStr, value);
			
			OpenSOAPStringRelease(typeNameStr);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeSetValueAsString(node, value, isDup)
    Node value set as string

    :Parameters
      :[out] OpenSOAPXMLNodePtr ((|node|))
        XML Node Pointer.
      :[in]  OpenSOAPStringPtr ((|value|))
        Setting Node Name
      :[in]  int ((|isDup|))
        duplicate flag. If non zero, then value is duplicate.
    :Return Value
      :int
	    error code.
=end
 */
int
OpenSOAPXMLNodeSetValueAsString(/* [out] */ OpenSOAPXMLNodePtr node,
								/* [in]  */ OpenSOAPStringPtr value,
								/* [in]  */ int isDup) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (node) {
        if (isDup) {
            ret = OpenSOAPStringDuplicate(value, &node->value);
        }
        else {
            if (value) {
                OpenSOAPStringRelease(node->value);
                node->value = value;
                ret = OPENSOAP_NO_ERROR;
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeSetValueAsStringMB(node, value)
    Node value set as string

    :Parameters
      :[out] OpenSOAPXMLNodePtr ((|node|))
        XML Node Pointer.
      :[in]  OpenSOAPStringPtr ((|value|))
        value as string
    :Return Value
      :int
	    error code.
=end
 */
int
OpenSOAPXMLNodeSetValueAsStringMB(/* [out] */ OpenSOAPXMLNodePtr node,
								  /* [in]  */ const char *value) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (node) {
		OpenSOAPStringPtr valueStr = NULL;
		ret = OpenSOAPStringCreateWithMB(value, &valueStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLNodeSetValueAsString(node, valueStr, 0);
			
			if (OPENSOAP_FAILED(ret)) {
				OpenSOAPStringRelease(valueStr);
			}
		}
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeSetValueAsStringWC(node, value)
    Node value set as string

    :Parameters
      :[out] OpenSOAPXMLNodePtr ((|node|))
        XML Node Pointer.
      :[in]  const wchar_t * ((|value|))
        value as string
    :Return Value
      :int
	    error code.
=end
 */
int
OpenSOAPXMLNodeSetValueAsStringWC(/* [out] */ OpenSOAPXMLNodePtr node,
								  /* [in]  */ const wchar_t *value) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (node) {
		OpenSOAPStringPtr valueStr = NULL;
		ret = OpenSOAPStringCreateWithWC(value, &valueStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLNodeSetValueAsString(node, valueStr, 0);
			
			if (OPENSOAP_FAILED(ret)) {
				OpenSOAPStringRelease(valueStr);
			}
		}
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeSetValueString(node, typeName, value)
    Set Value of XML Node using typename.

    :Parameters
      :[out] OpenSOAPXMLNodePtr ((|node|))
        OpenSOAP XML Element.
      :[in]  OpenSOAPStringPtr ((|typeName|))
        Type Name
      :[in]  void * ((|value|))
        Setting Buffer Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
/* OPENSOAP_API */
OpenSOAPXMLNodeSetValueString(/* [out] */ OpenSOAPXMLNodePtr node,
							  /* [in]  */ OpenSOAPStringPtr typeName,
							  /* [in]  */ void *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (node && typeName && value) {
		OpenSOAPSerializerFunc serializer = NULL;
		ret = OpenSOAPGetSerializer(typeName, &serializer);
		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPStringPtr *nodeValue = &(node->value);
			if (!*nodeValue) {
				ret = OpenSOAPStringCreate(nodeValue);
			}
			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = serializer(value, *nodeValue);
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeSetValueMB(node, typeName, value)
    Set Value of XML Node using typename.

    :Parameters
      :[out] OpenSOAPXMLNodePtr ((|node|))
        OpenSOAP XML Element.
      :[in]  const char * ((|typeName|))
        Type Name
      :[in]  void * ((|value|))
        Setting Buffer Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
/* OPENSOAP_API */
OpenSOAPXMLNodeSetValueMB(/* [out] */ OpenSOAPXMLNodePtr node,
						  /* [in]  */ const char *typeName,
						  /* [in]  */ void *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (node && typeName && *typeName && value) {
		OpenSOAPStringPtr typeNameStr = NULL;
		ret = OpenSOAPStringCreateWithMB(typeName, &typeNameStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLNodeSetValueString(node, typeNameStr, value);

			OpenSOAPStringRelease(typeNameStr);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeSetValueWC(node, typeName, value)
    Set Value of XML Node using typename.

    :Parameters
      :[out] OpenSOAPXMLNodePtr ((|node|))
        OpenSOAP XML Element.
      :[in]  const wchar_t * ((|typeName|))
        Type Name
      :[in]  void * ((|value|))
        Setting Buffer Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
/* OPENSOAP_API */
OpenSOAPXMLNodeSetValueWC(/* [out] */ OpenSOAPXMLNodePtr node,
						  /* [in]  */ const wchar_t *typeName,
						  /* [in]  */ void *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (node && typeName && *typeName && value) {
		OpenSOAPStringPtr typeNameStr = NULL;
		ret = OpenSOAPStringCreateWithWC(typeName, &typeNameStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLNodeSetValueString(node, typeNameStr, value);

			OpenSOAPStringRelease(typeNameStr);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeRemoveValue(node, isRelease, value)
    Node value remove.

    :Parameters
      :[out] OpenSOAPXMLNodePtr ((|node|))
        XML Node Pointer.
      :[in]  int ((|isRelease|))
        value release flag. if non zero, then call OpenSOAPStringRelease and
		*value set to NULL.
      :[out] OpenSOAPStringPtr * ((|value|))
        value buffer.
    :Return Value
      :int
	    error code.
=end
 */
int
OpenSOAPXMLNodeRemoveValue(/* [out] */ OpenSOAPXMLNodePtr node,
						   /* [in]  */ int isRelease,
						   /* [out] */ OpenSOAPStringPtr *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (node) {
		OpenSOAPStringPtr nodeValue = node->value;
		if (isRelease) {
			OpenSOAPStringRelease(nodeValue);
			nodeValue = NULL;
		}
		if (value) {
			*value = nodeValue;
		}
		node->value = NULL;
		
		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeInsertNext(node, next)
    Add Next Node to Current Node 

    :Parameters
      :[in, out] OpenSOAPXMLNodePtr ((|node|))
        XML Node.
      :[in, out] OpenSOAPXMLNodePtr ((|next|))
        XML Next Node.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
int
OpenSOAPXMLNodeInsertNext(/* [in, out] */ OpenSOAPXMLNodePtr node,
                          /* [in, out] */ OpenSOAPXMLNodePtr next) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (node) {
        OpenSOAPXMLNodePtr next_tmp = node;
        while (next_tmp->next) {
            next_tmp = next_tmp->next;
        }

        next_tmp->next = next;
        next->prev = next_tmp;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLNodeRemove(node)
    Remove Node

    :Parameters
      :[in, out] OpenSOAPXMLNodePtr ((|node|))
        XML Node
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OpenSOAPXMLNodeRemove(/* [in, out] */ OpenSOAPXMLNodePtr node) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (node) {
		OpenSOAPXMLNodePtr prev = node->prev;
		OpenSOAPXMLNodePtr next = node->next;

		if (prev) {
			prev->next = next;
		}
		if (next) {
			next->prev = prev;
		}
		ret = OpenSOAPXMLNodeRelease(node);
	}
	
    return ret;
}

/*
 */
int
OpenSOAPXMLNodeListPushBack(OpenSOAPXMLNodePtr *list,
							OpenSOAPXMLNodePtr node) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (list && node) {
		OpenSOAPXMLNodePtr *next = list;
		OpenSOAPXMLNodePtr itr = *next;
		/* set next variable pointer */
		if (*next) {
			next = &(*next)->next;
		}
		/* search list's end */
		for (; *next; itr = *next, next = &itr->next) {
		}
		/* insert list's end */
		*next = node;
		node->prev = itr;

		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

static
int
OpenSOAPXMLNodeUnlinkAndRelease(/* [in, out] */ OpenSOAPXMLNodePtr node,
								/* [in, out] */ OpenSOAPXMLNodePtr *nodeList) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (node) {
		OpenSOAPXMLNodePtr prev = node->prev;
		OpenSOAPXMLNodePtr next = node->next;

		ret = OpenSOAPXMLNodeRelease(node);
		if (OPENSOAP_SUCCEEDED(ret)) {
			/* list first check */
			if (nodeList && *nodeList == node) {
				/* set to node's next */
				*nodeList = next;
			}
			/* unlink */
			if (prev) {
				prev->next = next;
			}
			if (next) {
				next->prev = prev;
			}
		}
	}

	return ret;
}

extern
int
OpenSOAPXMLNodeListRelease(/* [out] */ OpenSOAPXMLNodePtr node) {
    int	ret = OPENSOAP_NO_ERROR;

    while (node && OPENSOAP_SUCCEEDED(ret)) {
        OpenSOAPXMLNodePtr next = node->next;
            
        ret = OpenSOAPXMLNodeRelease(node);

        node = next;
    }

    return ret;
}

extern
int
OpenSOAPXMLNodeListReleaseItem(/* [out] */ OpenSOAPXMLNodePtr *nodeList,
							   /* [in]  */ OpenSOAPXMLNodePtr node) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (nodeList && node) {
		OpenSOAPXMLNodePtr itr  = *nodeList;
		for (; itr && itr != node; itr = itr->next) {
		}
		if (itr == node) {
			/* found then remove */
			ret = OpenSOAPXMLNodeUnlinkAndRelease(node, nodeList);
		}
	}

	return ret;
}

/*
 */
extern
int
OpenSOAPXMLNodeListSearchNamedNodeString(/* [in]  */ OpenSOAPXMLNodePtr
										 *nodeList,
										 /* [in]  */ OpenSOAPStringPtr name,
										 /* [in]  */
										 OpenSOAPXMLNodePtr *node) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (nodeList && node) {
		OpenSOAPXMLNodePtr itr = *nodeList;
		ret = OPENSOAP_NO_ERROR;
		for (; itr; itr = itr->next) {
			int isSame = 0;
			ret = OpenSOAPXMLNodeIsSameNameString(itr,
												  name,
												  &isSame);
			if (OPENSOAP_FAILED(ret) || isSame) {
				break;
			}
		}
		/* found or error node */
		*node = itr;
	}

	return ret;
}

/*
 */
extern
int
OpenSOAPXMLNodeListGetNextNode(/* [in]  */ OpenSOAPXMLNodePtr *nodeList,
							   /* [in]  */ OpenSOAPXMLNodePtr node,
							   /* [out] */ OpenSOAPXMLNodePtr *nextNode) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (nodeList && nextNode) {
		if (!node) {
			/* list first */
			*nextNode = *nodeList;
			ret = OPENSOAP_NO_ERROR;
		}
		else {
			OpenSOAPXMLNodePtr itr = *nodeList;
			/* search match node */
			for (; itr && itr != node; itr = itr->next) {
			}
			if (itr) {
				/* include list */
				*nextNode = itr->next;
				ret = OPENSOAP_NO_ERROR;
			}
		}
	}

	return ret;
}

/*
 */
extern
int
OpenSOAPXMLNodeListRemoveNamedNodeString(/* [in, out] */ OpenSOAPXMLNodePtr
										 *nodeList,
										 /* [in]  */ OpenSOAPStringPtr name,
										 /* [in]  */ int isRelease,
										 /* [out] */
										 OpenSOAPStringPtr *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (nodeList) {
		OpenSOAPXMLNodePtr nodeFirst = *nodeList;
		OpenSOAPXMLNodePtr node = NULL;
		int ret = OpenSOAPXMLNodeListSearchNamedNodeString(nodeList,
														   name,
														   &node);
		if (OPENSOAP_SUCCEEDED(ret) && node) {
			ret = OpenSOAPXMLNodeRemoveValue(node,
											 isRelease,
											 value);
			if (OPENSOAP_SUCCEEDED(ret)) {
				OpenSOAPXMLNodePtr next = node->next;
				ret = OpenSOAPXMLNodeRemove(node);
				if (OPENSOAP_SUCCEEDED(ret) && nodeFirst == node) {
					*nodeList = next;
				}
				
			}
		}
	}
	
	return ret;
}

/*
 */
extern
int
OpenSOAPXMLNodeListGetNamedNodeString(/* [in, out] */ OpenSOAPXMLNodePtr
									  *nodeList,
									  /* [in]  */ OpenSOAPStringPtr name,
									  /* [in]  */ int isDup,
									  /* [in]  */
									  int (*creater)(OpenSOAPXMLNodePtr *),
									  /* [out] */ OpenSOAPXMLNodePtr *node) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (nodeList && name && creater && node) {
		OpenSOAPXMLNodePtr *next = nodeList;
		OpenSOAPXMLNodePtr last = NULL;
		OpenSOAPXMLNodePtr itr = *next;
		for (ret = OPENSOAP_NO_ERROR; itr; itr = itr->next) {
			int cmpResult = 0;
			ret = OpenSOAPStringCompare(name,
										itr->name,
										&cmpResult);
			if (OPENSOAP_FAILED(ret) || cmpResult == 0) {
				break;
			}
			/* last */
			if (!itr->next) {
				last = itr;
				next = &last->next;
			}
		}
		if (OPENSOAP_SUCCEEDED(ret)) {
			if (itr) {
				/* found */
				*node = itr;
			}
			else {
				/* not found */
				ret = creater(node);
				if (OPENSOAP_SUCCEEDED(ret)) {
					ret = OpenSOAPXMLNodeSetName(*node,
												 name,
												 isDup);
					if (OPENSOAP_SUCCEEDED(ret)) {
						/* link */
						*next = *node;
						(*node)->prev = last;
					}
					if (OPENSOAP_FAILED(ret)) {
						OpenSOAPXMLNodeRelease(*node);
						*node = NULL;
					}
				}
			}
		}
	}

	return ret;
}
