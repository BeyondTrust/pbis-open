#include <reg/lwreg.h>

#include <lw/base.h>
#include <lw/ntstatus.h>
#include <lw/rtlstring.h>
#include <lw/rtlmemory.h>
#include <regdef.h>
#include <regutils.h>
#include <regclient.h>

int main(int argc, char *argv[])
{
    DWORD dwError = 0;
    DWORD i = 0;
    HANDLE hReg = NULL;
    HKEY hRootKey = NULL;
    HKEY hSubKey = NULL;
    PSTR pszSubKey = "tests";


    if (argc == 1)
    {
        printf("usage: %s subkey [subkey2 ...]\n", argv[0]);
        return 0;
    }
    dwError = RegOpenServer(&hReg);
    printf("RegOpenServer()=%d\n", dwError);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, 
                            NULL, 
                            HKEY_THIS_MACHINE, 
                            0, 
                            KEY_READ, 
                            &hRootKey);
    printf("RegOpenKeyExA(%s)=%d\n", HKEY_THIS_MACHINE, dwError);
    BAIL_ON_REG_ERROR(dwError);

    for (i=1; i<argc; i++)
    {
        pszSubKey = argv[i];
        dwError = RegOpenKeyExA(
                  hReg,
                  hRootKey,
                  pszSubKey,
                  0,
                  KEY_READ,
                  &hSubKey);
        printf("RegOpenKeyExA(%s)=%d\n", argv[i], dwError);
        BAIL_ON_REG_ERROR(dwError);
    }

 
    printf("Sleeping forever with open key...\n");
    sleep(10000);


cleanup:
    return dwError ? 1 : 0;
error:
    printf("ERROR %d\n", dwError);
    goto cleanup;

}

