/*
 * Program to move Pstore registry entries from 6.0 Pstore\Default location
 * to per-domain location. 6.1 supports joining multiple domains, but
 * there is only one default domain in this upgrade scenario. Move Pstore
 * entries to a subkey under that domainname, and set domain Default entry.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int ParsePstoreSections(
    FILE *fp,
    char *domainDnsName)
{

    char buf[1024] = {0};
    char *cp = NULL;
    char *printBuf = NULL;
    char *newHkey = NULL;
    int len = 0;
    int state = 0;
    int bWroteDefaultDomain = 0;

    while (!feof(fp))
    {
        cp = fgets(buf, sizeof(buf)-1, fp);
        if (cp)
        {
            printBuf = buf;

            /* Search for line: [HKEY_THIS_MACHINE.*Pstore.*] */
            if (buf[0] == '[' && (cp = strstr(buf, "Pstore")))
            {
                state = 1;

                /* 
                 * Ignore line: [HKEY_THIS_MACHINE\Services.*Pstore] 
                 * The location of the default values has not changed
                 */
                if (cp[strlen("Pstore")] == ']')
                {
                    state = 0;
                }

                if (state)
                {
                   /*
                    * transform these:
                    * [HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ \
                    *  ActiveDirectory\Pstore\Default]
                    * [HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ \
                    *  ActiveDirectory\Pstore\Default\MachinePassword]
                    * into these:
                    * [HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ \
                    *  ActiveDirectory\DomainJoin\domainDnsName\Pstore]
                    * [HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ \
                    * ActiveDirectory\DomainJoin\domainDnsName\Pstore\MachinePassword]
                    */

                    len = strlen(buf) + sizeof("DomainJoin") + 
                          sizeof("MachinePassword") + strlen(domainDnsName);
                    newHkey = (char *) calloc(len, sizeof(char));
                    if (newHkey)
                    {
                        strcpy(newHkey, buf);
                        cp = strstr(newHkey, "Pstore");
                        *cp = '\0';
                        cp = strstr(buf, "Default");
                        if (cp)
                        {
                            cp += strlen("Default");
                        }
                        strcat(newHkey, "DomainJoin");

                        if (!bWroteDefaultDomain)
                        {
                            /* Emit subkey that contains default domain */
                            printf("%s]\n", newHkey);
                            printf("\"Default\"=\"%s\"\n\n", domainDnsName);
                            bWroteDefaultDomain = 1;
                        }

                        /* Subkeys that contain per-domain Pstore information */
                        strcat(newHkey, "\\");
                        strcat(newHkey, domainDnsName);
                        strcat(newHkey, "\\Pstore");
                        if (*cp == '\\')
                        {
                            strcat(newHkey, cp);
                        }
                        else
                        {
                            strcat(newHkey, "]\r\n");
                        }
                        printBuf = newHkey;
                    }
                }
            }
        
            if (state)
            {
                printf("%s", printBuf);
                if (buf[0] == '\r' || buf[0] == '\n')
                {
                    printf("\n");
                    state = 0;
                }
            }
        }
        if (newHkey)
        {
            free(newHkey);
            newHkey = NULL;
        }
    }
    return 0;
}


/*
 * Find "DomainDnsName" in current registry export. It is assumed this only occurs once and
 * under a Pstore subkey. This could break should this valueName be duplicated under another
 * subkey.
 */
char *
FindDomain(
    FILE *fp)
{
    char buf[1024] = {0};
    char *cp = NULL;
    char *retDomain = NULL;

    while (!feof(fp))
    {
        cp = fgets(buf, sizeof(buf)-1, fp);
        if (cp)
        {
            cp = strstr(buf, "DomainDnsName");
            if (cp)
            {
                cp = strchr(cp, '=');
                if (cp)
                {
                    cp++;
                    if (*cp == '"')
                    {
                        cp++;
                        retDomain = strdup(cp);
                        if (!retDomain)
                        {
                            return NULL;
                        }
                        cp = retDomain;
                        cp = strchr(cp, '"');
                        if (cp)
                        {
                            *cp = '\0';
                            break;
                        }
                    }
                }
            }
        }
    }
    return retDomain;
}


int main(int argc, char *argv[])
{
    FILE *fp = NULL;
    char *pszDomainDnsName = NULL;

    if (argc == 1)
    {
        fprintf(stderr, "usage: %s registry.txt\n", argv[0]);
        return 0;
    }

    fp = fopen(argv[1], "r");
    if (!fp)
    {
        return 1;
    }
    
    pszDomainDnsName = FindDomain(fp);
    if (!pszDomainDnsName)
    {
        return 1;
    }
    rewind(fp);

    ParsePstoreSections(fp, pszDomainDnsName);
    free(pszDomainDnsName);
    
    fclose(fp);
    return 0;
}
