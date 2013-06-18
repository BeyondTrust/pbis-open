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
#include "ctfileutils.h"
#include "djaixparser.h"

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

ssize_t
DJFindStanza(const DynamicArray *lines, PCSTR name)
{
    ssize_t i;
    size_t namelen = strlen(name);

    for(i = 0; i < lines->size; i++)
    {
        PCSTR line = *(PCSTR *)CTArrayGetItem((DynamicArray*)lines, i,
                sizeof(PCSTR));
        while (*line != '\0' && isspace((int)*line))
            line++;

        if(!strncmp(line, name, namelen) && line[namelen] == ':')
            return i;
    }
    return -1;
}

ssize_t DJFindLine(const DynamicArray *lines, const char *stanza, const char *name)
{
    ssize_t i = DJFindStanza(lines, stanza);
    
    if(i == -1)
        return -1;

    for(; i < lines->size; i++)
    {
        PCSTR line = *(PCSTR *)CTArrayGetItem((DynamicArray*)lines, i,
                sizeof(PCSTR));

        while (*line != '\0' && isspace((int)*line))
            line++;

        if(strncmp(line, name, strlen(name)))
            continue;
        line += strlen(name);
       
        while (*line != '\0' && isspace((int)*line))
            line++;

        if(*line != '=')
            continue;

        return i;
    }

    return -1;
}

DWORD DJGetOptionValue(const DynamicArray *lines, PCSTR stanza, PCSTR name, PSTR *value)
{
    DWORD ceError = ERROR_SUCCESS;
    ssize_t i = DJFindLine(lines, stanza, name);
    PCSTR line;
    PSTR _value = NULL;

    *value = NULL;
    
    if(i == -1)
        GCE(ceError = ERROR_NOT_FOUND);

    line = *(PCSTR *)CTArrayGetItem((DynamicArray*)lines, i,
            sizeof(PCSTR));

    while (*line != '\0' && isspace((int)*line))
        line++;

    line += strlen(name);
   
    while (*line != '\0' && isspace((int)*line))
        line++;

    if(*line != '=')
    {
        GCE(ceError = ERROR_BAD_FORMAT);
    }
    line++;

    GCE(ceError = CTStrdup(line, &_value));
    CTStripWhitespace(_value);
    /* Remove the quotes around the value, if they exist */
    if(CTStrStartsWith(_value, "\"") && CTStrEndsWith(_value, "\""))
    {
        size_t len = strlen(_value);
        memmove(_value, _value + 1, len - 2 );
        _value[len - 2 ] = '\0';
    }

    *value = _value;
    _value = NULL;

cleanup:
    CT_SAFE_FREE_STRING(_value);
    return ceError;
}

DWORD
DJSetOptionValue(DynamicArray *lines, PCSTR stanza, PCSTR name, PCSTR value)
{
    ssize_t index;
    PSTR newLine = NULL;
    DWORD ceError = ERROR_SUCCESS;

    if(strchr(value, ' '))
    {
        /* Fixes bug 5564 */
        GCE(ceError = CTAllocateStringPrintf(&newLine, "\t%s = \"%s\"\n", name, value));
    }
    else
        GCE(ceError = CTAllocateStringPrintf(&newLine, "\t%s = %s\n", name, value));

    index = DJFindLine(lines, stanza, name);

    if(index == -1)
    {
        index = DJFindStanza(lines, stanza);
        if(index == -1)
            GCE(ceError = ERROR_INVALID_OPERATION);
        index++;
    }
    else
    {
        CT_SAFE_FREE_STRING(*(PSTR *)CTArrayGetItem((DynamicArray*)lines,
                    index, sizeof(PSTR)));
        GCE(ceError = CTArrayRemove((DynamicArray *)lines, index,
                    sizeof(PSTR), 1));
    }

    GCE(ceError = CTArrayInsert((DynamicArray *)lines, index,
                sizeof(PSTR), &newLine, 1));

cleanup:
    return ceError;
}
