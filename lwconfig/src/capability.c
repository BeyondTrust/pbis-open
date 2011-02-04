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
xmlNodePtr
FindNext(
    xmlNodePtr pxChild,
    PSTR pszChild
    );

static
DWORD
DefaultValue(
    xmlDocPtr xmlDoc,
    xmlNodePtr pxDefault,
    PSTR *ppszValue
    );

static
DWORD
GetLocalPolicyValue(
    PCAPABILITY pCapability,
    PSTR *ppszaValue
    );

static
DWORD
RegistryAllocate(
    xmlDocPtr xmlDoc,
    xmlNodePtr pxRegistry,
    PREGISTRY *ppRegistry)
{
    DWORD dwError = 0;
    PREGISTRY pRegistry = NULL;
    xmlChar *xszLocalPath = NULL;
    PSTR pszLocalPath = NULL;
    xmlChar *xszPolicyPath = NULL;
    PSTR pszPolicyPath = NULL;
    xmlChar *xszType = NULL;
    PSTR pszType = NULL;
    xmlChar *xszDescription = NULL;
    PSTR pszDescription = NULL;
    PSTR pszaDefault = NULL;
    xmlNodePtr child = NULL;

    xszLocalPath = xmlGetProp(pxRegistry, (const xmlChar*)"lp-path");
    xszPolicyPath = xmlGetProp(pxRegistry, (const xmlChar*)"gp-path");
    xszType = xmlGetProp(pxRegistry, (const xmlChar*)"type");

    if (!xszLocalPath && !xszPolicyPath)
    {
        dwError = APP_ERROR_XML_MISSING_ATTRIBUTE;
        BAIL_ON_ERROR(dwError);
    }

    if (!xszType)
    {
        dwError = APP_ERROR_XML_MISSING_ATTRIBUTE;
        BAIL_ON_ERROR(dwError);
    }

    if (xszLocalPath)
    {
        dwError = LwAllocateString((PCSTR)xszLocalPath, &pszLocalPath);
        BAIL_ON_ERROR(dwError);
    }

    if (xszPolicyPath)
    {
        dwError = LwAllocateString((PCSTR)xszPolicyPath, &pszPolicyPath);
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwAllocateString((PCSTR)xszType, &pszType);
    BAIL_ON_ERROR(dwError);

    for (child = pxRegistry->xmlChildrenNode; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (!xmlStrcmp(child->name, (const xmlChar*)"default"))
        {
            dwError = DefaultValue(xmlDoc, child, &pszaDefault);
            BAIL_ON_ERROR(dwError);
        }
        else if (!xmlStrcmp(child->name, (const xmlChar*)"description"))
        {
            xszDescription = xmlNodeListGetString(xmlDoc,
                   child->xmlChildrenNode, TRUE);
            if (xszDescription)
            {
                dwError = LwAllocateString((PCSTR)xszDescription, &pszDescription);
                BAIL_ON_ERROR(dwError);
            }
        }
    }

    dwError = LwAllocateMemory(sizeof(*pRegistry), (PVOID*)&pRegistry);
    BAIL_ON_ERROR(dwError);

    pRegistry->pxRegistry = pxRegistry;
    pRegistry->pszLocalPath = pszLocalPath;
    pRegistry->pszPolicyPath = pszPolicyPath;
    pRegistry->pszType = pszType;
    pRegistry->pszDescription = pszDescription;
    pRegistry->pszaDefault = pszaDefault;

    *ppRegistry = pRegistry;

cleanup:

    xmlFree(xszLocalPath);
    xmlFree(xszPolicyPath);
    xmlFree(xszType);
    xmlFree(xszDescription);
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszLocalPath);
    LW_SAFE_FREE_STRING(pszPolicyPath);
    LW_SAFE_FREE_STRING(pszType);
    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_MEMORY(pszaDefault);
    LW_SAFE_FREE_MEMORY(pRegistry);
    goto cleanup;
}

static
void
RegistryFree(
    PREGISTRY pRegistry
    )
{
    if (pRegistry)
    {
        LW_SAFE_FREE_STRING(pRegistry->pszLocalPath);
        LW_SAFE_FREE_STRING(pRegistry->pszPolicyPath);
        LW_SAFE_FREE_STRING(pRegistry->pszType);
        LW_SAFE_FREE_STRING(pRegistry->pszDescription);
        LW_SAFE_FREE_MEMORY(pRegistry->pszaDefault);

        LW_SAFE_FREE_MEMORY(pRegistry);
    }
}


DWORD
CapabilityAllocate(
    xmlDocPtr xmlDoc,
    xmlNodePtr xmlNodeCapability,
    PCAPABILITY *ppCapability
    )
{
    DWORD dwError = 0;
    xmlChar *xszName = NULL;
    PSTR pszName = NULL;
    xmlChar *xszDescription = NULL;
    PSTR pszDescription = NULL;
    xmlNodePtr child = NULL;
    PREGISTRY pRegistry = NULL;
    PCAPABILITY pCapability = NULL;

    if (!xmlNodeCapability)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    for (child = xmlNodeCapability->xmlChildrenNode; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;

        if (!xmlStrcmp(child->name, (const xmlChar*)"name"))
        {
            if (xszName)
            {
                dwError = APP_ERROR_XML_DUPLICATED_ELEMENT;
                BAIL_ON_ERROR(dwError);
            }
            xszName = xmlNodeListGetString(
                            xmlDoc,
                            child->xmlChildrenNode,
                            TRUE);
        }
        else if (!xmlStrcmp(child->name, (const xmlChar*)"description"))
        {
            if (xszDescription)
            {
                dwError = APP_ERROR_XML_DUPLICATED_ELEMENT;
                BAIL_ON_ERROR(dwError);
            }
            xszDescription = xmlNodeListGetString(
                                    xmlDoc,
                                    child->xmlChildrenNode,
                                    TRUE);
        }
        else if (!xmlStrcmp(child->name, (const xmlChar*)"registry"))
        {
            if (pRegistry)
            {
                dwError = APP_ERROR_XML_DUPLICATED_ELEMENT;
                BAIL_ON_ERROR(dwError);
            }
            dwError = RegistryAllocate(xmlDoc, child, &pRegistry);
            BAIL_ON_ERROR(dwError);
        }
    }

    if (!xszName || xszName[0] == '\0')
    {
        fprintf(stderr, "Improper format: Element 'name' not found or empty.\n");
        dwError = APP_ERROR_XML_MISSING_ELEMENT;
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwAllocateString((char*)xszName, &pszName);
    BAIL_ON_ERROR(dwError);

    if (xszDescription)
    {
        dwError = LwAllocateString((char*)xszDescription, &pszDescription);
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(*pCapability), (PVOID*)&pCapability);
    BAIL_ON_ERROR(dwError);

    pCapability->xmlDoc = xmlDoc;

    pCapability->xmlNodeCapability = xmlNodeCapability;

    pCapability->pszName = pszName;
    pszName = NULL;

    pCapability->pszDescription = pszDescription;
    pszDescription = NULL;

    pCapability->pRegistry = pRegistry;
    pRegistry = NULL;

    *ppCapability = pCapability;

cleanup:
    xmlFree(xszName);
    xmlFree(xszDescription);
    return dwError;

error:

    LW_SAFE_FREE_STRING(pszName);
    LW_SAFE_FREE_STRING(pszDescription);
    RegistryFree(pRegistry);
    CapabilityFree(pCapability);
    goto cleanup;
}

void
CapabilityFree(
    PCAPABILITY pCapability
    )
{
    if (pCapability)
    {
        RegistryFree(pCapability->pRegistry);
        LW_SAFE_FREE_MEMORY(pCapability->pszName);
        LW_SAFE_FREE_MEMORY(pCapability->pszDescription);
        LW_SAFE_FREE_MEMORY(pCapability);
    }
}

/*
   If pszName matches exactly one item, use that.
   Else if pszName matches more than one item exactly, report an error.
   Else if pszName is prefix matches exactly one item, use that.
   Else if pszName prefix matches more than one item, print the matches and
   report an error.
*/
DWORD
CapabilityFindByName(
    xmlDocPtr xmlDoc,
    PCSTR pszName,
    PCAPABILITY *ppCapability
    )
{
    DWORD dwError = 0;

    xmlXPathContextPtr xpathCtx = NULL;
    xmlXPathObjectPtr xpathObj = NULL;
    xmlNodeSetPtr nodes = NULL; // Convienence pointer
    xmlNodePtr xmlNodeCapability = NULL;
    PCAPABILITY pCapability = NULL;
    DWORD dwNameLength = 0;
    BOOLEAN bNameMatchesMany = FALSE;
    BOOLEAN bNameMatchesExactly = FALSE;
    size_t i;
    xmlNodePtr child = NULL;
    xmlChar *xszName = NULL;

    dwNameLength = strlen(pszName); // Calculate once

    xpathCtx = xmlXPathNewContext(xmlDoc);
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

        if (xmlStrcmp((const xmlChar*)"capability", nodes->nodeTab[i]->name))
            continue;

        child = FindNext(nodes->nodeTab[i]->xmlChildrenNode, "name");
        if (!child)
            continue;

        xszName = xmlNodeListGetString(xmlDoc, child->xmlChildrenNode, TRUE);
        if (!xszName)
        {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_ERROR(dwError);
        }

        if (!xmlStrcasecmp(xszName, (const xmlChar*)pszName))
        {
            if (bNameMatchesExactly)
            {
                dwError = APP_ERROR_XML_DUPLICATED_ELEMENT;
                BAIL_ON_ERROR(dwError);
            }

            xmlNodeCapability = nodes->nodeTab[i];
            bNameMatchesExactly = TRUE;
        }
        else if (!xmlStrncasecmp(xszName, (const xmlChar*) pszName, dwNameLength))
        {
            if (!bNameMatchesExactly)
            {
                if (xmlNodeCapability)
                {
                    bNameMatchesMany = TRUE;
                }
                else
                {
                    xmlNodeCapability = nodes->nodeTab[i];
                }
            }
        }

        xmlFree(xszName);
        xszName = NULL;
    }

    if (!bNameMatchesExactly && bNameMatchesMany)
    {
        fprintf(stderr, "%s partially matches:\n", pszName);
        for (i = 0; i < nodes->nodeNr; i++)
        {
            if (nodes->nodeTab[i]->type != XML_ELEMENT_NODE)
                continue;

            if (xmlStrcmp((const xmlChar*)"capability", nodes->nodeTab[i]->name))
                continue;

            child = FindNext(nodes->nodeTab[i]->xmlChildrenNode, "name");
            if (!child)
                continue;


            xszName = xmlNodeListGetString(xmlDoc, child->xmlChildrenNode, TRUE);
            if (!xszName)
            {
                dwError = ERROR_OUTOFMEMORY;
                BAIL_ON_ERROR(dwError);
            }

            if (!xmlStrncasecmp(xszName, (const xmlChar*)pszName, dwNameLength))
            {
                fprintf(stderr, "%s\n", (PCSTR)xszName);
            }

            xmlFree(xszName);
            xszName = NULL;
        }
        dwError = APP_ERROR_CAPABILITY_MULTIPLE_MATCHES;
        BAIL_ON_ERROR(dwError);
    }
    else if (!xmlNodeCapability)
    {
        dwError = APP_ERROR_CAPABILITY_NOT_FOUND;
        BAIL_ON_ERROR(dwError);
    }

    dwError = CapabilityAllocate(xmlDoc, xmlNodeCapability, &pCapability);
    BAIL_ON_ERROR(dwError);

    *ppCapability = pCapability;

cleanup:

    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);

    xmlFree(xszName);
    xszName = NULL;

    return dwError;

error:

    CapabilityFree(pCapability);
    goto cleanup;
}

static
DWORD
ValidateString(
    PCSTR pszValue,
    xmlDocPtr xmlDoc,
    xmlNodePtr pxRegistry,
    PBOOLEAN pbAccept
    )
{
    DWORD dwError = 0;
    xmlNodePtr xmlAccept = NULL;
    xmlNodePtr xmlAcceptChild = NULL;
    const DWORD UNDECIDED = 0;
    const DWORD ACCEPT_VALUE = 1;
    const DWORD REJECT_VALUE = 2;
    DWORD dwDecision = UNDECIDED;
    const DWORD NOTHING = 0;
    const DWORD ACCEPT = 1;
    const DWORD REJECT = 2;
    DWORD dwState = NOTHING;
    xmlChar *xszValue = NULL;
    xmlChar *xszPattern = NULL;
    regex_t patternExp;
    regmatch_t matches[5];


    for (xmlAccept = pxRegistry->xmlChildrenNode;
         xmlAccept && dwDecision == UNDECIDED;
         xmlAccept = xmlAccept->next)
    {
        if (xmlAccept->type != XML_ELEMENT_NODE)
            continue;

        if (!xmlStrcmp(xmlAccept->name, (const xmlChar*)"accept"))
        {
            dwState = ACCEPT;
        }
        else if (!xmlStrcmp(xmlAccept->name, (const xmlChar*)"reject"))
        {
            dwState = REJECT;
        }
        else
        {
            continue;
        }

        for (xmlAcceptChild = xmlAccept->xmlChildrenNode;
             xmlAcceptChild && dwDecision == UNDECIDED;
             xmlAcceptChild = xmlAcceptChild->next)
        {
            if (xmlAcceptChild->type != XML_ELEMENT_NODE)
                continue;

            if (!xmlStrcmp(xmlAcceptChild->name, (const xmlChar*)"value"))
            {
                xszValue = xmlNodeListGetString(
                                xmlDoc,
                                xmlAcceptChild->xmlChildrenNode,
                                TRUE);
                if (xszValue && !strcmp((PCSTR)xszValue, pszValue))
                {
                    if (dwState == ACCEPT)
                    {
                        dwDecision = ACCEPT_VALUE;
                    }
                    else
                    {
                        dwDecision = REJECT_VALUE;
                    }
                }
                if (xszValue)
                {
                    xmlFree(xszValue);
                    xszValue = NULL;
                }

            } else if (!xmlStrcmp(
                        xmlAcceptChild->name,
                        (const xmlChar*)"pattern"))
            {
                xszPattern = xmlNodeListGetString(
                                xmlDoc,
                                xmlAcceptChild->xmlChildrenNode,
                                TRUE);

                if (xszPattern)
                {
                    if (!regcomp(&patternExp, (PCSTR)xszPattern, REG_EXTENDED))
                    {
                        if (!regexec(&patternExp, pszValue,
                                    sizeof(matches)/sizeof(matches[0]),
                                    matches,
                                    0))
                        {
                            if (dwState == ACCEPT)
                            {
                                dwDecision = ACCEPT_VALUE;
                            }
                            else
                            {
                                dwDecision = REJECT_VALUE;
                            }
                        }
                        regfree(&patternExp);
                    }
                }
                if (xszPattern)
                {
                    xmlFree(xszPattern);
                    xszPattern = NULL;
                }
            }
        }
    }

    if (dwDecision == ACCEPT_VALUE)
    {
        *pbAccept = TRUE;
    }
    else if (dwDecision == REJECT_VALUE)
    {
        *pbAccept = FALSE;
    }
    else
    {
        if (dwState == NOTHING)
        {
            *pbAccept = TRUE;
        }
        else if (dwState == ACCEPT)
        {
            *pbAccept = FALSE;
        }
        else if (dwState == REJECT)
        {
            *pbAccept = TRUE;
        }
    }

    if (xszValue)
    {
        xmlFree(xszValue);
        xszValue = NULL;
    }
    if (xszPattern)
    {
        xmlFree(xszPattern);
        xszPattern = NULL;
    }

    return dwError;
}

static
DWORD
CapabilityRegistryGetUnitMultiplier(
    PCSTR pszUnit,
    xmlNodePtr pxRegistry,
    PBOOLEAN pbMatched,
    PDWORD pdwMultiplier,
    PDWORD pdwDivisor
    )
{
    DWORD dwError = 0;
    xmlChar* xszUnitSuffix = NULL;
    xmlChar* xszUnitMultiplier = NULL;
    xmlChar* xszUnitDivisor = NULL;
    DWORD dwUnitMultiplier = 1;
    DWORD dwUnitDivisor = 1;
    xmlNodePtr child = NULL;

    if (pxRegistry)
        child = pxRegistry->xmlChildrenNode;
    for (; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;

        if (!xmlStrcmp(child->name, (const xmlChar*)"unit"))
        {
            xszUnitSuffix = xmlGetProp(child, (const xmlChar*)"suffix");
            if (xszUnitSuffix && !strcmp((PCSTR)xszUnitSuffix, pszUnit))
                break;
        }
    }

    if (child)
        xszUnitMultiplier = xmlGetProp(child, (const xmlChar*)"multiplier");
    if (xszUnitMultiplier)
    {
        dwUnitMultiplier = strtol((PCSTR)xszUnitMultiplier, NULL, 10);
    }

    if (child)
        xszUnitDivisor = xmlGetProp(child, (xmlChar*)"divisor");
    if (xszUnitDivisor)
    {
        dwUnitDivisor = strtol((PCSTR)xszUnitDivisor, NULL, 10);
    }

    *pdwMultiplier = dwUnitMultiplier;
    *pdwDivisor = dwUnitDivisor;
    *pbMatched = TRUE;

    xmlFree(xszUnitSuffix);
    xszUnitSuffix = NULL;

    xmlFree(xszUnitMultiplier);
    xszUnitMultiplier = NULL;

    xmlFree(xszUnitDivisor);
    xszUnitDivisor = NULL;

    return dwError;
}

static
DWORD
CapabilityNormalizeDword(
    PCSTR pszValue,
    xmlNodePtr pxRegistry,
    PDWORD pdwValue
    )
{
    DWORD dwError = 0;
    PSTR pszEndChar = NULL; /* Don't free */
    BOOLEAN bMatched = FALSE;
    DWORD dwValue = 0;

    dwValue = strtol(pszValue, &pszEndChar, 10);
    if (dwValue == 0 && pszValue == pszEndChar)
    {
        fprintf(
            stderr,
            "Invalid parameter '%s': could not interpret as dword\n",
            pszValue);
        dwError = APP_ERROR_INVALID_DWORD;
        BAIL_ON_ERROR(dwError);
    }

    // Left over characters indicate a suffix -- look for element 'unit'.
    if (pszEndChar && *pszEndChar != '\0')
    {
        DWORD dwUnitMultiplier = 1;
        DWORD dwUnitDivisor = 1;

        dwError = CapabilityRegistryGetUnitMultiplier(
                        pszEndChar,
                        pxRegistry,
                        &bMatched,
                        &dwUnitMultiplier,
                        &dwUnitDivisor);
        BAIL_ON_ERROR(dwError);

        if (!bMatched)
        {
            fprintf(stderr, "Invalid parameter '%s': could not interpret %s\n",
                    pszValue, pszEndChar);
            dwError = APP_ERROR_INVALID_SUFFIX;
            BAIL_ON_ERROR(dwError);
        }


        dwValue = dwValue * dwUnitMultiplier;
        dwValue = dwValue / dwUnitDivisor;
    }

    *pdwValue = dwValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ValidateDword(
    DWORD dwValue,
    xmlNodePtr pxRegistry,
    PBOOLEAN pbAccept
    )
{
    DWORD dwError = 0;
    xmlNodePtr xmlAccept = NULL;
    xmlNodePtr xmlRange = NULL;
    xmlChar *xszMin = NULL;
    xmlChar *xszMax = NULL;
    DWORD dwMin = 0;
    DWORD dwMax = 0;
    const DWORD UNDECIDED = 0;
    const DWORD ACCEPT_VALUE = 1;
    const DWORD REJECT_VALUE = 2;
    DWORD dwDecision = UNDECIDED;
    const DWORD NOTHING = 0;
    const DWORD ACCEPT = 1;
    const DWORD REJECT = 2;
    DWORD dwState = NOTHING;

    for (xmlAccept = pxRegistry->xmlChildrenNode;
         xmlAccept && dwDecision == UNDECIDED;
         xmlAccept = xmlAccept->next)
    {
        if (xmlAccept->type != XML_ELEMENT_NODE)
            continue;

        if (!xmlStrcmp(xmlAccept->name, (const xmlChar*)"accept"))
        {
            dwState = ACCEPT;
        }
        else if (!xmlStrcmp(xmlAccept->name, (const xmlChar*)"reject"))
        {
            dwState = REJECT;
        }
        else
        {
            continue;
        }

        for (xmlRange = xmlAccept->xmlChildrenNode;
             xmlRange && dwDecision == UNDECIDED;
             xmlRange = xmlRange->next)
        {
            if (xmlRange->type != XML_ELEMENT_NODE)
                continue;

            if (!xmlStrcmp(xmlRange->name, (const xmlChar*)"range"))
            {
                xszMin = xmlGetProp(xmlRange, (const xmlChar*)"min");
                xszMax = xmlGetProp(xmlRange, (const xmlChar*)"max");

                if (!xszMin || !xszMax)
                {
                    dwError = APP_ERROR_XML_MISSING_ATTRIBUTE;
                    goto error;
                }

                dwError = CapabilityNormalizeDword(
                                (PCSTR)xszMin,
                                pxRegistry,
                                &dwMin);
                BAIL_ON_ERROR(dwError);

                dwError = CapabilityNormalizeDword(
                                (PCSTR)xszMax,
                                pxRegistry,
                                &dwMax);
                BAIL_ON_ERROR(dwError);

                if (dwMin <= dwValue && dwValue <= dwMax)
                {
                    if (dwState == ACCEPT)
                    {
                        dwDecision = ACCEPT_VALUE;
                    }
                    else
                        dwDecision = REJECT_VALUE;
                }

                xmlFree(xszMin);
                xszMin = NULL;

                xmlFree(xszMax);
                xszMax = NULL;
            }
        }
    }

    if (dwDecision == ACCEPT_VALUE)
    {
        *pbAccept = TRUE;
    }
    else if (dwDecision == REJECT_VALUE)
    {
        *pbAccept = FALSE;
    }
    else
    {
        if (dwState == NOTHING)
        {
            *pbAccept = TRUE;
        }
        else if (dwState == ACCEPT)
        {
            *pbAccept = FALSE;
        }
        else if (dwState == REJECT)
        {
            *pbAccept = TRUE;
        }
    }

cleanup:

    if (xszMin)
    {
        xmlFree(xszMin);
        xszMin = NULL;
    }
    if (xszMax)
    {
        xmlFree(xszMax);
        xszMax = NULL;
    }
    return dwError;

error:
    goto cleanup;
}

static
DWORD
CapabilityNormalizeBoolean(
    PCSTR pszValue,
    PDWORD pdwValue
    )
{
    DWORD dwError = 0;
    DWORD dwValue = 0;

    if (!strcasecmp("true", pszValue) ||
        !strcasecmp("1", pszValue))
    {
        dwValue = 1;
    }
    else if (!strcasecmp("false", pszValue) ||
             !strcasecmp("0", pszValue))
    {
        dwValue = 0;
    }
    else
    {
        fprintf(stderr, "Invalid parameter: Cannot interpret '%s' as boolean\n",
                pszValue);
        dwError = APP_ERROR_INVALID_BOOLEAN;
        BAIL_ON_ERROR(dwError);
    }

    *pdwValue = dwValue;

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
DefaultValue(
    xmlDocPtr xmlDoc,
    xmlNodePtr pxDefault,
    PSTR *ppszValue
    )
{
    DWORD dwError = 0;
    xmlNodePtr child = NULL;
    xmlChar *xszDefault = NULL;
    DWORD dwLength = 0;
    PSTR pszValue = NULL;

    if (pxDefault->type != XML_ELEMENT_NODE)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    dwLength = 0;
    for (child = pxDefault->xmlChildrenNode; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
           continue;

        if (!xmlStrcmp(child->name, (const xmlChar*)"value"))
        {
            xszDefault = xmlNodeListGetString(
                                xmlDoc,
                                child->xmlChildrenNode,
                                TRUE);

            if (xszDefault)
                dwLength += strlen((PSTR)xszDefault);
            dwLength += 1;

            xmlFree(xszDefault);
            xszDefault = NULL;
        }
    }
    dwLength += 1;

    dwError = LwAllocateMemory(dwLength, (PVOID*)&pszValue);
    BAIL_ON_ERROR(dwError);

    dwLength = 0;
    for (child = pxDefault->xmlChildrenNode; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
           continue;

        if (!xmlStrcmp(child->name, (xmlChar*)"value"))
        {
            xszDefault = xmlNodeListGetString(
                                xmlDoc,
                                child->xmlChildrenNode,
                                TRUE);
            if (xszDefault)
            {
                strcpy(pszValue + dwLength, (PSTR)xszDefault);
                dwLength += strlen((PSTR)xszDefault);
            }
            pszValue[dwLength] = '\0';
            dwLength += 1;

            xmlFree(xszDefault);
            xszDefault = NULL;
        }
    }

    *ppszValue = pszValue;

cleanup:
    xmlFree(xszDefault);
    xszDefault = NULL;

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pszValue);
    goto cleanup;
}

DWORD
CapabilityEditRegistry(
    PCAPABILITY pCapability,
    PCSTR pszaArg,
    BOOLEAN bVerbose
    )
{
    DWORD dwError = 0;
    DWORD dwValue = 0;
    PREGISTRY pRegistry = NULL;
    DWORD dwType = -1;
    const BYTE *pData = NULL;
    DWORD dwDataSize = 0;
    PSTR pszRoot = NULL;
    PSTR pszKey = NULL;
    PSTR pszName = NULL;
    PSTR pszaValue = NULL;
    PCSTR pszValue = NULL;
    BOOLEAN bUsingDefault = FALSE;
    BOOLEAN bAccept = FALSE;

    if (!pCapability)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    if (!pCapability->pRegistry)
    {
       goto cleanup;
    }

    pRegistry = pCapability->pRegistry;

    if (pszaArg == NULL)
    {
        if (!pRegistry->pszaDefault)
        {
            fprintf(stderr, "Error: No parameter specified and no default value exist.\n");
            dwError = APP_ERROR_PARAMETER_REQUIRED;
            BAIL_ON_ERROR(dwError);
        }
        else
        {
            pszaArg = pRegistry->pszaDefault;
            bUsingDefault = TRUE;
        }
    }

    if (!strcmp(pRegistry->pszType, "string"))
    {
        dwError = ValidateString(pszaArg, pCapability->xmlDoc, pRegistry->pxRegistry, &bAccept);
        BAIL_ON_ERROR(dwError);

        if (!bAccept)
        {
            dwError = APP_ERROR_VALUE_NOT_ACCEPTED;
            BAIL_ON_ERROR(dwError);
        }
        dwType = REG_SZ;
        pData = (const BYTE*) pszaArg;
        dwDataSize = strlen(pszaArg) + 1;
    }
    else if (!strcmp(pRegistry->pszType, "multistring"))
    {
        dwType = REG_MULTI_SZ;
        pData = (const BYTE*) pszaArg;
        dwDataSize = UtilMultistringLength(pszaArg);
    }
    else if (!strcmp(pRegistry->pszType, "dword"))
    {
        dwError = CapabilityNormalizeDword(pszaArg, pRegistry->pxRegistry, &dwValue);
        BAIL_ON_ERROR(dwError);

        dwError = ValidateDword(dwValue, pRegistry->pxRegistry, &bAccept);
        BAIL_ON_ERROR(dwError);

        if (!bAccept)
        {
            dwError = APP_ERROR_VALUE_NOT_ACCEPTED;
            BAIL_ON_ERROR(dwError);
        }

        dwType = REG_DWORD;
        pData = (const BYTE*) &dwValue;
        dwDataSize = sizeof(dwValue);
    }
    else if (!strcmp(pRegistry->pszType, "boolean"))
    {
        dwError = CapabilityNormalizeBoolean(pszaArg, &dwValue);
        BAIL_ON_ERROR(dwError);

        dwType = REG_DWORD;
        pData = (const BYTE*) &dwValue;
        dwDataSize = sizeof(dwValue);
    }
    else
    {
        fprintf(stderr, "Unknown input type '%s'\n", pRegistry->pszType);
        dwError = APP_ERROR_UNKNOWN_TYPE;
        BAIL_ON_ERROR(dwError);
    }

    if (bVerbose)
    {
        dwError = GetLocalPolicyValue(pCapability, &pszaValue);
        BAIL_ON_ERROR(dwError);

        fprintf(stdout, "Current local policy value(s): ");
        if (pszaValue[0] == '\0')
        {
            fprintf(stdout, "\n");
        }
        else
        {
            for (pszValue = pszaValue;
                 *pszValue;
                 pszValue = pszValue + strlen(pszValue) + 1)
            {
                fprintf(stdout, "%s\n", pszValue);
            }
        }
    }

    if (bVerbose && bUsingDefault)
    {
        fprintf(stderr, "Using default value(s): ");
        if (pszaArg[0] == '\0')
        {
            fprintf(stdout, "\n");
        }
        else
        {
            for (pszValue = pszaArg;
                 *pszValue;
                 pszValue = pszValue + strlen(pszValue) + 1)
            {
                fprintf(stdout, "%s\n", pszValue);
            }
        }
    }

    dwError = UtilParseRegName(
                    pRegistry->pszLocalPath,
                    &pszRoot,
                    &pszKey,
                    &pszName);
    BAIL_ON_ERROR(dwError);

    dwError = UtilSetValueExA(
                    pszRoot,
                    pszKey,
                    pszName,
                    dwType,
                    pData,
                    dwDataSize);
    BAIL_ON_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszRoot);
    LW_SAFE_FREE_STRING(pszKey);
    LW_SAFE_FREE_STRING(pszName);
    LW_SAFE_FREE_MEMORY(pszaValue);

    return dwError;

error:
    goto cleanup;
}

DWORD
CapabilityApply(
    PCAPABILITY pCapability,
    BOOLEAN bVerbose
    )
{
    DWORD dwError = 0;
    PREGISTRY pRegistry = NULL;
    xmlNodePtr child = NULL;
    xmlChar *xszCommand = NULL;
    PSTR pszCommand = NULL;
    int ret = 0;

    if (!pCapability)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    if (!pCapability->pRegistry)
    {
       goto cleanup;
    }

    pRegistry = pCapability->pRegistry;

    for (child = pRegistry->pxRegistry->xmlChildrenNode; child; child =
            child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;

        if (!xmlStrcmp(child->name, (const xmlChar*) "apply"))
        {
            xszCommand = xmlGetProp(child, (const xmlChar*)"command");
            if (xszCommand)
            {
                if (bVerbose)
                {
                    dwError = LwAllocateStringPrintf(
                                    &pszCommand,
                                    "%s",
                                    (PCSTR)xszCommand);
                }
                else
                {
                    dwError = LwAllocateStringPrintf(
                                    &pszCommand,
                                    "%s >/dev/null 2>/dev/null",
                                    (PCSTR)xszCommand);
                }
                BAIL_ON_ERROR(dwError);

                if (bVerbose)
                    fprintf(stdout, "Executing command: %s\n", pszCommand);

                ret = system((PCSTR)pszCommand);
                if (ret == -1)
                {
                    fprintf(stderr, "Problem executing '%s'\n", pszCommand);
                    dwError = APP_ERROR_COULD_NOT_FORK;
                }
                else if (ret > 0)
                {
                    fprintf(stderr, "Problem executing '%s'\n", pszCommand);
                    dwError = APP_ERROR_PROGRAM_ERROR;
                }

                xmlFree(xszCommand);
                xszCommand = NULL;

                LW_SAFE_FREE_STRING(pszCommand);
            }
        }
    }

cleanup:
    LW_SAFE_FREE_STRING(pszCommand);
    xmlFree(xszCommand);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
GetLocalPolicyValue(
    PCAPABILITY pCapability,
    PSTR *ppszaValue
    )
{
    DWORD dwError = 0;
    PREGISTRY pRegistry = NULL;
    DWORD dwType = -1;
    const BYTE *pData = NULL;
    DWORD dwDataSize = 0;
    PSTR pszRoot = NULL;
    PSTR pszKey = NULL;
    PSTR pszName = NULL;
    DWORD dwValue = 0;
    PSTR pszaArg = NULL;
    PSTR pszaValue = NULL;
    PSTR pszBuf = NULL;

    if (!pCapability)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    if (!pCapability->pRegistry)
    {
       goto cleanup;
    }

    pRegistry = pCapability->pRegistry;

    if (!strcmp(pRegistry->pszType, "string"))
    {
        dwType = REG_SZ;
        pData = (const BYTE*) &pszaArg;
        dwDataSize = 0;
    }
    else if (!strcmp(pRegistry->pszType, "multistring"))
    {
        dwType = REG_MULTI_SZ;
        pData = (const BYTE*) &pszaArg;
        dwDataSize = 0;
    }
    else if (!strcmp(pRegistry->pszType, "dword"))
    {
        dwType = REG_DWORD;
        pData = (const BYTE*) &dwValue;
        dwDataSize = sizeof(dwValue);
    }
    else if (!strcmp(pRegistry->pszType, "boolean"))
    {
        dwType = REG_DWORD;
        pData = (const BYTE*) &dwValue;
        dwDataSize = sizeof(dwValue);
    }
    else
    {
        fprintf(stderr, "Unknown input type '%s'\n", pRegistry->pszType);
        dwError = APP_ERROR_UNKNOWN_TYPE;
        BAIL_ON_ERROR(dwError);
    }

    dwError = UtilParseRegName(
                    pRegistry->pszLocalPath,
                    &pszRoot,
                    &pszKey,
                    &pszName);
    BAIL_ON_ERROR(dwError);

    dwError = UtilGetValueExA(
                    pszRoot,
                    pszKey,
                    pszName,
                    dwType,
                    (PVOID)pData,
                    &dwDataSize);
    BAIL_ON_ERROR(dwError);

    if (!strcmp(pRegistry->pszType, "boolean"))
    {
        PCSTR pszTrue = "true";
        PCSTR pszFalse = "false";
        dwError = UtilAllocateMultistring(
                        dwValue ? &pszTrue : &pszFalse,
                        1,
                        &pszaValue);
        BAIL_ON_ERROR(dwError);
    }
    else if (dwType == REG_DWORD)
    {
        dwError = LwAllocateStringPrintf(
                        &pszBuf,
                        "%lu",
                        (unsigned long) dwValue);
        dwError = UtilAllocateMultistring((PCSTR*)&pszBuf, 1, &pszaValue);
        BAIL_ON_ERROR(dwError);
    }
    else if (dwType == REG_SZ)
    {
        dwError = UtilAllocateMultistring((PCSTR*)&pszaArg, 1, &pszaValue);
        BAIL_ON_ERROR(dwError);
    }
    else if (dwType == REG_MULTI_SZ)
    {
        dwError = UtilDuplicateMultistring(pszaArg, &pszaValue);
        BAIL_ON_ERROR(dwError);
    }

    *ppszaValue = pszaValue;

cleanup:

    LW_SAFE_FREE_STRING(pszRoot);
    LW_SAFE_FREE_STRING(pszKey);
    LW_SAFE_FREE_STRING(pszName);
    LW_SAFE_FREE_STRING(pszaArg);
    LW_SAFE_FREE_STRING(pszBuf);

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pszaValue);
    goto cleanup;

}

static
DWORD
CapabilityGetRegistry(
    PREGISTRY pRegistry,
    PSTR *ppszaArg,
    PDWORD pdwValue,
    PBOOLEAN pbLocalPolicy
)
{
    DWORD dwError = 0;
    DWORD dwType = -1;
    const BYTE *pData = NULL;
    DWORD dwDataSize = 0;
    PSTR pszRoot = NULL;
    PSTR pszKey = NULL;
    PSTR pszName = NULL;
    BOOLEAN bTryLocalPolicy = TRUE;

    if (!strcmp(pRegistry->pszType, "string"))
    {
        dwType = REG_SZ;
        pData = (const BYTE*) ppszaArg;
        dwDataSize = 0;
    }
    else if (!strcmp(pRegistry->pszType, "multistring"))
    {
        dwType = REG_MULTI_SZ;
        pData = (const BYTE*) ppszaArg;
        dwDataSize = 0;
    }
    else if (!strcmp(pRegistry->pszType, "dword"))
    {
        dwType = REG_DWORD;
        pData = (const BYTE*) pdwValue;
        dwDataSize = sizeof(*pdwValue);
    }
    else if (!strcmp(pRegistry->pszType, "boolean"))
    {
        dwType = REG_DWORD;
        pData = (const BYTE*) pdwValue;
        dwDataSize = sizeof(*pdwValue);
    }
    else
    {
        fprintf(stderr, "Unknown input type '%s'\n", pRegistry->pszType);
        dwError = APP_ERROR_UNKNOWN_TYPE;
        BAIL_ON_ERROR(dwError);
    }

    if (pRegistry->pszPolicyPath)
    {
        dwError = UtilParseRegName(
                        pRegistry->pszPolicyPath,
                        &pszRoot,
                        &pszKey,
                        &pszName);
        BAIL_ON_ERROR(dwError);

        dwError = UtilGetValueExA(
                        pszRoot,
                        pszKey,
                        pszName,
                        dwType,
                        (PVOID) pData,
                        &dwDataSize);
        LW_SAFE_FREE_STRING(pszRoot);
        LW_SAFE_FREE_STRING(pszKey);
        LW_SAFE_FREE_STRING(pszName);

        if (!dwError)
        {
            bTryLocalPolicy = FALSE;
        }
    }

    if (bTryLocalPolicy)
    {
        dwError = UtilParseRegName(
                        pRegistry->pszLocalPath,
                        &pszRoot,
                        &pszKey,
                        &pszName);
        BAIL_ON_ERROR(dwError);

        dwError = UtilGetValueExA(
                        pszRoot,
                        pszKey,
                        pszName,
                        dwType,
                        (PVOID)pData,
                        &dwDataSize);
        BAIL_ON_ERROR(dwError);

        *pbLocalPolicy = TRUE;
    }

error:
    return dwError;
}

DWORD
CapabilityDump(
    PCAPABILITY pCapability
    )
{
    DWORD dwError = 0;
    PREGISTRY pRegistry = NULL;
    DWORD dwValue = 0;
    PSTR pszaArg = NULL;
    PSTR pszEsc = NULL;
    BOOLEAN bLocalPolicy = FALSE;

    if (!pCapability)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    if (!pCapability->pRegistry)
    {
       goto cleanup;
    }

    pRegistry = pCapability->pRegistry;

    dwError = CapabilityGetRegistry(pRegistry, &pszaArg, &dwValue, &bLocalPolicy);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = 0;
        goto cleanup;
    }
    BAIL_ON_ERROR(dwError);

    fprintf(stdout, "%s", pCapability->pszName);
    if (!strcmp(pRegistry->pszType, "boolean"))
    {
        fprintf(stdout, " %s\n", dwValue ? "true" : "false");
    }
    else if (!strcmp(pRegistry->pszType, "dword"))
    {
        fprintf(stdout, " %lu\n", (unsigned long)dwValue);
    }
    else if (!strcmp(pRegistry->pszType, "string"))
    {
        dwError = UtilAllocateEscapedString(pszaArg, &pszEsc);
        BAIL_ON_ERROR(dwError);

        fprintf(stdout, " \"%s\"\n", pszEsc);

        LW_SAFE_FREE_STRING(pszEsc);
    }
    else if (!strcmp(pRegistry->pszType, "multistring"))
    {
        PCSTR pszStr = pszaArg;
        while (*pszStr)
        {
            dwError = UtilAllocateEscapedString(pszStr, &pszEsc);
            BAIL_ON_ERROR(dwError);

            fprintf(stdout, " \"%s\"", pszEsc);

            LW_SAFE_FREE_STRING(pszEsc);

            pszStr += strlen(pszStr) + 1;
        }
        fprintf(stdout, "\n");
    }

cleanup:

    LW_SAFE_FREE_STRING(pszaArg);
    LW_SAFE_FREE_STRING(pszEsc);

    return dwError;

error:
    goto cleanup;
}

DWORD
CapabilityShow(
    PCAPABILITY pCapability,
    BOOL bConcise
    )
{
    DWORD dwError = 0;
    PREGISTRY pRegistry = NULL;
    DWORD dwValue = 0;
    PSTR pszaArg = NULL;
    PSTR pszEsc = NULL;
    BOOLEAN bRegistryValueFound = FALSE;
    BOOLEAN bLocalPolicy = FALSE;

    if (!pCapability)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    if (!pCapability->pRegistry)
    {
       goto cleanup;
    }

    pRegistry = pCapability->pRegistry;

    dwError = CapabilityGetRegistry(pRegistry, &pszaArg, &dwValue, &bLocalPolicy);
    if (dwError == 0)
    {
        bRegistryValueFound = TRUE;
    }
    else if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = 0;
        bRegistryValueFound = FALSE;
    }
    BAIL_ON_ERROR(dwError);

    if (bConcise)
    {
        if (!bRegistryValueFound)
        {
            fprintf(stdout, "missing\n%s\n", pRegistry->pszType);
        }
        else if (!strcmp(pRegistry->pszType, "boolean"))
        {
            fprintf(stdout, "boolean\n%s\n", dwValue ? "true" : "false");
        }
        else if (!strcmp(pRegistry->pszType, "dword"))
        {
            fprintf(stdout, "dword\n%lu\n", (unsigned long) dwValue);
        }
        else if (!strcmp(pRegistry->pszType, "string"))
        {
            fprintf(stdout, "string\n%s\n", pszaArg);
        }
        else if (!strcmp(pRegistry->pszType, "multistring"))
        {
            fprintf(stdout, "multistring\n");
            PCSTR pszStr = pszaArg;
            while (*pszStr)
            {
                fprintf(stdout, "%s\n", pszStr);
                pszStr += strlen(pszStr) + 1;
            }
            fprintf(stdout, "\n");
        }

        if (bRegistryValueFound)
        {
            if (bLocalPolicy)
            {
                fprintf(stdout, "local policy\n");
            }
            else
            {
                fprintf(stdout, "group policy\n");
            }
        }
        else
        {
            fprintf(stdout, "no policy\n");
        }
    }
    else
    {
        fprintf(stdout, "Name: %s\n", pCapability->pszName);
        fprintf(stdout, "Description: %s\n",
                pCapability->pszDescription ? pCapability->pszDescription : "");
        if (!bRegistryValueFound)
        {
            fprintf(stdout, "Type (Missing From Registry): %s\n",
                    pCapability->pRegistry->pszType);
        }
        else
        {
            fprintf(stdout, "Type: %s\n", pCapability->pRegistry->pszType);
            if (!strcmp(pRegistry->pszType, "boolean"))
            {
                fprintf(stdout, "Current Value: %s\n", dwValue ? "true" : "false");
                fprintf(stdout, "Accepted Values: true, false\n");
            }
            else if (!strcmp(pRegistry->pszType, "dword"))
            {
                xmlNodePtr xmlAccept = NULL;
                fprintf(stdout, "Current Value: %lu\n", (unsigned long) dwValue);

                xmlAccept = FindNext(
                               pRegistry->pxRegistry->xmlChildrenNode,
                               "accept");
                if (!xmlAccept)
                {
                    fprintf(stdout, "Accepted Range: [0, 4294967295]\n");
                }
                while (xmlAccept)
                {
                    xmlNodePtr xmlRange = NULL;

                    xmlRange = FindNext(xmlAccept->xmlChildrenNode, "range");
                    while(xmlRange)
                    {
                        xmlChar *xszMin = NULL;
                        xmlChar *xszMax = NULL;

                        xszMin = xmlGetProp(xmlRange, (const xmlChar*)"min");
                        xszMax = xmlGetProp(xmlRange, (const xmlChar*)"max");
                        if (xszMin && xszMax)
                        {
                            fprintf(stdout,
                                    "Accepted Range: [%s, %s]\n",
                                    xszMin,
                                    xszMax);
                        }
                        if (xszMin)
                        {
                            xmlFree(xszMin);
                            xszMin = NULL;
                        }
                        if (xszMax)
                        {
                            xmlFree(xszMax);
                            xszMax = NULL;
                        }
                        xmlRange = FindNext(xmlRange->next, "range");
                    }
                    xmlAccept = FindNext(xmlAccept->next, "accept");
                }
            }
            else if (!strcmp(pRegistry->pszType, "string"))
            {
                xmlNodePtr xmlAccept = NULL;

                dwError = UtilAllocateEscapedString(pszaArg, &pszEsc);
                BAIL_ON_ERROR(dwError);

                fprintf(stdout, "Current Value: \"%s\"\n", pszEsc);

                LW_SAFE_FREE_STRING(pszEsc);

                xmlAccept = FindNext(
                               pRegistry->pxRegistry->xmlChildrenNode,
                               "accept");
                while (xmlAccept)
                {
                    xmlNodePtr xmlValue = NULL;

                    xmlValue = FindNext(xmlAccept->xmlChildrenNode, "value");
                    while(xmlValue)
                    {
                        xmlChar *xszValue = NULL;

                        xszValue = xmlNodeListGetString(
                                        pCapability->xmlDoc,
                                        xmlValue->xmlChildrenNode,
                                        TRUE);
                        if (xszValue)
                        {
                            fprintf(stdout, "Acceptable Value: \"%s\"\n", (PSTR)xszValue);
                            xmlFree(xszValue);
                            xszValue = NULL;
                        }
                        xmlValue = FindNext(xmlValue->next, "value");
                    }
                    xmlAccept = FindNext(xmlAccept->next, "accept");
                }
            }
            else if (!strcmp(pRegistry->pszType, "multistring"))
            {
                fprintf(stdout, "Current Values:\n");
                PCSTR pszStr = pszaArg;
                while (*pszStr)
                {
                    dwError = UtilAllocateEscapedString(pszStr, &pszEsc);
                    BAIL_ON_ERROR(dwError);

                    fprintf(stdout, "\"%s\"\n", pszEsc);

                    LW_SAFE_FREE_STRING(pszEsc);

                    pszStr += strlen(pszStr) + 1;
                }
            }

            if (bLocalPolicy)
                fprintf(stdout, "Current Value is determined by local policy.\n");
            else
                fprintf(stdout, "Value is determined by group policy in Active Directory.\n");
        }
    }
cleanup:

    LW_SAFE_FREE_STRING(pszaArg);
    LW_SAFE_FREE_STRING(pszEsc);

    return dwError;

error:
    goto cleanup;
}

static
xmlNodePtr
FindNext(
    xmlNodePtr pxChild,
    PSTR pszChild)
{
    while (pxChild)
    {
        if (pxChild->type == XML_ELEMENT_NODE)
        {
            if (!xmlStrcmp(pxChild->name, (const xmlChar*)pszChild))
            {
                break;
            }
        }
        pxChild = pxChild->next;
    }
    return pxChild;
}

