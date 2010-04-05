#include "gpagent.h"

int
main(int argc, char* argv[])
{
    DWORD dwError = 0;
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientExtensions = NULL;

    if (argc < 2)
    {
        printf("Usage: %s <config file>\n", argv[0]);
        return 1;
    }
    
    dwError = ParseConfigurationFile(argv[1], &pGPClientExtensions);
    if (dwError == 0)
    {
        PGROUP_POLICY_CLIENT_EXTENSION pExtension = pGPClientExtensions;
        int iExtension = 0;
        CHAR pszUUID[64];

        printf("Parse successful\n");
        
        while (pExtension != NULL) {
            
            printf("Extension::%d\n", iExtension);
            printf("=============\n");
            printf("Name: %s\n", (pExtension->pszName ? pExtension->pszName : "NULL"));
            uuid_unparse(pExtension->UUID, pszUUID);
            printf("UUID: %s\n", pszUUID);
            printf("Dll Name: %s\n", (pExtension->pszDllName ? pExtension->pszDllName : "NULL"));
            printf("Enable Async Processing: %d\n", pExtension->dwEnableAsynchronousProcessing);
            printf("No Background Policy: %d\n", pExtension->dwNoBackgroundPolicy);
            printf("No GPOList Changes: %d\n", pExtension->dwNoGPOListChanges);
            printf("No Machine Policy: %d\n", pExtension->dwNoMachinePolicy);
            printf("No Slow Link: %d\n", pExtension->dwNoSlowLink);
            printf("Per User Settings: %d\n", pExtension->dwPerUserSettings);
            printf("Group Policy Function: %s\n", pExtension->pszProcessGroupPolicyFunction ? pExtension->pszProcessGroupPolicyFunction : "NULL");
            
            iExtension++;
            pExtension = pExtension->pNext;
        }
    }
    else
    {
        printf("Parse unsuccessful (Error code: %d)\n", dwError);
    }

    if (pGPClientExtensions) {
       DestroyClientSideExtensions(pGPClientExtensions);
    }
    
    return (dwError);
    
/*error:*/

    if (pGPClientExtensions) {
       DestroyClientSideExtensions(pGPClientExtensions);
    }
    
    return(dwError);
}
