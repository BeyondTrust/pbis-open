/*
 * Copyright (c) Likewise Software.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

#include "includes.h"

static
DWORD
SetOperation(
    xmlDocPtr doc,
    PCSTR pszName,
    PCSTR pszaArg,
    BOOLEAN bVerbose
    );

static
DWORD
ShowOperation(
    xmlDocPtr doc,
    PCSTR pszName,
    BOOLEAN bConcise
    );

static
void
PrintUsage()
{
    fputs(
"Usage: lwconfig [OPTIONS] [COMMAND]\n"
"Modify or view the configuration.\n"
"\n"
" Options:\n"
"    --verbose               Display additional information.\n"
"\n"
" Commands:\n"
"  SETTING [VALUE]           Change SETTING to the given VALUE(s) or the\n"
"                              default value if no value is specified.\n"
"  --list                    Display names of all settings.\n"
"  --show SETTING            Display current value(s) of SETTING.\n"
"  --detail SETTING          Display current value(s) and details of SETTING.\n"
"  --file FILE               Read FILE with each line beginning with a\n"
"                              setting name followed by value(s). Use '.'\n"
"                              for reading from stdin.\n"
"  --dump                    Dump all settings in a format suitable for use\n"
"                              with --file.\n"
    ,stderr);
}

static
DWORD
ListCapabilities(
    xmlDocPtr doc
    )
{
    DWORD dwError = 0;
    xmlXPathContextPtr xpathCtx = NULL;
    xmlXPathObjectPtr xpathObj = NULL;
    xmlNodeSetPtr nodes = NULL;
    size_t i;

    xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx)
    {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_ERROR(dwError);
    }

    xpathObj = xmlXPathEvalExpression(
                    (const xmlChar*)"/capabilities/section",
                    xpathCtx);
    if (!xpathObj)
    {
        dwError = APP_ERROR_XPATH_EVAL_FAILED;
        BAIL_ON_ERROR(dwError);
    }

    nodes = xpathObj->nodesetval;
    for (i = 0; i < nodes->nodeNr; i++)
    {
        if (nodes->nodeTab[i]->type == XML_ELEMENT_NODE)
        {
            xmlNodePtr cur = nodes->nodeTab[i];
            xmlNodePtr child = NULL;
            xmlChar *section = NULL;

            section = xmlGetProp(cur, (const xmlChar*) "section");
            if (section)
                fprintf(stdout, "[%s]\n", section);
            else
                fprintf(stdout, "\n");
            xmlFree(section);
            section = NULL;

            for (child = cur->xmlChildrenNode; child; child = child->next)
            {
                PCAPABILITY cap = NULL;
                if (child->type != XML_ELEMENT_NODE)
                    continue;
                if (xmlStrcmp(child->name, (const xmlChar*)"capability"))
                    continue;

                dwError = CapabilityAllocate(doc, child, &cap);
                BAIL_ON_ERROR(dwError);

                if (cap->pszName)
                    fprintf(stdout, "\t%s\n", cap->pszName);

                CapabilityFree(cap);
                cap = NULL;
            }
        }
    }

cleanup:
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
SetOperation(
    xmlDocPtr doc,
    PCSTR pszName,
    PCSTR pszaArg,
    BOOLEAN bVerbose
    )
{
    DWORD dwError = 0;
    PCAPABILITY pCapability = NULL;

    dwError = CapabilityFindByName(doc, pszName, &pCapability);
    BAIL_ON_ERROR(dwError);

    dwError = CapabilityEditRegistry(pCapability, pszaArg, bVerbose);
    BAIL_ON_ERROR(dwError);

    dwError = CapabilityApply(pCapability, bVerbose);
    BAIL_ON_ERROR(dwError);

cleanup:
    CapabilityFree(pCapability);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
ShowOperation(
    xmlDocPtr doc,
    PCSTR pszName,
    BOOLEAN bConcise
    )
{
    DWORD dwError = 0;
    PCAPABILITY pCapability = NULL;

    dwError = CapabilityFindByName(doc, pszName, &pCapability);
    BAIL_ON_ERROR(dwError);

    dwError = CapabilityShow(pCapability, bConcise);
    BAIL_ON_ERROR(dwError);

cleanup:
    CapabilityFree(pCapability);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
DumpOperation(
    xmlDocPtr doc,
    BOOLEAN bPolicyOnly
    )
{
    DWORD dwError = 0;
    PCAPABILITY pCapability = NULL;
    xmlXPathContextPtr xpathCtx = NULL;
    xmlXPathObjectPtr xpathObj = NULL;
    xmlNodeSetPtr nodes = NULL;
    size_t i;

    xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx)
    {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_ERROR(dwError);
    }

    xpathObj = xmlXPathEvalExpression(
                    (const xmlChar*)"/capabilities/section/capability",
                    xpathCtx);
    if (!xpathObj)
    {
        dwError = APP_ERROR_XPATH_EVAL_FAILED;
        BAIL_ON_ERROR(dwError);
    }

    nodes = xpathObj->nodesetval;
    for (i = 0; i < nodes->nodeNr; i++)
    {
        if (nodes->nodeTab[i]->type != XML_ELEMENT_NODE)
            continue;

        if (xmlStrcmp(nodes->nodeTab[i]->name, (const xmlChar*)"capability"))
                continue;

        dwError = CapabilityAllocate(doc, nodes->nodeTab[i], &pCapability);
        BAIL_ON_ERROR(dwError);

        dwError = CapabilityDump(pCapability);
        BAIL_ON_ERROR(dwError);

        CapabilityFree(pCapability);
        pCapability = NULL;
    }

cleanup:
    CapabilityFree(pCapability);
    pCapability = NULL;
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    return dwError;

error:
    goto cleanup;
}

int main(int argc, const char *argv[])
{
    DWORD dwError = 0;
    BOOLEAN bListCapabilities = FALSE;
    BOOLEAN bFile = FALSE;
    PCSTR pszFile = NULL;
    BOOLEAN bShowCapability = FALSE;
    BOOLEAN bDumpCapability = FALSE;
    BOOLEAN bDetailCapability = FALSE;
    BOOLEAN bVerbose = FALSE;
    PCSTR pszCapability = NULL;
    FILE *fpFile = NULL;
    PSTR pszLine = NULL;
    PSTR *ppszArgs = NULL;
    DWORD dwArgs = 0;
    PSTR pszaArg = NULL;
    xmlDocPtr doc = NULL;
    int dwArgc = 1;
    int returnErrorCode = 1;
    size_t i;

    setlocale(LC_ALL, "");

    xmlInitParser();

    doc = xmlReadFile(LWCONFIG_XML, NULL, XML_PARSE_NOWARNING |
            XML_PARSE_NOERROR);
    if (!doc)
    {
        dwError = APP_ERROR_BAD_XML;
        BAIL_ON_ERROR(dwError);
    }
    xmlXIncludeProcessFlags(doc, XML_PARSE_NOWARNING | XML_PARSE_NOERROR);

    if (argc < 2)
    {
        PrintUsage();
        goto cleanup;
    }

    for (dwArgc = 1; dwArgc < argc; dwArgc++)
    {
        if (!strcmp(argv[dwArgc], "--help"))
        {
            PrintUsage();
            goto cleanup;
        }
        else if (!strcmp(argv[dwArgc], "--file"))
        {
            if (argc == dwArgc+1)
            {
                PrintUsage();
                goto cleanup;
            }

            bFile = TRUE;
            pszFile = argv[++dwArgc];
        }
        else if (!strcmp(argv[dwArgc], "--list"))
        {
            bListCapabilities = TRUE;
        }
        else if (!strcmp(argv[dwArgc], "--dump"))
        {
            bDumpCapability = TRUE;
        }
        else if (!strcmp(argv[dwArgc], "--show"))
        {
            if (argc == dwArgc+1)
            {
                PrintUsage();
                goto cleanup;
            }

            bShowCapability = TRUE;
            pszCapability = argv[++dwArgc];
        }
        else if (!strcmp(argv[dwArgc], "--detail") ||
                 !strcmp(argv[dwArgc], "--details"))
        {
            if (argc == dwArgc+1)
            {
                PrintUsage();
                goto cleanup;
            }
            bDetailCapability = TRUE;
            pszCapability = argv[++dwArgc];
        }
        else if (!strcmp(argv[dwArgc], "--verbose"))
        {
            bVerbose = TRUE;
        }
        else if (!strncmp(argv[dwArgc], "--", 2))
        {
            PrintUsage();
            goto cleanup;
        }
        else
            break;
    }

    if (bListCapabilities)
    {
        dwError = ListCapabilities(doc);
        BAIL_ON_ERROR(dwError);
    }
    else if (bDumpCapability)
    {
        dwError = DumpOperation(doc, TRUE);
        BAIL_ON_ERROR(dwError);
    }
    else if (bFile)
    {
        if (!strcmp(pszFile, "."))
            fpFile = stdin;
        else if ((fpFile = fopen(pszFile, "r")) == NULL)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }

        while ((dwError = UtilReadLine(fpFile, &pszLine)) == 0)
        {
            dwError = UtilParseLine(pszLine, &ppszArgs, &dwArgs);
            BAIL_ON_ERROR(dwError);

            if (dwArgs > 0)
            {
                dwError = UtilAllocateMultistring((PCSTR*)ppszArgs + 1, dwArgs -
                        1, &pszaArg);
                BAIL_ON_ERROR(dwError);

                dwError = SetOperation(doc, ppszArgs[0], pszaArg, bVerbose);
                BAIL_ON_ERROR(dwError);

                for (i = 0; i < dwArgs; i++)
                    LW_SAFE_FREE_MEMORY(ppszArgs[i]);
                dwArgs = 0;
                LW_SAFE_FREE_MEMORY(ppszArgs);
                LW_SAFE_FREE_MEMORY(pszaArg);
             }

            LW_SAFE_FREE_STRING(pszLine);
        }
        if (dwError == ERROR_HANDLE_EOF)
        {
            dwError = 0;
        }
        BAIL_ON_ERROR(dwError);
    }
    else if (bShowCapability || bDetailCapability)
    {
        dwError = ShowOperation(doc, pszCapability, bShowCapability);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        if (argc - dwArgc > 0)
        {
            if (argc - dwArgc > 1)
            {
                dwError = UtilAllocateMultistring((PCSTR*)argv + dwArgc + 1,
                        argc - dwArgc - 1, &pszaArg);
                BAIL_ON_ERROR(dwError);
            }

            dwError = SetOperation(doc, argv[dwArgc], pszaArg, bVerbose);
            BAIL_ON_ERROR(dwError);
        }
        else
        {
            PrintUsage();
            goto cleanup;
        }
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pszaArg);

    for (i = 0; i < dwArgs; i++)
        LW_SAFE_FREE_MEMORY(ppszArgs[i]);
    dwArgs = 0;
    LW_SAFE_FREE_MEMORY(ppszArgs);

    if (fpFile && fpFile != stdin)
    {
        fclose(fpFile);
        fpFile = NULL;
    }
    xmlFreeDoc(doc);
    doc = NULL;

    xmlCleanupParser();
    if (dwError)
    {
        size_t size = 0;
        char buf[1024];

        buf[0] = '\0';

        if (40700 <= dwError && dwError <= 41200)
        {
            size = LwRegGetErrorString(dwError, buf, sizeof(buf));
            if (size > sizeof(buf))
                buf[0] = '\0';
        }
        else if (dwError >= 0x20000000)
        {
            switch(dwError)
            {
                case APP_ERROR_XML_DUPLICATED_ELEMENT:
                    strcpy(buf, "Duplicated XML element");
                    break;
                case APP_ERROR_XML_MISSING_ELEMENT:
                    strcpy(buf, "Missing XML element");
                    break;
                case APP_ERROR_XPATH_EVAL_FAILED:
                    strcpy(buf, "Could not evaluate XPath expression");
                    break;
                case APP_ERROR_CAPABILITY_NOT_FOUND:
                    strcpy(buf, "Capability not found");
                    returnErrorCode = 2;
                    break;
                case APP_ERROR_INVALID_DWORD:
                    strcpy(buf, "Could not interpret value as dword");
                    returnErrorCode = 3;
                    break;
                case APP_ERROR_INVALID_BOOLEAN:
                    strcpy(buf, "Could not interpret value as boolean");
                    returnErrorCode = 3;
                    break;
                case APP_ERROR_INVALID_SUFFIX:
                    strcpy(buf, "Suffix on DWORD does not match known units");
                    returnErrorCode = 3;
                    break;
                case APP_ERROR_PARAMETER_REQUIRED:
                    strcpy(buf, "Argument required -- no default is present");
                    returnErrorCode = 4;
                    break;
                case APP_ERROR_UNEXPECTED_VALUE:
                    strcpy(buf, "Bad value in " LWCONFIG_XML);
                    break;
                case APP_ERROR_COULD_NOT_FORK:
                    strcpy(buf, "Could not fork");
                    break;
                case APP_ERROR_BAD_XML:
                    strcpy(buf, "Malformed XML in " LWCONFIG_XML);
                    break;
                case APP_ERROR_INVALID_ESCAPE_SEQUENCE:
                    strcpy(buf, "Bad escape sequence");
                    returnErrorCode = 3;
                    break;
                case APP_ERROR_UNTERMINATED_QUOTE:
                    strcpy(buf, "Unterminated quote");
                    returnErrorCode = 3;
                    break;
                case APP_ERROR_VALUE_NOT_ACCEPTED:
                    strcpy(buf, "Value not in accepted range");
                    returnErrorCode = 3;
                    break;
                case APP_ERROR_PROGRAM_ERROR:
                    strcpy(buf, "Error returned by external program");
                    returnErrorCode = 5;
                    break;
                case APP_ERROR_CAPABILITY_MULTIPLE_MATCHES:
                    strcpy(buf, "Multiple capabilties matched; aborting.");
                    returnErrorCode = 6;
                    break;
                case APP_ERROR_XML_MISSING_ATTRIBUTE:
                    strcpy(buf, "Missing XML attribute");
                    break;
            }
        }
        if (buf[0] == '\0')
        {
            size = LwGetErrorString(dwError, buf, sizeof(buf));
            if (size > sizeof(buf))
                buf[0] = '\0';
        }

        if (buf[0])
        {
            fprintf(stderr, "Error: %s\n", buf);
        }
        else
        {
            fprintf(stderr, "Error: %lu\n", (unsigned long) dwError);
        }
        return returnErrorCode;
    }
    return 0;

error:
    goto cleanup;
}
