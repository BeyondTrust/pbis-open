#include "includes.h"

GPOCSEINFO gCSEInfo;

void
InitCSEGlobals()
{
    pthread_rwlock_init(&gCSEInfo.lock, NULL);
    gCSEInfo.pGPCSEList = NULL;
}

CENTERROR
LoadLibraryInformation(
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientSideExts
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszExtensionLibPath = NULL;
    PCSTR pszError = NULL;
    CHAR szLibPath[PATH_MAX+1];
    PGROUP_POLICY_CLIENT_EXTENSION pExtension = pGPClientSideExts;

    ceError = GetExtensionLibPath(&pszExtensionLibPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while (pExtension != NULL) {
        if ((pExtension->pszDllName != NULL &&
             *(pExtension->pszDllName) != '\0') &&
            (pExtension->pszProcessGroupPolicyFunction != NULL &&
             *(pExtension->pszProcessGroupPolicyFunction) != '\0')) {

            strcpy(szLibPath, pszExtensionLibPath);
            strcat(szLibPath, PATH_SEPARATOR_STR);
            strcat(szLibPath, pExtension->pszDllName);

            if (pExtension->dlHandle != NULL) {
                dlclose(pExtension->dlHandle);
                pExtension->dlHandle = NULL;
            }

            pExtension->dlHandle = dlopen(szLibPath, RTLD_LAZY);
            if (pExtension->dlHandle == NULL) {
                pszError = dlerror();
                GPA_LOG_ERROR(
                    "Failed to load library [%s]. Error [%s]",
                    szLibPath,
                    (IsNullOrEmptyString(pszError) ? "" : pszError));
                goto try_next_ext;
            }

            dlerror(); /* Clear any existing error */
            pExtension->pfnProcessGroupPolicy =
                (PFNPROCESSGROUPPOLICY)dlsym(
                    pExtension->dlHandle,
                    pExtension->pszProcessGroupPolicyFunction
                    );
            pszError = dlerror();
            if (!IsNullOrEmptyString(pszError)) {
                GPA_LOG_ERROR("Failed to load symbol [%s] from library [%s]. Error [%s]",
                              pExtension->pszProcessGroupPolicyFunction,
                              szLibPath,
                              pszError);
                ceError = CENTERROR_NO_SUCH_SYMBOL;
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            dlerror(); /* Clear any existing error */
            pExtension->pfnResetGroupPolicy =
                (PFNRESETGROUPPOLICY)dlsym(
                    pExtension->dlHandle,
                    pExtension->pszResetGroupPolicyFunction
                    );
            pszError = dlerror();
            if (!IsNullOrEmptyString(pszError)) {
                GPA_LOG_ERROR("Failed to load symbol [%s] from library [%s]. Error [%s]",
                              pExtension->pszResetGroupPolicyFunction,
                              szLibPath,
                              pszError);
                ceError = CENTERROR_NO_SUCH_SYMBOL;
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            /* Call loaded extension so that it can initialize it's state */
            ceError = pExtension->pfnResetGroupPolicy(NULL);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

    try_next_ext:

        pExtension = pExtension->pNext;
    }

error:

    if (pszExtensionLibPath) {
        LwFreeString(pszExtensionLibPath);
    }
    return (ceError);
}

void
DestroyClientSideExtensions(
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientSideExts)
{
    PGROUP_POLICY_CLIENT_EXTENSION pExtension = pGPClientSideExts;
    while (pExtension != NULL) {

        if (pExtension->dlHandle != NULL) {
            dlclose(pExtension->dlHandle);
        }

        pExtension = pExtension->pNext;
    }

    FreeClientExtensionList(pGPClientSideExts);
}

CENTERROR
LoadClientSideExtensions()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientSideExts= NULL;
    PSTR pszConfigFilePath = NULL;

    GPA_LOG_INFO("Loading client side extensions");
#if 0
    ceError = GetConfigPath(&pszConfigFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ParseConfigurationFile(
        pszConfigFilePath,
        &pGPClientSideExts
        );
#endif
    ceError = GetCSEListFromRegistry(
                "Services\\gpagent\\GPExtensions",
                &pGPClientSideExts
                );
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pszConfigFilePath) {
        LwFreeString(pszConfigFilePath);
        pszConfigFilePath = NULL;
    }

    if (pGPClientSideExts) {
        ceError = LoadLibraryInformation(pGPClientSideExts);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pthread_rwlock_wrlock(&gCSEInfo.lock);

    if (gCSEInfo.pGPCSEList) {
        DestroyClientSideExtensions(gCSEInfo.pGPCSEList);
    }
    gCSEInfo.pGPCSEList = pGPClientSideExts;

    pthread_rwlock_unlock(&gCSEInfo.lock);

    return (ceError);

error:

    if (pszConfigFilePath) {
        LwFreeString(pszConfigFilePath);
    }

    if (pGPClientSideExts) {
        DestroyClientSideExtensions(pGPClientSideExts);
    }

    return (ceError);
}

BOOLEAN
UserPoliciesAreAvailable()
{
    BOOLEAN bFound = FALSE;
    PGROUP_POLICY_CLIENT_EXTENSION pGPCSE= gCSEInfo.pGPCSEList;

    pthread_rwlock_rdlock(&gCSEInfo.lock);

    for (; !bFound && (pGPCSE != NULL); pGPCSE = pGPCSE->pNext) {
        bFound = !pGPCSE->dwNoUserPolicy;
    }
    
    pthread_rwlock_unlock(&gCSEInfo.lock);

    return bFound;
}
