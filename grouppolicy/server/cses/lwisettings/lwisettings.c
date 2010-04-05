#include "includes.h"

CENTERROR
SignalLwiauthd()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bRunning = FALSE;
    pid_t pid = 0;

    ceError = GPAIsProgramRunning( LWI_CSE_AUTH_PID_FILE,
                                  LWI_CSE_AUTH_DAEMON_NAME,
                                  &pid,
                                  &bRunning );
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( bRunning ) {
        GPA_LOG_VERBOSE( "Sending SIGHUP to %s pid=%d",
                         LWI_CSE_AUTH_DAEMON_NAME,
                         pid );

        ceError = GPASendSignal( pid,
                                1);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;
}

CENTERROR
ResetLikewiseSettingsGroupPolicy(
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* There is nothing to reset for this policy */

    return ceError;
}

CENTERROR
ResetLWEDSPluginSettingsGroupPolicy(
    PGPUSER pUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* There is nothing to reset for this policy */

    return ceError;
}

static
CENTERROR
ComputeLWSettingsRSOP(
    PGPOLWIGPITEM *ppGPAuthItem,
    PGPOLWIGPITEM *ppRsopGPAuthItem,
    PGPOLWIGPITEM *ppGPLogonItem,
    PGPOLWIGPITEM *ppRsopGPLogonItem,
    PGPOLWIGPITEM *ppGPSettingsItem,
    PGPOLWIGPITEM *ppRsopGPSettingsItem,
    PGPOLWIGPITEM *ppEVTSettingsItem,
    PGPOLWIGPITEM *ppRsopEVTSettingsItem,
    PGPOLWIGPITEM *ppEVTFWDSettingsItem,
    PGPOLWIGPITEM *ppRsopEVTFWDSettingsItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* Merge with any previous GP settings */
    if ( *ppGPAuthItem ) {

        if ( *ppRsopGPAuthItem ) {
            ceError = GPOCalculateRSOP( *ppRsopGPAuthItem,
                                        *ppGPAuthItem );
            BAIL_ON_CENTERIS_ERROR(ceError);

            GPODestroyGPItem( *ppRsopGPAuthItem,
                              FALSE );
            *ppRsopGPAuthItem = NULL;
        }

        *ppRsopGPAuthItem = *ppGPAuthItem;
        *ppGPAuthItem = NULL;
    }

    if ( *ppGPLogonItem ) {

        if ( *ppRsopGPLogonItem ) {
            ceError = GPOCalculateRSOP( *ppRsopGPLogonItem,
                                        *ppGPLogonItem );
            BAIL_ON_CENTERIS_ERROR(ceError);

            GPODestroyGPItem( *ppRsopGPLogonItem,
                              FALSE );
            *ppRsopGPLogonItem = NULL;
        }

        *ppRsopGPLogonItem = *ppGPLogonItem;
        *ppGPLogonItem = NULL;
    }

    if ( *ppGPSettingsItem ) {

        if ( *ppRsopGPSettingsItem ) {
            ceError = GPOCalculateRSOP( *ppRsopGPSettingsItem,
                                        *ppGPSettingsItem );
            BAIL_ON_CENTERIS_ERROR(ceError);

            GPODestroyGPItem( *ppRsopGPSettingsItem,
                              FALSE );
            *ppRsopGPSettingsItem = NULL;
        }

        *ppRsopGPSettingsItem = *ppGPSettingsItem;
        *ppGPSettingsItem = NULL;
    }

    if ( *ppEVTSettingsItem ) {

        if ( *ppRsopEVTSettingsItem ) {
            ceError = GPOCalculateRSOP( *ppRsopEVTSettingsItem,
                                        *ppEVTSettingsItem );
            BAIL_ON_CENTERIS_ERROR(ceError);

            GPODestroyGPItem( *ppRsopEVTSettingsItem,
                              FALSE );
            *ppRsopEVTSettingsItem = NULL;
        }

        *ppRsopEVTSettingsItem = *ppEVTSettingsItem;
        *ppEVTSettingsItem = NULL;
    }

    if ( *ppEVTFWDSettingsItem ) {

        if ( *ppRsopEVTFWDSettingsItem ) {
            ceError = GPOCalculateRSOP( *ppRsopEVTFWDSettingsItem,
                                        *ppEVTFWDSettingsItem );
            BAIL_ON_CENTERIS_ERROR(ceError);

            GPODestroyGPItem( *ppRsopEVTFWDSettingsItem,
                              FALSE );
            *ppRsopEVTFWDSettingsItem = NULL;
        }

        *ppRsopEVTFWDSettingsItem = *ppEVTFWDSettingsItem;
        *ppEVTFWDSettingsItem = NULL;
    }
error:

    return ceError;
}

static
CENTERROR
ComputeLWEDSPluginRSOP(
    PGPOLWIGPITEM *ppLWEDSPluginSettingsItem,
    PGPOLWIGPITEM *ppRsopLWEDSPluginSettingsItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* Merge with any previous GP settings */
    if ( *ppLWEDSPluginSettingsItem ) {

        if ( *ppRsopLWEDSPluginSettingsItem ) {
            ceError = GPOCalculateRSOP( *ppRsopLWEDSPluginSettingsItem,
                                        *ppLWEDSPluginSettingsItem );
            BAIL_ON_CENTERIS_ERROR(ceError);

            GPODestroyGPItem( *ppRsopLWEDSPluginSettingsItem,
                              FALSE );
            *ppRsopLWEDSPluginSettingsItem = NULL;
        }

        *ppRsopLWEDSPluginSettingsItem = *ppLWEDSPluginSettingsItem;
        *ppLWEDSPluginSettingsItem = NULL;
    }

error:

    return ceError;
}

static
CENTERROR
GetGPItem(
    PGPOLWIDATA   *ppLwidata,
    PGPOLWIGPITEM *ppGPItem,
    PSTR GP_ITEM_GUID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* Fetch settings */
    ceError = GPOGetGPItem( MACHINE_GROUP_POLICY,
                            *ppLwidata,
                            GP_ITEM_GUID,
                            ppGPItem);
    if (!CENTERROR_IS_OK(ceError) &&
        CENTERROR_EQUAL( ceError,
                         CENTERROR_GP_XML_GPITEM_NOT_FOUND)) {
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
ApplyLWSettingsPolicy(
    PGPOLWIGPITEM *ppRsopGPAuthItem,
    PGPOLWIGPITEM *ppRsopGPLogonItem,
    PGPOLWIGPITEM *ppRsopGPSettingsItem,
    PGPOLWIGPITEM *ppRsopEVTSettingsItem,
    PGPOLWIGPITEM *ppRsopEVTFWDSettingsItem
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* Apply AuthD settings */
    ceError = ProcessLsassSettingsMode( *ppRsopGPAuthItem,
                                        *ppRsopGPLogonItem);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Apply GP settings */
    ceError = ApplyLwiGPSettingsPolicy( ppRsopGPSettingsItem );
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Apply EVT settings */
    ceError = ApplyLwiEVTSettingsPolicy( ppRsopEVTSettingsItem );
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ApplyLwiEVTFWDSettingsPolicy( ppRsopEVTFWDSettingsItem );
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
ApplyLWEDSPluginPolicy(
    PGPOLWIGPITEM *ppRsopLWEDSPluginSettingsItem,
    BOOLEAN bGPModified
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* Apply LWEDSPlugin settings */
    ceError = ApplyLWEDSPluginSettingsPolicy( *ppRsopLWEDSPluginSettingsItem);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
InitLWSettingsData(
    PGPOLWIDATA   *ppLweData,
    PGPOLWIGPITEM *ppGPAuthItem,
    PGPOLWIGPITEM *ppGPLogonItem,
    PGPOLWIGPITEM *ppGPSettingsItem,
    PGPOLWIGPITEM *ppEVTSettingsItem,
    PGPOLWIGPITEM *ppEVTFWDSettingsItem,
    PGROUP_POLICY_OBJECT pGPOModifiedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = GPOInitLwiData( NULL,
                              MACHINE_GROUP_POLICY,
                              ppLweData,
                              pGPOModifiedList->pszgPCFileSysPath,
                              NULL,
                              LWISETTINGS_GUID);

    if (ceError == CENTERROR_SUCCESS)
    {
        ceError = GetGPItem( ppLweData,
                             ppGPAuthItem,
                             GPITEM_GUID_LWIAUTHD);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GetGPItem( ppLweData,
                             ppGPLogonItem,
                             GPITEM_GUID_LWILOGON);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GetGPItem( ppLweData,
                             ppGPSettingsItem,
                             LWIGPSETTINGS_ITEM_GUID);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GetGPItem( ppLweData,
                             ppEVTSettingsItem,
                             LWIEVTSETTINGS_ITEM_GUID);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GetGPItem( ppLweData,
                             ppEVTFWDSettingsItem,
                             LWIEVTFWD_ITEM_GUID);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = CENTERROR_SUCCESS;
    }

error:

    return ceError;
}

static
CENTERROR
InitLWEDSPluginData(
    PGPOLWIDATA   *ppLWEDSPluginData,
    PGPOLWIGPITEM *ppLWEDSPluginSettingsItem,
    PGROUP_POLICY_OBJECT pGPOModifiedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

#if defined(__LWI_DARWIN__)
    ceError = GPOInitLwiData( NULL,
                              MACHINE_GROUP_POLICY,
                              ppLWEDSPluginData,
                              pGPOModifiedList->pszgPCFileSysPath,
                              NULL,
                              LWEDSPLUGIN_SETTINGS_CLIENT_GUID);
    if (ceError == CENTERROR_SUCCESS)
    {
        ceError = GetGPItem( ppLWEDSPluginData,
                             ppLWEDSPluginSettingsItem,
                             LWEDSPLUGIN_SETTINGS_ITEM_GUID);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = CENTERROR_SUCCESS;
    }
#else
    BAIL_ON_CENTERIS_ERROR(ceError);
#endif

error:

    return ceError;
}

static
CENTERROR
CacheLwiData(
    PGPOLWIDATA *ppLwidata,
    PGPOLWIDATALIST *ppLwidataList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOLWIDATALIST pTemp = *ppLwidataList;
    PGPOLWIDATALIST pPrev = NULL;
    PGPOLWIDATALIST pNew = NULL;

    ceError = LwAllocateMemory( sizeof(GPOLWIDATALIST),
                                (PVOID*)&pNew);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pNew->pLwidata = *ppLwidata;

    // Insert the new node with its lwidata object to list (if any)
    while (pTemp) {
        pPrev = pTemp;
        pTemp = pTemp->pNext;
    }

    if (pPrev) {
        pPrev->pNext = pNew;
    } else {
        *ppLwidataList = pNew;
    }
    *ppLwidata = NULL;

error:

    return ceError;
}

CENTERROR
ProcessLikewiseSettingsGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN   bSomeGPModified = FALSE;

    PGPOLWIDATA pLweData = NULL;
    PGPOLWIDATALIST pLweDataList = NULL;

    PGPOLWIGPITEM pGPAuthItem = NULL;
    PGPOLWIGPITEM pRsopGPAuthItem = NULL;

    PGPOLWIGPITEM pGPLogonItem = NULL;
    PGPOLWIGPITEM pRsopGPLogonItem = NULL;

    PGPOLWIGPITEM pGPSettingsItem = NULL;
    PGPOLWIGPITEM pRsopGPSettingsItem = NULL;

    PGPOLWIGPITEM pEVTSettingsItem = NULL;
    PGPOLWIGPITEM pRsopEVTSettingsItem = NULL;

    PGPOLWIGPITEM pEVTFWDSettingsItem = NULL;
    PGPOLWIGPITEM pRsopEVTFWDSettingsItem = NULL;

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
            GPA_LOG_VERBOSE( "GPO(%s) disabled by platform targeting",
                             pGPOModifiedList->pszDisplayName);
        } else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE( "Ignoring GPO(%s) disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);
        } else {
            ceError = InitLWSettingsData(&pLweData,
                                         &pGPAuthItem,
                                         &pGPLogonItem,
                                         &pGPSettingsItem,
                                         &pEVTSettingsItem,
                                         &pEVTFWDSettingsItem,
                                         pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = ComputeLWSettingsRSOP(&pGPAuthItem,
                                        &pRsopGPAuthItem,
                                        &pGPLogonItem,
                                        &pRsopGPLogonItem,
                                        &pGPSettingsItem,
                                        &pRsopGPSettingsItem,
                                        &pEVTSettingsItem,
                                        &pRsopEVTSettingsItem,
                                        &pEVTFWDSettingsItem,
                                        &pRsopEVTFWDSettingsItem);
        BAIL_ON_CENTERIS_ERROR(ceError);

        // Hold on to each LweData, so that it can be reference for the life of this function
        if ( pLweData ) {
            ceError = CacheLwiData( &pLweData,
                                    &pLweDataList);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        pGPOModifiedList = pGPOModifiedList->pNext;
    }

    if ( bSomeGPModified )
    {
        ceError = ApplyLWSettingsPolicy(&pRsopGPAuthItem,
                                        &pRsopGPLogonItem,
                                        &pRsopGPSettingsItem,
                                        &pRsopEVTSettingsItem,
                                        &pRsopEVTFWDSettingsItem);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if ( pGPAuthItem ) {
        GPODestroyGPItem( pGPAuthItem,
                          FALSE );
    }

    if ( pRsopGPAuthItem ) {
        GPODestroyGPItem( pRsopGPAuthItem,
                          FALSE );
    }

    if ( pGPLogonItem ) {
        GPODestroyGPItem( pGPLogonItem,
                          FALSE );
    }

    if ( pRsopGPLogonItem ) {
        GPODestroyGPItem( pRsopGPLogonItem,
                          FALSE );
    }

    if ( pGPSettingsItem ) {
        GPODestroyGPItem( pGPSettingsItem,
                          FALSE );
    }

    if ( pRsopGPSettingsItem ) {
        GPODestroyGPItem( pRsopGPSettingsItem,
                          FALSE );
    }

    if ( pEVTSettingsItem ) {
        GPODestroyGPItem( pEVTSettingsItem,
                          FALSE );
    }

    if ( pRsopEVTSettingsItem ) {
        GPODestroyGPItem( pRsopEVTSettingsItem,
                          FALSE );
    }

    if ( pEVTFWDSettingsItem ) {
        GPODestroyGPItem( pEVTFWDSettingsItem,
                          FALSE );
    }

    if ( pRsopEVTFWDSettingsItem ) {
        GPODestroyGPItem( pRsopEVTFWDSettingsItem,
                          FALSE );
    }

    // Now free any Lwidata settings we have used.
    while ( pLweDataList ) {
        PGPOLWIDATALIST pTemp = pLweDataList;
        pLweDataList = pLweDataList->pNext;

        GPODestroyLwiData(pTemp->pLwidata);
        LwFreeMemory(pTemp);
    }

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return(ceError);
}

CENTERROR
ProcessLWEDSPluginSettingsGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN   bSomeGPModified = FALSE;

    PGPOLWIDATA pLWEDSPluginData = NULL;
    PGPOLWIDATALIST pLweDataList = NULL;

    PGPOLWIGPITEM pLWEDSPluginSettingsItem = NULL;
    PGPOLWIGPITEM pRsopLWEDSPluginSettingsItem = NULL;

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
            GPA_LOG_VERBOSE( "GPO(%s) disabled by platform targeting",
                             pGPOModifiedList->pszDisplayName);
        } else if (pGPOModifiedList->dwFlags & 0x00000002) {
            GPA_LOG_VERBOSE( "Ignoring GPO(%s) disabled by flags: 0x%.08x",
                             pGPOModifiedList->pszDisplayName,
                             pGPOModifiedList->dwFlags);
        } else {
            ceError = InitLWEDSPluginData(&pLWEDSPluginData,
                                          &pLWEDSPluginSettingsItem,
                                          pGPOModifiedList);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        ceError = ComputeLWEDSPluginRSOP(&pLWEDSPluginSettingsItem,
                                         &pRsopLWEDSPluginSettingsItem);
        BAIL_ON_CENTERIS_ERROR(ceError);

        // Hold on to each LWEDSPlugin data, so that it can be reference for the life of this function
        if ( pLWEDSPluginData ) {
            ceError = CacheLwiData( &pLWEDSPluginData,
                                    &pLweDataList);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        pGPOModifiedList = pGPOModifiedList->pNext;
    }

    ceError = ApplyLWEDSPluginPolicy(&pRsopLWEDSPluginSettingsItem,
                                     bSomeGPModified);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if ( pLWEDSPluginSettingsItem ) {
        GPODestroyGPItem( pLWEDSPluginSettingsItem,
                          FALSE );
    }

    if ( pRsopLWEDSPluginSettingsItem ) {
        GPODestroyGPItem( pRsopLWEDSPluginSettingsItem,
                          FALSE );
    }

    // Now free any Lwidata settings we have used.
    while ( pLweDataList ) {
        PGPOLWIDATALIST pTemp = pLweDataList;
        pLweDataList = pLweDataList->pNext;

        GPODestroyLwiData(pTemp->pLwidata);
        LwFreeMemory(pTemp);
    }

    GPA_LOG_FUNCTION_LEAVE(ceError);

    return(ceError);
}

