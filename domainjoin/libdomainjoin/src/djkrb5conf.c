/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "domainjoin.h"
#include "ctarray.h"
#include "ctstrutils.h"
#include "ctshell.h"
#include "djstr.h"
#include "djauthinfo.h"
#include <lsa/lsa.h>
#include <reg/regutil.h>

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

/*Krb5 configuration files follow a modified ini file format. In it there are several stanzas:
[logging]

[realms]

Each stanza can contain name = value pairs. There can be multiple names in the same stanza:
[stanza]
name1 = value
name2 = value
name2 = value

Stanzas can also contain compound elements which contain more compound elements or name value pairs:
[stanza]
name1 = {
subelement1 = value
subelement2 = value
}

Comments are started with a # or a ;. They must be on lines by themselves. So there cannot be a comment on a name value line.
[stanza]
#comment
;comment
name = value ;not a comment

This file format is parsed as nested Krb5Entrys. The top most entry is the root node. It contains stanzas as subelements. The stanzas contain name value pairs and compound elements.

Comment lines can also be included in any of the entry types. Comments have no children, no name, no beginSeparator, and no value. The comment mark and value are stored in the leadingWhiteSpace variable.

Name value pairs can be distinguished from compound elements because name value pairs are leaf entries (have no children).

Stanzas can have no children, so they must be distinguished by their level in the hierarchy. Stanzas are always at the second level.

A parsed name value pair can be written out in the following form (spaces are added for readability and should not be written):
<leadingWhiteSpace> <name> <beginSeparator (is =)> <value>

A parsed compound element can be written out as:
<leadingWhiteSpace> <name> <beginSeparator (is = {)> <newline>
<subelements>
<leadingWhiteSpace> <value (is })>
*/

typedef struct _Krb5Entry
{
    struct _Krb5Entry *parent;
    char *leadingWhiteSpace;
    CTParseToken name;
    CTParseToken beginSeparator;
    DynamicArray subelements;
    CTParseToken value;
} Krb5Entry;

typedef BOOLEAN (*NodeClassifier)(const Krb5Entry *entry);

static BOOLEAN IsStanzaEntry(const Krb5Entry *entry)
{
    //Stanzas have a name, but no beginning separator or value
    return entry->name.value != NULL &&
        entry->beginSeparator.value == NULL &&
        entry->value.value == NULL;
}

static BOOLEAN IsGroupEntry(const Krb5Entry *entry)
{
    return CTStrEndsWith(entry->beginSeparator.value, "{");
}

static BOOLEAN IsValueEntry(const Krb5Entry *entry)
{
    return entry->beginSeparator.value != NULL &&
        !strcmp(entry->beginSeparator.value, "=");
}

static BOOLEAN IsCommentEntry(const Krb5Entry *entry)
{
    return entry->name.value == NULL &&
        entry->beginSeparator.value == NULL &&
        entry->value.value == NULL &&
        entry->subelements.size == 0;
}

static BOOLEAN IsRootEntry(const Krb5Entry *entry)
{
    return entry->parent == NULL;
}

static Krb5Entry * GetChild(Krb5Entry *entry, size_t child)
{
    if(child >= entry->subelements.size)
        return NULL;
    return ((Krb5Entry **)entry->subelements.data)[child];
}

static const Krb5Entry * GetChildConst(const Krb5Entry *entry, size_t child)
{
    if(child >= entry->subelements.size)
        return NULL;
    return ((const Krb5Entry **)entry->subelements.data)[child];
}

static DWORD InsertChildNode(Krb5Entry *parent, size_t index, Krb5Entry *child)
{
    child->parent = parent;
    return CTArrayInsert(&parent->subelements, index, sizeof(child),
            &child, 1);
}

static DWORD AddChildNode(Krb5Entry *parent, Krb5Entry *child)
{
    //If the new node is a comment, add it at the bottom, otherwise add it at
    //the bottom before the existing comments (mostly blank lines).
    size_t index = parent->subelements.size;
    const char *parentName = parent->name.value;
    const char *childName = child->name.value;
    if(parentName == NULL)
        parentName = "(null)";
    if(childName == NULL)
        childName = "(null)";
    DJ_LOG_VERBOSE("Adding child '%s' to '%s'", childName, parentName);
    if(!IsCommentEntry(child))
    {
        while(index > 0)
        {
            Krb5Entry *child = GetChild(parent, index - 1);
            if(!IsCommentEntry(child))
                break;
            index--;
        }
    }
    return InsertChildNode(parent, index, child);
}

static void FreeKrb5Entry(Krb5Entry **entry);

static void DeleteAllChildren(Krb5Entry *entry)
{
    size_t i;
    for(i = 0; i < entry->subelements.size; i++)
    {
        Krb5Entry *child = GetChild(entry, i);
        FreeKrb5Entry(&child);
    }
    CTArrayFree(&entry->subelements);
}

static void FreeKrb5EntryContents(Krb5Entry *entry)
{
    DeleteAllChildren(entry);
    CT_SAFE_FREE_STRING(entry->leadingWhiteSpace);
    CTFreeParseTokenContents(&entry->name);
    CTFreeParseTokenContents(&entry->beginSeparator);
    CTFreeParseTokenContents(&entry->value);
}

static void FreeKrb5Entry(Krb5Entry **entry)
{
    if(*entry != NULL)
    {
        FreeKrb5EntryContents(*entry);
        CT_SAFE_FREE_MEMORY(*entry);
    }
}

static DWORD CopyEntry(const Krb5Entry *source, Krb5Entry **copy)
{
    DWORD ceError = ERROR_SUCCESS;
    Krb5Entry *created = NULL;
    Krb5Entry *copiedChild = NULL;
    size_t i;
    GCE(ceError = CTAllocateMemory(sizeof(*created), (void **) (void*)&created));
    GCE(ceError = CTDupOrNullStr(source->leadingWhiteSpace, &created->leadingWhiteSpace));
    GCE(ceError = CTCopyTokenContents(&created->name, &source->name));
    GCE(ceError = CTCopyTokenContents(&created->beginSeparator, &source->beginSeparator));
    GCE(ceError = CTCopyTokenContents(&created->value, &source->value));
    for(i = 0; i < source->subelements.size; i++)
    {
        const Krb5Entry *sourceChild = GetChildConst(source, i);
        GCE(ceError = CopyEntry(sourceChild, &copiedChild));
        GCE(ceError = InsertChildNode(created, i, copiedChild));
        copiedChild = NULL;
    }
    *copy = created;
    created = NULL;

cleanup:
    FreeKrb5Entry(&created);
    FreeKrb5Entry(&copiedChild);
    return ceError;
}

static DWORD ReadKrb5File(const char *rootPrefix, const char *filename, Krb5Entry *conf);

static DWORD WriteKrb5Configuration(const char *rootPrefix, const char *filename, Krb5Entry *conf, BOOLEAN *modified);

static Krb5Entry * GetRootNode(Krb5Entry *child)
{
    while(child->parent != NULL)
    {
        child = child->parent;
    }
    return child;
}

static int GetEntryDepth(const Krb5Entry *entry)
{
    int depth = -1;
    while(entry != NULL)
    {
        entry = entry->parent;
        depth++;
    }
    return depth;
}

static DWORD ParseLine(Krb5Entry **parent, const char *linestr, const char **endptr)
{
    DWORD ceError = ERROR_SUCCESS;
    const char *pos = linestr;
    const char *token_start = NULL;
    const char *oldpos;
    Krb5Entry *line = NULL;
    BOOLEAN expectChildren = FALSE;

    GCE(ceError = CTAllocateMemory(sizeof(*line), (void**) (void*)&line));

    /* Find the leading whitespace in the line */
    token_start = pos;
    while(isblank(*pos)) pos++;
    if(*pos == '#' || *pos == ';')
    {
        //This is a comment line. The whole line is leading white space
        while(*pos != '\0' && *pos != '\n' && *pos != '\r') pos++;
    }
    GCE(ceError = CTStrndup(token_start, pos - token_start, &line->leadingWhiteSpace));

    if(*pos == '\0' || *pos == '\n' || *pos == '\r')
    {
        DJ_LOG_VERBOSE("Found krb5 comment '%s'", linestr);
        //This is a comment line
    }
    else if(*pos == '}')
    {
        DJ_LOG_VERBOSE("Found krb5 compound end '%s'", linestr);
        //This is the end of a compound statement
        if(!IsGroupEntry(*parent))
        {
            DJ_LOG_ERROR("Expecting line '%s' to end a compound statement, but no compound statement appears before it",
                    linestr);
            GCE(ceError = ERROR_BAD_FORMAT);
        }
        GCE(ceError = CTReadToken(&pos, &(*parent)->value, "", "\r\n", " \t"));
        *parent = (*parent)->parent;
        FreeKrb5Entry(&line);
        goto cleanup;
    }
    else if(*pos == '[')
    {
        size_t len;
        DJ_LOG_VERBOSE("Found krb5 stanza '%s'", linestr);
        //This is a stanza
        *parent = GetRootNode(*parent);
        //Trim [
        pos++;
        GCE(ceError = CTReadToken(&pos, &line->name, "", "\r\n", " \t"));
        //Trim ]
        len = strlen(line->name.value);
        if(line->name.value[len - 1] == ']')
            line->name.value[len - 1] = 0;
        else
        {
            DJ_LOG_ERROR("Expecting krb5 stanza name '%s' to end with ]",
                    line->name.value);
            GCE(ceError = ERROR_BAD_FORMAT);
        }
        //Add future lines under this stanza
        expectChildren = TRUE;
    }
    else
    {
        //This is either a name value pair, or a compound element
        GCE(ceError = CTReadToken(&pos, &line->name, " \t", "=\r\n", ""));
        if(*pos != '=')
        {
            DJ_LOG_ERROR("Expecting krb5 name value or compound statement '%s' to have a = at position %d",
                    linestr, pos - linestr);
            GCE(ceError = ERROR_BAD_FORMAT);
        }
        oldpos = pos;
        GCE(ceError = CTReadToken(&pos, &line->beginSeparator, " \t", "\r\n", ""));
        if(*pos == '{')
        {
            DJ_LOG_VERBOSE("Found krb5 compound statement '%s'", linestr);
            //Oops, looks like this was really a compound statement, so we want to store the = and the { in the beginSeparator.
            CTFreeParseTokenContents(&line->beginSeparator);
            pos = oldpos;
            GCE(ceError = CTReadToken(&pos, &line->beginSeparator, "", "\r\n", " \t"));
            if(!CTStrEndsWith(line->beginSeparator.value, "{"))
            {
                DJ_LOG_ERROR("Expecting krb5 compound statement line '%s' to end with a {",
                        linestr);
                GCE(ceError = ERROR_BAD_FORMAT);
            }
            //Add future lines under this statement
            expectChildren = TRUE;
        }
        else
        {
            DJ_LOG_VERBOSE("Found krb5 name value pair '%s'", linestr);
            //This is name value statement
            GCE(ceError = CTReadToken(&pos, &line->value, "", "\r\n", " \t"));
        }
    }

    GCE(ceError = InsertChildNode(*parent, (*parent)->subelements.size, line));
    if(expectChildren)
    {
        *parent = line;
    }

cleanup:
    if(endptr != NULL)
        *endptr = pos;

    if(ceError)
        FreeKrb5Entry(&line);
    return ceError;
}

static DWORD ReadKrb5File(const char *rootPrefix, const char *filename, Krb5Entry *conf)
{
    DWORD ceError = ERROR_SUCCESS;
    FILE *file = NULL;
    PSTR buffer = NULL;
    char *fullPath = NULL;
    BOOLEAN endOfFile = FALSE;
    BOOLEAN exists;
    Krb5Entry *currentEntry = conf;

    memset(currentEntry, 0, sizeof(*currentEntry));

    GCE(ceError = CTAllocateStringPrintf(
            &fullPath, "%s%s", rootPrefix, filename));
    DJ_LOG_INFO("Reading krb5 file %s", fullPath);
    GCE(ceError = CTCheckFileOrLinkExists(fullPath, &exists));
    if(!exists)
    {
        DJ_LOG_INFO("File %s does not exist", fullPath);
        ceError = ERROR_FILE_NOT_FOUND;
        goto cleanup;
    }

    GCE(ceError = CTOpenFile(fullPath, "r", &file));
    while(TRUE)
    {
        GCE(ceError = CTReadNextLine(file, &buffer, &endOfFile));
        if(endOfFile)
            break;
        GCE(ceError = ParseLine(&currentEntry, buffer, NULL));
    }
    GCE(ceError = CTCloseFile(file));
    file = NULL;

cleanup:
    CT_SAFE_FREE_STRING(buffer);
    if(file != NULL)
        CTCloseFile(file);
    CT_SAFE_FREE_STRING(fullPath);
    if(ceError)
        FreeKrb5EntryContents(conf);
    return ceError;
}

static ssize_t FindNodeIndex(Krb5Entry *parent, size_t startIndex, const char *name)
{
    size_t i;
    for(i = startIndex; i < parent->subelements.size; i++)
    {
        Krb5Entry *child = GetChild(parent, i);
        if(child->name.value != NULL && !strcmp(child->name.value, name))
            return i;
    }
    return -1;
}

static Krb5Entry *FindEntryOfNameAndType(Krb5Entry *parent, int desiredDepth, const char *name, NodeClassifier type)
{
    int parentDepth = GetEntryDepth(parent);
    size_t i;
    if(parentDepth + 1 == desiredDepth)
    {
        for(i = 0; i < parent->subelements.size; i++)
        {
            Krb5Entry *child = GetChild(parent, i);
            if(child->name.value != NULL &&
                    !strcmp(child->name.value, name) && type(child))
            {
                return child;
            }
        }
    }
    else if(parentDepth < desiredDepth)
    {
        for(i = 0; i < parent->subelements.size; i++)
        {
            Krb5Entry *child = GetChild(parent, i);
            Krb5Entry *result = FindEntryOfNameAndType(child, desiredDepth, name, type);
            if(result != NULL)
                return result;
        }
    }
    return NULL;
}

static Krb5Entry *FindEntryOfType(Krb5Entry *parent, int desiredDepth, NodeClassifier type)
{
    int parentDepth = GetEntryDepth(parent);
    size_t i;
    if(parentDepth + 1 == desiredDepth)
    {
        for(i = 0; i < parent->subelements.size; i++)
        {
            Krb5Entry *child = GetChild(parent, i);
            if(type(child))
                return child;
        }
    }
    else if(parentDepth < desiredDepth)
    {
        for(i = 0; i < parent->subelements.size; i++)
        {
            Krb5Entry *child = GetChild(parent, i);
            Krb5Entry *result = FindEntryOfType(child, desiredDepth, type);
            if(result != NULL)
                return result;
        }
    }
    return NULL;
}

static DWORD CreateValueNode(Krb5Entry *conf, int depth, const char *name, const char *value, Krb5Entry **result)
{
    DWORD ceError = ERROR_SUCCESS;
    Krb5Entry *existing;
    Krb5Entry *created = NULL;

    *result = NULL;

    //Try to find a similar node to copy. This will keep the spacing consistent.
    existing = FindEntryOfNameAndType(conf, depth, name, IsValueEntry);
    if(existing == NULL)
        existing = FindEntryOfType(conf, depth, IsValueEntry);

    if(existing == NULL)
    {
        //Couldn't find a template. Got to create it from scratch
        GCE(ceError = CTAllocateMemory(sizeof(*created), (void**) (void*)&created));
        //Use one space to indent everything past the stanza level
        GCE(ceError = CTAllocateMemory(depth + 1, (void**) (void*)&created->leadingWhiteSpace));
        memset(created->leadingWhiteSpace, ' ', depth);
        created->leadingWhiteSpace[depth] = 0;
        GCE(ceError = CTStrdup(" ", &created->name.trailingSeparator));
        GCE(ceError = CTStrdup("=", &created->beginSeparator.value));
        GCE(ceError = CTStrdup(" ", &created->beginSeparator.trailingSeparator));
    }
    else
    {
        GCE(ceError = CopyEntry(existing, &created));
    }
    CT_SAFE_FREE_STRING(created->name.value);
    GCE(ceError = CTStrdup(name, &created->name.value));
    CT_SAFE_FREE_STRING(created->value.value);
    GCE(ceError = CTStrdup(value, &created->value.value));

cleanup:
    if(!ceError)
        *result = created;
    else
        FreeKrb5Entry(&created);
    return ceError;
}

static DWORD CreateGroupNode(Krb5Entry *conf, int depth, const char *name, Krb5Entry **result)
{
    DWORD ceError = ERROR_SUCCESS;
    Krb5Entry *existing;
    Krb5Entry *created = NULL;

    *result = NULL;

    //Try to find a similar node to copy. This will keep the spacing consistent.
    existing = FindEntryOfNameAndType(conf, depth, name, IsGroupEntry);
    if(existing == NULL)
        existing = FindEntryOfType(conf, depth, IsGroupEntry);

    if(existing == NULL)
    {
        //Couldn't find a template. Got to create it from scratch
        GCE(ceError = CTAllocateMemory(sizeof(*created), (void**) (void*)&created));
        //Use one space to indent everything past the stanza level
        GCE(ceError = CTAllocateMemory(depth + 1, (void**) (void*)&created->leadingWhiteSpace));
        memset(created->leadingWhiteSpace, ' ', depth);
        created->leadingWhiteSpace[depth] = 0;
        GCE(ceError = CTStrdup(" ", &created->name.trailingSeparator));
        GCE(ceError = CTStrdup("= {", &created->beginSeparator.value));
        GCE(ceError = CTStrdup("}", &created->value.value));
    }
    else
    {
        GCE(ceError = CopyEntry(existing, &created));
    }
    CT_SAFE_FREE_STRING(created->name.value);
    GCE(ceError = CTStrdup(name, &created->name.value));
    DeleteAllChildren(created);

cleanup:
    if(!ceError)
        *result = created;
    else
        FreeKrb5Entry(&created);
    return ceError;
}

static DWORD CreateStanzaNode(Krb5Entry *conf, const char *name, Krb5Entry **result)
{
    DWORD ceError = ERROR_SUCCESS;
    Krb5Entry *existing;
    Krb5Entry *created = NULL;

    *result = NULL;

    //Try to find a similar node to copy. This will keep the spacing consistent.
    existing = FindEntryOfNameAndType(conf, 1, name, IsStanzaEntry);
    if(existing == NULL)
        existing = FindEntryOfType(conf, 1, IsStanzaEntry);

    if(existing == NULL)
    {
        //Couldn't find a template. Got to create it from scratch
        GCE(ceError = CTAllocateMemory(sizeof(*created), (void**) (void*)&created));
    }
    else
    {
        GCE(ceError = CopyEntry(existing, &created));
    }
    CT_SAFE_FREE_STRING(created->name.value);
    GCE(ceError = CTStrdup(name, &created->name.value));
    DeleteAllChildren(created);

cleanup:
    if(!ceError)
        *result = created;
    else
        FreeKrb5Entry(&created);
    return ceError;
}

static Krb5Entry *GetFirstNode(Krb5Entry *parent, const char *name)
{
    ssize_t index = FindNodeIndex(parent, 0, name);
    if(index != -1)
    {
        Krb5Entry *child = GetChild(parent, index);
        return child;
    }
    return NULL;
}

static DWORD EnsureGroupNode(Krb5Entry *parent, const char *name, Krb5Entry **result)
{
    DWORD ceError = ERROR_SUCCESS;
    Krb5Entry *created = NULL;
    *result = GetFirstNode(parent, name);
    if(*result == NULL)
    {
        //Got to create it
        GCE(ceError = CreateGroupNode(GetRootNode(parent),
                    GetEntryDepth(parent) + 1,
                    name, &created));
        GCE(ceError = AddChildNode(parent, created));
        *result = created;
        created = NULL;
    }

cleanup:
    FreeKrb5Entry(&created);
    return ceError;
}

static DWORD EnsureStanzaNode(Krb5Entry *conf, const char *name, Krb5Entry **result)
{
    DWORD ceError = ERROR_SUCCESS;
    Krb5Entry *created = NULL;
    *result = GetFirstNode(conf, name);
    if(*result == NULL)
    {
        //Got to create it
        DJ_LOG_INFO("Creating krb5 stanza '%s'", name);
        GCE(ceError = CreateStanzaNode(conf,
                    name, &created));
        GCE(ceError = AddChildNode(conf, created));
        *result = created;
        created = NULL;
    }

cleanup:
    FreeKrb5Entry(&created);
    return ceError;
}

//Deletes all children that have the specified name
DWORD DeleteChildNode(Krb5Entry *parent, const char *name, size_t *removed)
{
    DWORD ceError = ERROR_SUCCESS;
    if(removed)
        *removed = 0;
    while(TRUE)
    {
        ssize_t index = FindNodeIndex(parent, 0, name);
        if(index == -1)
            break;

        GCE(ceError = CTArrayRemove(&parent->subelements, index,
                        sizeof(Krb5Entry *), 1));
        if(removed)
            (*removed)++;
    }

cleanup:
    return ceError;
}

static DWORD SetNodeValue(Krb5Entry *parent, const char *name, const char *value)
{
    DWORD ceError = ERROR_SUCCESS;
    Krb5Entry *created = NULL;
    ssize_t existingIndex = FindNodeIndex(parent, 0, name);
    DJ_LOG_VERBOSE("Setting krb5 name value '%s' to '%s' ", name, value);
    GCE(ceError = CreateValueNode(parent, GetEntryDepth(parent) + 1, name,
                value, &created));
    GCE(ceError = DeleteChildNode(parent, name, NULL));
    if(existingIndex != -1)
        GCE(ceError = InsertChildNode(parent, existingIndex, created));
    else
        GCE(ceError = AddChildNode(parent, created));
    created = NULL;
cleanup:
    FreeKrb5Entry(&created);
    return ceError;
}

const char *GetFirstNodeValue(Krb5Entry *parent, const char *name)
{
    Krb5Entry *entry = GetFirstNode(parent, name);
    if(entry == NULL)
        return NULL;
    return entry->value.value;
}

static DWORD SetChildNode(Krb5Entry *parent, Krb5Entry *child)
{
    DWORD ceError = ERROR_SUCCESS;
    ssize_t existingIndex = FindNodeIndex(parent, 0, child->name.value);
    if(existingIndex != -1)
    {
        GCE(ceError = DeleteChildNode(parent, child->name.value, NULL));
        GCE(ceError = InsertChildNode(parent, existingIndex, child));
    }
    else
        GCE(ceError = AddChildNode(parent, child));

cleanup:
    return ceError;
}

static DWORD
AddEncTypes(Krb5Entry *parent, const char *elementName, const char **add, size_t addCount)
{
    DWORD ceError = ERROR_SUCCESS;
    char *setEncTypes = NULL;
    const char *currentEncTypes = GetFirstNodeValue(parent, elementName);
    size_t i;

    if(currentEncTypes != NULL)
    {
        GCE(ceError = CTStrdup(currentEncTypes, &setEncTypes));
    }
    for(i = 0; i < addCount; i++)
    {
        if(setEncTypes == NULL)
        {
            GCE(ceError = CTStrdup(add[i], &setEncTypes));
        }
        else if(strstr(setEncTypes, add[i]) == NULL)
        {
            char *newString;
            GCE(ceError=CTAllocateStringPrintf(&newString, "%s %s", setEncTypes, add[i]));
            CT_SAFE_FREE_STRING(setEncTypes);
            setEncTypes = newString;
        }
    }
    GCE(ceError = SetNodeValue( parent, elementName, setEncTypes ));

cleanup:
    CT_SAFE_FREE_STRING(setEncTypes);
    return ceError;
}

typedef struct
{
    PSTR shortName;
    PSTR longName;
} DomainMapping;

static DWORD GetEscapedDomainName(const char *input, char **result)
{
    DWORD ceError = ERROR_SUCCESS;
    size_t i;
    DynamicArray array;
    *result = NULL;
    memset(&array, 0, sizeof(array));
    GCE(ceError = CTSetCapacity(&array, sizeof(char), strlen(input) + 3));
    for(i = 0; input[i]; i++)
    {
        if(input[i] == '.')
        {
            GCE(ceError = CTArrayAppend(&array, sizeof(char), "\\", 1));
        }
        GCE(ceError = CTArrayAppend(&array, sizeof(char), &input[i], 1));
    }
    GCE(ceError = CTArrayAppend(&array, sizeof(char), "\0", 1));
    *result = array.data;
    array.data = NULL;

cleanup:
    CTArrayFree(&array);
    return ceError;
}

static
DWORD
GetAuthToLocalRule(
    DomainMapping *mapping,
    PCSTR pAssumedPrefix,
    char **result
    )
{
    DWORD ceError = ERROR_SUCCESS;
    char *escapedDomain = NULL;
    char *shortUpper = NULL;
    *result = NULL;
    GCE(ceError = GetEscapedDomainName(mapping->longName, &escapedDomain));
    CTStrToUpper(escapedDomain);
    GCE(ceError = CTStrdup(mapping->shortName, &shortUpper));
    CTStrToUpper(shortUpper);

    if (pAssumedPrefix && !strcmp(shortUpper, pAssumedPrefix))
    {
        ceError = CTAllocateStringPrintf(result,
                        "RULE:[1:$0\\$1](^%s\\\\.*)s/^%s\\\\//",
                        escapedDomain, escapedDomain);
        GCE(ceError);
    }
    else
    {
        ceError = CTAllocateStringPrintf(result,
                        "RULE:[1:$0\\$1](^%s\\\\.*)s/^%s/%s/",
                        escapedDomain, escapedDomain, shortUpper);
        GCE(ceError);
    }

cleanup:
    CT_SAFE_FREE_STRING(escapedDomain);
    CT_SAFE_FREE_STRING(shortUpper);
    return ceError;
}

static DWORD GetMappingsValueString(DomainMapping *mapping, char **result)
{
    DWORD ceError = ERROR_SUCCESS;
    *result = NULL;
    GCE(ceError = CTAllocateStringPrintf(result,
                "%s\\\\(.*) $1@%s",
                mapping->shortName, mapping->longName));
    CTStrToUpper(*result);
cleanup:
    return ceError;
}

static DWORD GetReverseMappingsValueString(DomainMapping *mapping, char **result)
{
    DWORD ceError = ERROR_SUCCESS;
    char *escapedDomain = NULL;
    *result = NULL;
    GCE(ceError = GetEscapedDomainName(mapping->longName, &escapedDomain));
    GCE(ceError = CTAllocateStringPrintf(result,
                "(.*)@%s %s\\$1",
                escapedDomain, mapping->shortName));
    CTStrToUpper(*result);
cleanup:
    CT_SAFE_FREE_STRING(escapedDomain);
    return ceError;
}

static void FreeDomainMappings(DynamicArray *mappings)
{
    size_t i;
    for(i = 0; i < mappings->size; i++)
    {
        DomainMapping *current = ((DomainMapping *)mappings->data) + i;
        CT_SAFE_FREE_STRING(current->shortName);
        CT_SAFE_FREE_STRING(current->longName);
    }
    CTArrayFree(mappings);
}

static DWORD
GatherDomainMappings(
    DynamicArray *mappings,
    PCSTR pszShortDomainName,
    PCSTR pszDomainName)
{
    DWORD ceError = ERROR_SUCCESS;
    DomainMapping add;
    HANDLE hLsa = NULL;
    PLSASTATUS pStatus = NULL;
    size_t providerIndex = 0;
    size_t domainIndex = 0;

    memset(mappings, 0, sizeof(*mappings));
    memset(&add, 0, sizeof(add));

    ceError = LsaOpenServer(&hLsa);
    if (ceError == ERROR_FILE_NOT_FOUND || ceError == LW_ERROR_ERRNO_ECONNREFUSED)
    {
        DJ_LOG_INFO("Unable to get trust list because lsass is not running");
        ceError = 0;
    }
    else
    {
        GCE(ceError);

        GCE(ceError = LsaGetStatus(hLsa, &pStatus));

        for (providerIndex = 0; providerIndex < pStatus->dwCount; providerIndex++)
        {
            PLSA_AUTH_PROVIDER_STATUS provider =
                &pStatus->pAuthProviderStatusList[providerIndex];
            for (domainIndex = 0;
                    domainIndex < provider->dwNumTrustedDomains;
                    domainIndex++)
            {
                GCE(ceError = CTStrdup(provider->
                            pTrustedDomainInfoArray[domainIndex].pszDnsDomain,
                            &add.longName));
                GCE(ceError = CTStrdup(provider->
                                pTrustedDomainInfoArray[domainIndex].
                                pszNetbiosDomain,
                            &add.shortName));
                GCE(ceError = CTArrayAppend(mappings, sizeof(add), &add, 1));
                memset(&add, 0, sizeof(add));
            }
        }
    }
    
    if (mappings->size == 0)
    {
        // Put in the default entry
        GCE(ceError = CTStrdup(pszDomainName, &add.longName));
        GCE(ceError = CTStrdup(pszShortDomainName, &add.shortName));
        GCE(ceError = CTArrayAppend(mappings, sizeof(add), &add, 1));
        memset(&add, 0, sizeof(add));
    }

cleanup:
    if(ceError)
        FreeDomainMappings(mappings);
    CT_SAFE_FREE_STRING(add.shortName);
    CT_SAFE_FREE_STRING(add.longName);
    if (pStatus)
    {
        LsaFreeStatus(pStatus);
    }
    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    return ceError;
}

static DWORD
RestoreMacKeberosFile(
    void
    )
{
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN   bFileExists = FALSE;

    (void) CTRemoveFile("/Library/Preferences/edu.mit.Kerberos");

    GCE(ceError = CTCheckFileExists("/Library/Preferences/edu.mit.Kerberos.orig", &bFileExists));

    if (bFileExists)
    {
        DJ_LOG_VERBOSE("Resstoring original /Library/Preferences/edu.mit.Kerberos");

        GCE(ceError = CTCopyFileWithOriginalPerms("/Library/Preferences/edu.mit.Kerberos.orig",
                                                  "/Library/Preferences/edu.mit.Kerberos"));
    }

cleanup:
    return ceError;
}

static DWORD
CreateMacKeberosFile(
    PCSTR pszDomainName,
    PSTR pszRealm)
{
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN   bDirExists = FALSE;
    BOOLEAN   bFileExists = FALSE;
    FILE    * file = NULL;

    /* Test for typical Mac system directory where edu.mit.Kerberos lives */
    GCE(ceError = CTCheckDirectoryExists("/Library/Preferences", &bDirExists));

    if (bDirExists)
    {
        /* Backup existing file */
        GCE(ceError = CTCheckFileExists("/Library/Preferences/edu.mit.Kerberos", &bFileExists));

        if (bFileExists)
        {
            GCE(ceError = CTCheckFileExists("/Library/Preferences/edu.mit.Kerberos.orig", &bFileExists));

            if (bFileExists)
            {
                /* There is already a backup version, delete the edu.mit.Kerberos since we will create
                   a new one below */
                (void) CTRemoveFile("/Library/Preferences/edu.mit.Kerberos");
            }
            else
            {
                DJ_LOG_VERBOSE("Backing up original /Library/Preferences/edu.mit.Kerberos");

                GCE(ceError = CTMoveFile("/Library/Preferences/edu.mit.Kerberos",
                                         "/Library/Preferences/edu.mit.Kerberos.orig"));
            }
        }

        ceError = CTOpenFile("/Library/Preferences/edu.mit.Kerberos", "w", &file);
        if(ceError)
        {
            DJ_LOG_ERROR("Unable to open '%s' for writing", "/Library/Preferences/edu.mit.Kerberos");
            GCE(ceError);
        }

        GCE(ceError = CTFilePrintf(file, "# WARNING This file is created during Likewise domain join.\n"));
        GCE(ceError = CTFilePrintf(file, "# Any previous version of edu.mit.Kerberos is backed up to\n"));
        GCE(ceError = CTFilePrintf(file, "# /Likewise/Preferences/edu.mit.Kerberos.orig\n"));
        GCE(ceError = CTFilePrintf(file, "# Leaving the current domain will restore the file above.\n"));
        GCE(ceError = CTFilePrintf(file, "[libdefaults]\n"));
        GCE(ceError = CTFilePrintf(file, "\tdefault_realm = %s\n", pszRealm));
        GCE(ceError = CTFilePrintf(file, "\tdns_lookup_kdc = yes\n"));
        GCE(ceError = CTFilePrintf(file, "[domain_realm]\n"));
        GCE(ceError = CTFilePrintf(file, "\t.%s = %s\n", pszDomainName, pszRealm));
        CTCloseFile(file);
        file = NULL;

        GCE(ceError = CTChangeOwnerAndPermissions("/Library/Preferences/edu.mit.Kerberos",
                                                   0, 80, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));
    }

cleanup:

    if(file != NULL)
        CTCloseFile(file);

    return ceError;
}

static DWORD
Krb5LeaveDomain(Krb5Entry *conf)
{
    DWORD ceError = ERROR_SUCCESS;
    Krb5Entry *libdefaults;
    GCE(ceError = EnsureStanzaNode(conf, "libdefaults", &libdefaults));
    GCE(ceError = DeleteChildNode(libdefaults, "default_realm", NULL));

    /* Revert changes needed for SSO support for Mac platforms */
    GCE(ceError = RestoreMacKeberosFile());

cleanup:
    return ceError;
}

static
DWORD
Krb5JoinDomain(
    Krb5Entry *conf,
    PCSTR pszDomainName,
    PCSTR pszShortDomainName,
    PCSTR pDefaultPrefix)
{
    DWORD ceError = ERROR_SUCCESS;
    Krb5Entry *libdefaults;
    Krb5Entry *realms;
    Krb5Entry *domain_realm = NULL;
    Krb5Entry *appdefaults;
    Krb5Entry *pamGroup;
    Krb5Entry *httpdGroup;
    Krb5Entry *domainGroup = NULL;
    Krb5Entry *addNode = NULL;
    char *domainUpper = NULL;
    char *domainLower = NULL;
    DynamicArray trusts;
    char *mappingString = NULL;
    size_t i;
    const char *wantEncTypes[] = {
        "RC4-HMAC",
        "DES-CBC-MD5",
	"DES-CBC-CRC",
    };

    memset(&trusts, 0, sizeof(trusts));

    if(IsNullOrEmptyString(pszDomainName))
    {
        DJ_LOG_ERROR("Please specify the long domain name");
        GCE(ceError = ERROR_INVALID_PARAMETER);
    }

    GCE(ceError = GatherDomainMappings( &trusts,
                pszShortDomainName, pszDomainName));

    GCE(ceError = EnsureStanzaNode(conf, "libdefaults", &libdefaults));
    GCE(ceError = CTStrdup(pszDomainName, &mappingString));
    CTStrToUpper(mappingString);
    GCE(ceError = SetNodeValue( libdefaults, "default_realm",
                 mappingString ));
    GCE(ceError = AddEncTypes(libdefaults, "default_tgs_enctypes",
                wantEncTypes,
                sizeof(wantEncTypes)/sizeof(wantEncTypes[0])));
    GCE(ceError = AddEncTypes(libdefaults, "default_tkt_enctypes",
                wantEncTypes,
                sizeof(wantEncTypes)/sizeof(wantEncTypes[0])));
    GCE(ceError = SetNodeValue( libdefaults, "preferred_enctypes",
                "RC4-HMAC DES-CBC-MD5 DES-CBC-CRC" ));
    GCE(ceError = SetNodeValue( libdefaults, "dns_lookup_kdc", "true" ));

    GCE(ceError = SetNodeValue( libdefaults, "pkinit_kdc_hostname",
                "<DNS>" ));
    GCE(ceError = SetNodeValue( libdefaults, "pkinit_anchors",
                "DIR:/var/lib/likewise/trusted_certs" ));
    GCE(ceError = SetNodeValue( libdefaults, "pkinit_cert_match",
                "&&<EKU>msScLogin<PRINCIPAL>" ));
    GCE(ceError = SetNodeValue( libdefaults, "pkinit_eku_checking",
                "kpServerAuth" ));
    GCE(ceError = SetNodeValue( libdefaults, "pkinit_win2k_require_binding",
                "false" ));
    GCE(ceError = SetNodeValue( libdefaults, "pkinit_identities",
                "PKCS11:" LIBDIR "/libpkcs11.so" ));

    GCE(ceError = EnsureStanzaNode(conf, "domain_realm", &domain_realm));
    GCE(ceError = EnsureStanzaNode(conf, "realms", &realms));
    GCE(ceError = CTStrdup(pszDomainName, &domainUpper));
    CTStrToUpper(domainUpper);
    GCE(ceError = CreateGroupNode(conf, 2, domainUpper, &domainGroup));
    /* Enable SSO for Mac platforms, by creating a suitable /Library/Preferences/edu.mit.Kerberos file */
    GCE(ceError = CreateMacKeberosFile(pszDomainName, domainUpper));

    for(i = 0; i < trusts.size; i++)
    {
        DomainMapping *current = ((DomainMapping *)trusts.data) + i;
        CT_SAFE_FREE_STRING(mappingString);

        ceError = GetAuthToLocalRule(
                        current,
                        pDefaultPrefix,
                        &mappingString);
        GCE(ceError);

        GCE(ceError = CreateValueNode(conf, 3, "auth_to_local", mappingString, &addNode));
        GCE(ceError = AddChildNode(domainGroup, addNode));
        addNode = NULL;

        CT_SAFE_FREE_STRING(domainUpper);
        CT_SAFE_FREE_STRING(domainLower);
        GCE(ceError = CTStrdup(current->longName, &domainUpper));
        CTStrToUpper(domainUpper);
        GCE(ceError = CTAllocateStringPrintf(
                &domainLower, ".%s", current->longName));
        CTStrToLower(domainLower);
        GCE(ceError = SetNodeValue(domain_realm, domainLower, domainUpper));
    }
    GCE(ceError = CreateValueNode(conf, 3, "auth_to_local", "DEFAULT", &addNode));
    GCE(ceError = AddChildNode(domainGroup, addNode));
    addNode = NULL;

    //Replaces old auth to local rules
    GCE(ceError = SetChildNode(realms, domainGroup));
    domainGroup = NULL;

    GCE(ceError = EnsureStanzaNode(conf, "appdefaults", &appdefaults));
    GCE(ceError = EnsureGroupNode(appdefaults, "pam", &pamGroup));
    CT_SAFE_FREE_STRING(mappingString);
    //The first trust has the primary short domain name and long domain name
    GCE(ceError = GetMappingsValueString( (DomainMapping *)trusts.data, &mappingString ));
    GCE(ceError = SetNodeValue( pamGroup, "mappings", mappingString ));
    GCE(ceError = SetNodeValue( pamGroup, "forwardable", "true" ));
    GCE(ceError = SetNodeValue( pamGroup, "validate", "true" ));

    GCE(ceError = EnsureGroupNode(appdefaults, "httpd", &httpdGroup));
    CT_SAFE_FREE_STRING(mappingString);
    GCE(ceError = GetMappingsValueString( (DomainMapping *)trusts.data, &mappingString ));
    GCE(ceError = SetNodeValue( httpdGroup, "mappings", mappingString ));
    CT_SAFE_FREE_STRING(mappingString);
    //The first trust has the primary short domain name and long domain name
    GCE(ceError = GetReverseMappingsValueString( (DomainMapping *)trusts.data, &mappingString ));
    GCE(ceError = SetNodeValue( httpdGroup, "reverse_mappings", mappingString ));

cleanup:
    CT_SAFE_FREE_STRING(domainLower);
    CT_SAFE_FREE_STRING(mappingString);
    CT_SAFE_FREE_STRING(domainUpper);
    FreeDomainMappings(&trusts);
    FreeKrb5Entry(&domainGroup);
    FreeKrb5Entry(&addNode);
    return ceError;
}

static
DWORD
ReadKrb5Configuration(
    const char *rootPrefix,
    Krb5Entry *conf,
    BOOLEAN *modified
    )
{
    char *fullPath = NULL;
    char *altPath = NULL;
    char *altDir = NULL;
    FILE *file = NULL;
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN _modified = FALSE;
    BOOLEAN exists;
    BOOLEAN solarisTemplateFile;
    if(rootPrefix == NULL)
        rootPrefix = "";

    GCE(ceError = CTAllocateStringPrintf(
            &fullPath, "%s%s", rootPrefix, "/etc/krb5.conf"));
    GCE(ceError = CTCheckFileOrLinkExists(fullPath, &exists));
    if(!exists)
    {
        CT_SAFE_FREE_STRING(altPath);
        GCE(ceError = CTAllocateStringPrintf(
                &altPath, "%s%s", rootPrefix, "/etc/krb5/krb5.conf"));
        GCE(ceError = CTCheckFileOrLinkExists(altPath, &exists));
        if(exists)
        {
            DJ_LOG_INFO("Symlinking system /etc/krb5/krb5.conf to /etc/krb5.conf");
            GCE(ceError = CTCreateSymLink(altPath, fullPath));
            _modified = TRUE;
        }
        else
        {
            DJ_LOG_INFO("Creating blank krb5.conf");
            ceError = CTOpenFile(fullPath, "w", &file);
            if(ceError)
            {
                DJ_LOG_ERROR("Unable to open '%s' for writing", fullPath);
                GCE(ceError);
            }
            GCE(ceError = CTCloseFile(file));
            file = NULL;
            _modified = TRUE;

            GCE(ceError = CTAllocateStringPrintf(
                    &altDir, "%s%s", rootPrefix, "/etc/krb5"));
            GCE(ceError = CTCheckDirectoryExists(altDir, &exists));
            if(exists)
            {
                DJ_LOG_INFO("Symlinking /etc/krb5.conf to /etc/krb5/krb5.conf");
                GCE(ceError = CTCreateSymLink(fullPath, altPath));
                _modified = TRUE;
            }
        }
    }

    GCE(ceError = CTCheckFileHoldsPattern(fullPath, "^[[:space:]]*___slave_kdcs___[[:space:]]*$", &solarisTemplateFile));

    if(solarisTemplateFile)
    {
        DJ_LOG_WARNING("The system krb5.conf is the default template file (which is syntactically incorrect). It will be replaced.");
    }
    else
    {
        GCE(ceError = ReadKrb5File(rootPrefix, "/etc/krb5.conf", conf));
    }

    if(modified)
        *modified = _modified;

cleanup:
    if(file != NULL)
        CTCloseFile(file);
    CT_SAFE_FREE_STRING(fullPath);
    CT_SAFE_FREE_STRING(altDir);
    CT_SAFE_FREE_STRING(altPath);
    return ceError;
}

DWORD
DJModifyKrb5Conf(
    const char *testPrefix,
    BOOLEAN enable,
    PCSTR pszDomainName,
    PCSTR pszShortDomainName,
    const JoinProcessOptions *options,
    BOOLEAN *modified
    )
{
    DWORD ceError = ERROR_SUCCESS;
    Krb5Entry conf;
    BOOLEAN readModified = FALSE;
    memset(&conf, 0, sizeof(conf));
    REG_DATA_TYPE readType = 0;
    PVOID pReadData = NULL;
    DWORD readLen = 0;
    BOOLEAN bAssumeDefaultDomain = FALSE;
    PCSTR pDefaultPrefix = NULL;
    PSTR pAutoShortDomain = NULL;

    DJ_LOG_INFO("Starting krb5.conf configuration (%s)", enable? "enabling" : "disabling");

    if(testPrefix == NULL)
        testPrefix = "";

    if (IsNullOrEmptyString(pszShortDomainName) && pszDomainName)
    {
        char *dotPosition = NULL;
        CTStrdup(pszDomainName, &pAutoShortDomain);
        dotPosition = strchr(pAutoShortDomain, '.');
        if(dotPosition)
            *dotPosition = 0;
        pszShortDomainName = pAutoShortDomain;

        DJ_LOG_WARNING("Short domain name not specified. Defaulting to '%s'", pszShortDomainName);
    }

    ceError = RegUtilGetValue(
                    NULL,
                    HKEY_THIS_MACHINE,
                    NULL,
                    "Services\\lsass\\Parameters\\Providers\\ActiveDirectory",
                    "AssumeDefaultDomain",
                    &readType,
                    &pReadData,
                    &readLen);
    if (ceError != LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        GCE(ceError);

        if (readType != REG_DWORD || readLen != sizeof(DWORD))
        {
            GCE(ceError = ERROR_INVALID_PARAMETER);
        }
        bAssumeDefaultDomain = (DWORD)(size_t)pReadData;
    }
    else
    {
        ceError = 0;
    }

    if (options && options->setAssumeDefaultDomain)
    {
        bAssumeDefaultDomain = options->assumeDefaultDomain;
    }

    if (bAssumeDefaultDomain)
    {
        pDefaultPrefix = pszShortDomainName;

        if (pReadData && readType != REG_DWORD)
        {
            RegFreeMemory(pReadData);
            pReadData = NULL;
        }

        ceError = RegUtilGetValue(
            NULL,
            HKEY_THIS_MACHINE,
            NULL,
            "Services\\lsass\\Parameters\\Providers\\ActiveDirectory",
            "UserDomainPrefix",
            &readType,
            &pReadData,
            &readLen);
        if (ceError != LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
        {
            GCE(ceError);

            if (readType != REG_SZ || readLen < 1 ||
                    ((PSTR)pReadData)[readLen - 1] != 0)
            {
                GCE(ceError = ERROR_INVALID_PARAMETER);
            }
            if (readLen > 1)
            {
                pDefaultPrefix = pReadData;
            }
        }
        else
        {
            ceError = 0;
        }

        if (options && options->userDomainPrefix &&
                options->userDomainPrefix[0])
        {
            pDefaultPrefix = options->userDomainPrefix;
        }
    }

    GCE(ceError = ReadKrb5Configuration(testPrefix, &conf, &readModified));
    if(enable)
    {
        ceError = Krb5JoinDomain(
                    &conf,
                    pszDomainName,
                    pszShortDomainName,
                    pDefaultPrefix);
        GCE(ceError);
    }
    else
    {
        GCE(ceError = Krb5LeaveDomain(&conf));
    }
    GCE(ceError = WriteKrb5Configuration(testPrefix, "/etc/krb5.conf", &conf, modified));
    if(readModified && (modified != NULL))
        *modified = TRUE;
    DJ_LOG_INFO("Finishing krb5.conf configuration");

cleanup:
    if (pReadData && readType != REG_DWORD)
    {
        RegFreeMemory(pReadData);
    }
    CT_SAFE_FREE_STRING(pAutoShortDomain);
    FreeKrb5EntryContents(&conf);
    return ceError;
}

static DWORD WriteEntry(FILE *file, Krb5Entry *lineObj)
{
    DWORD ceError = ERROR_SUCCESS;
    size_t i;
    if(lineObj->leadingWhiteSpace != NULL)
    {
        GCE(ceError = CTFilePrintf(file, "%s", lineObj->leadingWhiteSpace));
    }
    if(IsStanzaEntry(lineObj))
    {
        GCE(ceError = CTFilePrintf(file, "[%s]%s",
                lineObj->name.value,
                lineObj->name.trailingSeparator == NULL ?
                "" : lineObj->name.trailingSeparator));
    }
    else
    {
        GCE(ceError = CTWriteToken(file, &lineObj->name));
    }
    GCE(ceError = CTWriteToken(file, &lineObj->beginSeparator));
    if(IsStanzaEntry(lineObj) || IsGroupEntry(lineObj))
    {
        GCE(ceError = CTFilePrintf(file, "\n"));
    }
    for(i = 0; i < lineObj->subelements.size; i++)
    {
        GCE(ceError = WriteEntry(file, GetChild(lineObj, i)));
    }
    if(lineObj->value.value != NULL && lineObj->value.value[0] == '}')
    {
        //The leading whitespace for the ending brace isn't stored. We'll
        //assume it's the same as the line's leading white space. Even if it's
        //different we'll just end up fixing the tabbing for the user.
        GCE(ceError = CTFilePrintf(file, "%s", lineObj->leadingWhiteSpace));
    }
    GCE(ceError = CTWriteToken(file, &lineObj->value));
    if(!IsRootEntry(lineObj) && !IsStanzaEntry(lineObj))
        GCE(ceError = CTFilePrintf(file, "\n"));

cleanup:
    return ceError;
}

static DWORD WriteKrb5Configuration(const char *rootPrefix, const char *filename, Krb5Entry *conf, BOOLEAN *modified)
{
    DWORD ceError = ERROR_SUCCESS;
    FILE *file = NULL;
    char *prefixedPath = NULL;
    char *tempName = NULL;
    char *finalName = NULL;
    char *symtarget = NULL;
    BOOLEAN same;
    BOOLEAN islink;

    if(rootPrefix == NULL)
        rootPrefix = "";

    GCE(ceError = CTAllocateStringPrintf(&prefixedPath, "%s%s", rootPrefix, filename));

    ceError = CTGetFileTempPath(
                        prefixedPath,
                        &finalName,
                        &tempName);
    GCE(ceError);

    DJ_LOG_INFO("Writing krb5 file %s", finalName);

    ceError = CTOpenFile(tempName, "w", &file);
    if(ceError)
    {
        DJ_LOG_ERROR("Unable to open '%s' for writing", tempName);
        GCE(ceError);
    }

    GCE(ceError = WriteEntry(file, conf));

    GCE(ceError = CTCloseFile(file));
    file = NULL;

    GCE(ceError = CTFileContentsSame(tempName, finalName, &same));
    if(modified != NULL)
        *modified = !same;
    if(same)
    {
        DJ_LOG_INFO("File %s unmodified", finalName);
        GCE(ceError = CTRemoveFile(tempName));
    }
    else
    {
        DJ_LOG_INFO("File %s modified", finalName);
        GCE(ceError = CTCheckLinkExists(finalName, &islink));
        if(islink)
        {
            GCE(ceError = CTGetSymLinkTarget(finalName, &symtarget));
            DJ_LOG_INFO("Overwriting symlink target '%s' instead of link name '%s'", symtarget, finalName);
            CT_SAFE_FREE_STRING(finalName);
            finalName = symtarget;
            symtarget = NULL;
        }
#if defined(MINIMAL_JOIN)
        GCE(ceError = CTCloneFilePerms(finalName, tempName));
        GCE(ceError = CTMoveFile(tempName, finalName));
#else
        GCE(ceError = CTSafeReplaceFile(finalName, tempName));
#endif
    }

cleanup:
    if(file != NULL)
        CTCloseFile(file);
    CT_SAFE_FREE_STRING(prefixedPath);
    CT_SAFE_FREE_STRING(tempName);
    CT_SAFE_FREE_STRING(finalName);
    CT_SAFE_FREE_STRING(symtarget);
    return ceError;
}

void
DJCopyKrb5ToRootDir(
        const char *srcPrefix,
        const char *destPrefix,
        LWException **exc
        )
{
    PSTR srcPath = NULL;
    PSTR destPath = NULL;
    BOOLEAN exists;

    if(srcPrefix == NULL)
        srcPrefix = "";
    if(destPrefix == NULL)
        destPrefix = "";

    CT_SAFE_FREE_STRING(srcPath);
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&srcPath, "%s/etc", srcPrefix));
    LW_CLEANUP_CTERR(exc, CTCheckDirectoryExists(srcPath, &exists));
    if(exists)
    {
        CT_SAFE_FREE_STRING(destPath);
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&destPath, "%s/etc", destPrefix));
        LW_CLEANUP_CTERR(exc, CTCreateDirectory(destPath, 0700));
    }

    CT_SAFE_FREE_STRING(srcPath);
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&srcPath, "%s/etc/krb5", srcPrefix));
    LW_CLEANUP_CTERR(exc, CTCheckDirectoryExists(srcPath, &exists));
    if(exists)
    {
        CT_SAFE_FREE_STRING(destPath);
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&destPath, "%s/etc/krb5", destPrefix));
        LW_CLEANUP_CTERR(exc, CTCreateDirectory(destPath, 0700));
    }

    CT_SAFE_FREE_STRING(srcPath);
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&srcPath, "%s/etc/krb5/krb5.conf", srcPrefix));
    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(srcPath, &exists));
    if(exists)
    {
        CT_SAFE_FREE_STRING(destPath);
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&destPath, "%s/etc/krb5/krb5.conf", destPrefix));
        LW_CLEANUP_CTERR(exc, CTCopyFileWithOriginalPerms(srcPath, destPath));
    }

    CT_SAFE_FREE_STRING(srcPath);
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&srcPath, "%s/etc/krb5.conf", srcPrefix));
    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(srcPath, &exists));
    if(exists)
    {
        CT_SAFE_FREE_STRING(destPath);
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&destPath, "%s/etc", destPrefix));
        LW_CLEANUP_CTERR(exc, CTCreateDirectory(destPath, 0700));
        CT_SAFE_FREE_STRING(destPath);
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&destPath, "%s/etc/krb5.conf", destPrefix));
        LW_CLEANUP_CTERR(exc, CTCopyFileWithOriginalPerms(srcPath, destPath));
    }

cleanup:
    CT_SAFE_FREE_STRING(srcPath);
    CT_SAFE_FREE_STRING(destPath);
}

static QueryResult QueryKrb5(const JoinProcessOptions *options, LWException **exc)
{
    QueryResult result = FullyConfigured;
    BOOLEAN modified;
    PSTR tempDir = NULL;
    PSTR mappingString = NULL;
    PSTR shortName = NULL;
    Krb5Entry conf;
    Krb5Entry *libdefaults;
    Krb5Entry *default_realm;
    DWORD ceError;

    memset(&conf, 0, sizeof(conf));

    if (options->enableMultipleJoins && !options->joiningDomain)
    {
        result = NotApplicable;
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, CTCreateTempDirectory(&tempDir));
    LW_TRY(exc, DJCopyKrb5ToRootDir(NULL, tempDir, &LW_EXC));
    ceError = ReadKrb5Configuration(tempDir, &conf, &modified);
    if(ceError == ERROR_BAD_FORMAT)
    {
        LW_RAISE_EX(exc, ceError, "Unable to parse krb5.conf", "The krb5.conf file on your system (located in either /etc/krb5.conf or /etc/krb5/krb5.conf) could not be parsed. Please send the file to Likewise technical support.");
        goto cleanup;
    }
    else
        LW_CLEANUP_CTERR(exc, ceError);
    if(modified)
    {
        if(options->joiningDomain)
        {
            result = NotConfigured;
        }
        goto cleanup;
    }

    libdefaults = GetFirstNode(&conf, "libdefaults");
    if(libdefaults == NULL)
    {
        if(options->joiningDomain)
            result = NotConfigured;
        goto cleanup;
    }
    default_realm = GetFirstNode(libdefaults, "default_realm");
    if(options->joiningDomain)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup(options->domainName, &mappingString));
        CTStrToUpper(mappingString);
        if(default_realm == NULL ||
            default_realm->value.value == NULL ||
            strcmp(default_realm->value.value, mappingString))
        {
            result = NotConfigured;
            goto cleanup;
        }
    }
    else
    {
        if(default_realm != NULL)
        {
            result = NotConfigured;
            goto cleanup;
        }
    }

    if(!options->joiningDomain)
        shortName = NULL;
    else if(options->shortDomainName != NULL)
        LW_CLEANUP_CTERR(exc, CTStrdup(options->shortDomainName, &shortName));
    else
    {
        //Ignore failures from this command
        DJGuessShortDomainName(options->domainName, &shortName, NULL);
    }
    LW_CLEANUP_CTERR(exc, DJModifyKrb5Conf(
                            tempDir,
                            options->joiningDomain,
                            options->domainName,
                            shortName,
                            options,
                            &modified));
    if(modified)
    {
        result = SufficientlyConfigured;
        goto cleanup;
    }

cleanup:
    if(tempDir != NULL)
    {
        CTRemoveDirectory(tempDir);
        CT_SAFE_FREE_STRING(tempDir);
    }
    CT_SAFE_FREE_STRING(mappingString);
    CT_SAFE_FREE_STRING(shortName);
    FreeKrb5EntryContents(&conf);
    return result;
}

static void DoKrb5(JoinProcessOptions *options, LWException **exc)
{
    LW_CLEANUP_CTERR(exc, DJModifyKrb5Conf(
                            NULL,
                            options->joiningDomain,
                            options->domainName,
                            options->shortDomainName,
                            options,
                            NULL));
cleanup:
    ;
}

static PSTR GetKrb5Description(const JoinProcessOptions *options, LWException **exc)
{
    PSTR tempDir = NULL;
    PSTR origPath = NULL;
    PSTR finalPath = NULL;
    PSTR ret = NULL;
    PSTR diff = NULL;
    PSTR shortName = NULL;
    BOOLEAN exists;
    BOOLEAN modified;

    LW_CLEANUP_CTERR(exc, CTCreateTempDirectory(&tempDir));
    LW_TRY(exc, DJCopyKrb5ToRootDir(NULL, tempDir, &LW_EXC));

    if(options->shortDomainName != NULL)
        LW_CLEANUP_CTERR(exc, CTStrdup(options->shortDomainName, &shortName));
    else
    {
        //Ignore failures from this command
        DJGuessShortDomainName(options->domainName, &shortName, NULL);
    }
    LW_CLEANUP_CTERR(exc, DJModifyKrb5Conf(
                            tempDir,
                            options->joiningDomain,
                            options->domainName,
                            shortName,
                            options,
                            &modified));

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
            &finalPath, "%s%s", tempDir, "/etc/krb5.conf"));

    if(modified)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
                &origPath, "%s%s", tempDir, "/etc/krb5.conf.lwidentity.orig"));
        LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(origPath, &exists));
        if(!exists)
        {
            LW_CLEANUP_CTERR(exc, CTStrdup("/dev/null", &origPath));
        }
    }
    else
    {
        LW_CLEANUP_CTERR(exc, CTStrdup(finalPath, &origPath));
    }

    LW_CLEANUP_CTERR(exc, CTGetFileDiff(origPath, finalPath, &diff, FALSE));
    CTStripTrailingWhitespace(diff);

    if(strlen(diff) < 1)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
            &ret, "Fully configured"));
    }
    else
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf( &ret,
"The krb5.conf must have the default realm set when joined to a domain. Other optional settings may be set if krb5.conf is configured automatically. Here is a list of the changes that would be performed automatically:\n%s", diff));
    }

cleanup:
    if(tempDir != NULL)
    {
        CTRemoveDirectory(tempDir);
        CT_SAFE_FREE_STRING(tempDir);
    }
    CT_SAFE_FREE_STRING(origPath);
    CT_SAFE_FREE_STRING(finalPath);
    CT_SAFE_FREE_STRING(diff);
    CT_SAFE_FREE_STRING(shortName);
    return ret;
}

const JoinModule DJKrb5Module = { TRUE, "krb5", "configure krb5.conf", QueryKrb5, DoKrb5, GetKrb5Description };

static QueryResult QueryOrDoKeytab(const JoinProcessOptions *options, PSTR *description, BOOLEAN makeChanges, LWException **exc)
{
    QueryResult result = FullyConfigured;
    Krb5Entry *libdefaults;
    Krb5Entry *default_keytab_name;
    BOOLEAN exists;
    DWORD ceError;
    PSTR tempDir = NULL;
    PSTR currentTarget = NULL;
    Krb5Entry conf;
    PSTR trueLocation;
    PSTR trueLocationPath = NULL;
    PSTR pszPtr = NULL;

    if(description)
        *description = NULL;

    memset(&conf, 0, sizeof(conf));

    if(!options->joiningDomain)
        goto nochanges;

    if(!makeChanges)
    {
        LW_CLEANUP_CTERR(exc, CTCreateTempDirectory(&tempDir));
        LW_TRY(exc, DJCopyKrb5ToRootDir(NULL, tempDir, &LW_EXC));
    }

    ceError = ReadKrb5Configuration(tempDir, &conf, NULL);
    if(ceError == ERROR_BAD_FORMAT)
    {
        LW_RAISE_EX(exc, ceError, "Unable to parse krb5.conf", "The krb5.conf file on your system (located in either /etc/krb5.conf or /etc/krb5/krb5.conf) could not be parsed. Please send the file to Likewise technical support.");
        goto cleanup;
    }
    else
        LW_CLEANUP_CTERR(exc, ceError);

    libdefaults = GetFirstNode(&conf, "libdefaults");
    if(libdefaults == NULL)
    {
        goto nochanges;
    }

    default_keytab_name = GetFirstNode(libdefaults, "default_keytab_name");
    if (default_keytab_name == NULL)
    {
        if (description)
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
                    description,
                    "Add the default_keytab_name setting in krb5.conf and set it to /etc/krb5.keytab"));
        }
        if (makeChanges)
        {
            LW_CLEANUP_CTERR(exc, SetNodeValue(libdefaults,
                        "default_keytab_name", "/etc/krb5.keytab"));
            LW_CLEANUP_CTERR(exc, WriteKrb5Configuration(tempDir,
                        "/etc/krb5.conf", &conf, NULL));
        }
        else
        {
            result = NotConfigured;
        }
        goto cleanup;
    }

    if(default_keytab_name->value.value == NULL)
    {
        LW_CLEANUP_CTERR(exc, ERROR_BAD_FORMAT);
    }

    trueLocation = default_keytab_name->value.value;
    if(CTStrStartsWith(trueLocation, "FILE:"))
        trueLocation += strlen("FILE:");
    else if(CTStrStartsWith(trueLocation, "WRFILE:"))
        trueLocation += strlen("WRFILE:");

    if(!strcmp(trueLocation, "/etc/krb5.keytab"))
    {
        //It's already pointing where we want
        goto nochanges;
    }

    LW_CLEANUP_CTERR(exc, CTDupOrNullStr(trueLocation, &trueLocationPath));
    pszPtr = strrchr(trueLocationPath, '/');
    if(pszPtr)
    {
        *pszPtr = '\0';
    }
    LW_CLEANUP_CTERR(exc, CTCheckDirectoryExists(trueLocationPath, &exists));
    if(!exists)
    {
        ceError = ERROR_BAD_FORMAT;
        LW_RAISE_EX(exc, ceError, "Value for default_keytab_name in /etc/krb5.conf invalid", "The directory specified by default_keytab_name in /etc/krb5.conf does not exist. Correct this error and re-run domainjoin-cli.");
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(trueLocation, &exists));
    if(!exists)
    {
        if(description)
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
                    description,
                    "Change the default_keytab_name setting in krb5.conf from '%s' to '%s' because the file '%s' does not exist.",
                    default_keytab_name->value.value, "/etc/krb5.keytab",
                    trueLocation));
        }
    }

    ceError = CTGetSymLinkTarget("/etc/krb5.keytab", &currentTarget);
    if(!ceError && !strcmp(currentTarget, trueLocation))
    {
        //Already points to the right place
        goto nochanges;
    }
    else if(ceError == LwMapErrnoToLwError(EINVAL) || ceError == ERROR_SUCCESS)
    {
        // The file already exists and isn't a symlink (EINVAL) or it exists
        // and is a symlink (0).
        if(description)
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
                    description,
                    "Delete either %s or %s.\n"
                    "\n"
                    "Both of these are locations for kerberos keytabs. Your krb5.conf file points to %s, but a legacy application is most likely using %s. After one of the files is deleted and this program is re-run, a symlink will be created from %s to %s so that the keytabs stay synchronized.\n",
                    "/etc/krb5.keytab", trueLocation,
                    trueLocation,
                    "/etc/krb5.keytab",
                    "/etc/krb5.keytab", trueLocation));
        }
        if(makeChanges)
        {
            LW_CLEANUP_CTERR(exc, ERROR_INVALID_OPERATION);
        }
        else
        {
            result = CannotConfigure;
        }
        ceError = ERROR_SUCCESS;
    }
    else if(ceError == LwMapErrnoToLwError(ENOENT))
    {
        //We can make the symlink
        if(description)
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
                    description,
                    "Create a symlink at %s that points to %s\n",
                    "/etc/krb5.keytab", trueLocation));
        }
        if(makeChanges)
        {
            LW_CLEANUP_CTERR(exc, CTCreateSymLink(trueLocation,
                        "/etc/krb5.keytab"));
        }
        else
        {
            result = NotConfigured;
        }
        ceError = ERROR_SUCCESS;
    }
    LW_CLEANUP_CTERR(exc, ceError);

cleanup:
    if(tempDir != NULL)
    {
        CTRemoveDirectory(tempDir);
        CT_SAFE_FREE_STRING(tempDir);
    }
    FreeKrb5EntryContents(&conf);
    CT_SAFE_FREE_STRING(trueLocationPath);
    return result;

nochanges:
    if(description)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup("Fully configured", description));
    }
    goto cleanup;
}

static QueryResult QueryKeytab(const JoinProcessOptions *options, LWException **exc)
{
    return QueryOrDoKeytab(options, NULL, FALSE, exc);
}

static void DoKeytab(JoinProcessOptions *options, LWException **exc)
{
    QueryOrDoKeytab(options, NULL, TRUE, exc);
}

static PSTR GetKeytabDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR description = NULL;
    QueryOrDoKeytab(options, &description, FALSE, exc);
    return description;
}

const JoinModule DJKeytabModule = { TRUE, "keytab", "initialize kerberos keytab", QueryKeytab, DoKeytab, GetKeytabDescription };
