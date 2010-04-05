#ifndef __GPCSE_H__
#define __GPCSE_H__

typedef struct __LWIGPITEM {
    VOID* xmlNode;
    struct __LWIGPITEM *pNext;
} GPOLWIGPITEM, *PGPOLWIGPITEM;

typedef struct __LWIDATA {

    VOID* xmlDoc;

} GPOLWIDATA, *PGPOLWIDATA;

typedef struct __LWIDATALIST {

    PGPOLWIDATA pLwidata;
    struct __LWIDATALIST *pNext;

} GPOLWIDATALIST, *PGPOLWIDATALIST;

CENTERROR
GPOInitLwiData(
    PGPUSER pUser,
    DWORD dwPolicyType,
    PGPOLWIDATA *ppLwidata,
    PSTR pszgGpSysVolPath,
    PSTR pszDestFolderRootPath,
    PSTR pszgCseIdentifier );

CENTERROR
GPOGetGPItem(
    DWORD dwPolicyType,
    PGPOLWIDATA pLwiadata,
    PSTR pszGPItemGUID,
    PGPOLWIGPITEM *ppGPItem );

CENTERROR
GPOCopyGPItem(
    PGPOLWIGPITEM pSrcItem,
    PGPOLWIGPITEM *ppGPItem,
    BOOLEAN bDeep );

CENTERROR
GPOCalculateRSOP(
    PGPOLWIGPITEM pOldItem,
    PGPOLWIGPITEM pNewItem );

CENTERROR
GPOMergeGPItems(
    PGPOLWIGPITEM pRSOPItem,
    PGPOLWIGPITEM pFileItem );

/* Function equivalent to SelectNodes function that, given a node, uses XPath to return an xmlNodeSet
   (IIRC, that's the type in libxml2 that represents the final result of an XPath query) resulting from
   applying the query starting at the specified node, returning an empty set if no results are found. */
CENTERROR
GPOXmlSelectNodes(
    xmlNodePtr Node,
    const char* Query,
    xmlNodeSetPtr* Result );

/* SelectSingleNode function that does the same as SelectNodes, but just returns the first node
   resulting from the query, or NULL if no results are found. */
CENTERROR
GPOXmlSelectSingleNode(
    xmlNodePtr Node,
    const char* Query,
    xmlNodePtr* Result );

/* A [get] text function that, given an element node, returns the text/cadata enclosed. */
CENTERROR
GPOXmlGetInnerText(
    xmlNodePtr Node,
    char** Result );

CENTERROR
get_node_text(
    xmlNodePtr node,
    xmlChar **ppszText );

VOID
GPODestroyLwiData(
    PGPOLWIDATA pLwidata );

VOID
GPODestroyGPItem(
    PGPOLWIGPITEM pGPItem,
    BOOLEAN bDeep );

/* Given a LWIMachine/User Policy node, determines if the settings contained within are applicable
   to this operating system type, distro, and version.  If applicable, Result is set to TRUE,
   otherwise FALSE */
CENTERROR
GPOXmlVerifyPlatformApplicable(
    PGPUSER pUser,
    DWORD dwPolicyType,
    PGROUP_POLICY_OBJECT Policy,
    PSTR pszHomeDir,
    BOOLEAN* Result);

CENTERROR
GPACrackFileSysPath(
    PSTR pszFileSysPath,
    PSTR * ppszDomainName,
    PSTR * ppszSourcePath,
    PSTR * ppszPolicyIdentifier
    );

CENTERROR
GPOGetPolicyFiles(
    PGPUSER pUser,
    DWORD   dwPolicyType,
    PSTR    pszgGpSysVolPath,
    PSTR    pszgCseIdentifier,
    PSTR    pszDestFolderRootPath,
    PSTR    *ppszDestFolder,
    BOOLEAN *pbPolicyExists );

CENTERROR
GPOGetPolicyFile(
    PGPUSER pUser,
    DWORD   dwPolicyType,
    PSTR    pszgGpSysVolPath,
    PSTR    pszgCseIdentifier,
    PSTR    pszgFileName,
    PSTR    pszDestFolderRootPath,
    PSTR    *ppszDestFile,
    BOOLEAN *pbFileExists );

CENTERROR
GPARemovePolicy(
    PGROUP_POLICY_OBJECT pGPO,
    PSTR pszCSEType
    );

CENTERROR
GPADeleteFormerPolicies(
    PSTR pszCSEType
    );

BOOLEAN
GPAIsAbsolutePath(
    PCSTR pszPath
    );

BOOLEAN
GPAIsComment(
    PCSTR pszPath
    );

#endif /* __GPCSE_H__ */

