#include "includes.h"

static 
CENTERROR
parseSettingLine(
    PSTR pszSettingLine,
    PSTR *ppszName,
    PSTR *ppszValue )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR saveptr = NULL;
    PSTR pTmp = NULL;
    PSTR token = NULL;
    
    token = strtok_r( pszSettingLine, 
                      "=", 
                      &saveptr);
    if ( token == NULL ) {
        GPA_LOG_WARNING( "Invalid option name %s", 
                         pszSettingLine);
        ceError = CENTERROR_GP_FILE_PARSE_ERROR;
        BAIL_ON_CENTERIS_ERROR(ceError);	
    }

    pTmp = token + strlen(token) - 1;
    while( pTmp > token && isspace((int)*pTmp) ) {
        *pTmp = '\0';
        pTmp--;
    }
    
    pTmp = token;
    while( isspace((int) *pTmp ) ) {
        pTmp++;
    }	
    
    ceError = LwAllocateString( pTmp, 
                                ppszName);
    BAIL_ON_CENTERIS_ERROR(ceError);
        
    token = strtok_r( NULL, 
                      "=", 
                      &saveptr);

    // if token is null, then we have no "value"
    if ( token == NULL ) {
        *ppszValue = NULL;
    }
    else {    
        pTmp = token + strlen(token) - 1;

        while( pTmp > token && isspace((int)*pTmp) )
            *pTmp = '\0';

        pTmp--;

        while( isspace((int) *token ) ) {
            token++;
        }
    
        ceError = LwAllocateString( token, 
                                    ppszValue );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    return ceError;
    
error:
    
    if (*ppszName) {
        LwFreeString( *ppszName );
        *ppszName = NULL;
    }
    
    if (*ppszValue) {
        LwFreeString( *ppszValue );
        *ppszValue = NULL;
    }
    
    return ceError;	
}
   
static 
CENTERROR
addCDataNode(
    xmlNodePtr pParent,
    PSTR pszName,
    PSTR pszText,
    xmlNodePtr *ppNewNode )
{	
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cdataNode = NULL;
    
    *ppNewNode = xmlNewChild( pParent, 
                              NULL, 
                              (xmlChar*)pszName, 
                              NULL );
    if (*ppNewNode == NULL ) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
   
    if (pszText){
        cdataNode = xmlNewCDataBlock( (*ppNewNode)->doc, 
                                      (xmlChar*)pszText, 
                                      strlen(pszText));
    }
    else {
        cdataNode = xmlNewCDataBlock( (*ppNewNode)->doc, 
                                      (const xmlChar*)"", 
                                      0);
    }

    if (cdataNode == NULL ) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    if (xmlAddChild( *ppNewNode,
                     cdataNode) == NULL ){
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    return ceError;
    
error:

    if (*ppNewNode) {
        xmlFreeNode(*ppNewNode);
        *ppNewNode = NULL;
    }
    
    if (cdataNode) {
        xmlFreeNode(cdataNode);
    }
    
    return ceError;
}

static 
CENTERROR
addSectionNode( 
    xmlNodePtr pParentNode,
    PSTR pszSectionLine,
    xmlNodePtr *ppSectionNode
    )
{	
    CENTERROR ceError = CENTERROR_SUCCESS;
    /*xmlNodePtr cdataNode = NULL;*/
    PSTR pszSectionName = NULL;
    xmlAttrPtr attrPtr = NULL;
    LONG len = 0;
    
    *ppSectionNode = NULL;
    
    len = strlen( pszSectionLine );
    if ( len < 3 || pszSectionLine[len-1] != ']') {
        GPA_LOG_WARNING( "Invalid section name %s", 
                         pszSectionLine);
        ceError = CENTERROR_GP_FILE_PARSE_ERROR;
        BAIL_ON_CENTERIS_ERROR(ceError);		
    }
            
    len--;
    ceError = LwAllocateMemory( len, 
                                (VOID*)&pszSectionName );
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    strncpy( pszSectionName, 
             ++pszSectionLine, 
             len - 1);
    pszSectionName[len-1] = '\0';	
    
    *ppSectionNode = xmlNewChild( pParentNode, 
                                  NULL, 
                                  (const xmlChar*)LWI_TAG_SECTION, 
                                  NULL );
    if (*ppSectionNode == NULL ) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    attrPtr = xmlNewProp( *ppSectionNode, 
                          (const xmlChar*)LWI_ATTR_NAME, 
                          (const xmlChar*)pszSectionName);
    if ( attrPtr == NULL ) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_PROP;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
        
error:

    if ( pszSectionName ) {
        LwFreeMemory(pszSectionName);
    }
    
    if (*ppSectionNode && ceError != CENTERROR_SUCCESS) {
        xmlFreeNode(*ppSectionNode);
        *ppSectionNode = NULL;
    }
    
    return ceError;
}

static 
CENTERROR
addSettingNode(
    xmlNodePtr pParentNode,
    PSTR pszSettingLine )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr pSettingNode = NULL;
    PSTR pszName = NULL;
    PSTR pszValue = NULL;
    
    ceError = parseSettingLine( pszSettingLine, 
                                &pszName,
                                &pszValue );
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = addCDataNode( pParentNode,
                            LWI_TAG_SETTING,
                            pszValue,
                            &pSettingNode );
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if ( xmlNewProp( pSettingNode, 
                     (const xmlChar*)LWI_ATTR_NAME, 
                     (const xmlChar*)pszName) == NULL ) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_PROP;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    /* assume string type */
    if ( xmlNewProp( pSettingNode, 
                     (const xmlChar*)LWI_ATTR_TYPE, 
                     (const xmlChar*)LWI_ATTR_TYPE_STRING) == NULL ) {
        ceError = CENTERROR_GP_XML_FAILED_TO_ADD_PROP;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
error:

    if (pszName) {
        LwFreeString( pszName );
    }
    
    if (pszValue) {
        LwFreeString( pszValue );
    }		
    
    if ( pSettingNode && ceError != CENTERROR_SUCCESS) {
        xmlFreeNode( pSettingNode );
    }
    
    return ceError;
}

CENTERROR
GPOLwiReadItem(
    PSTR pszFilePath,
    PGPOLWIGPITEM pGPItem,
    PSTR pszTagName)

{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszBuf = NULL;
    PSTR token = NULL;
    PSTR saveptr = NULL;
    LONG size = 0;
    xmlNodePtr tagNode = NULL, sectionNode = NULL, node = NULL;
            
    ceError = GPAReadFile( pszFilePath, 
                          &pszBuf, 
                          &size);
    if (ceError != CENTERROR_SUCCESS) {
        GPA_LOG_ERROR( "Failed to read file %s", 
                       pszFilePath);
    }
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if ( size > 0 ) {        
        tagNode = xmlNewNode( NULL, 
                              (const xmlChar*)pszTagName); // LWI_TAG_LWIAUTHD);
        if ( xmlAddChild( (xmlNodePtr)pGPItem->xmlNode, 
                          tagNode ) == NULL ) {
            ceError = CENTERROR_GP_XML_FAILED_TO_ADD_NODE;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        
        token = strtok_r( pszBuf, 
                          "\n", 
                          &saveptr);
        while( token ) {
            if ( strlen(token) > 0 ) {                                
                if ( token[0] == '#' || token[0] == ';' ) {
                    ceError = addCDataNode( sectionNode ? sectionNode : tagNode,
                                            LWI_TAG_COMMENT,
                                            token,
                                            &node );
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
                else if ( token[0] == '[' ) {
                    ceError = addSectionNode( tagNode,
                                              token,
                                              &sectionNode );
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
                else if ( strchr(token, '=') != NULL ) {
                    ceError = addSettingNode( sectionNode,
                                              token );
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
                else {
                    ceError = addCDataNode( sectionNode ? sectionNode : tagNode,
                                            LWI_TAG_COMMENT,
                                            token,
                                            &node );
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
            }            
            token = strtok_r(NULL, "\n", &saveptr);
        }
    }

error:

    if (pszBuf) {
        LwFreeMemory(pszBuf);
    }

    return ceError;
}

BOOLEAN
IsComment(
    PCSTR pszBuf
    )
{
    PCSTR pszTmp = pszBuf;

    if (IsNullOrEmptyString(pszTmp))
        return TRUE;

    while (*pszTmp != '\0' && isspace((int)*pszTmp))
        pszTmp++;

    return (*pszTmp == '#' || *pszTmp == '\0' || *pszTmp == ';');
}

static
CENTERROR
BackupKrb5ConfFile()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    ceError = GPACheckFileExists( LWI_KRB5_CONF_OLD_FILE,
                                 &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bFileExists) {
        /* Back up the original krb5.conf file so that we are able to revert to it. */
        ceError = GPACheckFileExists( LWI_KRB5_CONF_FILE,
                                     &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            GPA_LOG_ALWAYS("Backing up original krb5.conf system file.");
            ceError = GPACopyFileWithOriginalPerms( LWI_KRB5_CONF_FILE,
                                                   LWI_KRB5_CONF_OLD_FILE);
            BAIL_ON_CENTERIS_ERROR( ceError );
        }
        else{
            GPA_LOG_ALWAYS( "%s system file is not present. Hence, skipping the setting...", 
                            LWI_KRB5_CONF_FILE);
        }
    }

error:

    return ceError;
}

CENTERROR
write_krb5_setting( 
    PSTR pszName,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszLine = NULL;
    FILE *sfp = NULL;
    FILE *dfp = NULL;
    BOOLEAN bClockSkewExists = FALSE; 

    ceError = BackupKrb5ConfFile();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID *)&pszLine);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAOpenFile( LWI_KRB5_CONF_OLD_FILE,
                          "r", 
                          &sfp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAOpenFile( LWI_KRB5_CONF_FILE_GP, 
                          "w", 
                          &dfp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while( fgets( pszLine,
                  STATIC_PATH_BUFFER_SIZE,
                  sfp ) != NULL ) {

        if ( strstr( pszLine,
                     "[libdefaults]") != NULL ) {

            ceError = GPAFilePrintf( dfp,
                                    "%s",
                                    pszLine);
            BAIL_ON_CENTERIS_ERROR(ceError);

            while( fgets( pszLine,
                          STATIC_PATH_BUFFER_SIZE,
                          sfp ) != NULL ) {

                if ( strstr( pszLine,
                             "[libdefaults]") == NULL &&
                     strstr( pszLine,
                             "[") != NULL) {
                    break;                    
                }

                if( strstr( pszLine,
                            LWI_CLOCKSKEW) != NULL ){
                    ceError = GPAFilePrintf( dfp,
                                            " %s = %s\n",
                                            pszName,
                                            pszValue);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    bClockSkewExists = TRUE; 
                }
                else {
                    ceError = GPAFilePrintf( dfp,
                                            "%s",
                                            pszLine);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

                memset( pszLine,
                        0,
                        STATIC_PATH_BUFFER_SIZE);
            }

            if( !strcmp( (char *)pszName,
                         (char *)LWI_CLOCKSKEW) &&
                !bClockSkewExists ) {

                ceError = GPAFilePrintf( dfp,
                                        " %s = %s\n",
                                        pszName,
                                        pszValue);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            ceError = GPAFilePrintf( dfp,
                                    "%s",
                                    pszLine);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else {
            ceError = GPAFilePrintf( dfp,
                                    "%s",
                                    pszLine);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        memset( pszLine,
                0,
                STATIC_PATH_BUFFER_SIZE);
    }

    if (sfp) {
        GPACloseFile(sfp);
        sfp = NULL;
    }

    if (dfp) {
        GPACloseFile(dfp);
        dfp = NULL;
    }

    ceError = LwMoveFile( LWI_KRB5_CONF_FILE_GP,
                          LWI_KRB5_CONF_FILE);
    BAIL_ON_CENTERIS_ERROR(ceError);

    LW_SAFE_FREE_STRING(pszLine);

    return ceError;
    
error:

    if (sfp) {
        GPACloseFile(sfp);
        sfp = NULL;
    }

    if (dfp) {
        GPACloseFile(dfp);
        dfp = NULL;
    }

    LW_SAFE_FREE_STRING(pszLine);
    
    return ceError;
}

static
CENTERROR
ProcessSettingTag(
    xmlNodePtr cur_node,
    xmlChar *nameAttrValue,
    FILE *fp
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlChar *pszNodeText = NULL;
    xmlChar *typeAttrValue = NULL;

    ceError = get_node_text( cur_node, 
                             &pszNodeText);
    BAIL_ON_CENTERIS_ERROR( ceError );
             
    typeAttrValue = xmlGetProp( cur_node, 
                                (const xmlChar*)LWI_ATTR_TYPE );
          
    if ( !strcmp( (PCSTR)nameAttrValue,
                  (PCSTR)LWI_CLOCKSKEW)) {
        ceError = write_krb5_setting( (PSTR)nameAttrValue, 
                                      (PSTR)pszNodeText); 
        BAIL_ON_CENTERIS_ERROR( ceError );
    } else if ( !strcmp( (PCSTR)nameAttrValue,
                         (PCSTR)LWI_SERVER_SIGN) || 
                !strcmp( (PCSTR)nameAttrValue,
                         (PCSTR)LWI_NULL_PASSWD)) {
        ceError = AddSettings( (PSTR)nameAttrValue,
                               (PSTR)pszNodeText);
        BAIL_ON_CENTERIS_ERROR( ceError );
    } else if ( !strcmp( (PCSTR)nameAttrValue,
                         (PCSTR)LWI_NAME_CACHE_TIMEOUT)) {
        ceError = AddSettings( (PSTR)nameAttrValue,
                               (PSTR)pszNodeText);
        BAIL_ON_CENTERIS_ERROR( ceError );
 
        ceError = write_setting( (PSTR)nameAttrValue, 
                                 (PSTR)pszNodeText, 
                                 (PSTR)typeAttrValue, fp );
        BAIL_ON_CENTERIS_ERROR( ceError );
    } else {
        ceError = write_setting( (PSTR)nameAttrValue, 
                                 (PSTR)pszNodeText, 
                                 (PSTR)typeAttrValue, fp );
        BAIL_ON_CENTERIS_ERROR( ceError );
    }

error:

    if (typeAttrValue) {
        xmlFree( typeAttrValue );
        typeAttrValue = NULL;
    }

    if ( pszNodeText ) {
        xmlFree( pszNodeText );
        pszNodeText = NULL;
    }

    return ceError;
}

static
CENTERROR
ProcessOtherTag(
    xmlNodePtr cur_node,
    xmlChar *nameAttrValue,
    FILE *fp
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlChar *pszNodeText = NULL;

    if (strcmp( (PCSTR)cur_node->name, 
                LWI_TAG_COMMENT) == 0 ) {
        ceError = get_node_text( cur_node, 
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );
                
        ceError = GPAFilePrintf( fp, 
                                "%s\n", 
                                pszNodeText);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else if (strcmp( (PCSTR)cur_node->name, 
                       LWI_TAG_LINE) == 0 ) {
        ceError = get_node_text( cur_node, 
                                 &pszNodeText );
        BAIL_ON_CENTERIS_ERROR( ceError );
                
        ceError = GPAFilePrintf( fp, 
                                "%s\n", 
                                pszNodeText);
        BAIL_ON_CENTERIS_ERROR( ceError );
    } else if (strcmp( (PCSTR)cur_node->name, 
                        LWI_TAG_SECTION) == 0 ) {                
        if ( nameAttrValue != NULL ) {
            ceError = GPAFilePrintf( fp, 
                                    "[%s]\n", 
                                    nameAttrValue);
            BAIL_ON_CENTERIS_ERROR( ceError );
        }
    }

error:

    if ( pszNodeText ) {
        xmlFree( pszNodeText );
        pszNodeText = NULL;
    }

    return ceError;
}

static 
CENTERROR
write_tree( 
    xmlNodePtr root_node,
    FILE *fp
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    xmlChar *nameAttrValue = NULL;
    
    for (cur_node = root_node; cur_node; cur_node = cur_node->next) {
        
        if (cur_node->name) {
            
            nameAttrValue = xmlGetProp( cur_node, 
                                        (const xmlChar*)LWI_ATTR_NAME );
            
            if (strcmp( (PCSTR)cur_node->name, 
                        (PCSTR)LWI_TAG_SETTING) == 0 ) {
                ceError = ProcessSettingTag( cur_node,
                                             nameAttrValue,
                                             fp);
                BAIL_ON_CENTERIS_ERROR( ceError );
            } else {
                ceError = ProcessOtherTag( cur_node,
                                           nameAttrValue,
                                           fp);
                BAIL_ON_CENTERIS_ERROR( ceError );
            }

            if ( nameAttrValue ) {
                xmlFree( nameAttrValue );
                nameAttrValue = NULL;
            }

        }
        
        write_tree( cur_node->children, 
                    fp);
    }
    
error:

    if ( nameAttrValue ) {
        xmlFree( nameAttrValue );				
        nameAttrValue = NULL;
    }
    
    return ceError;
}

CENTERROR
GPOLwiWriteItem(
    PSTR pszFilePath,
    PSTR pszFileHeader,
    PGPOLWIGPITEM pGPItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;    
    FILE *fp = NULL;
    
    if ( pGPItem == NULL || pGPItem->xmlNode == NULL ) {
        return ceError;
    }

    ceError = GPAOpenFile( pszFilePath, 
                          "w", 
                          &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if ( pszFileHeader ) {
        ceError = GPAFilePrintf( fp, 
                                "%s\n\n", 
                                pszFileHeader);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    ceError = write_tree( (xmlNodePtr)pGPItem->xmlNode, 
                          fp);
    BAIL_ON_CENTERIS_ERROR( ceError );	
                    
error:	
    
    if ( fp ) {
        GPACloseFile(fp);
        fp = NULL;
    }		
    
    return ceError;
}

CENTERROR
ComputeFileTimeStamps(
    PSTR pszBaseFile,
    PSTR pszConfFilePath,
    BOOLEAN *pbGPModified
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    time_t mtime_base;
    time_t mtime_current;

    ceError = GPAGetFileTimeStamps( pszBaseFile,
                                   NULL,
                                   &mtime_base,
                                   NULL );
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetFileTimeStamps( pszConfFilePath,
                                   NULL,
                                   &mtime_current,
                                   NULL );
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( mtime_base > mtime_current) {
        GPA_LOG_VERBOSE( "Base file %s was modified.",
                         pszBaseFile);
        *pbGPModified = TRUE;
    }

error:

    return ceError;
}

