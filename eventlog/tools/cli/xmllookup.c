/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

#include "includes.h"

static
xmlNodePtr
FindNext(
    xmlNodePtr pxChild,
    PCSTR pszChild
    );

DWORD
XmlGetSqlQuery(
    PCSTR pszName,
    BOOLEAN bFuzzyMatch,
    PSTR *ppszQuery
    )
{
    DWORD dwError = 0;
    xmlDocPtr xmlDoc = NULL;
    xmlXPathContextPtr xpathCtx = NULL;
    xmlXPathObjectPtr xpathObj = NULL;
    xmlNodeSetPtr nodes = NULL; // Convienence pointer
    xmlNodePtr xmlReport = NULL;
    DWORD dwNameLength = 0;
    BOOLEAN bNameMatchesMany = FALSE;
    BOOLEAN bNameMatchesExactly = FALSE;
    size_t i;
    xmlNodePtr child = NULL;
    xmlChar *xszName = NULL;
    xmlNodePtr xmlQuery = NULL;
    xmlChar *xszQuery = NULL;
    PSTR pszQuery = NULL;

    xmlInitParser();

    xmlDoc = xmlParseFile(LWREPORT_XML);
    if (!xmlDoc)
    {
        dwError = APP_ERROR_XML_PARSE_FAILED;
        BAIL_ON_EVT_ERROR(dwError);
    }

    xpathCtx = xmlXPathNewContext(xmlDoc);
    if (!xpathCtx)
    {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_EVT_ERROR(dwError);
    }

    xpathObj = xmlXPathEvalExpression(
                    (const xmlChar*)"/reports/section/report",
                    xpathCtx);
    if (!xpathObj)
    {
        dwError = APP_ERROR_XPATH_EVAL_FAILED;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwNameLength = strlen(pszName); // Calculate once
    nodes = xpathObj->nodesetval;
    for (i = 0; i < nodes->nodeNr; i++)
    {
        if (nodes->nodeTab[i]->type != XML_ELEMENT_NODE)
            continue;

        if (xmlStrcmp((const xmlChar*)"report", nodes->nodeTab[i]->name))
            continue;

        child = FindNext(nodes->nodeTab[i]->xmlChildrenNode, "name");
        if (!child)
            continue;

        xszName = xmlNodeListGetString(xmlDoc, child->xmlChildrenNode, TRUE);
        if (!xszName)
        {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_EVT_ERROR(dwError);
        }

        if (!xmlStrcasecmp(xszName, (const xmlChar*)pszName))
        {
            if (bNameMatchesExactly)
            {
                dwError = APP_ERROR_XML_DUPLICATED_ELEMENT;
                BAIL_ON_EVT_ERROR(dwError);
            }

            xmlReport = nodes->nodeTab[i];
            bNameMatchesExactly = TRUE;
        }
        else if (bFuzzyMatch &&
                 !xmlStrncasecmp(xszName, (const xmlChar*)pszName, dwNameLength)
                 && dwNameLength == strcspn(pszName, " =()<>"))
        {
            if (!bNameMatchesExactly)
            {
                if (xmlReport)
                {
                    bNameMatchesMany = TRUE;
                }
                else
                {
                    xmlReport = nodes->nodeTab[i];
                }
            }
        }

        xmlFree(xszName);
        xszName = NULL;
    }

    if (bFuzzyMatch && !bNameMatchesExactly && bNameMatchesMany)
    {
        fprintf(stderr, "%s partially matches:\n", pszName);
        for (i = 0; i < nodes->nodeNr; i++)
        {
            if (nodes->nodeTab[i]->type != XML_ELEMENT_NODE)
                continue;

            if (xmlStrcmp((const xmlChar*)"report", nodes->nodeTab[i]->name))
                continue;

            child = FindNext(nodes->nodeTab[i]->xmlChildrenNode, "name");
            if (!child)
                continue;

            xszName = xmlNodeListGetString(xmlDoc, child->xmlChildrenNode, TRUE);
            if (!xszName)
            {
                dwError = ERROR_OUTOFMEMORY;
                BAIL_ON_EVT_ERROR(dwError);
            }

            if (!xmlStrncasecmp(xszName, (const xmlChar*)pszName, dwNameLength)
                && dwNameLength == strcspn(pszName, " =()<>"))
            {
                fprintf(stderr, "%s\n", (PCSTR)xszName);
            }

            xmlFree(xszName);
            xszName = NULL;
        }
        dwError = APP_ERROR_REPORT_MULTIPLE_MATCHES;
        BAIL_ON_EVT_ERROR(dwError);
    }
    else if (!xmlReport)
    {
        dwError = APP_ERROR_REPORT_NOT_FOUND;
        BAIL_ON_EVT_ERROR(dwError);
    }

    xmlQuery = FindNext(xmlReport->xmlChildrenNode, "query");
    if (!xmlQuery)
    {
        dwError = APP_ERROR_XML_MISSING_ELEMENT;
        BAIL_ON_EVT_ERROR(dwError);
    }

    xszQuery = xmlGetProp(xmlQuery, (const xmlChar*)"sql");
    if (!xszQuery)
    {
        dwError = APP_ERROR_XML_MISSING_ATTRIBUTE;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(&pszQuery, "%s", (PCSTR)xszQuery);
    BAIL_ON_EVT_ERROR(dwError);

    *ppszQuery = pszQuery;

cleanup:

    xmlFree(xszQuery);
    xszQuery = NULL;

    xmlFreeDoc(xmlDoc);
    xmlDoc = NULL;

    xmlCleanupParser();

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszQuery);

    goto cleanup;
}

DWORD
XmlList(
    VOID
    )
{
    DWORD dwError = 0;
    xmlDocPtr xmlDoc = NULL;
    xmlXPathContextPtr xpathCtx = NULL;
    xmlXPathObjectPtr xpathObj = NULL;
    xmlNodeSetPtr nodes = NULL;
    size_t i;

    xmlInitParser();

    xmlDoc = xmlParseFile(LWREPORT_XML);
    if (!xmlDoc)
    {
        dwError = APP_ERROR_XML_PARSE_FAILED;
        BAIL_ON_EVT_ERROR(dwError);
    }

    xpathCtx = xmlXPathNewContext(xmlDoc);
    if (!xpathCtx)
    {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_EVT_ERROR(dwError);
    }

    xpathObj = xmlXPathEvalExpression(
                    (const xmlChar*)"/reports/section",
                    xpathCtx);
    if (!xpathObj)
    {
        dwError = APP_ERROR_XPATH_EVAL_FAILED;
        BAIL_ON_EVT_ERROR(dwError);
    }

    nodes = xpathObj->nodesetval;
    for (i = 0; i < nodes->nodeNr; i++)
    {
        xmlNodePtr cur = NULL;
        xmlNodePtr child = NULL;
        xmlChar *section = NULL;

        if (nodes->nodeTab[i]->type != XML_ELEMENT_NODE)
            continue;

        cur = nodes->nodeTab[i];

        section = xmlGetProp(cur, (const xmlChar*) "section");
        if (section)
            fprintf(stdout, "[%s]\n", section);
        else
            fprintf(stdout, "\n");
        xmlFree(section);
        section = NULL;

        for (child = cur->xmlChildrenNode; child; child = child->next)
        {
            xmlNodePtr xmlName = NULL;
            xmlChar *xszName = NULL;

            if (child->type != XML_ELEMENT_NODE)
                continue;
            if (xmlStrcmp(child->name, (const xmlChar*)"report"))
                continue;

            xmlName = FindNext(child->xmlChildrenNode, "name");
            if (!xmlName)
                continue;

            xszName = xmlNodeListGetString(xmlDoc, xmlName->xmlChildrenNode, TRUE);
            if (!xszName)
            {
                dwError = ERROR_OUTOFMEMORY;
                BAIL_ON_EVT_ERROR(dwError);
            }

            fprintf(stdout, "\t%s\n", (PCSTR) xszName);

            xmlFree(xszName);
            xszName = NULL;
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
xmlNodePtr
FindNext(
    xmlNodePtr pxChild,
    PCSTR pszChild
    )
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

