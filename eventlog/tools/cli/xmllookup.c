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
XmlFindByName(
    xmlDocPtr xmlDoc,
    PCSTR pszName,
    BOOLEAN bFuzzyMatch,
    xmlNodePtr *pxmlReport
    );

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
    PWSTR *ppwszQuery
    )
{
    DWORD dwError = 0;
    xmlDocPtr xmlDoc = NULL;
    xmlNodePtr xmlReport = NULL;
    xmlNodePtr xmlQuery = NULL;
    xmlChar *xszQuery = NULL;
    PWSTR pwszQuery = NULL;

    xmlInitParser();

    xmlDoc = xmlParseFile(LWREPORT_XML);
    if (!xmlDoc)
    {
        dwError = APP_ERROR_XML_PARSE_FAILED;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = XmlFindByName(xmlDoc, pszName, bFuzzyMatch, &xmlReport);
    BAIL_ON_EVT_ERROR(dwError);

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

    dwError = LwAllocateWc16sPrintfW(&pwszQuery, L"%s", (PCSTR)xszQuery);
    BAIL_ON_EVT_ERROR(dwError);

    *ppwszQuery = pwszQuery;

cleanup:

    xmlFree(xszQuery);
    xszQuery = NULL;

    xmlFreeDoc(xmlDoc);
    xmlDoc = NULL;

    xmlCleanupParser();

    return dwError;

error:

    LW_SAFE_FREE_MEMORY(pwszQuery);

    goto cleanup;
}

static
DWORD
XmlFindByName(
    xmlDocPtr xmlDoc,
    PCSTR pszName,
    BOOLEAN bFuzzyMatch,
    xmlNodePtr *pxmlReport
    )
{
    DWORD dwError = 0;

    xmlXPathContextPtr xpathCtx = NULL;
    xmlXPathObjectPtr xpathObj = NULL;
    xmlNodeSetPtr nodes = NULL; // Convienence pointer
    xmlNodePtr xmlNodeReport = NULL;
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

            xmlNodeReport = nodes->nodeTab[i];
            bNameMatchesExactly = TRUE;
        }
        else if (bFuzzyMatch &&
                 !xmlStrncasecmp(xszName, (const xmlChar*)pszName, dwNameLength)
                 && dwNameLength == strcspn(pszName, " =()<>"))
        {
            if (!bNameMatchesExactly)
            {
                if (xmlNodeReport)
                {
                    bNameMatchesMany = TRUE;
                }
                else
                {
                    xmlNodeReport = nodes->nodeTab[i];
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
    else if (!xmlNodeReport)
    {
        dwError = APP_ERROR_REPORT_NOT_FOUND;
        BAIL_ON_EVT_ERROR(dwError);
    }

    *pxmlReport = xmlNodeReport;

cleanup:

    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);

    xmlFree(xszName);
    xszName = NULL;

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

