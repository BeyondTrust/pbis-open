#include "includes.h"

#define LWISETTINGS_FILE "lwisettings.xml"
#define GPITEM_XPATH_EXPR "/LWIMachinePolicy/GPItem[@itemGUID=\"%s\"]"
#define GPITEM_USER_XPATH_EXPR "/LWIUserPolicy/GPItem[@itemGUID=\"%s\"]"
#define GPITEM_TAG_SETTING "setting"
#define GPITEM_ATTR_NAME "name"


CENTERROR ParseLwisettings( 
    char* szfilePath, 
    PGPOLWIDATA pLwiSettings )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlDocPtr xmlDoc;

    xmlDoc = xmlReadFile(szfilePath, NULL, 0);

    if (xmlDoc == NULL) 
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pLwiSettings->xmlDoc = xmlDoc;

error:

    return (ceError);
}


CENTERROR
GPOInitLwiData(
    PGPUSER     pUser,
    DWORD       dwPolicyType,
    PGPOLWIDATA *ppLwidata, 
    PSTR pszgGpSysVolPath,
    PSTR pszDestFolderRootPath,
    PSTR pszgCseIdentifier )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN   bExists = FALSE;
    PSTR      pszSettingsFile = NULL;    

    ceError = GPOGetPolicyFile(pUser,
                               dwPolicyType,
                               pszgGpSysVolPath,
                               pszgCseIdentifier,
                               LWISETTINGS_FILE,
                               pszDestFolderRootPath,
                               &pszSettingsFile,
                               &bExists );    
    if (ceError == CENTERROR_GP_PATH_NOT_FOUND ||
        ceError == CENTERROR_GP_CREATE_FAILED)
    {
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bExists) {
        ceError = LwAllocateMemory(sizeof(GPOLWIDATA), (void**)ppLwidata);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = ParseLwisettings( pszSettingsFile, *ppLwidata );
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else {
        ceError = CENTERROR_GP_FILE_COPY_FAILED;
    }

error:

    if( pszSettingsFile ) {
        LwFreeString(pszSettingsFile);
    }

    return(ceError);
}


void
GPODestroyLwiData(
    PGPOLWIDATA pLwidata )
{
    if ( pLwidata )
    {
        if (pLwidata->xmlDoc) {
            xmlFreeDoc(pLwidata->xmlDoc);
        }

        LwFreeMemory(pLwidata);
    }
}


CENTERROR
GPOGetGPItem( 
    DWORD dwPolicyType,
    PGPOLWIDATA pLwiadata, 
    PSTR pszGPItemGUID,
    PGPOLWIGPITEM *ppGPItem )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlDocPtr xmlDoc = NULL;
    xmlXPathContextPtr xpathCtx = NULL; 
    xmlXPathObjectPtr xpathObj = NULL; 
    PSTR pXpathExpr = NULL;

    /* Pre-check our parameter for the LWI data */
    if ( pLwiadata && pLwiadata->xmlDoc ) {
        xmlDoc = (xmlDocPtr)pLwiadata->xmlDoc;
    }

    if (xmlDoc == NULL)
    {
         GPA_LOG_WARNING("GPOGetGPItem: group policy object was not initialized");
         ceError = CENTERROR_GP_LWIDATA_NOT_INITIALIZED;
         BAIL_ON_CENTERIS_ERROR(ceError);
    }

    xpathCtx = xmlXPathNewContext(xmlDoc);
    if(xpathCtx == NULL) {
        ceError = CENTERROR_GP_XPATH_CONTEXT_INIT_ERR;
        GPA_LOG_ERROR("Unable to create new XPath context in GPOGetGPItem");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if(dwPolicyType == MACHINE_GROUP_POLICY) {
        ceError = LwAllocateStringPrintf( &pXpathExpr,
                                          GPITEM_XPATH_EXPR,
                                          pszGPItemGUID );
    } else {
        ceError = LwAllocateStringPrintf( &pXpathExpr,
                                          GPITEM_USER_XPATH_EXPR,
                                          pszGPItemGUID );

    }
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    xpathObj = xmlXPathEvalExpression((xmlChar*)pXpathExpr, xpathCtx);
    if(xpathObj == NULL) {
        GPA_LOG_WARNING("Error: unable to evaluate xpath expression \"%s\"", pXpathExpr);                
        ceError = CENTERROR_GP_XPATH_BAD_EXPRESSION;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *ppGPItem = NULL;

    if (xpathObj->nodesetval && xpathObj->nodesetval->nodeNr > 0)
    {
        if (xpathObj->nodesetval->nodeTab[0]->type != XML_NAMESPACE_DECL)
        {
            ceError = LwAllocateMemory(sizeof(GPOLWIGPITEM), (PVOID*)ppGPItem);
            BAIL_ON_CENTERIS_ERROR(ceError);

            (*ppGPItem)->xmlNode = xpathObj->nodesetval->nodeTab[0];
            (*ppGPItem)->pNext = NULL;

            /* don't let xmlXPathFreeObject() destroy the node we are interested in */
            xpathObj->nodesetval->nodeTab[0] = NULL;
        }
    }

    /*
     * Did we find what we were looking for?
     */
    if (*ppGPItem == NULL)
    {
        ceError = CENTERROR_GP_XML_GPITEM_NOT_FOUND;
    }

error:

    if (xpathCtx) {
        xmlXPathFreeContext(xpathCtx);
    }

    if (xpathObj) {
        xmlXPathFreeObject(xpathObj);
    }

    if (pXpathExpr) {
        LwFreeString(pXpathExpr);
    }

    return ceError;
}


VOID
GPODestroyGPItem(
    PGPOLWIGPITEM pGPItem, 
    BOOLEAN bDeep )
{
    if ( bDeep ) {
        xmlFreeNode( pGPItem->xmlNode );
    }

    LwFreeMemory( pGPItem );
}


static xmlNodePtr
find_node( xmlNodePtr nodeList, const xmlChar *nodeName, xmlChar *nameAttrValue )
{
    xmlNodePtr cur_node = NULL;
    xmlChar *attrValue = NULL;

    for (cur_node = nodeList; cur_node; cur_node = cur_node->next) {

        if (cur_node->type == XML_ELEMENT_NODE && strcmp( (char*)nodeName, (char*)cur_node->name ) == 0 ) {

            if ( nameAttrValue == NULL ) {
                return cur_node;
            }

            attrValue = xmlGetProp( cur_node, (xmlChar*)GPITEM_ATTR_NAME );

            if ( attrValue ){
                if ( strcmp( (char*)attrValue, (char*)nameAttrValue ) == 0 ){
                    xmlFree( attrValue );
                    return cur_node;
                }
                xmlFree( attrValue );
            }
        }
    }

    return NULL;
}


static CENTERROR
merge_tree(
    xmlNode * old_node,
    xmlNode * new_node )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    xmlNodePtr mirror_node = NULL;
    xmlNodePtr added_node = NULL;
    xmlChar *nameAttrValue = NULL;

    for (cur_node = old_node; cur_node; cur_node = cur_node->next) {

        if (cur_node->type == XML_ELEMENT_NODE) {

            nameAttrValue = xmlGetProp( cur_node, (xmlChar*)GPITEM_ATTR_NAME );
            mirror_node = find_node( new_node, cur_node->name, nameAttrValue );

            if ( mirror_node == NULL )
            {
                mirror_node = xmlCopyNode( cur_node, 1 );
                if ( mirror_node == NULL) {
                    ceError = CENTERROR_GP_XML_FAILED_TO_COPY_NODE;
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

                added_node = xmlAddSibling( new_node, mirror_node );
                mirror_node = added_node;

                if (added_node == NULL) {
                    xmlFreeNode( mirror_node );
                    ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
            }

            if ( nameAttrValue ) {
                xmlFree( nameAttrValue );
            }

            merge_tree(cur_node->children, mirror_node->children);
        }
    }

error:

    return ceError;
}


CENTERROR
GPOCalculateRSOP( 
    PGPOLWIGPITEM pOldItem, 
    PGPOLWIGPITEM pNewItem )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = merge_tree( pOldItem->xmlNode, pNewItem->xmlNode );
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}


CENTERROR
GPOCopyGPItem( 
    PGPOLWIGPITEM pSrcItem, 
    PGPOLWIGPITEM *ppGPItem, 
    BOOLEAN bDeep )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pXmlNode = NULL;

    *ppGPItem = NULL;

    if ( pSrcItem == NULL )
        return CENTERROR_SUCCESS;

    ceError = LwAllocateMemory(sizeof(GPOLWIGPITEM), (PVOID*)ppGPItem);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( pSrcItem->xmlNode ) {

        if ( bDeep ) {
            pXmlNode = xmlCopyNode( (xmlNodePtr)pSrcItem->xmlNode, 1 );
        }
        else {
            pXmlNode = xmlCopyNode( (xmlNodePtr)pSrcItem->xmlNode, 0 );

            if ( pXmlNode == NULL) {
                ceError = CENTERROR_GP_XML_FAILED_TO_COPY_NODE;
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            if ( ((xmlNodePtr)pSrcItem->xmlNode)->properties != NULL ) {
                pXmlNode->properties = 
                    xmlCopyPropList( pXmlNode, 
                                     ((xmlNodePtr)pSrcItem->xmlNode)->properties );

                if ( pXmlNode->properties == NULL ) {
                    ceError = CENTERROR_GP_XML_FAILED_TO_COPY_NODE;
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
            }
        }

        (*ppGPItem)->xmlNode = pXmlNode;

    } else {
        (*ppGPItem)->xmlNode = NULL;
    }

    return ceError;

error:

    if ( *ppGPItem ) {
        LwFreeMemory( *ppGPItem );
        *ppGPItem = NULL;
    }

    if (pXmlNode) {
        xmlFreeNode( pXmlNode );
    }

    return ceError;
}


static CENTERROR
merge_tree2(
    xmlNode * rsop_node,
    xmlNode * file_node)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    xmlNodePtr mirror_node = NULL;
    xmlNodePtr added_node = NULL;
    xmlChar *nameAttrValue = NULL;

    for (cur_node = rsop_node; cur_node; cur_node = cur_node->next) {

        if (cur_node->type == XML_ELEMENT_NODE) {
            nameAttrValue = xmlGetProp( cur_node, (xmlChar*)GPITEM_ATTR_NAME );
            mirror_node = find_node( file_node, cur_node->name, nameAttrValue );

            if ( mirror_node == NULL )
            {
                mirror_node = xmlCopyNode( cur_node, 1 );
                if ( mirror_node == NULL) {
                    ceError = CENTERROR_GP_XML_FAILED_TO_COPY_NODE;
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

                added_node = xmlAddSibling( file_node, mirror_node );
                if (added_node == NULL) {
                    xmlFreeNode( mirror_node );
                    ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

                mirror_node = added_node;
            }
            else if ( strcmp( (char*)cur_node->name, GPITEM_TAG_SETTING ) == 0 ) {

                added_node = xmlCopyNode( cur_node, 1 );
                if ( added_node == NULL) {
                    ceError = CENTERROR_GP_XML_FAILED_TO_COPY_NODE;
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

                mirror_node = xmlReplaceNode( mirror_node, added_node );

                mirror_node = added_node;
            }

            if ( nameAttrValue ) {
                xmlFree( nameAttrValue );
            }

            merge_tree2(cur_node->children, mirror_node->children);
        }
    }

error:

    return ceError;
}

CENTERROR 
GPOMergeGPItems(
    PGPOLWIGPITEM pRSOPItem,
    PGPOLWIGPITEM pFileItem )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = merge_tree2( pRSOPItem->xmlNode, pFileItem->xmlNode );
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

CENTERROR
get_node_text(
    xmlNodePtr node,
    xmlChar **ppszText )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    xmlNodePtr cur_node = node->children;
    while( cur_node ) {
        if ( cur_node->type == XML_TEXT_NODE ||
             cur_node->type == XML_CDATA_SECTION_NODE )
        {
            *ppszText = xmlNodeGetContent( cur_node );
            break;
        }
        cur_node = cur_node->next;
    }

/*error:*/

    if (*ppszText == NULL) {
        ceError = CENTERROR_GP_XML_NO_NODE_TEXT;
    }

    return ceError;
}

CENTERROR
GPOXmlSelectNodes(
    xmlNodePtr Node,
    const char* Query,
    xmlNodeSetPtr* Result )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    xmlDocPtr xmlDoc = Node->doc;
    xmlXPathContextPtr xpathCtx = NULL; 
    xmlXPathObjectPtr xpathObj = NULL;
    
    xpathCtx = xmlXPathNewContext(xmlDoc);
    
    if(xpathCtx == NULL) {
        ceError = CENTERROR_GP_XPATH_CONTEXT_INIT_ERR;
        GPA_LOG_ERROR("Unable to create new XPath context in GPOGXmlSelectNodes");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    BAIL_ON_CENTERIS_ERROR(ceError);
    
    xpathCtx->node = Node;
    
    xpathObj = xmlXPathEvalExpression((xmlChar*)Query, xpathCtx);
    if(xpathObj == NULL) {
        GPA_LOG_WARNING("Error: unable to evaluate xpath expression \"%s\"", Query);                
        ceError = CENTERROR_GP_XPATH_BAD_EXPRESSION;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *Result = xpathObj->nodesetval;
    
    // null the nodeset pointer in the path object so it does not get freed
    xpathObj->nodesetval = NULL;

error:

    if (xpathObj) {
        xmlXPathFreeObject(xpathObj);
    }

    if (xpathCtx) {
        xmlXPathFreeContext(xpathCtx);
    }

    return ceError;
}


CENTERROR
GPOXmlSelectSingleNode(
    xmlNodePtr Node,
    const char* Query,
    xmlNodePtr* Result )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodeSetPtr nodeSet = NULL;
   
    *Result = NULL;

    ceError = GPOXmlSelectNodes(Node, Query, &nodeSet);
    BAIL_ON_CENTERIS_ERROR( ceError );

    if (nodeSet->nodeNr > 1) {
        GPA_LOG_WARNING("Warning: GPOXmlSelectSingleNode found multiple matches, returning only the first");     
    }

    if (nodeSet->nodeNr) {
        *Result = nodeSet->nodeTab[0];
    }
    else {
        ceError = CENTERROR_GP_XML_NODE_NOT_FOUND;
    }

error:

    if (nodeSet)
        xmlXPathFreeNodeSet(nodeSet);

    return ceError;
}


CENTERROR
GPOXmlGetAttributeValue(
    xmlNodePtr Node,
    char** ppszResult )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlChar* pszText = NULL;

    if (Node->type != XML_ATTRIBUTE_NODE) {
        GPA_LOG_WARNING("Error: GPOXmlGetAttributeValue called on non-attribute node");                
        ceError = CENTERROR_GP_XML_TYPE_MISMATCH;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pszText = xmlNodeGetContent(Node);
    if (pszText == NULL) {
        ceError = CENTERROR_GP_XML_NO_NODE_TEXT;
        BAIL_ON_CENTERIS_ERROR(ceError);
    } 
    
    ceError = LwAllocateString((const char*)pszText, ppszResult);
    BAIL_ON_CENTERIS_ERROR( ceError );

error:

    if (pszText) {
       xmlFree(pszText);
    }

    return ceError;
}


CENTERROR
GPOXmlGetInnerText(
    xmlNodePtr Node,
    char** ppszResult )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlChar* pszText = NULL;

    if (Node->type != XML_ELEMENT_NODE) {
        GPA_LOG_WARNING("Error: GPOXmlGetInnerText called on non-element node");                
        ceError = CENTERROR_GP_XML_TYPE_MISMATCH;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (!Node->children) {
        GPA_LOG_WARNING("Error: GPOXmlGetInnerText called on node with no content");
        ceError = CENTERROR_GP_XML_NO_NODE_TEXT;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (Node->children->children) {
        GPA_LOG_WARNING("Error: GPOXmlGetInnerText called on node with multiple children");
        ceError = CENTERROR_GP_XML_TYPE_MISMATCH;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (Node->children->type != XML_TEXT_NODE &&
        Node->children->type != XML_CDATA_SECTION_NODE) {
        GPA_LOG_WARNING("Error: GPOXmlGetInnerText called on node with a non-text child node");
        ceError = CENTERROR_GP_XML_TYPE_MISMATCH;
        BAIL_ON_CENTERIS_ERROR(ceError);   
    }

    pszText = xmlNodeGetContent(Node);
    if (pszText == NULL) {
        ceError = CENTERROR_GP_XML_NO_NODE_TEXT;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwAllocateString((const char*)pszText, ppszResult);
    BAIL_ON_CENTERIS_ERROR( ceError );

error:

    if (pszText) {
       xmlFree(pszText);
    }

    return ceError;
}


CENTERROR
GPOXmlGetOptionalAttributeValue(
    xmlNodePtr Node,
    const char* AttributeQuery,
    char** Result )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    xmlNodePtr attrNode;
    
    ceError = GPOXmlSelectSingleNode(Node, AttributeQuery, &attrNode);
    BAIL_ON_CENTERIS_ERROR( ceError );

    if (attrNode && attrNode->type == XML_ATTRIBUTE_NODE) {
        GPOXmlGetAttributeValue(attrNode, Result);
    } else {
        *Result = NULL;
    }

error:

    return ceError;
}

static BOOLEAN
version_in_range(int arity, int* version, int* lower, int* upper)
{
    int i;
    for (i = 0; i < arity; i++)
    {
        if (!(version[i] >= lower[i] && version[i] <= upper[i]))
            return FALSE;
    }
    return TRUE;
}

static int
version_parse(int max_arity, const char* version, int* dest)
{
    int i = 0;
    char* pos = (char*) version;
    
    do {
        dest[i] = atoi(pos);
        pos = strchr(pos, '.');
        if (pos)
            pos++;
    } while (pos && i < max_arity);
    return i;
}

static BOOLEAN
version_matches_pattern(const char* version, const char* pattern)
{
    char *pDash;
    
    if (pattern == NULL)
    {
        return TRUE;
    }
    else if ((pDash = strchr(pattern, '-')))
    {
        BOOLEAN result;
        int version_actual[16], version_lower[16], version_upper[16];
        int actual_arity, lower_arity, upper_arity;

        *pDash = 0;

        actual_arity = version_parse(16, version, version_actual);
        lower_arity = version_parse(16, pattern, version_lower);
        upper_arity = version_parse(16, pDash+1, version_upper);
       
        result = 
            (actual_arity == lower_arity && lower_arity == upper_arity &&
            version_in_range(actual_arity, version_actual, version_lower, version_upper));
        *pDash = '-';
        
        return result;
    }
    else if (!strcmp(pattern, "*"))
    {
        return TRUE;
    }
    else
    {
        int version_actual[16], version_pattern[16];
        int actual_arity, pattern_arity;
        
        actual_arity = version_parse(16, version, version_actual);
        pattern_arity = version_parse(16, pattern, version_pattern);
        
        return (actual_arity == pattern_arity && 
                version_in_range(actual_arity, version_actual, version_pattern, version_pattern));
    }
}

static BOOLEAN
string_matches_pattern(const char* value, const char* pattern)
{
    if (pattern == NULL)
    {
        return TRUE;
    }
    else return !fnmatch(pattern, value, 0);
}

static char* cache_os_name = NULL;
static char* cache_os_distro = NULL;
static char* cache_os_version = NULL;

static CENTERROR
get_os_name(PSTR* name)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    if (!cache_os_name)
    {
        ceError = GPACaptureOutput(PREFIXDIR "/bin/domainjoin-cli get_os_type", &cache_os_name);
        BAIL_ON_CENTERIS_ERROR(ceError);
        LwStrToLower(cache_os_name);
    }
    *name = cache_os_name;
error:
    return ceError;
}

static CENTERROR
get_os_distro(PSTR* distro)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    if (!cache_os_distro)
    {
        ceError = GPACaptureOutput(PREFIXDIR "/bin/domainjoin-cli get_distro", &cache_os_distro);
        BAIL_ON_CENTERIS_ERROR(ceError);
        LwStrToLower(cache_os_distro);
    }
    *distro = cache_os_distro;
error:
    return ceError;
}

static CENTERROR
get_os_version(PSTR* version)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    if (!cache_os_version)
    {
        ceError = GPACaptureOutput(PREFIXDIR "/bin/domainjoin-cli get_distro_version", &cache_os_version);
        BAIL_ON_CENTERIS_ERROR(ceError);
        LwStrToLower(cache_os_version);
    }
    *version = cache_os_version;
error:
    return ceError;
}

CENTERROR
GPOGetPlatformInfo(
    PSTR* os,
    PSTR* distro,
    PSTR* version
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char* osName = NULL;
    char* osDistro = NULL;
    char* osVersion = NULL;
    char* Name = NULL;
    char* Distro = NULL;
    char* Version = NULL;

    if ( !os && !distro && !version ) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    ceError = get_os_name(&osName);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = get_os_distro(&osDistro);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = get_os_version(&osVersion);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( os ) {
        ceError = LwAllocateString(osName, &Name);
        BAIL_ON_CENTERIS_ERROR(ceError);
        *os = Name;
        Name = NULL;
    }

    if ( distro ) {
        ceError = LwAllocateString(osDistro, &Distro);
        BAIL_ON_CENTERIS_ERROR(ceError);
        *distro = Distro;
        Distro = NULL;
    }

    if ( version ) {
        ceError = LwAllocateString(osVersion, &Version);
        BAIL_ON_CENTERIS_ERROR(ceError);
        *version = Version;
        Version = NULL;
    }

error:

    if ( Name ) {
        LwFreeString( Name );
    }

    if ( Distro ) {
        LwFreeString( Distro );
    }

    if ( Version ) {
        LwFreeString( Version );
    }

    return ceError;
}

#define GLOBAL_POLICY_ITEM_GUID             "{F81FFA93-7E08-4b49-A3D1-D5EBC93203EF}"
#define GLOBAL_POLICY_CLIENT_GUID           "{E04493F3-5BE9-4e47-8B5B-9FC52FA2485D}"

#define GLOBAL_POLICY_USER_CLIENT_GUID      "{38645510-0528-470b-8AA6-0BCF151D9B41}"
#define GLOBAL_POLICY_USER_ITEM_GUID        "{EFE58EBA-B01E-4f39-A7BD-09E9528AD93C}"    

static PSTR
GetClientGUID( DWORD dwPolicyType )
{
    if(dwPolicyType == MACHINE_GROUP_POLICY)
        return GLOBAL_POLICY_CLIENT_GUID;
    else 
        return GLOBAL_POLICY_USER_CLIENT_GUID;
}

static PSTR
GetItemGUID( DWORD dwPolicyType )
{
    if(dwPolicyType == MACHINE_GROUP_POLICY)
        return GLOBAL_POLICY_ITEM_GUID;
    else 
        return GLOBAL_POLICY_USER_ITEM_GUID;
}

CENTERROR
GPOXmlVerifyPlatformApplicable(
    PGPUSER pUser,
    DWORD dwPolicyType,
    PGROUP_POLICY_OBJECT Policy,
    PSTR pszHomeDir,
    BOOLEAN* Result)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char* osName = NULL;
    char* osVersion = NULL;
    char* targetName = NULL;
    char* targetVersion = NULL;
    xmlNodeSetPtr targets = NULL;
    PGPOLWIDATA pLwidata = NULL;
    PGPOLWIGPITEM pGPItem = NULL;
    int i;

    *Result = FALSE;
    
    ceError = get_os_name(&osName);
    BAIL_ON_CENTERIS_ERROR(ceError);
    LwStripWhitespace(osName,1,1);
    
    ceError = get_os_version(&osVersion);
    BAIL_ON_CENTERIS_ERROR(ceError);
    LwStripWhitespace(osVersion,1,1);

    // Look in new policy file location
    ceError = GPOInitLwiData( pUser,
                              dwPolicyType,
                              (PGPOLWIDATA*)&pLwidata,
                              Policy->pszgPCFileSysPath,
                              pszHomeDir,
                              GetClientGUID(dwPolicyType) );
    if ( ceError ) {
        // Look in old policy file location
        ceError = GPOInitLwiData( pUser,
                                  dwPolicyType,
                                  (PGPOLWIDATA*)&pLwidata,
                                  Policy->pszgPCFileSysPath,
                                  pszHomeDir,
                                  NULL );
        if (!CENTERROR_IS_OK(ceError) &&
            (CENTERROR_EQUAL(ceError, CENTERROR_GP_FILE_COPY_FAILED) ||
             CENTERROR_EQUAL(ceError, CENTERROR_GP_CREATE_FAILED) ||
             CENTERROR_EQUAL(ceError, CENTERROR_GP_PATH_NOT_FOUND)))
        {
            // No target platform policy file exist, so assume policies in this file apply to us
            ceError = CENTERROR_SUCCESS;
            *Result = TRUE;
            goto error;
        }
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    ceError = GPOGetGPItem( dwPolicyType,
                            pLwidata,
                            GetItemGUID(dwPolicyType),
                            &pGPItem);
    if (!CENTERROR_IS_OK(ceError) &&
        CENTERROR_EQUAL(ceError, CENTERROR_GP_XML_GPITEM_NOT_FOUND)) {
       ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if (!pGPItem)
    {
        // No target platform policy set, so assume policies in this file apply to us
        *Result = TRUE;
        goto error;
    }
    
    ceError = GPOXmlSelectNodes((xmlNodePtr) pGPItem->xmlNode, "setting/TargetPlatforms/platform", &targets);
    BAIL_ON_CENTERIS_ERROR ( ceError );

    if (!targets || targets->nodeNr == 0)
    {
        // No target platform policy set, so assume policies in this file apply to us
        *Result = TRUE;
        goto error;
    }
    else for (i = 0; i < targets->nodeNr; i++)
    {
        xmlNodePtr attr = NULL;
        BOOLEAN passName = FALSE, passVersion = FALSE;

        ceError = GPOXmlSelectSingleNode(targets->nodeTab[i], "@os", &attr);
        BAIL_ON_CENTERIS_ERROR ( ceError );
        
        if (attr != NULL)
        {  
            ceError = GPOXmlGetAttributeValue(attr, &targetName);
            BAIL_ON_CENTERIS_ERROR ( ceError );
            LwStrToLower(targetName);
        }
        
        ceError = GPOXmlSelectSingleNode(targets->nodeTab[i], "@version", &attr);
        BAIL_ON_CENTERIS_ERROR ( ceError );
        
        if (attr != NULL)
        {
            ceError = GPOXmlGetAttributeValue(attr, &targetVersion);
            BAIL_ON_CENTERIS_ERROR ( ceError );
            LwStrToLower(targetVersion);
        }
            
        passName = string_matches_pattern(osName, targetName);
        passVersion = version_matches_pattern(osVersion, targetVersion);
        
        if (passName && passVersion)
        {
            *Result = TRUE;
            goto error;
        }
        
        LW_SAFE_FREE_STRING(targetName); targetName = NULL;
        LW_SAFE_FREE_STRING(targetVersion); targetVersion = NULL;
    }
    
error:

    LW_SAFE_FREE_STRING(targetName);
    LW_SAFE_FREE_STRING(targetVersion);

    if (targets) {
       xmlXPathFreeNodeSet(targets);
    }

    if (pGPItem) {
        GPODestroyGPItem( pGPItem, FALSE );
    }

    if (pLwidata) {
        GPODestroyLwiData(pLwidata);
    }
        
    return ceError;
}
