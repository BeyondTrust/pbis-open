/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
