#include "includes.h"

#define FILE_PATH_DELIM ";"

PLOGROTATE_POLICY pXMLStruct = NULL;
PLOGROTATE_POLICY pXMLStructNode = NULL;
PLOGROTATE_POLICY pXMLStructHead = NULL;

PLOGROTATE_POLICY pLogSection = NULL;
PLOGROTATE_POLICY pLogSectionHead = NULL;

static
void
InitializeLogSecNode(
    PLOGROTATE_POLICY *ppLogSecNode
    )
{

    (*ppLogSecNode)->cCompress        = 'n';
    (*ppLogSecNode)->cCopy            = 'n';
    (*ppLogSecNode)->cCopyTruncate    = 'n';
    (*ppLogSecNode)->cDelayCompress   = 'n';
    (*ppLogSecNode)->cIfempty         = 'n';
    (*ppLogSecNode)->cMailFirst       = 'n';
    (*ppLogSecNode)->cMailLast        = 'n';
    (*ppLogSecNode)->cMissingOk       = 'n';
    (*ppLogSecNode)->cNoCompress      = 'n';
    (*ppLogSecNode)->cNoCopy          = 'n';
    (*ppLogSecNode)->cNoCopyTruncate  = 'n';
    (*ppLogSecNode)->cNoCreate        = 'n';
    (*ppLogSecNode)->cNoDelayCompress = 'n';
    (*ppLogSecNode)->cNoMail          = 'n';
    (*ppLogSecNode)->cNoMissingOk     = 'n';
    (*ppLogSecNode)->cNoOldDir        = 'n';
    (*ppLogSecNode)->cNoSharedScripts = 'n';
    (*ppLogSecNode)->cNotIfempty      = 'n';
    (*ppLogSecNode)->cSharedScripts   = 'n';
    (*ppLogSecNode)->cDone            = 'n';

    (*ppLogSecNode)->pNext = NULL;

}

static
void
FreeList(
    PLOGROTATE_POLICY *ppList
    )
{

    PLOGROTATE_POLICY pList = *ppList;

    PLOGFILEPATH pFilePath = NULL;

    while( pList ) {

        PLOGROTATE_POLICY pLogNode = pList;
        pList = pList->pNext;

        pFilePath = pLogNode->pFilePath;

        while( pFilePath ) {
            PLOGFILEPATH pFilePathNode = pFilePath;
            pFilePath = pFilePath->pNext;

            LW_SAFE_FREE_STRING(pFilePathNode->pszFile);
            LwFreeMemory(pFilePathNode);

            pFilePathNode = NULL;            
        }

        LW_SAFE_FREE_STRING(pLogNode->pszFileType);
        LW_SAFE_FREE_STRING(pLogNode->pszLogFrequency);
        LW_SAFE_FREE_STRING(pLogNode->pszRotate);
        LW_SAFE_FREE_STRING(pLogNode->pszPreRotate);
        LW_SAFE_FREE_STRING(pLogNode->pszPostRotate);
        LW_SAFE_FREE_STRING(pLogNode->pszMaxLogSize);
        LW_SAFE_FREE_STRING(pLogNode->pszAppName);
        LW_SAFE_FREE_STRING(pLogNode->pszFileType);
        LW_SAFE_FREE_STRING(pLogNode->pszAppName);
        LW_SAFE_FREE_STRING(pLogNode->pszCompressCmd);
        LW_SAFE_FREE_STRING(pLogNode->pszCompressExt);
        LW_SAFE_FREE_STRING(pLogNode->pszCompressOptions);
        LW_SAFE_FREE_STRING(pLogNode->pszCreate);
        LW_SAFE_FREE_STRING(pLogNode->pszExtension);
        LW_SAFE_FREE_STRING(pLogNode->pszFirstAction);
        LW_SAFE_FREE_STRING(pLogNode->pszInclude);
        LW_SAFE_FREE_STRING(pLogNode->pszLastAction);
        LW_SAFE_FREE_STRING(pLogNode->pszMail);
        LW_SAFE_FREE_STRING(pLogNode->pszMaxLogSize);
        LW_SAFE_FREE_STRING(pLogNode->pszOldDir);
        LW_SAFE_FREE_STRING(pLogNode->pszStart);
        LW_SAFE_FREE_STRING(pLogNode->pszTabooExt);
        LW_SAFE_FREE_STRING(pLogNode->pszUnCompressCmd);

        LwFreeMemory(pLogNode);

        pLogNode = NULL;
    }

    *ppList = NULL;    

}

static 
void
DeleteFilePathList(
    PLOGFILEPATH *ppFilePath
    )
{
    while( *ppFilePath ) {
        PLOGFILEPATH pFilePathNode = *ppFilePath;
        *ppFilePath = (*ppFilePath)->pNext;

        LW_SAFE_FREE_STRING(pFilePathNode->pszFile);
        LwFreeMemory(pFilePathNode);

        pFilePathNode = NULL;        
    }
}

static
CENTERROR
FilePathToken(
    xmlNodePtr cur_node,
    PSTR pszLogFilePath,
    PLOGROTATE_POLICY *ppXMLStruct
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PLOGFILEPATH pTmpFilePath = NULL;
    PLOGFILEPATH pCurFilePath = NULL;

    PSTR pszToken = NULL;
    PSTR pszTmpLine = NULL;

    if ( !strcmp( (const char*)cur_node->name,
                  (const char*)LWI_LOG_FILE_PATH_TAG)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pszTmpLine);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pszTmpLine,
                pszLogFilePath);

        pszToken = strtok( pszTmpLine,
                           (char *)FILE_PATH_DELIM);
 
        while( pszToken != NULL ) {
    
            ceError = LwAllocateMemory( sizeof(LOGFILEPATH),
                                        (PVOID *)&pTmpFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pTmpFilePath->pszFile);
            BAIL_ON_CENTERIS_ERROR(ceError);

            strcpy( pTmpFilePath->pszFile,
                    pszToken);
    
            if( (*ppXMLStruct)->pFilePath == NULL ) {
                (*ppXMLStruct)->pFilePath = pTmpFilePath;
            }
            else {
                pCurFilePath = (*ppXMLStruct)->pFilePath;
                while(pCurFilePath->pNext != NULL) {
                    pCurFilePath = pCurFilePath->pNext;
                }
                pCurFilePath->pNext = pTmpFilePath;
            }
            pszToken = strtok( NULL,
                               (char *)FILE_PATH_DELIM);
        }
    }

cleanup:

    LW_SAFE_FREE_STRING(pszTmpLine);   

    return ceError;
    
error:

    DeleteFilePathList( &(*ppXMLStruct)->pFilePath );

    goto cleanup;

}

static
CENTERROR
PopulateValues(
    PSTR pszName,
    PSTR pszXmlValue,
    PLOGROTATE_POLICY *ppXMLStructNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    if ( !strcmp( (const char*)pszName,
                  (const char*)LWI_LOG_PRE_COMMAND_TAG)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszPreRotate);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszPreRotate,
                pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_POST_COMMAND_TAG)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszPostRotate);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszPostRotate,
                pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_ROTATE_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszRotate);
        BAIL_ON_CENTERIS_ERROR(ceError);

        sprintf( (*ppXMLStructNode)->pszRotate,
                 "%s %s",
                 LWI_LOG_ROTATE_ATTR,
                 pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_SIZE_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszMaxLogSize);
        BAIL_ON_CENTERIS_ERROR(ceError);

        sprintf( (*ppXMLStructNode)->pszMaxLogSize,
                 "%s=%s",
                 LWI_LOG_SIZE_ATTR,
                 pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_APP_NAME_TAG)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszAppName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszAppName,
                pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_COMPRESS_ATTR)) {

        (*ppXMLStructNode)->cCompress = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_COMPRESSCMD_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszCompressCmd);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszCompressCmd,
                pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_UNCOMPRESSCMD_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszUnCompressCmd);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszUnCompressCmd,
                pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_COMPRESSEXT_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszCompressExt);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszCompressExt,
                pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_COMPRESSOPTIONS_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszCompressOptions);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszCompressOptions,
                pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_COPY_ATTR)) {

        (*ppXMLStructNode)->cCopy = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_COPYTRUNCATE_ATTR)) {

        (*ppXMLStructNode)->cCopyTruncate = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_CREATE_ATTR)    ||
              !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_MODE_ATTR)      ||
              !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_OWNER_ATTR)     ||
              !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_GROUP_ATTR)) {

        if( (*ppXMLStructNode)->pszCreate == NULL ) {

            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&(*ppXMLStructNode)->pszCreate);
            BAIL_ON_CENTERIS_ERROR(ceError);

            strcpy( (*ppXMLStructNode)->pszCreate,
                    LWI_LOG_CREATE_ATTR);
        }
        else {
            strcat( (*ppXMLStructNode)->pszCreate,
                    " ");
            strcat( (*ppXMLStructNode)->pszCreate,
                    pszXmlValue);
        }
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_DELAYCOMPRESS_ATTR)) {

        (*ppXMLStructNode)->cDelayCompress = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_EXTENSION_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszExtension);
        BAIL_ON_CENTERIS_ERROR(ceError);

        sprintf( (*ppXMLStructNode)->pszExtension,
                 "%s .%s",
                 LWI_LOG_EXTENSION_ATTR,
                 pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_FIRSTACTION_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszFirstAction);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszFirstAction,
                pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_IFEMPTY_ATTR)) {

        (*ppXMLStructNode)->cIfempty = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_INCLUDE_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszInclude);
        BAIL_ON_CENTERIS_ERROR(ceError);

        sprintf( (*ppXMLStructNode)->pszInclude,
                 "%s %s",
                 LWI_LOG_INCLUDE_ATTR,
                 pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_LASTACTION_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszLastAction);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszLastAction,
                pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_MAIL_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszMail);
        BAIL_ON_CENTERIS_ERROR(ceError);

        sprintf( (*ppXMLStructNode)->pszMail,
                 "%s %s",
                 LWI_LOG_MAIL_ATTR,
                 pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_MAILFIRST_ATTR)) {

        (*ppXMLStructNode)->cMailFirst = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_MAILLAST_ATTR)) {

        (*ppXMLStructNode)->cMailLast = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_MISSINGOK_ATTR)) {

        (*ppXMLStructNode)->cMissingOk = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_DAILY_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszLogFrequency);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszLogFrequency,
                LWI_LOG_DAILY_ATTR);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_WEEKLY_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszLogFrequency);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszLogFrequency,
                LWI_LOG_WEEKLY_ATTR);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_MONTHLY_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszLogFrequency);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszLogFrequency,
                LWI_LOG_MONTHLY_ATTR);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_YEARLY_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszLogFrequency);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszLogFrequency,
                LWI_LOG_YEARLY_ATTR);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_NOCOMPRESS_ATTR)) {

        (*ppXMLStructNode)->cNoCompress = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_NOCOPY_ATTR)) {

        (*ppXMLStructNode)->cNoCopy = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_NOCOPYTRUNCATE_ATTR)) {

        (*ppXMLStructNode)->cNoCopyTruncate = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_NOCREATE_ATTR)) {

        (*ppXMLStructNode)->cNoCreate = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_NODELAYCOMPRESS_ATTR)) {

        (*ppXMLStructNode)->cNoDelayCompress = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_NOMAIL_ATTR)) {

        (*ppXMLStructNode)->cNoMail = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_NOMISSINGOK_ATTR)) {

        (*ppXMLStructNode)->cNoMissingOk = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*) LWI_LOG_NOOLDDIR_ATTR)) {

        (*ppXMLStructNode)->cNoOldDir = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_NOSHAREDSCRIPTS_ATTR)) {

        (*ppXMLStructNode)->cNoSharedScripts = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_NOTIFEMPTY_ATTR)) {

        (*ppXMLStructNode)->cNotIfempty = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_OLDDIR_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszOldDir);
        BAIL_ON_CENTERIS_ERROR(ceError);

        sprintf( (*ppXMLStructNode)->pszOldDir,
                 "%s %s",
                 LWI_LOG_OLDDIR_ATTR,
                 pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_SHAREDSCRIPTS_ATTR)) {

        (*ppXMLStructNode)->cSharedScripts = 'y';
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_START_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszStart);
        BAIL_ON_CENTERIS_ERROR(ceError);

        sprintf( (*ppXMLStructNode)->pszStart,
                 "%s %s",
                 LWI_LOG_START_ATTR, 
                 pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_TABOOEXT_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszTabooExt);
        BAIL_ON_CENTERIS_ERROR(ceError);

        sprintf( (*ppXMLStructNode)->pszTabooExt,
                 "%s %s",
                 LWI_LOG_TABOOEXT_ATTR, 
                 pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_UNCOMPRESSCMD_ATTR)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszUnCompressCmd);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszUnCompressCmd,
                pszXmlValue);
    }
    else if ( !strcmp( (const char*)pszName,
                       (const char*)LWI_LOG_FILE_TYPE_TAG)) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&(*ppXMLStructNode)->pszFileType);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( (*ppXMLStructNode)->pszFileType,
                pszXmlValue);

        if ( !strcmp( (const char*)pszXmlValue,
                      LWI_LOG_ROTATE)) {

            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&(*ppXMLStructNode)->pszAppName);
            BAIL_ON_CENTERIS_ERROR(ceError);

            strcpy( (*ppXMLStructNode)->pszAppName,
                    pszXmlValue);
        }
    }

    return ceError;

error:

    FreeList( ppXMLStructNode );

    return ceError;

}

CENTERROR
ReadXMLTree( 
    xmlNodePtr root_node
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    xmlNodePtr cur_node = NULL;
    xmlChar *pszNodeText = NULL;

    struct _xmlAttr *pAttrNode = NULL;
    
    for (cur_node = root_node; cur_node != NULL ; cur_node = cur_node->next) {

        pszNodeText = NULL;
        
        if (cur_node->name) { 
        
            if ( !strcmp( (const char*)cur_node->name,
                          (const char*)LWI_LOG_FILE_PATH_TAG)) {

                ceError = get_node_text( cur_node, 
                                         &pszNodeText );
                BAIL_ON_CENTERIS_ERROR( ceError );

                ceError = LwAllocateMemory( sizeof(LOGROTATE_POLICY),
                                            (PVOID *)&pXMLStruct);
                BAIL_ON_CENTERIS_ERROR(ceError);

                InitializeLogSecNode( &pXMLStruct );

                ceError = FilePathToken( cur_node,
                                         (PSTR)pszNodeText,
                                         &pXMLStruct);
                BAIL_ON_CENTERIS_ERROR( ceError );

                pXMLStructNode = pXMLStruct;

                if( pXMLStructHead == NULL ) {
                    pXMLStructHead = pXMLStruct;
                }
                else {
                    PLOGROTATE_POLICY pTmpXMLStruct = pXMLStructHead;
                    while( pTmpXMLStruct->pNext != NULL ) {
                        pTmpXMLStruct = pTmpXMLStruct->pNext;
                    }
                    pTmpXMLStruct->pNext = pXMLStruct;
                }
            }
            else if ( !strcmp( (const char*)cur_node->name,
                               (const char*)LWI_LOG_PRE_COMMAND_TAG)   ||
                      !strcmp( (const char*)cur_node->name,
                               (const char*)LWI_LOG_POST_COMMAND_TAG)  ||
                      !strcmp( (const char*)cur_node->name,
                               (const char*)LWI_LOG_FIRSTACTION_ATTR)  ||
                      !strcmp( (const char*)cur_node->name,
                               (const char*)LWI_LOG_LASTACTION_ATTR)   ||
                      !strcmp( (const char*)cur_node->name,
                               (const char*)LWI_LOG_FILE_TYPE_TAG)     ||
                      !strcmp( (const char*)cur_node->name,
                               (const char*)LWI_LOG_APP_NAME_TAG)) {

                ceError = get_node_text( cur_node,
                                         &pszNodeText );
                BAIL_ON_CENTERIS_ERROR( ceError );

                ceError = PopulateValues( (PSTR)cur_node->name,
                                          (PSTR)pszNodeText,
                                          &pXMLStructNode);
                BAIL_ON_CENTERIS_ERROR( ceError );
            }
            else if ( !strcmp( (const char*)cur_node->name,
                               (const char*)LWI_LOG_OPTIONS_TAG)) {

                for ( pAttrNode = cur_node->properties; pAttrNode; pAttrNode = pAttrNode->next ) { 

                    if ( !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_COMPRESS_ATTR)                     ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_COMPRESSCMD_ATTR)                  ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_UNCOMPRESSCMD_ATTR)                ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_COMPRESSEXT_ATTR)                  ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_COMPRESSOPTIONS_ATTR)              ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_COPY_ATTR)                         ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_COPYTRUNCATE_ATTR)                 ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_CREATE_ATTR)                       ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_DAILY_ATTR)                        ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_DELAYCOMPRESS_ATTR)                ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_ERRORS_ATTR)                       ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_EXTENSION_ATTR)                    ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_GROUP_ATTR)                        ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_IFEMPTY_ATTR)                      ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_INCLUDE_ATTR)                      ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_MAIL_ATTR)                         ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_MAILFIRST_ATTR)                    ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_MAILLAST_ATTR)                     ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_MISSINGOK_ATTR)                    ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_MODE_ATTR)                         ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_MONTHLY_ATTR)                      ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_NOCOMPRESS_ATTR)                   ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_NOCOPY_ATTR)                       ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_NOCOPYTRUNCATE_ATTR)               ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_NOCREATE_ATTR)                     ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_NODELAYCOMPRESS_ATTR)              ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_NOMAIL_ATTR)                       ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_NOMISSINGOK_ATTR)                  ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_NOOLDDIR_ATTR)                     ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_NOSHAREDSCRIPTS_ATTR)              ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_NOTIFEMPTY_ATTR)                   ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_OLDDIR_ATTR)                       ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_OWNER_ATTR)                        ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_ROTATE_ATTR)                       ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_SIZE_ATTR)                         ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_SHAREDSCRIPTS_ATTR)                ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_START_ATTR)                        ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_TABOOEXT_ATTR)                     ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_WEEKLY_ATTR)                       ||
                         !strcmp( (char*)pAttrNode->name,
                                  (char*)LWI_LOG_YEARLY_ATTR)) {

                        ceError = PopulateValues( (PSTR)pAttrNode->name,
                                                  (PSTR)pAttrNode->children->content,
                                                  &pXMLStructNode);
                        BAIL_ON_CENTERIS_ERROR( ceError );
                    }
                } 
            }

            if ( pszNodeText ) {
                xmlFree( pszNodeText );
                pszNodeText = NULL;
            }
        }
        
        ReadXMLTree(cur_node->children);
    }

    return ceError;
    
error:

    if ( pszNodeText ) {
        xmlFree( pszNodeText );
        pszNodeText = NULL;
    }

    FreeList( &pXMLStruct );

    return ceError;

}

#if 0
static
void
FreeListNode(
    PLOGROTATE_POLICY *ppLogNode
    )
{

    PLOGROTATE_POLICY pLogNode = *ppLogNode;

    if( pLogNode == pXMLStructHead ) {
        pXMLStructHead = pXMLStructHead->pNext;
    }
    else if( pLogNode == pLogSectionHead ) {
        pLogSectionHead = pLogSectionHead->pNext;
    }

    DeleteFilePathList( &(pLogNode)->pFilePath );

    LW_SAFE_FREE_STRING(pLogNode->pszFileType);
    LW_SAFE_FREE_STRING(pLogNode->pszLogFrequency);
    LW_SAFE_FREE_STRING(pLogNode->pszRotate);
    LW_SAFE_FREE_STRING(pLogNode->pszPreRotate);
    LW_SAFE_FREE_STRING(pLogNode->pszPostRotate);
    LW_SAFE_FREE_STRING(pLogNode->pszMaxLogSize);
    LW_SAFE_FREE_STRING(pLogNode->pszAppName);
    LW_SAFE_FREE_STRING(pLogNode->pszFileType);
    LW_SAFE_FREE_STRING(pLogNode->pszAppName);
    LW_SAFE_FREE_STRING(pLogNode->pszCompressCmd);
    LW_SAFE_FREE_STRING(pLogNode->pszCompressExt);
    LW_SAFE_FREE_STRING(pLogNode->pszCompressOptions);
    LW_SAFE_FREE_STRING(pLogNode->pszCreate);
    LW_SAFE_FREE_STRING(pLogNode->pszExtension);
    LW_SAFE_FREE_STRING(pLogNode->pszFirstAction);
    LW_SAFE_FREE_STRING(pLogNode->pszInclude);
    LW_SAFE_FREE_STRING(pLogNode->pszLastAction);
    LW_SAFE_FREE_STRING(pLogNode->pszMail);
    LW_SAFE_FREE_STRING(pLogNode->pszMaxLogSize);
    LW_SAFE_FREE_STRING(pLogNode->pszOldDir);
    LW_SAFE_FREE_STRING(pLogNode->pszStart);
    LW_SAFE_FREE_STRING(pLogNode->pszTabooExt);
    LW_SAFE_FREE_STRING(pLogNode->pszUnCompressCmd);

    LwFreeMemory(pLogNode);

    pLogNode = NULL;

}
#endif

static
CENTERROR
WriteIncludeDirective(
    PLOGROTATE_POLICY pLogSecNode,
    PSTR pszAppName,
    PSTR pszFilePath,
    FILE *fp
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bIncludeEntryExists = FALSE;

    for( ; pLogSecNode; pLogSecNode = pLogSecNode->pNext ) {

        if( strcmp( pszAppName,
                    pLogSecNode->pszAppName) != 0 ) {
            continue;
        }

        if( pLogSecNode->pszInclude ) {
            ceError = GPACheckFileHoldsPattern( pszFilePath,
                                               pLogSecNode->pszInclude,
                                               &bIncludeEntryExists);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if( !bIncludeEntryExists ) {
                ceError = GPAFilePrintf( fp, 
                                        "%s\n",
                                        pLogSecNode->pszInclude);
                BAIL_ON_CENTERIS_ERROR(ceError);

                fflush( fp );
            }
        }
    }

error:

    return ceError;

}

static
CENTERROR
WriteLogrotateSettings(
    PSTR pszFilePath,
    PSTR pszAppName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;

    PLOGROTATE_POLICY pLogSecNode = NULL;
    PLOGFILEPATH pLogSecFilePathNode = NULL;

    FILE *fp = NULL;

    ceError = GPACheckFileExists( LWI_LOGROTATE_CENTERIS_GP_DIRECTORY, 
                                 &bFileExists );
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( !bFileExists ) {
        ceError = GPAOpenFile( pszFilePath,
                              "w",
                              &fp);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else {
        ceError = GPAOpenFile( pszFilePath,
                              "a+",
                              &fp);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if( pLogSectionHead == NULL ) {
        pLogSecNode = pXMLStructHead;
    }
    else {
        pLogSecNode = pLogSectionHead;
    }

    ceError = WriteIncludeDirective( pLogSecNode,
                                     pszAppName,
                                     pszFilePath,
                                     fp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for( ; pLogSecNode; pLogSecNode = pLogSecNode->pNext ) {

        if( strcmp( pszAppName,
                    pLogSecNode->pszAppName) != 0 ) {
            continue;
        }
        
        pLogSecFilePathNode = pLogSecNode->pFilePath; 

        while( pLogSecFilePathNode ) {

            ceError = GPAFilePrintf( fp, 
                                    "%s ", 
                                    pLogSecFilePathNode->pszFile);
            BAIL_ON_CENTERIS_ERROR(ceError);
        
            pLogSecFilePathNode = pLogSecFilePathNode->pNext;
        }

        ceError = GPAFilePrintf( fp, 
                                "{\n");
        BAIL_ON_CENTERIS_ERROR(ceError);

        if( pLogSecNode->cCompress == 'y' ) {
            ceError = GPAFilePrintf( fp,
                                    "    %s\n",
                                    LWI_LOG_COMPRESS_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cCopyTruncate == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_COPYTRUNCATE_ATTR);
        }

        if( pLogSecNode->cCopy == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_COPY_ATTR);
        }

        if( pLogSecNode->cNoCopy == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_NOCOPY_ATTR);
        }

        if( pLogSecNode->pszCreate ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    pLogSecNode->pszCreate);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->pszLogFrequency ) {
            if( strstr( pLogSecNode->pszLogFrequency,
                        LWI_LOG_DAILY_ATTR)) {
                ceError = GPAFilePrintf( fp, 
                                        "    %s\n",
                                        LWI_LOG_DAILY_ATTR);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
            else if( strstr( pLogSecNode->pszLogFrequency,
                             LWI_LOG_WEEKLY_ATTR)) {
                ceError = GPAFilePrintf( fp, 
                                        "    %s\n",
                                        LWI_LOG_WEEKLY_ATTR);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
            else if( strstr( pLogSecNode->pszLogFrequency,
                             LWI_LOG_MONTHLY_ATTR)) {
                ceError = GPAFilePrintf( fp, 
                                        "    %s\n",
                                        LWI_LOG_MONTHLY_ATTR);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
            else if( strstr( pLogSecNode->pszLogFrequency,
                             LWI_LOG_YEARLY_ATTR)) {
                ceError = GPAFilePrintf( fp, 
                                        "    %s\n",
                                        LWI_LOG_YEARLY_ATTR);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        if( pLogSecNode->cDelayCompress == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_DELAYCOMPRESS_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->pszExtension ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    pLogSecNode->pszExtension);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cIfempty == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_IFEMPTY_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->pszMail ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    pLogSecNode->pszMail);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cMailFirst == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_MAILFIRST_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cMailLast == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_MAILLAST_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cMissingOk == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_MISSINGOK_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cNoCompress == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_NOCOMPRESS_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cNoCopyTruncate == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_NOCOPYTRUNCATE_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cNoCreate == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_NOCREATE_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cNoDelayCompress == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_NODELAYCOMPRESS_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cNoMail == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_NOMAIL_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cNoMissingOk == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_NOMISSINGOK_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cNoOldDir == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_NOOLDDIR_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cNoSharedScripts == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_NOSHAREDSCRIPTS_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cNotIfempty == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_NOTIFEMPTY_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->pszOldDir ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    pLogSecNode->pszOldDir);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->pszRotate ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    pLogSecNode->pszRotate);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->pszPreRotate ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n        %s\n    %s\n",
                                    LWI_LOG_PRE_COMMAND_TAG,
                                    pLogSecNode->pszPreRotate,
                                    LWI_LOG_ENDSCRIPT_TAG);
             BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->pszPostRotate ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n        %s\n    %s\n",
                                    LWI_LOG_POST_COMMAND_TAG,
                                    pLogSecNode->pszPostRotate,
                                    LWI_LOG_ENDSCRIPT_TAG);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->pszFirstAction ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n%s\n    %s",
                                    LWI_LOG_FIRSTACTION_ATTR,
                                    pLogSecNode->pszFirstAction,
                                    LWI_LOG_ENDSCRIPT_TAG);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->pszLastAction ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n%s\n    %s",
                                    LWI_LOG_LASTACTION_ATTR,
                                    pLogSecNode->pszLastAction,
                                    LWI_LOG_ENDSCRIPT_TAG);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->pszMaxLogSize ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    pLogSecNode->pszMaxLogSize);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if( pLogSecNode->cSharedScripts == 'y' ) {
            ceError = GPAFilePrintf( fp, 
                                    "    %s\n",
                                    LWI_LOG_SHAREDSCRIPTS_ATTR);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = GPAFilePrintf( fp, 
                                "}\n\n");
        BAIL_ON_CENTERIS_ERROR(ceError);

        //FreeListNode( &pLogSecNode );
    }
       
error:

    if(fp) {
        fclose(fp);
    }

    return ceError;

}

static
CENTERROR
BackupOriginalFile()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    BOOLEAN bFileExists = FALSE;

    ceError = GPACheckFileExists( LWI_LOGROTATE_CENTERIS_GP_DIRECTORY, 
                                 &bFileExists );
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( !bFileExists ) {

        ceError = GPACheckFileExists( LWI_LOGROTATE_CONF_FILE, 
                                     &bFileExists );
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if ( bFileExists ) {

            GPA_LOG_VERBOSE("Backing up original logrotate.conf system file.");

            ceError = GPACopyFileWithOriginalPerms( LWI_LOGROTATE_CONF_FILE,
                                                   LWI_LOGROTATE_CENTERIS_GP_DIRECTORY );
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else if ( !bFileExists ) {
            GPA_LOG_VERBOSE("logrotate.conf system file not found ....");
        }
    }

error:

     return ceError;    

}

static
CENTERROR
ParseConfFile(
    PSTR pszLogFilePath,
    PSTR pszLogAppName
    )
{
    
    CENTERROR ceError = CENTERROR_SUCCESS;

    PLOGFILEPATH  pFilePathNode = NULL, 
                  pCurFilePath = NULL;
               
    PLOGROTATE_POLICY pLogSectionNode = NULL,
                      pCurLogSectionNode = NULL;

    CHAR szLogFileGP[STATIC_PATH_BUFFER_SIZE];

    int nStartSec = 0;
    int nHeadSec = 0;

    PSTR pszToken = NULL;
    PSTR pszLine = NULL;

    FILE *fp = NULL;
    FILE *fpGp = NULL;
    
    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                (PVOID*)&pszLine);
    BAIL_ON_CENTERIS_ERROR(ceError);

    memset( pszLine,
            0,
            STATIC_PATH_BUFFER_SIZE);

    ceError = GPAOpenFile( pszLogFilePath, 
                          "r",
                          &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf( szLogFileGP,
             "%s.gp",
             pszLogFilePath);

    ceError = GPAOpenFile( szLogFileGP, 
                          "w",
                          &fpGp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAFilePrintf( fpGp,
                            "%s\n", 
                            LWI_LOGROTATE_POLICY_FILE_HEADER);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while( fgets( pszLine,
                  STATIC_PATH_BUFFER_SIZE,
                  fp)) {

        LwStripWhitespace(pszLine,1,1);

        if( strstr( pszLine,
                    "{")        &&
            strcspn( pszLine, 
                     "#") != 0) {

            ceError = LwAllocateMemory( sizeof(LOGROTATE_POLICY),
                                        (PVOID *)&pLogSectionNode);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSectionNode->pszAppName);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSectionNode->pszFileType);
            BAIL_ON_CENTERIS_ERROR(ceError);

            InitializeLogSecNode( &pLogSectionNode );

            if( !strcmp( pszLogAppName,
                         LWI_LOG_ROTATE)) {
                sprintf( pLogSectionNode->pszAppName,
                         "%s",
                         LWI_LOG_ROTATE);
                sprintf( pLogSectionNode->pszFileType,
                         "%s",
                         LWI_LOG_ROTATE);
            }
            else {
                sprintf( pLogSectionNode->pszAppName,
                         "%s",
                         pszLogAppName);
                sprintf( pLogSectionNode->pszFileType,
                         "%s",
                         LWI_LOG_ROTATE_D);
            }
              
            nStartSec = 1;
            nHeadSec = 1;
        }
        else if( strstr( pszLine,
                         "}")       &&
                 strcspn( pszLine, 
                          "#") != 0) {
            nStartSec = 0;
                
            if ( pLogSectionHead == NULL ) {
                pLogSectionHead = pLogSectionNode;
            }
            else {
                pCurLogSectionNode = pLogSectionHead;
                while( pCurLogSectionNode->pNext != NULL) {
                    pCurLogSectionNode = pCurLogSectionNode->pNext;
                }
                pCurLogSectionNode->pNext = pLogSectionNode;
            }
        }

        if( nStartSec || ((!nStartSec       && 
                          strstr( pszLine,
                                  "}"))     && 
                          strcspn( pszLine, 
                                   "#") != 0)) {

            if( nHeadSec ) {

                pszToken = (char *)strtok( pszLine,
                                           (char *)" ");

                while( pszToken != NULL ) {

                    ceError = LwAllocateMemory( sizeof(LOGROTATE_POLICY),
                                                (PVOID *)&pFilePathNode);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                    
                    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                                (PVOID *)&pFilePathNode->pszFile);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    strcpy( pFilePathNode->pszFile,
                            pszToken);
    
                    if( pLogSectionNode->pFilePath == NULL ) {
                        pLogSectionNode->pFilePath = pFilePathNode;
                    }
                    else {
                        pCurFilePath = pLogSectionNode->pFilePath;
                        while( pCurFilePath->pNext != NULL ) {
                            pCurFilePath = pCurFilePath->pNext;
                        }
                        pCurFilePath->pNext = pFilePathNode;
                    }

                    pszToken = (char *)strtok( NULL,
                                               (char *)" ");

                    if( strstr( pszToken,
                                (char *)"{")) {
                        break;
                    }
                }
               
                nHeadSec = 0;
            }
            else if( strstr( pszLine,
                             LWI_LOG_DAILY_ATTR)       ||
                     strstr( pszLine,
                             LWI_LOG_WEEKLY_ATTR)      ||
                     strstr( pszLine,
                             LWI_LOG_MONTHLY_ATTR)     ||
                     strstr( pszLine,
                             LWI_LOG_YEARLY_ATTR)) {
                ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                            (PVOID *)&pLogSectionNode->pszLogFrequency);
                BAIL_ON_CENTERIS_ERROR(ceError);

                /* strip trailing newline */
                LwStripWhitespace(pszLine,1,1);

                strcpy( pLogSectionNode->pszLogFrequency,
                        pszLine);     
            }
            else if( strstr( pszLine,
                             LWI_LOG_SIZE_ATTR)) {

                ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                            (PVOID *)&pLogSectionNode->pszMaxLogSize);
                BAIL_ON_CENTERIS_ERROR(ceError);

                strcpy( pLogSectionNode->pszMaxLogSize,
                        pszLine);     
            }
            else if( strstr( pszLine,
                             LWI_LOG_CREATE_ATTR)) {

                ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                            (PVOID *)&pLogSectionNode->pszCreate);
                BAIL_ON_CENTERIS_ERROR(ceError);

                strcpy( pLogSectionNode->pszCreate,
                        pszLine); 
            }
            else if( strstr( pszLine,
                             LWI_LOG_EXTENSION_ATTR)) {

                ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                            (PVOID *)&pLogSectionNode->pszExtension);
                BAIL_ON_CENTERIS_ERROR(ceError);

                strcpy( pLogSectionNode->pszExtension,
                        pszLine); 
            }
            else if( strstr( pszLine,
                             LWI_LOG_INCLUDE_ATTR)) {

                ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                            (PVOID *)&pLogSectionNode->pszInclude);
                BAIL_ON_CENTERIS_ERROR(ceError);

                strcpy( pLogSectionNode->pszInclude,
                        pszLine); 
            }
            else if( strstr( pszLine,
                             LWI_LOG_OLDDIR_ATTR)) {

                ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                            (PVOID *)&pLogSectionNode->pszOldDir);
                BAIL_ON_CENTERIS_ERROR(ceError);

                strcpy( pLogSectionNode->pszOldDir,
                        pszLine); 
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_PRE_COMMAND_TAG)) {

                if( fgets( pszLine,
                           STATIC_PATH_BUFFER_SIZE,
                           fp)) {

                    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                                (PVOID *)&pLogSectionNode->pszPreRotate);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    LwStripWhitespace(pszLine,1,1);

                    strcpy( pLogSectionNode->pszPreRotate,
                            pszLine); 
                }
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_POST_COMMAND_TAG)) {

                if( fgets( pszLine,
                           STATIC_PATH_BUFFER_SIZE,
                           fp)) {

                    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                                (PVOID *)&pLogSectionNode->pszPostRotate);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    LwStripWhitespace(pszLine,1,1);

                    strcpy( pLogSectionNode->pszPostRotate,
                            pszLine); 
                }
            }
            else if( strstr( pszLine,
                             LWI_LOG_ROTATE_ATTR)) {

                ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                            (PVOID *)&pLogSectionNode->pszRotate);
                BAIL_ON_CENTERIS_ERROR(ceError);

                strcpy( pLogSectionNode->pszRotate,
                        pszLine);     
            }
            else if( strstr( pszLine,
                             LWI_LOG_FIRSTACTION_ATTR)) {

                if( fgets( pszLine,
                           STATIC_PATH_BUFFER_SIZE,
                           fp)) {

                    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                                (PVOID *)&pLogSectionNode->pszFirstAction);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    strcpy( pLogSectionNode->pszFirstAction,
                            pszLine); 
                }
            }
            else if( strstr( pszLine,
                             LWI_LOG_LASTACTION_ATTR)) {

                if( fgets( pszLine,
                           STATIC_PATH_BUFFER_SIZE,
                           fp)) {

                    ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                                (PVOID *)&pLogSectionNode->pszLastAction);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    strcpy( pLogSectionNode->pszLastAction,
                            pszLine); 
                }
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_COMPRESS_ATTR)) {

                pLogSectionNode->cCompress = 'y';     
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_COPY_ATTR)) {

                pLogSectionNode->cCopy = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_NOCOPY_ATTR)) {

                pLogSectionNode->cNoCopy = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_COPYTRUNCATE_ATTR)) {

                pLogSectionNode->cCopyTruncate = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_DELAYCOMPRESS_ATTR)) {

                pLogSectionNode->cDelayCompress = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_IFEMPTY_ATTR)) {

                pLogSectionNode->cIfempty = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_MAILFIRST_ATTR)) {

                pLogSectionNode->cMailFirst = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_MAILLAST_ATTR)) {

                pLogSectionNode->cMailLast = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_MISSINGOK_ATTR)) {

                pLogSectionNode->cMissingOk = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_NOCOMPRESS_ATTR)) {

                pLogSectionNode->cNoCompress = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_NOCOPYTRUNCATE_ATTR)) {

                pLogSectionNode->cNoCopyTruncate = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_NOCREATE_ATTR)) {

                pLogSectionNode->cNoCreate = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_NODELAYCOMPRESS_ATTR)) {

                pLogSectionNode->cNoDelayCompress = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_NOCREATE_ATTR)) {

                pLogSectionNode->cNoCreate = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_NOMAIL_ATTR)) {

                pLogSectionNode->cNoMail = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_NOMISSINGOK_ATTR)) {

                pLogSectionNode->cNoMissingOk = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_NOOLDDIR_ATTR)) {

                pLogSectionNode->cNoOldDir = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_NOSHAREDSCRIPTS_ATTR)) {

                pLogSectionNode->cNoSharedScripts = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_NOTIFEMPTY_ATTR)) {

                pLogSectionNode->cNotIfempty = 'y';
            }
            else if( !strcmp( pszLine,
                              LWI_LOG_SHAREDSCRIPTS_ATTR)) {

                pLogSectionNode->cSharedScripts = 'y';
            }
            else if( strstr( pszLine,
                             LWI_LOG_MAIL_ATTR)) {

                ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                            (PVOID *)&pLogSectionNode->pszMail);
                BAIL_ON_CENTERIS_ERROR(ceError);

                strcpy( pLogSectionNode->pszMail,
                        pszLine); 
            }
        }
        else if( strstr( pszLine,
                         "##")) {
            continue;
        }
        else {
            ceError = GPAFilePrintf( fpGp,
                                    "%s\n", 
                                    pszLine);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    if(fp) {
        fclose(fp);  
    }
    
    if(fpGp) {
        fclose(fpGp);
    }

    LW_SAFE_FREE_STRING(pszLine);

    return ceError;

error:
    
    if(fp) {
        fclose(fp);  
    }
    
    if(fpGp) {
        fclose(fpGp);
    }

    LW_SAFE_FREE_STRING(pszLine);

    FreeList( &pLogSectionNode );

    return ceError;

}

static
CENTERROR
CreateAndLinkNode(
    PLOGROTATE_POLICY pXMLStructNode 
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
            
    PLOGROTATE_POLICY pLogSecNode = NULL;
    PLOGROTATE_POLICY pTmpSecNode = NULL;

    PLOGFILEPATH pLogFilePathNode = NULL;
    PLOGFILEPATH pTmpLogFilePathNode = NULL;
    PLOGFILEPATH pTmpFilePath = NULL;

    ceError = LwAllocateMemory( sizeof(LOGROTATE_POLICY),
                                (PVOID *)&pLogSecNode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pTmpSecNode = pLogSectionHead;

    while( pTmpSecNode->pNext != NULL ) {
        pTmpSecNode = pTmpSecNode->pNext;
    }

    if( pXMLStructNode->pFilePath ) {
        
        pTmpFilePath = pXMLStructNode->pFilePath;

        while( pTmpFilePath ) {

            ceError = LwAllocateMemory( sizeof(LOGFILEPATH),
                                        (PVOID *)&pLogFilePathNode);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogFilePathNode->pszFile);
            BAIL_ON_CENTERIS_ERROR(ceError);

            strcpy( pLogFilePathNode->pszFile,
                    pTmpFilePath->pszFile );

            pLogFilePathNode->pNext = NULL;

            pTmpLogFilePathNode = pLogSecNode->pFilePath;

            if( pTmpLogFilePathNode == NULL ) {
                pLogSecNode->pFilePath = pLogFilePathNode;
            }
            else {
                while( pTmpLogFilePathNode->pNext != NULL ) {
                    pTmpLogFilePathNode = pTmpLogFilePathNode->pNext;
                }
                pTmpLogFilePathNode->pNext = pLogFilePathNode;
            }

            pTmpFilePath = pTmpFilePath->pNext;
        }
    }

    if( pXMLStructNode->pszRotate ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszRotate);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszRotate,
                pXMLStructNode->pszRotate);
    }

    if( pXMLStructNode->pszMaxLogSize ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszMaxLogSize);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszMaxLogSize,
                pXMLStructNode->pszMaxLogSize);
    }

    if( pXMLStructNode->pszLogFrequency ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszLogFrequency);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszLogFrequency,
                pXMLStructNode->pszLogFrequency);
    }

    if( pXMLStructNode->pszFileType ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszFileType);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszFileType,
                pXMLStructNode->pszFileType);
    }

    if( pXMLStructNode->pszAppName ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszAppName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszAppName,
                pXMLStructNode->pszAppName);
    }

    if ( pXMLStructNode->pszPreRotate) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszPreRotate);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszPreRotate,
                pXMLStructNode->pszPreRotate);
    }
 
    if ( pXMLStructNode->pszPostRotate) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszPostRotate);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszPostRotate,
                pXMLStructNode->pszPostRotate);
    }

    if ( pXMLStructNode->cCompress == 'y' ) {
        pLogSecNode->cCompress = 'y';
    }

    if ( pXMLStructNode->pszCompressCmd ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszCompressCmd);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszCompressCmd,
                pXMLStructNode->pszCompressCmd);
    }

    if ( pXMLStructNode->pszUnCompressCmd ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszUnCompressCmd);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszUnCompressCmd,
                pXMLStructNode->pszUnCompressCmd);
    }

    if ( pXMLStructNode->pszCompressExt ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszCompressExt);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszCompressExt,
                pXMLStructNode->pszCompressExt);
    }

    if ( pXMLStructNode->pszCompressOptions ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszCompressOptions);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszCompressOptions,
                pXMLStructNode->pszCompressOptions);
    }

    if ( pXMLStructNode->cCopy == 'y' ) {
        pLogSecNode->cCopy = 'y';
    }

    if ( pXMLStructNode->cCopyTruncate == 'y' ) {
        pLogSecNode->cCopyTruncate = 'y';
    }

    if ( pXMLStructNode->pszCreate ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszCreate);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszCreate,
                pXMLStructNode->pszCreate);
    }

    if ( pXMLStructNode->cDelayCompress == 'y' ) {
        pLogSecNode->cDelayCompress = 'y';
    }

    if ( pXMLStructNode->pszExtension ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszExtension);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszExtension,
                pXMLStructNode->pszExtension);
    }

    if ( pXMLStructNode->pszFirstAction ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszFirstAction);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszFirstAction,
                pXMLStructNode->pszFirstAction);
    }

    if ( pXMLStructNode->cIfempty == 'y' ) {
        pLogSecNode->cIfempty = 'y';
    }

    if ( pXMLStructNode->pszInclude ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszInclude);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszInclude,
                pXMLStructNode->pszInclude);
    }

    if ( pXMLStructNode->pszLastAction ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszLastAction);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszLastAction,
                pXMLStructNode->pszLastAction);
    }

    if ( pXMLStructNode->pszMail ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszMail);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszMail,
                pXMLStructNode->pszMail);
    }

    if ( pXMLStructNode->cMailFirst == 'y' ) {
        pLogSecNode->cMailFirst = 'y';
    }

    if ( pXMLStructNode->cMailLast == 'y' ) {
        pLogSecNode->cMailLast = 'y';
    }

    if ( pXMLStructNode->cMissingOk == 'y' ) {
        pLogSecNode->cMissingOk = 'y';
    }

    if ( pXMLStructNode->cNoCompress == 'y' ) {
        pLogSecNode->cNoCompress = 'y';
    }

    if ( pXMLStructNode->cNoCopy == 'y' ) {
        pLogSecNode->cNoCopy = 'y';
    }

    if ( pXMLStructNode->cNoCopyTruncate == 'y' ) {
        pLogSecNode->cNoCopyTruncate = 'y';
    }

    if ( pXMLStructNode->cNoCreate == 'y' ) {
        pLogSecNode->cNoCreate = 'y';
    }

    if ( pXMLStructNode->cNoDelayCompress == 'y' ) {
        pLogSecNode->cNoDelayCompress = 'y';
    }
    
    if ( pXMLStructNode->cNoMail == 'y' ) {
        pLogSecNode->cNoMail = 'y';
    }

    if ( pXMLStructNode->cNoMissingOk == 'y' ) {
        pLogSecNode->cNoMissingOk = 'y';
    }

    if ( pXMLStructNode->cNoOldDir == 'y' ) {
        pLogSecNode->cNoOldDir = 'y';
    }

    if ( pXMLStructNode->cNoSharedScripts == 'y' ) {
        pLogSecNode->cNoSharedScripts = 'y';
    }

    if ( pXMLStructNode->cNotIfempty == 'y' ) {
        pLogSecNode->cNotIfempty = 'y';
    }

    if ( pXMLStructNode->pszOldDir ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszOldDir);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszOldDir,
                pXMLStructNode->pszOldDir);
    }

    if ( pXMLStructNode->cSharedScripts == 'y' ) {
        pLogSecNode->cSharedScripts = 'y';
    }

    if ( pXMLStructNode->pszStart ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszStart);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszStart,
                pXMLStructNode->pszStart);
    }

    if ( pXMLStructNode->pszTabooExt ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pLogSecNode->pszTabooExt);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pLogSecNode->pszTabooExt,
                pXMLStructNode->pszTabooExt);
    }

    pTmpSecNode->pNext = pLogSecNode;

    return ceError;

error:

    FreeList( &pLogSecNode );

    return ceError;

}

static
CENTERROR
DeleteFilePathNode(
    PLOGROTATE_POLICY pLogSectionNode,
    PSTR pszFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PLOGFILEPATH pLogSecFilePathNode = pLogSectionNode->pFilePath;
    PLOGFILEPATH pTmpLogSecFilePathNode = NULL;
    PLOGFILEPATH pPrevLogSecFilePathNode = NULL;

    pPrevLogSecFilePathNode = pLogSecFilePathNode; 

    while(pLogSecFilePathNode) {

        pTmpLogSecFilePathNode = pLogSecFilePathNode;
        pLogSecFilePathNode = pLogSecFilePathNode->pNext;

        if( !strcmp( pTmpLogSecFilePathNode->pszFile,
                     pszFilePath)) {

            if( pPrevLogSecFilePathNode == pTmpLogSecFilePathNode ) {
                pLogSectionNode->pFilePath = pTmpLogSecFilePathNode->pNext; 
            }
            else {
                pPrevLogSecFilePathNode->pNext = pTmpLogSecFilePathNode->pNext; 
            }

            LW_SAFE_FREE_STRING(pTmpLogSecFilePathNode->pszFile);
            LwFreeMemory(pTmpLogSecFilePathNode);
            pTmpLogSecFilePathNode = NULL;
        }
        else {
            pPrevLogSecFilePathNode = pTmpLogSecFilePathNode;
        }
    }

    return ceError;

}

static
CENTERROR
AddEntriesToXMLList(
    PLOGROTATE_POLICY pXMLStructNode,
    PLOGROTATE_POLICY pLogSecNode
)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if( pLogSecNode->pszRotate &&
        !pXMLStructNode->pszRotate ) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszRotate);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszRotate,
                pLogSecNode->pszRotate);       
    }    

    if( pLogSecNode->pszMaxLogSize && 
        !pXMLStructNode->pszMaxLogSize ) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszMaxLogSize);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszMaxLogSize,
                pLogSecNode->pszMaxLogSize);
    }

    if( pLogSecNode->pszLogFrequency &&
        !pXMLStructNode->pszLogFrequency ) {

        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszLogFrequency);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszLogFrequency,
                pLogSecNode->pszLogFrequency);
    }

    if( pLogSecNode->pszFileType && 
        !pXMLStructNode->pszFileType ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszFileType);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszFileType,
                pLogSecNode->pszFileType);
    }

    if( pLogSecNode->pszAppName &&
        !pXMLStructNode->pszAppName ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszAppName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszAppName,
                pLogSecNode->pszAppName);
    }

    if( pLogSecNode->pszPreRotate && 
        !pXMLStructNode->pszPreRotate ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszPreRotate);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszPreRotate,
                pLogSecNode->pszPreRotate);
    }

    if( pLogSecNode->pszPostRotate &&
        !pXMLStructNode->pszPostRotate ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszPostRotate);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszPostRotate,
                pLogSecNode->pszPostRotate);
    }

    if ( pLogSecNode->cCompress == 'y' &&
         pXMLStructNode->cCompress != 'y' ) {
        pXMLStructNode->cCompress = 'y';
    }

    if( pLogSecNode->pszCompressCmd &&
        !pXMLStructNode->pszCompressCmd ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszCompressCmd);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszCompressCmd,
                pLogSecNode->pszCompressCmd);
    }

    if( pLogSecNode->pszUnCompressCmd &&
        !pXMLStructNode->pszUnCompressCmd ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszUnCompressCmd);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszUnCompressCmd,
                pLogSecNode->pszUnCompressCmd);
    }

    if( pLogSecNode->pszCompressExt &&
        !pXMLStructNode->pszCompressExt ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszCompressExt);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszCompressExt,
                pLogSecNode->pszCompressExt);
    }

    if( pLogSecNode->pszCompressOptions &&
        !pXMLStructNode->pszCompressOptions ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszCompressOptions);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszCompressOptions,
                pLogSecNode->pszCompressOptions);
    }

    if( pLogSecNode->cCopy == 'y' &&
        pXMLStructNode->cCopy != 'y' ) {
        pXMLStructNode->cCopy = 'y';
    }

    if( pLogSecNode->cCopyTruncate == 'y' &&
        pXMLStructNode->cCopyTruncate != 'y' ) {
        pXMLStructNode->cCopyTruncate = 'y';
    }

    if( pLogSecNode->pszCreate &&
        !pXMLStructNode->pszCreate ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszCreate);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszCreate,
                pLogSecNode->pszCreate);
    }

    if ( pLogSecNode->cDelayCompress == 'y' &&
         pXMLStructNode->cDelayCompress != 'y' ) {
        pXMLStructNode->cDelayCompress = 'y';
    }

    if( pLogSecNode->pszExtension &&
        !pXMLStructNode->pszExtension ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszExtension);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszExtension,
                pLogSecNode->pszExtension);
    }

    if( pLogSecNode->pszFirstAction &&
        !pXMLStructNode->pszFirstAction ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszFirstAction);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszFirstAction,
                pLogSecNode->pszFirstAction);
    }

    if ( pLogSecNode->cIfempty == 'y' &&
         pXMLStructNode->cIfempty != 'y' ) {
        pXMLStructNode->cIfempty = 'y';
    }

    if( pLogSecNode->pszInclude &&
        !pXMLStructNode->pszInclude ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszInclude);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszInclude,
                pLogSecNode->pszInclude);
    }

    if( pLogSecNode->pszLastAction &&
        !pXMLStructNode->pszLastAction ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszLastAction);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszLastAction,
                pLogSecNode->pszLastAction);
    }

    if( pLogSecNode->pszMail &&
        !pXMLStructNode->pszMail ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszMail);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszMail,
                pLogSecNode->pszMail);
    }

    if( pLogSecNode->cMailFirst == 'y' &&
        pXMLStructNode->cMailFirst != 'y' ) {
        pXMLStructNode->cMailFirst = 'y';
    }

    if( pLogSecNode->cMailLast == 'y' &&
        pXMLStructNode->cMailLast != 'y' ) {
        pXMLStructNode->cMailLast = 'y';
    }

    if( pLogSecNode->cMissingOk == 'y' &&
        pXMLStructNode->cMissingOk != 'y' ) {
        pXMLStructNode->cMissingOk = 'y';
    }

    if( pLogSecNode->cNoCompress == 'y' &&
        pXMLStructNode->cNoCompress != 'y' ) {
        pXMLStructNode->cNoCompress = 'y';
    }

    if( pLogSecNode->cNoCopy == 'y' &&
        pXMLStructNode->cNoCopy == 'y' ) {
        pXMLStructNode->cNoCopy = 'y';
    }

    if( pLogSecNode->cNoCopyTruncate == 'y' &&
        pXMLStructNode->cNoCopyTruncate != 'y' ) {
        pXMLStructNode->cNoCopyTruncate = 'y';
    }

    if( pLogSecNode->cNoCreate == 'y' &&
        pXMLStructNode->cNoCreate != 'y' ) {
        pXMLStructNode->cNoCreate = 'y';
    }

    if( pLogSecNode->cNoDelayCompress == 'y' &&
        pXMLStructNode->cNoDelayCompress != 'y' ) {
        pXMLStructNode->cNoDelayCompress = 'y';
    }
    
    if( pLogSecNode->cNoMail == 'y' &&
        pXMLStructNode->cNoMail != 'y' ) {
        pXMLStructNode->cNoMail = 'y';
    }

    if( pLogSecNode->cNoMissingOk == 'y' &&
        pXMLStructNode->cNoMissingOk != 'y' ) {
        pXMLStructNode->cNoMissingOk = 'y';
    }

    if( pLogSecNode->cNoOldDir == 'y' &&
        pXMLStructNode->cNoOldDir != 'y' ) {
        pXMLStructNode->cNoOldDir = 'y';
    }

    if( pLogSecNode->cNoSharedScripts == 'y' &&
        pXMLStructNode->cNoSharedScripts != 'y' ) {
        pXMLStructNode->cNoSharedScripts = 'y';
    }

    if( pLogSecNode->cNotIfempty == 'y' &&
        pXMLStructNode->cNotIfempty != 'y' ) {
        pXMLStructNode->cNotIfempty = 'y';
    }

    if( pLogSecNode->pszOldDir &&
        !pXMLStructNode->pszOldDir ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszOldDir);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszOldDir,
                pLogSecNode->pszOldDir);
    }

    if ( pLogSecNode->cSharedScripts == 'y' &&
         pXMLStructNode->cSharedScripts != 'y' ) {
        pXMLStructNode->cSharedScripts = 'y';
    }

    if( pLogSecNode->pszStart &&
        !pXMLStructNode->pszStart ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszStart);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszStart,
                pLogSecNode->pszStart);
    }

    if( pLogSecNode->pszTabooExt &&
        !pXMLStructNode->pszTabooExt ) {
        ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                    (PVOID *)&pXMLStructNode->pszTabooExt);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strcpy( pXMLStructNode->pszTabooExt,
                pLogSecNode->pszTabooExt);
    }

    return ceError;

error:

    FreeList( &pXMLStructNode );

    return ceError;

}

static
CENTERROR
ModifyLogEntries(
    PLOGROTATE_POLICY pXMLStructNode,
    PLOGROTATE_POLICY pLogSecNode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if( pXMLStructNode->pszRotate ) {
        
        if(pLogSecNode->pszRotate) {
    
            sprintf( pLogSecNode->pszRotate,
                     "%s",
                     pXMLStructNode->pszRotate);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszRotate);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszRotate,
                     "%s",
                     pXMLStructNode->pszRotate);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszRotate);
        pLogSecNode->pszRotate = NULL;
    }


    if( pXMLStructNode->pszMaxLogSize ) {
        
        if(pLogSecNode->pszMaxLogSize) {
    
            sprintf( pLogSecNode->pszMaxLogSize,
                     "%s",
                     pXMLStructNode->pszMaxLogSize);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszMaxLogSize);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszMaxLogSize,
                     "%s",
                     pXMLStructNode->pszMaxLogSize);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszMaxLogSize);
        pLogSecNode->pszMaxLogSize = NULL;
    }

    if( pXMLStructNode->pszLogFrequency ) {
        
        if(pLogSecNode->pszLogFrequency) {
    
            sprintf( pLogSecNode->pszLogFrequency,
                     "%s",
                     pXMLStructNode->pszLogFrequency);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszLogFrequency);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszLogFrequency,
                     "%s",
                     pXMLStructNode->pszLogFrequency);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszLogFrequency);
        pLogSecNode->pszLogFrequency = NULL;
    }

    if( pXMLStructNode->pszFileType ) {
        
        if(pLogSecNode->pszFileType) {
    
            sprintf( pLogSecNode->pszFileType,
                     "%s",
                     pXMLStructNode->pszFileType);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszFileType);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszFileType,
                     "%s",
                     pXMLStructNode->pszFileType);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszFileType);
        pLogSecNode->pszFileType = NULL;
    }

    if( pXMLStructNode->pszAppName ) {
        
        if(pLogSecNode->pszAppName) {
    
            sprintf( pLogSecNode->pszAppName,
                     "%s",
                     pXMLStructNode->pszAppName);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszAppName);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszAppName,
                     "%s",
                     pXMLStructNode->pszAppName);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszAppName);
        pLogSecNode->pszAppName = NULL;
    }

    if( pXMLStructNode->pszPreRotate ) {
        
        if(pLogSecNode->pszPreRotate) {
    
            sprintf( pLogSecNode->pszPreRotate,
                     "%s",
                     pXMLStructNode->pszPreRotate);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszPreRotate);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszPreRotate,
                     "%s",
                     pXMLStructNode->pszPreRotate);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszPreRotate);
        pLogSecNode->pszPreRotate = NULL;
    }

    if( pXMLStructNode->pszPostRotate ) {
        
        if(pLogSecNode->pszPostRotate) {
    
            sprintf( pLogSecNode->pszPostRotate,
                     "%s",
                     pXMLStructNode->pszPostRotate);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszPostRotate);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszPostRotate,
                     "%s",
                     pXMLStructNode->pszPostRotate);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszPostRotate);
        pLogSecNode->pszPostRotate = NULL;
    }

    if( pXMLStructNode->pszCompressCmd ) {
        
        if(pLogSecNode->pszCompressCmd) {
    
            sprintf( pLogSecNode->pszCompressCmd,
                     "%s",
                     pXMLStructNode->pszCompressCmd);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszCompressCmd);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszCompressCmd,
                     "%s",
                     pXMLStructNode->pszCompressCmd);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszCompressCmd);
        pLogSecNode->pszCompressCmd = NULL;
    }

    if( pXMLStructNode->pszUnCompressCmd ) {
        
        if(pLogSecNode->pszUnCompressCmd) {
    
            sprintf( pLogSecNode->pszUnCompressCmd,
                     "%s",
                     pXMLStructNode->pszUnCompressCmd);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszUnCompressCmd);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszUnCompressCmd,
                     "%s",
                     pXMLStructNode->pszUnCompressCmd);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszUnCompressCmd);
        pLogSecNode->pszUnCompressCmd = NULL;
    }

    if( pXMLStructNode->pszCompressExt ) {
        
        if(pLogSecNode->pszCompressExt) {
    
            sprintf( pLogSecNode->pszCompressExt,
                     "%s",
                     pXMLStructNode->pszCompressExt);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszCompressExt);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszCompressExt,
                     "%s",
                     pXMLStructNode->pszCompressExt);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszCompressExt);
        pLogSecNode->pszCompressExt = NULL;
    }

    if( pXMLStructNode->pszCompressOptions ) {
        
        if(pLogSecNode->pszCompressOptions) {
    
            sprintf( pLogSecNode->pszCompressOptions,
                     "%s",
                     pXMLStructNode->pszCompressOptions);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszCompressOptions);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszCompressOptions,
                     "%s",
                     pXMLStructNode->pszCompressOptions);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszCompressOptions);
        pLogSecNode->pszCompressOptions = NULL;
    }

    if ( pXMLStructNode->cCopy == 'y' ) {
        pLogSecNode->cCopy = 'y';
    }

    if ( pXMLStructNode->cNoCopy == 'y' ) {
        pLogSecNode->cNoCopy = 'y';
    }

    if( pXMLStructNode->pszCreate ) {
        
        if(pLogSecNode->pszCreate) {
    
            sprintf( pLogSecNode->pszCreate,
                     "%s",
                     pXMLStructNode->pszCreate);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszCreate);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszCreate,
                     "%s",
                     pXMLStructNode->pszCreate);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszCreate);
        pLogSecNode->pszCreate = NULL;
    }

    if( pXMLStructNode->pszExtension ) {
        
        if(pLogSecNode->pszExtension) {
    
            sprintf( pLogSecNode->pszExtension,
                     "%s",
                     pXMLStructNode->pszExtension);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszExtension);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszExtension,
                     "%s",
                     pXMLStructNode->pszExtension);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszExtension);
        pLogSecNode->pszExtension = NULL;
    }

    if( pXMLStructNode->pszFirstAction ) {
        
        if(pLogSecNode->pszFirstAction) {
    
            sprintf( pLogSecNode->pszFirstAction,
                     "%s",
                     pXMLStructNode->pszFirstAction);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszFirstAction);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszFirstAction,
                     "%s",
                     pXMLStructNode->pszFirstAction);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszFirstAction);
        pLogSecNode->pszFirstAction = NULL;
    }

    if( pXMLStructNode->pszInclude ) {
        
        if(pLogSecNode->pszInclude) {
    
            sprintf( pLogSecNode->pszInclude,
                     "%s",
                     pXMLStructNode->pszInclude);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszInclude);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszInclude,
                     "%s",
                     pXMLStructNode->pszInclude);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszInclude);
        pLogSecNode->pszInclude = NULL;
    }

    if( pXMLStructNode->pszLastAction ) {
        
        if(pLogSecNode->pszLastAction) {
    
            sprintf( pLogSecNode->pszLastAction,
                     "%s",
                     pXMLStructNode->pszLastAction);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszLastAction);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszLastAction,
                     "%s",
                     pXMLStructNode->pszLastAction);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszLastAction);
        pLogSecNode->pszLastAction = NULL;
    }

    if( pXMLStructNode->pszMail ) {
        
        if(pLogSecNode->pszMail) {
    
            sprintf( pLogSecNode->pszMail,
                     "%s",
                     pXMLStructNode->pszMail);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszMail);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszMail,
                     "%s",
                     pXMLStructNode->pszMail);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszMail);
        pLogSecNode->pszMail = NULL;
    }

    if( pXMLStructNode->pszOldDir ) {
        
        if(pLogSecNode->pszOldDir) {
    
            sprintf( pLogSecNode->pszOldDir,
                     "%s",
                     pXMLStructNode->pszOldDir);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszOldDir);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszOldDir,
                     "%s",
                     pXMLStructNode->pszOldDir);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszOldDir);
        pLogSecNode->pszOldDir = NULL;
    }

    if( pXMLStructNode->pszStart ) {
        
        if(pLogSecNode->pszStart) {
    
            sprintf( pLogSecNode->pszStart,
                     "%s",
                     pXMLStructNode->pszStart);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszStart);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszStart,
                     "%s",
                     pXMLStructNode->pszStart);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszStart);
        pLogSecNode->pszStart = NULL;
    }

    if( pXMLStructNode->pszTabooExt ) {
        
        if(pLogSecNode->pszTabooExt) {
    
            sprintf( pLogSecNode->pszTabooExt,
                     "%s",
                     pXMLStructNode->pszTabooExt);
        }
        else {
            ceError = LwAllocateMemory( STATIC_PATH_BUFFER_SIZE,
                                        (PVOID *)&pLogSecNode->pszTabooExt);
            BAIL_ON_CENTERIS_ERROR(ceError);

            sprintf( pLogSecNode->pszTabooExt,
                     "%s",
                     pXMLStructNode->pszTabooExt);
        }
    }
    else {
        LW_SAFE_FREE_STRING(pLogSecNode->pszTabooExt);
        pLogSecNode->pszTabooExt = NULL;
    }

    if ( pXMLStructNode->cCompress == 'y' ) {
        pLogSecNode->cCompress = 'y';
    }

    if ( pXMLStructNode->cCopyTruncate == 'y' ) {
        pLogSecNode->cCopyTruncate = 'y';
    }

    if ( pXMLStructNode->cDelayCompress == 'y' ) {
        pLogSecNode->cDelayCompress = 'y';
    }

    if ( pXMLStructNode->cMailFirst == 'y' ) {
        pLogSecNode->cMailFirst = 'y';
    }

    if ( pXMLStructNode->cMailLast == 'y' ) {
        pLogSecNode->cMailLast = 'y';
    }

    if ( pXMLStructNode->cMissingOk == 'y' ) {
        pLogSecNode->cMissingOk = 'y';
    }

    if ( pXMLStructNode->cNoCompress == 'y' ) {
        pLogSecNode->cNoCompress = 'y';
    }

    if ( pXMLStructNode->cNoCopyTruncate == 'y' ) {
        pLogSecNode->cNoCopyTruncate = 'y';
    }

    if ( pXMLStructNode->cNoCreate == 'y' ) {
        pLogSecNode->cNoCreate = 'y';
    }

    if ( pXMLStructNode->cNoDelayCompress == 'y' ) {
        pLogSecNode->cNoDelayCompress = 'y';
    }
    
    if ( pXMLStructNode->cNoMail == 'y' ) {
        pLogSecNode->cNoMail = 'y';
    }

    if ( pXMLStructNode->cNoMissingOk == 'y' ) {
        pLogSecNode->cNoMissingOk = 'y';
    }

    if ( pXMLStructNode->cNoOldDir == 'y' ) {
        pLogSecNode->cNoOldDir = 'y';
    }

    if ( pXMLStructNode->cNoSharedScripts == 'y' ) {
        pLogSecNode->cNoSharedScripts = 'y';
    }

    if ( pXMLStructNode->cNotIfempty == 'y' ) {
        pLogSecNode->cNotIfempty = 'y';
    }

    if ( pXMLStructNode->cSharedScripts == 'y' ) {
        pLogSecNode->cSharedScripts = 'y';
    }

    return ceError;

error:

    FreeList( &pLogSecNode );

    return ceError;

}

static
int
ValidateMultipleFilePath(
    PLOGROTATE_POLICY *ppXMLStructNode,
    PLOGROTATE_POLICY *ppLogSectionNode
)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PLOGFILEPATH pXMLFilePathNode = (*ppXMLStructNode)->pFilePath;
    PLOGFILEPATH pLogSecFilePathNode = (*ppLogSectionNode)->pFilePath;

    BOOLEAN bMatch = TRUE;

    for( ; pXMLFilePathNode && pLogSecFilePathNode; pXMLFilePathNode = pXMLFilePathNode->pNext, pLogSecFilePathNode = pLogSecFilePathNode->pNext ) {
        if( strcmp( pXMLFilePathNode->pszFile,
                    pLogSecFilePathNode->pszFile) != 0 ) {
            bMatch = FALSE;
        }
    }

    if( bMatch && (pXMLFilePathNode == NULL && pLogSecFilePathNode == NULL )) {
        ceError = ModifyLogEntries( *ppXMLStructNode,
                                    *ppLogSectionNode);
        BAIL_ON_CENTERIS_ERROR(ceError);
        return bMatch;
    }
    else {
        bMatch = FALSE;
        return bMatch;
    }

error:

    return ceError;

}

static
CENTERROR
ProcessMultipleFilePath(
    PLOGROTATE_POLICY *ppXMLStructNode,
    PLOGROTATE_POLICY *ppLogSectionNode,
    BOOLEAN *pbDone
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PLOGFILEPATH pXMLFilePathNode = NULL;
    PLOGFILEPATH pLogSecFilePathNode = NULL;

    BOOLEAN bSubFilePath = TRUE; 

    *pbDone = FALSE;

    for( pXMLFilePathNode = (*ppXMLStructNode)->pFilePath; pXMLFilePathNode; pXMLFilePathNode = pXMLFilePathNode->pNext ) {

        pLogSecFilePathNode = (*ppLogSectionNode)->pFilePath; 

        while(pLogSecFilePathNode) {

            if( !strcmp( pXMLFilePathNode->pszFile,
                         pLogSecFilePathNode->pszFile)) {

                if( (pLogSecFilePathNode->pNext == NULL && 
                    pXMLFilePathNode->pNext == NULL)  &&
                    !*pbDone && bSubFilePath ) {

                    ceError = DeleteFilePathNode( *ppLogSectionNode,
                                                  pLogSecFilePathNode->pszFile);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    ceError = CreateAndLinkNode( *ppXMLStructNode );
                    BAIL_ON_CENTERIS_ERROR(ceError); 
                }
                else if( pLogSecFilePathNode->pNext == NULL && 
                         !*pbDone && !bSubFilePath ) {

                    ceError = ModifyLogEntries( *ppXMLStructNode,
                                                *ppLogSectionNode);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    ceError = DeleteFilePathNode( *ppXMLStructNode,
                                                  pXMLFilePathNode->pszFile);
                    BAIL_ON_CENTERIS_ERROR(ceError);                

                    ceError = CreateAndLinkNode( *ppXMLStructNode );
                    BAIL_ON_CENTERIS_ERROR(ceError); 
                }
                else {
                    ceError = DeleteFilePathNode( *ppLogSectionNode,
                                                  pLogSecFilePathNode->pszFile);
                    BAIL_ON_CENTERIS_ERROR(ceError);                

                   *pbDone = TRUE;
                }

                break;
            }
            else {
                if( (pLogSecFilePathNode->pNext == NULL && 
                    pXMLFilePathNode->pNext != NULL)  &&
                         !*pbDone ) {
                   bSubFilePath = FALSE; 
                } 
                pLogSecFilePathNode = pLogSecFilePathNode->pNext;
            }
        }
    }

    if( *pbDone ) {

        ceError = AddEntriesToXMLList(  *ppXMLStructNode,
                                        *ppLogSectionNode);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CreateAndLinkNode( *ppXMLStructNode );
        BAIL_ON_CENTERIS_ERROR(ceError);

        *pbDone = TRUE;
    }

error:

    return ceError;

}

static
int
FindSubFilePathEntry(
    PLOGROTATE_POLICY *ppXMLStructNode,
    PLOGROTATE_POLICY *ppLogSectionNode
    )
{

    BOOLEAN bMatch = FALSE;

    PLOGFILEPATH pXMLFilePathNode = (*ppXMLStructNode)->pFilePath;
    PLOGFILEPATH pLogSecFilePathNode = (*ppLogSectionNode)->pFilePath;

    for( ; pLogSecFilePathNode; pLogSecFilePathNode = pLogSecFilePathNode->pNext ) {

        for( ; pXMLFilePathNode; pXMLFilePathNode = pXMLFilePathNode->pNext ) {

            if( !strcmp( pXMLFilePathNode->pszFile,
                         pLogSecFilePathNode->pszFile)) {
                bMatch = TRUE;
            }
        }
    }

    return bMatch;

}

static
CENTERROR
ComputeModifiedEntries(
    PSTR pszFileType
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    BOOLEAN bFilePathFound = FALSE;
    BOOLEAN bDone = FALSE;

    PLOGROTATE_POLICY pXMLStructNode = NULL;
    PLOGROTATE_POLICY pLogSectionNode = NULL;

    for( pXMLStructNode = pXMLStructHead; pXMLStructNode; pXMLStructNode = pXMLStructNode->pNext) {

        bDone = FALSE;
        bFilePathFound = FALSE;

        if( pXMLStructNode->cDone == 'y' ) {
            continue;
        }

        for( pLogSectionNode = pLogSectionHead; pLogSectionNode; pLogSectionNode = pLogSectionNode->pNext ) {

            if( (!strcmp( pszFileType,
                          pXMLStructNode->pszFileType)  &&
                !strcmp( pXMLStructNode->pszAppName,
                         pLogSectionNode->pszAppName))) {

                pXMLStructNode->cDone = 'y';

                if( pLogSectionNode->pFilePath->pNext != NULL ) {

                    if( ValidateMultipleFilePath( &pXMLStructNode,
                                                  &pLogSectionNode)) {
                        bDone = TRUE;
                        break;
                    }

                    ceError = ProcessMultipleFilePath( &pXMLStructNode,
                                                       &pLogSectionNode,
                                                       &bDone);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
                else if( !strcmp( pLogSectionNode->pFilePath->pszFile,
                                  pXMLStructNode->pFilePath->pszFile)) {
                
                    ceError = ModifyLogEntries( pXMLStructNode,
                                                pLogSectionNode);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    bFilePathFound = TRUE;
                }
                else if( FindSubFilePathEntry( &pXMLStructNode,
                                               &pLogSectionNode )) {

                    ceError = ProcessMultipleFilePath( &pXMLStructNode,
                                                       &pLogSectionNode,
                                                       &bDone);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

                if( bDone ) {
                    break;
                }
            }
            else {
                continue;
            }
        } 

        if( (!bFilePathFound &&
            !strcmp( pszFileType,
                     pXMLStructNode->pszFileType)) &&
            !bDone) {
            ceError = CreateAndLinkNode( pXMLStructNode );
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

error:

    return ceError;

}

static 
CENTERROR
ParseXMLStruct()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    CHAR szAppFilePath[STATIC_PATH_BUFFER_SIZE];
    CHAR szAppFilePathGP[STATIC_PATH_BUFFER_SIZE];

    PLOGROTATE_POLICY pXMLStructNode = NULL;

    BOOLEAN bFileExists = FALSE;

    BOOLEAN bDoneLogrotate = FALSE;

    FILE *fpGp = NULL;

    for( pXMLStructNode = pXMLStructHead; pXMLStructNode; pXMLStructNode = pXMLStructNode->pNext ) {

        if( !strcmp( pXMLStructNode->pszFileType,
                     LWI_LOG_ROTATE) &&
            !bDoneLogrotate ) {
        
            ceError = BackupOriginalFile();
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = GPACheckFileExists( LWI_LOGROTATE_CONF_FILE, 
                                         &bFileExists );
            BAIL_ON_CENTERIS_ERROR(ceError);

            if(bFileExists) {

                ceError = ParseConfFile( LWI_LOGROTATE_CONF_FILE,
                                         LWI_LOG_ROTATE);
                BAIL_ON_CENTERIS_ERROR(ceError);       

                if( pLogSectionHead != NULL ) {
                    ceError = ComputeModifiedEntries( pXMLStructNode->pszFileType );
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
            }
            else {
                ceError = GPAOpenFile( LWI_LOGROTATE_CONF_FILE_GP, 
                                      "w",
                                      &fpGp);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ceError = GPAFilePrintf( fpGp,
                                        "%s\n", 
                                        LWI_LOGROTATE_POLICY_FILE_HEADER);
                BAIL_ON_CENTERIS_ERROR(ceError);

                if( fpGp ) {
                    fclose(fpGp);
                }
            }
    
            ceError = WriteLogrotateSettings( LWI_LOGROTATE_CONF_FILE_GP,
                                              LWI_LOG_ROTATE);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = GPAMoveFileAcrossDevices( LWI_LOGROTATE_CONF_FILE_GP,
                                               LWI_LOGROTATE_CONF_FILE);
            BAIL_ON_CENTERIS_ERROR(ceError);

            FreeList( &pLogSectionHead );

            bDoneLogrotate = TRUE;
        }
        else if( !strcmp( pXMLStructNode->pszFileType,
                          LWI_LOG_ROTATE_D)) {

            if( pXMLStructNode->cDone == 'y' ) {
                continue;
            }

            ceError = GPACheckDirectoryExists( LWI_LOGROTATED_DIR_PATH,
                                              &bFileExists);
            BAIL_ON_CENTERIS_ERROR(ceError);
          
            if( !bFileExists ) {
                ceError = LwCreateDirectory( LWI_LOGROTATED_DIR_PATH,
                                             DIR_PERMS);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            sprintf( szAppFilePath,
                     "%s/%s",
                     LWI_LOGROTATED_DIR_PATH,
                     pXMLStructNode->pszAppName);

            sprintf( szAppFilePathGP,
                     "%s/%s.gp",
                     LWI_LOGROTATED_DIR_PATH,
                     pXMLStructNode->pszAppName);

            ceError = GPACheckFileExists( szAppFilePath, 
                                         &bFileExists );
            BAIL_ON_CENTERIS_ERROR(ceError);

            if( bFileExists ) {
                ceError = ParseConfFile( szAppFilePath,
                                         pXMLStructNode->pszAppName);
                BAIL_ON_CENTERIS_ERROR(ceError);       
                                                
                if( pLogSectionHead != NULL ) {
                    ceError = ComputeModifiedEntries( pXMLStructNode->pszFileType ); 
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
            }
            else {
                ceError = GPAOpenFile( szAppFilePathGP, 
                                      "w",
                                      &fpGp);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ceError = GPAFilePrintf( fpGp,
                                        "%s\n", 
                                        LWI_LOGROTATE_POLICY_FILE_HEADER);
                BAIL_ON_CENTERIS_ERROR(ceError);

                if( fpGp ) {
                    fclose(fpGp);
                }
            }
    
            ceError = WriteLogrotateSettings( szAppFilePathGP,
                                              pXMLStructNode->pszAppName);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = GPAMoveFileAcrossDevices( szAppFilePathGP,
                                               szAppFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            FreeList( &pLogSectionHead );
        }        
    } 

error:

      return ceError;  

}

static
CENTERROR
ApplyLwiLogRotatePolicy(
    PGPOLWIGPITEM pGPRsopItem,
    BOOLEAN bGPModified
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if ( bGPModified ) {
        ceError = ReadXMLTree( (xmlNodePtr)pGPRsopItem->xmlNode);
        BAIL_ON_CENTERIS_ERROR(ceError);

        GPA_LOG_VERBOSE( "Applying logrotate policy ...");

        ceError = ParseXMLStruct();
        BAIL_ON_CENTERIS_ERROR(ceError);

        FreeList( &pXMLStructHead );
    }

error:

    return ceError;
}

CENTERROR
ResetLogRotateGroupPolicy()
{
    return CENTERROR_SUCCESS;
}

CENTERROR
ResetLogRotateConfFile()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    BOOLEAN bFileExists = FALSE;

    ceError = GPACheckFileExists( LWI_LOGROTATE_CENTERIS_GP_DIRECTORY,
                                 &bFileExists );
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( bFileExists ) {
        GPA_LOG_VERBOSE( "Reverting %s to it's original version.",
                         LWI_LOGROTATE_CONF_FILE);

        ceError = GPACopyFileWithOriginalPerms( LWI_LOGROTATE_CENTERIS_GP_DIRECTORY,
                                               LWI_LOGROTATE_CONF_FILE );
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwRemoveFile( LWI_LOGROTATE_CENTERIS_GP_DIRECTORY );
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;

}

CENTERROR
ProcessLogRotateGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = 0;
    BOOLEAN bSomeGPModified = FALSE;
    PGPOLWIDATA pLwidata = NULL;
    PGPOLWIDATALIST pLwidataList = NULL;
    PGPOLWIGPITEM pGPLogRotateItem = NULL;
    PGPOLWIGPITEM pRsopGPLogRotateItem = NULL;

    GPA_LOG_FUNCTION_ENTER();
    
    bSomeGPModified = ( pGPODeletedList != NULL );

    while (pGPOModifiedList) {
        BOOLEAN applicable;

        if (pGPOModifiedList->bNewVersion) {
            bSomeGPModified = TRUE;
        }
        
        ceError = GPOXmlVerifyPlatformApplicable( NULL,
                                                  MACHINE_GROUP_POLICY,
                                                  pGPOModifiedList,
                                                  NULL,
                                                  &applicable);

        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if (!applicable) {
            GPA_LOG_VERBOSE("GPO(%s) disabled by platform targeting",
                            pGPOModifiedList->pszDisplayName);
            ceError = ResetLogRotateConfFile();
            BAIL_ON_CENTERIS_ERROR(ceError);
        } 
        else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE("Ignoring GPO(%s) disabled by flags: 0x%.08x",
                            pGPOModifiedList->pszDisplayName,
                            pGPOModifiedList->dwFlags);
            ceError = ResetLogRotateConfFile();
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else {
            // Look in new policy file location for Auth settings
            ceError = GPOInitLwiData( NULL,
                                      MACHINE_GROUP_POLICY,
                                      (PGPOLWIDATA*)&pLwidata,
                                      pGPOModifiedList->pszgPCFileSysPath,
                                      NULL,
                                      LWILOGROTATE_CLIENT_GUID);
            if ( ceError ) {
                // Look in old policy file location
                ceError = GPOInitLwiData( NULL,
                                          MACHINE_GROUP_POLICY,
                                          (PGPOLWIDATA*)&pLwidata,
                                          pGPOModifiedList->pszgPCFileSysPath,
                                          NULL,
                                          NULL );
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
            // fetch any logrotate settings 
            ceError = GPOGetGPItem( MACHINE_GROUP_POLICY,
                                    pLwidata,
                                    LWILOGROTATE_ITEM_GUID,
                                    &pGPLogRotateItem );
            if (!CENTERROR_IS_OK(ceError) &&
                CENTERROR_EQUAL(ceError, CENTERROR_GP_XML_GPITEM_NOT_FOUND)) {
                ceError = CENTERROR_SUCCESS;
            }
            BAIL_ON_CENTERIS_ERROR(ceError);                
        }
      
        // merge with any previous GP settings
        if ( pGPLogRotateItem ) {

            if ( pRsopGPLogRotateItem ) {
                /* pGPLogonItem is modified to include items from pRsopGPLogonItem */
                ceError = GPOCalculateRSOP( pRsopGPLogRotateItem,
                                            pGPLogRotateItem );
                BAIL_ON_CENTERIS_ERROR(ceError);

                if(pRsopGPLogRotateItem) {
                    GPODestroyGPItem( pRsopGPLogRotateItem,
                                      FALSE );
                }

                pRsopGPLogRotateItem = NULL;
            }

            /* the current pRsopGPLogRotateItem has all RSOP changes */
            pRsopGPLogRotateItem = pGPLogRotateItem;
            pGPLogRotateItem = NULL;
        }

        // Hold on to each Lwidata, so that it can be reference for the life of this function
        if ( pLwidata ) {
            PGPOLWIDATALIST pTemp = pLwidataList;
            PGPOLWIDATALIST pPrev = NULL;
            PGPOLWIDATALIST pNew = NULL;

            ceError = LwAllocateMemory( sizeof(GPOLWIDATALIST),
                                        (PVOID*)&pNew);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pNew->pLwidata = pLwidata;

            // Insert the new node with its lwidata object to list (if any)
            while (pTemp) {
                pPrev = pTemp;
                pTemp = pTemp->pNext;
            }

            if (pPrev) {
                pPrev->pNext = pNew;
            }
            else {
                pLwidataList = pNew;
            }

            pLwidata = NULL;
        }

        pGPOModifiedList = pGPOModifiedList->pNext;
    }   
   
     // apply logrotate stuff
    if ( pRsopGPLogRotateItem ) {
        //Reset before applying policy. Fix for Bug#7475
        ceError = ResetLogRotateConfFile( );
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = ApplyLwiLogRotatePolicy( pRsopGPLogRotateItem,
                                           bSomeGPModified);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else {

        ceError = ResetLogRotateConfFile();
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
 
error:
 
    if(pRsopGPLogRotateItem) {
        GPODestroyGPItem( pRsopGPLogRotateItem, 
                          FALSE );
    }

    // Now free any Lwidata settings we have used.
    while ( pLwidataList ) {
        PGPOLWIDATALIST pTemp = pLwidataList;
        pLwidataList = pLwidataList->pNext;

        GPODestroyLwiData(pTemp->pLwidata);
        LwFreeMemory(pTemp);
    }

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return(ceError);
}
