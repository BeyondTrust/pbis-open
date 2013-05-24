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
#include "ctfileutils.h"
#include "ctshell.h"
#include "djstr.h"
#include "djdistroinfo.h"
#include "djpamconf.h"
#include <libgen.h>
#include <lwstr.h>

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

struct PamLine
{
    char *fromFile;
    char *leadingWhiteSpace;
    CTParseToken *service;
    CTParseToken *phase; /*auth, account, etc*/
    CTParseToken *control; /*required, sufficient, [success=1 default=die] etc*/
    CTParseToken *module; /*pam_unix2.so, /usr/lib/security/pam_aix, etc.*/
    int optionCount;
    CTParseToken *options;
    char *comment;

    // Filled in only for pam_per_user.so (read only)
    char *defaultInclude;
};

struct PamServiceAlias
{
    char *pFromService;
    char *pToService;
};

struct PamConf
{
    struct PamLine *lines;
    int lineCount;
    DynamicArray private_lines;
    DynamicArray private_aliases;
    int modified;
};

/* return the next line in the pam configuration, or -1 if there are no more lines. */
static int NextLine(const struct PamConf *conf, int line);

/* return the previous line in the pam configuration, or -1 if there are no more lines. */
static int PrevLine(const struct PamConf *conf, int line);

/* return the next line in the pam configuration that has the given service
 * name and phase name. So you could search for the next su auth line. NULL
 * may be passed for service or phase to match any value. If there are no more
 * matching lines, -1 is returned.
 *
 * -1 may be passed for the line to indicate that the first line with that
 * service should be found */
static int NextLineForService(const struct PamConf *conf, int line, const char *service, const char *phase);

/* Search backwards in the pam configuration for the previous line with given
 * service name and phase name. So you could search for the previous su auth
 * line.
 * If there are no more matching lines, -1 is returned.
 * NULL may be passed for service or phase to match any value.
 *
 * -1 may be passed for the line to indicate that the last line with that
 * service should be found */
static int PrevLineForService(const struct PamConf *conf, int line, const char *service, const char *phase);

/* Find a line with a different service. This can be used to iterate through
 * all services in the file. Doing this may repeat service names though.
 */
static int NextService(struct PamConf *conf, int line);

/* Returns an array of all the service names in the file. The array is NULL
 * terminated, and the size can optionally be returned through serviceCount.
 */
static DWORD ListServices(struct PamConf *conf, char ***result, int *serviceCount);

static void FreeServicesList(char **list);

/* Copy all lines that contain a given service name. */
static DWORD CopyService(struct PamConf *conf, const char *oldName, const char *newName);

#if 0
/* Parse and add a service to the list. */
static DWORD AddFormattedService(struct PamConf *conf, const char *filename, const char *contents);
#endif

/* Copy a pam configuration line and add it below the old line. */
static DWORD CopyLine(struct PamConf *conf, int oldLine, int *newLine);

static DWORD CopyLineAndUpdateSkips(struct PamConf *conf, int oldLine, int *newLine);

static DWORD RemoveLine(struct PamConf *conf, int *line);

static DWORD AddOption(struct PamConf *conf, int line, const char *option);

static int ContainsOption(struct PamConf *conf, int line, const char *option);

static DWORD RemoveOption(struct PamConf *conf, int line, const char *option, int *pFound);

static
const char *
GetUnaliasedServiceName(
    struct PamConf *conf,
    const char *name
    );

static
DWORD
ReadPamDirFile(
    IN OUT struct PamConf *conf,
    IN PCSTR pRootPrefix,
    IN PSTR pDirPath,
    IN PSTR pFileEntry
    );

/* On a real system, set the rootPrefix to "". When testing, set it to the
 * test directory that mirrors the target file system.
 */
static DWORD ReadPamConfiguration(const char *rootPrefix, struct PamConf *conf);

static DWORD WritePamConfiguration(const char *rootPrefix, struct PamConf *conf, PSTR *diff);

static DWORD FindModulePath(const char *testPrefix, const char *basename, char **destName, DWORD *moduleFlags);

static BOOLEAN IsRequiredService(PCSTR service, const struct PamConf *conf);

static BOOLEAN NormalizeModuleName( char *destName, const char *srcName, size_t bufferSize);

static int NextLine(const struct PamConf *conf, int line)
{
    line++;
    if(line >= conf->lineCount)
        line = -1;
    return line;
}

static int PrevLine(const struct PamConf *conf, int line)
{
    return line - 1;
}

static int NextLineForService(const struct PamConf *conf, int line, const char *service, const char *phase)
{
    /* Start the search on the next line */
    line++;
    if(line >= conf->lineCount)
        return -1;

    for(; line != -1; line = NextLine(conf, line))
    {
        struct PamLine *lineObj = &conf->lines[line];
        if(service != NULL && (lineObj->service == NULL ||
                    strcmp(lineObj->service->value, service)))
            continue;
        if(lineObj->phase != NULL &&
                    !strcmp(lineObj->phase->value, "@include"))
            return line;
        if(phase != NULL && (lineObj->phase == NULL ||
                    strcmp(lineObj->phase->value, phase)))
            continue;
        return line;
    }
    return -1;
}

static int PrevLineForService(const struct PamConf *conf, int line, const char *service, const char *phase)
{
    /* Start the search on the prev line */
    if (line == -1)
    {
        line = conf->lineCount - 1;
    }
    else
    {
        line--;
    }
    if(line < 0)
        return -1;

    for(; line != -1; line = PrevLine(conf, line))
    {
        struct PamLine *lineObj = &conf->lines[line];
        if(service != NULL && (lineObj->service == NULL ||
                    strcmp(lineObj->service->value, service)))
            continue;
        if(lineObj->phase != NULL &&
                    !strcmp(lineObj->phase->value, "@include"))
            return line;
        if(phase != NULL && (lineObj->phase == NULL ||
                    strcmp(lineObj->phase->value, phase)))
            continue;
        return line;
    }
    return -1;
}

static int NextService(struct PamConf *conf, int line)
{
    const char *currentService;
    if(line == -1)
       currentService = NULL;
    else
       currentService = conf->lines[line].service->value;
    /* Start the search on the next line */
    for(line = NextLine(conf, line); line != -1; line = NextLine(conf, line))
    {
        if(conf->lines[line].service == NULL)
            continue;
        if(currentService == NULL || strcmp(conf->lines[line].service->value, currentService))
            return line;
    }
    return -1;
}

static DWORD ListServices(struct PamConf *conf, char ***result, int *serviceCount)
{
    DWORD ceError = ERROR_SUCCESS;
    int line;
    DynamicArray services;
    char *service;
    int i;
    memset(&services, 0, sizeof(services));

    for(line = NextService(conf, -1); line != -1; line = NextService(conf, line))
    {
        service = conf->lines[line].service->value;
        for(i = 0; i < services.size; i++)
        {
            if(!strcmp(((char **)services.data)[i], service))
                break;
        }
        if(i == services.size)
        {
            BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(service, &service));
            BAIL_ON_CENTERIS_ERROR(ceError = CTArrayAppend(&services, sizeof(char *), &service, 1));
        }
    }

    if(serviceCount != NULL)
        *serviceCount = services.size;
    service = NULL;
    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayAppend(&services, sizeof(char *), &service, 1));
    *result = (char **)services.data;

error:
    return ceError;
}

static void FreeServicesList(char **list)
{
    CTFreeNullTerminatedStringArray(list);
}

static void UpdatePublicLines(struct PamConf *conf)
{
    conf->lines = conf->private_lines.data;
    conf->lineCount = conf->private_lines.size;
}

static const char *Basename(const char *path)
{
    const char *lastslash = strrchr(path, '/');
    if(lastslash != NULL)
        return lastslash + 1;
    return path;
}

//This function is only used on AIX right now
static DWORD CopyService(struct PamConf *conf, const char *oldName, const char *newName)
{
    DWORD ceError = ERROR_SUCCESS;
    int line = -1;
    int newLine;
    struct PamLine *newLineObj;
    for(;;)
    {
        line = NextLineForService(conf, line, oldName, NULL);
        if(line == -1)
            break;

        BAIL_ON_CENTERIS_ERROR(ceError = CopyLine(conf, line, &newLine));
        newLineObj = &conf->lines[newLine];
        /* Update the service name */
        CT_SAFE_FREE_STRING(newLineObj->service->value);
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(newName, &newLineObj->service->value));
        if(!strcmp(Basename(newLineObj->fromFile), oldName))
        {
            /* The file name is something like /etc/pam.d/<oldName>. It should
             * be updated. */
            char *newPath;

            /* This trims off the service name from the path */
            *strrchr(newLineObj->fromFile, '/') = '\0';
            BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&newPath, "%s/%s", newLineObj->fromFile, newName));
            CTFreeString(newLineObj->fromFile);
            newLineObj->fromFile = newPath;
        }
    }
    UpdatePublicLines(conf);
    conf->modified = 1;

error:
    return ceError;
}

static void FreePamLineContents(struct PamLine *line)
{
    int i;
    CT_SAFE_FREE_STRING(line->fromFile);
    CT_SAFE_FREE_STRING(line->leadingWhiteSpace);
    CT_SAFE_FREE_STRING(line->defaultInclude);
    CTFreeParseToken(&line->service);
    CTFreeParseToken(&line->phase);
    CTFreeParseToken(&line->control);
    CTFreeParseToken(&line->module);

    for(i = 0; i < line->optionCount; i++)
    {
        CTFreeParseTokenContents(&line->options[i]);
    }
    CT_SAFE_FREE_MEMORY(line->options);
    CT_SAFE_FREE_STRING(line->comment);
    line->optionCount = 0;
}

static
PCSTR
SkipBlankSpace(
    PCSTR pPos
    )
{
    BOOLEAN keepSkipping = TRUE;
    // do not skip unescaped newlines
    while (keepSkipping)
    {
        switch (pPos[0])
        {
            case '\\':
                if (pPos[1] == '\r' && pPos[2] == '\n')
                {
                    pPos += 3;
                }
                else if (pPos[1] == '\n')
                {
                    pPos += 2;
                }
                break;
            case ' ':
            case '\t':
                pPos++;
                break;
            default:
                keepSkipping = FALSE;
        }
    }

    return pPos;
}

BOOLEAN
IsWhitespace(
    PCSTR pPos
    )
{
    switch (pPos[0])
    {
        case '\\':
            if (pPos[1] == '\r' && pPos[2] == '\n')
            {
                return TRUE;
            }
            else if (pPos[1] == '\n')
            {
                return TRUE;
            }
            return FALSE;
        case ' ':
        case '\r':
        case '\n':
        case '\t':
            return TRUE;
        default:
            return FALSE;
    }
}

static DWORD ParsePamLine(struct PamLine *lineObj, const char *filename, const char *linestr, const char **endptr)
{
    DWORD ceError = ERROR_SUCCESS;
    const char *pos = linestr;
    const char *token_start = NULL;
    DynamicArray tokens;

    memset(&tokens, 0, sizeof(tokens));
    memset(lineObj, 0, sizeof(*lineObj));
    BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateString(filename, &lineObj->fromFile));

    /* Find the leading whitespace in the line */
    token_start = pos;
    pos = SkipBlankSpace(pos);
    BAIL_ON_CENTERIS_ERROR(ceError = CTStrndup(token_start, pos - token_start, &lineObj->leadingWhiteSpace));

    /* Create an array of white space separated tokens */
    /* Due to what pos points to when this gets calls, !isspace(*pos) actually
     * makes sure that pos is not pointing to '\n' or '\r'. pos will not be
     * pointing to a space or tab anyway.
     */
    while(!IsWhitespace(pos) && *pos != '\0' && *pos != '#')
    {
        CTParseToken token;
        token_start = pos;
        while(!IsWhitespace(pos) && *pos != '\0' && *pos != '#')
        {
            if(*pos == '[')
            {
                /* This could be a token like [success=1 default=die] . So
                 * this token continues until it finds the matching ], or end
                 * of line.
                 */
                do
                {
                    pos++;
                } while(*pos != '\0' && *pos != '#' && *pos != ']');
                if(*pos == ']')
                    pos++;
            }
            else
                pos++;
        }
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrndup(token_start, pos - token_start, &token.value));

        token_start = pos;
        pos = SkipBlankSpace(pos);
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrndup(token_start, pos - token_start, &token.trailingSeparator));
        BAIL_ON_CENTERIS_ERROR(ceError = CTArrayAppend(&tokens, sizeof(CTParseToken), &token, 1));
    }

    if(!strcmp(Basename(filename), "pam.conf"))
    {
        /* this did not come from a pam.d system */
        if(tokens.size > 0)
        {
            BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken), (void **)(PVOID) &lineObj->service));
            CTArrayRemoveHead(&tokens, sizeof(CTParseToken), lineObj->service, 1);
        }
    }
    else
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken), (void **) (PVOID)&lineObj->service));
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateString(Basename(filename), &lineObj->service->value));
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateString(" ", &lineObj->service->trailingSeparator));
    }

    if(tokens.size > 0)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken), (void **) (PVOID)&lineObj->phase));
        CTArrayRemoveHead(&tokens, sizeof(CTParseToken), lineObj->phase, 1);
    }

    if(tokens.size > 0)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken), (void **) (PVOID)&lineObj->control));
        CTArrayRemoveHead(&tokens, sizeof(CTParseToken), lineObj->control, 1);
    }

    if(tokens.size > 0)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken), (void **) (PVOID)&lineObj->module));
        CTArrayRemoveHead(&tokens, sizeof(CTParseToken), lineObj->module, 1);
    }

    lineObj->optionCount = tokens.size;
    lineObj->options = tokens.data;
    memset(&tokens, 0, sizeof(DynamicArray));

    token_start = pos;
    while(*pos != '\0' && *pos != '\n' && *pos != '\r') pos++;

    if(pos != token_start)
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrndup(token_start, pos - token_start, &lineObj->comment));
    else
        lineObj->comment = NULL;

    if(endptr != NULL)
        *endptr = pos;
    return ceError;

error:
    FreePamLineContents(lineObj);
    if(tokens.data != NULL)
    {
        int i;
        for(i = 0; i < tokens.size; i++)
        {
            CTFreeParseTokenContents(&((CTParseToken *)tokens.data)[i]);
        }
        CTArrayFree(&tokens);
    }
    return ceError;
}

static DWORD AddFormattedLine(struct PamConf *conf, const char *filename, const char *linestr, const char **endptr)
{
    DWORD ceError = ERROR_SUCCESS;
    struct PamLine lineObj;

    BAIL_ON_CENTERIS_ERROR(ceError = ParsePamLine(&lineObj, filename, linestr, endptr));

    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayAppend(&conf->private_lines, sizeof(struct PamLine), &lineObj, 1));
    memset(&lineObj, 0, sizeof(struct PamLine));
    UpdatePublicLines(conf);
    conf->modified = 1;

error:
    return ceError;
}

//This function is currently unused
#if 0
/* Parse and add a service to the list. */
static DWORD AddFormattedService(struct PamConf *conf, const char *filename, const char *contents)
{
    DWORD ceError = ERROR_SUCCESS;
    const char *pos = contents;
    while(*pos != '\0')
    {
        BAIL_ON_CENTERIS_ERROR(ceError = AddFormattedLine(conf, filename, pos, &pos));
        if(*pos == '\r')
            pos++;
        if(*pos == '\n')
            pos++;
    }

error:
    return ceError;
}
#endif

/* Copy a pam configuration line and add it below the old line. */
static DWORD CopyLine(struct PamConf *conf, int oldLine, int *newLine)
{
    DWORD ceError = ERROR_SUCCESS;
    struct PamLine lineObj;
    struct PamLine *oldObj = &conf->lines[oldLine];
    int i;
    memset(&lineObj, 0, sizeof(struct PamLine));

    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(oldObj->fromFile, &lineObj.fromFile));
    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(oldObj->leadingWhiteSpace, &lineObj.leadingWhiteSpace));
    BAIL_ON_CENTERIS_ERROR(ceError = CTDupOrNullStr(oldObj->defaultInclude, &lineObj.defaultInclude));
    BAIL_ON_CENTERIS_ERROR(ceError = CTCopyToken(oldObj->service, &lineObj.service));
    BAIL_ON_CENTERIS_ERROR(ceError = CTCopyToken(oldObj->phase, &lineObj.phase));
    BAIL_ON_CENTERIS_ERROR(ceError = CTCopyToken(oldObj->control, &lineObj.control));
    BAIL_ON_CENTERIS_ERROR(ceError = CTCopyToken(oldObj->module, &lineObj.module));
    BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken)*oldObj->optionCount, (void **) (PVOID)&lineObj.options));
    lineObj.optionCount = oldObj->optionCount;

    for(i = 0; i < oldObj->optionCount; i++)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(oldObj->options[i].value, &lineObj.options[i].value));
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(oldObj->options[i].trailingSeparator, &lineObj.options[i].trailingSeparator));
    }

    if(oldObj->comment != NULL)
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(oldObj->comment, &lineObj.comment));
    else
        lineObj.comment = NULL;

    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayInsert(&conf->private_lines, oldLine + 1, sizeof(struct PamLine), &lineObj, 1));
    memset(&lineObj, 0, sizeof(lineObj));
    if(newLine != NULL)
        *newLine = oldLine + 1;
    UpdatePublicLines(conf);
    conf->modified = 1;

error:
    FreePamLineContents(&lineObj);
    return ceError;
}

static DWORD UpdateSkipCounts(
        struct PamLine *lineObj,
        unsigned long minSkipCount,
        int offsetSkipCount)
{
    DWORD ceError = ERROR_SUCCESS;
    PCSTR inputPos = lineObj->control->value;
    PCSTR inputCopiedPos = lineObj->control->value;
    PSTR parsedSkipCountEnd = NULL;
    unsigned long parsedSkipCount = 0;
    StringBuffer outputBuffer = {0};
    char newSkipCount[32] = {0};

    if (inputPos == NULL)
        goto done;

    /* Skip the leading whitespace in the control */
    while (isblank((int) *inputPos)) inputPos++;

    if (*inputPos != '[')
    {
        // This is not an extended control option (there are no skip counts)
        goto done;
    }

    inputPos++;

    while (*inputPos != 0)
    {
        while (*inputPos != 0 && *inputPos != '=') inputPos++;

        if (*inputPos == 0)
            break;

        // Skip the =
        inputPos++;

        parsedSkipCount = strtoul(inputPos, &parsedSkipCountEnd, 10);

        if (parsedSkipCountEnd > inputPos && parsedSkipCount >= minSkipCount)
        {
            // This skip count needs to be updated
            BAIL_ON_CENTERIS_ERROR(ceError =
                    CTStringBufferAppendLength(&outputBuffer, inputCopiedPos,
                    inputPos - inputCopiedPos));

            snprintf(newSkipCount, sizeof(newSkipCount), "%lu",
                    parsedSkipCount + offsetSkipCount);
            newSkipCount[sizeof(newSkipCount) - 1] = 0;

            BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppend(
                        &outputBuffer, newSkipCount));

            inputCopiedPos = parsedSkipCountEnd;
        }
    }

    if (outputBuffer.data != NULL)
    {
        // This string has been modified. It needs to be saved back
        BAIL_ON_CENTERIS_ERROR(ceError =
                CTStringBufferAppendLength(&outputBuffer, inputCopiedPos,
                inputPos - inputCopiedPos));

        CT_SAFE_FREE_STRING(lineObj->control->value);
        lineObj->control->value = outputBuffer.data;
        outputBuffer.data = NULL;
    }

done:
error:

    CTStringBufferDestroy(&outputBuffer);
    return ceError;
}

static DWORD CopyLineAndUpdateSkips(
        struct PamConf *conf, int oldLine, int *newLine)
{
    DWORD ceError = ERROR_SUCCESS;
    int searchLine = oldLine;
    struct PamLine *oldLineObj = NULL;
    int minSkipDistance = 0;

    BAIL_ON_CENTERIS_ERROR(ceError = CopyLine(conf, oldLine, newLine));

    oldLineObj = &conf->lines[oldLine];
    searchLine = PrevLineForService(conf, searchLine,
            oldLineObj->service->value, oldLineObj->phase->value);
    minSkipDistance++;

    while (searchLine != -1)
    {
        UpdateSkipCounts(&conf->lines[searchLine], minSkipDistance,
                1);

        searchLine = PrevLineForService(conf, searchLine,
                oldLineObj->service->value, oldLineObj->phase->value);

        minSkipDistance++;
    }

error:
    return ceError;
}

static DWORD RemoveLine(struct PamConf *conf, int *line)
{
    DWORD ceError = ERROR_SUCCESS;
    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayRemove(&conf->private_lines, *line, sizeof(struct PamLine),
            1));
    UpdatePublicLines(conf);
    conf->modified = 1;

    if(*line >= conf->lineCount)
        *line = -1;

error:
    return ceError;
}

static DWORD RemoveLineAndUpdateSkips(struct PamConf *conf, int *line)
{
    DWORD ceError = ERROR_SUCCESS;
    int searchLine = *line;
    struct PamLine *oldLineObj = &conf->lines[*line];
    int minSkipDistance = 0;

    searchLine = PrevLineForService(conf, searchLine,
            oldLineObj->service->value, oldLineObj->phase->value);
    minSkipDistance++;

    while (searchLine != -1)
    {
        UpdateSkipCounts(&conf->lines[searchLine], minSkipDistance,
                -1);

        searchLine = PrevLineForService(conf, searchLine,
                oldLineObj->service->value, oldLineObj->phase->value);
        minSkipDistance++;
    }

    BAIL_ON_CENTERIS_ERROR(ceError = RemoveLine(conf, line));

error:
    return ceError;
}

static int ContainsOption(struct PamConf *conf, int line, const char *option)
{
    struct PamLine *lineObj = &conf->lines[line];
    int i;
    int found = 0;

    for(i = 0; i < lineObj->optionCount; i++)
    {
        if(!strcmp(lineObj->options[i].value, option))
            found++;
    }
    return found;
}

static DWORD AddOption(struct PamConf *conf, int line, const char *option)
{
    DWORD ceError = ERROR_SUCCESS;
    struct PamLine *lineObj = &conf->lines[line];
    CTParseToken newOption;
    CTParseToken *prevToken;
    CTParseToken *prevprevToken;
    /* Do not free the options variable */
    DynamicArray options;
    memset(&newOption, 0, sizeof(CTParseToken));

    if(ContainsOption(conf, line, option))
        return ceError;

    options.data = lineObj->options;
    options.size = lineObj->optionCount;
    options.capacity = lineObj->optionCount;

    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(option, &newOption.value));
    if(options.size >= 1)
    {
        prevprevToken = lineObj->module;
        prevToken = &lineObj->options[options.size - 1];
    }
    else
    {
        prevprevToken = lineObj->control;
        prevToken = lineObj->module;
    }

    /* Copy the white space from the previous token. Hopefully this will
     * make the spacing look consistent.
     */
    newOption.trailingSeparator = prevToken->trailingSeparator;
    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(prevprevToken->trailingSeparator, &prevToken->trailingSeparator));

    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayAppend(&options, sizeof(CTParseToken), &newOption, 1));
    memset(&newOption, 0, sizeof(CTParseToken));

    lineObj->options = options.data;
    lineObj->optionCount = options.size;
    conf->modified = 1;

error:
    /* Do not free the options variable */
    CTFreeParseTokenContents(&newOption);
    return ceError;
}

static DWORD RemoveOption(struct PamConf *conf, int line, const char *option, int *pFound)
{
    DWORD ceError = ERROR_SUCCESS;
    struct PamLine *lineObj = &conf->lines[line];
    /* Do not free the options variable */
    DynamicArray options;
    int i;
    int found = 0;
    options.data = lineObj->options;
    options.size = lineObj->optionCount;
    options.capacity = lineObj->optionCount;

    for(i = 0; i < options.size;)
    {
        if(!strcmp(((CTParseToken *)options.data)[i].value, option))
        {
            if(i == options.size - 1)
            {
                /* If this is the last option in the list, then move it's trailing white space value back one */
                if(i >= 1)
                {
                    CT_SAFE_FREE_STRING(((CTParseToken *)options.data)[i - 1].trailingSeparator);
                    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(
                        ((CTParseToken *)options.data)[i].trailingSeparator,
                        &((CTParseToken *)options.data)[i - 1].trailingSeparator));
                }
                else if(lineObj->module != NULL)
                {
                    CT_SAFE_FREE_STRING(lineObj->module->trailingSeparator);
                    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(
                        ((CTParseToken *)options.data)[i].trailingSeparator,
                        &lineObj->module->trailingSeparator));
                }
            }
            BAIL_ON_CENTERIS_ERROR(ceError = CTArrayRemove(&options, i, sizeof(CTParseToken), 1));
            found++;
        }
        else
            i++;
    }
    lineObj->options = options.data;
    lineObj->optionCount = options.size;
    if(found != 0)
    {
        conf->modified = 1;
    }
    if(pFound != NULL)
        *pFound = found;

error:
    /* Do not free the options variable */
    return ceError;
}

static
DWORD
AddPamServiceAlias(
    struct PamConf *conf,
    const char *pFromName,
    const char *pToName
    )
{
    struct PamServiceAlias alias = { 0 };
    DWORD ceError = 0;

    ceError = CTStrdup(
            pFromName,
            &alias.pFromService);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTStrdup(
            pToName,
            &alias.pToService);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTArrayAppend(
                    &conf->private_aliases,
                    sizeof(alias),
                    &alias,
                    1);
    BAIL_ON_CENTERIS_ERROR(ceError);
    memset(&alias, 0, sizeof(alias));

error:
    CT_SAFE_FREE_STRING(alias.pToService);
    CT_SAFE_FREE_STRING(alias.pFromService);
    return ceError;
}

static
const char *
GetUnaliasedServiceName(
    struct PamConf *conf,
    const char *name
    )
{
    struct PamServiceAlias *aliases = (struct PamServiceAlias *)conf->
        private_aliases.data;
    int i = 0;
    int aliasCount = conf->private_aliases.size;

    for (i = 0; i < aliasCount; i++)
    {
        if (!strcmp(aliases[i].pFromService, name))
        {
            return aliases[i].pToService;
        }
    }
    return name;
}

void GetModuleControl(struct PamLine *lineObj, const char **module, const char **control);

static DWORD ReadPamFile(struct PamConf *conf, const char *rootPrefix, const char *filename)
{
    DWORD ceError = ERROR_SUCCESS;
    struct stat statbuf;
    FILE *file = NULL;
    PSTR buffer = NULL;
    char *fullPath = NULL;
    BOOLEAN endOfFile = FALSE;
    // Do not free
    struct PamLine *parsedLine = NULL;
    const char *module;
    const char *control;
    char normalizedModule[256];
    FILE *loginMapFile = NULL;
    // Do no free
    char *defaultInclude = NULL;
    PSTR pCombinedLine = NULL;
    PSTR pNextLine = NULL;

    BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(
            &fullPath, "%s%s", rootPrefix, filename));
    DJ_LOG_INFO("Reading pam file %s", fullPath);
    if (stat(fullPath, &statbuf) < 0)
    {
        DJ_LOG_INFO("File %s does not exist", fullPath);
        ceError = ERROR_FILE_NOT_FOUND;
        goto error;
    }
    if (!S_ISREG(statbuf.st_mode))
    {
        DJ_LOG_INFO("File %s is not a regular file", fullPath);
        ceError = ERROR_FILE_NOT_FOUND;
        goto error;
    }

    BAIL_ON_CENTERIS_ERROR(ceError = CTOpenFile(fullPath, "r", &file));
    while(TRUE)
    {
        CT_SAFE_FREE_STRING(buffer);
        BAIL_ON_CENTERIS_ERROR(ceError = CTReadNextLine(file, &buffer, &endOfFile));
        if(endOfFile)
            break;

        // A backslash followed by a newline seems to be treated as whitespace.
        // Two backslashes followed by a newline turns into one backslash then
        // whitespace. 
        while (CTStrEndsWith(buffer, "\\\n") || CTStrEndsWith(buffer,
                    "\\\r\n"))
        {
            LW_SAFE_FREE_STRING(pNextLine);
            ceError = CTReadNextLine(file, &pNextLine, &endOfFile);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (endOfFile)
            {
                break;
            }

            ceError = LwAllocateStringPrintf(
                            &pCombinedLine,
                            "%s%s",
                            buffer,
                            pNextLine);
            BAIL_ON_CENTERIS_ERROR(ceError);

            LW_SAFE_FREE_STRING(buffer);
            LW_SAFE_FREE_STRING(pNextLine);
            buffer = pCombinedLine;
            pCombinedLine = NULL;
        }
        BAIL_ON_CENTERIS_ERROR(ceError = AddFormattedLine(conf, filename, buffer, NULL));

        // If the line is a pam_per_user, determine what the default include
        // name is.
        parsedLine = &conf->lines[conf->lineCount - 1];
        GetModuleControl(parsedLine, &module, &control);
        NormalizeModuleName( normalizedModule, module, sizeof(normalizedModule));
        if (!strcmp(normalizedModule, "pam_per_user") &&
                parsedLine->optionCount >= 1)
        {
            CT_SAFE_FREE_STRING(fullPath);
            BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(
                    &fullPath, "%s%s",
                    rootPrefix,
                    parsedLine->options[0].value));
            BAIL_ON_CENTERIS_ERROR(ceError = CTOpenFile(fullPath, "r",
                    &loginMapFile));
            while (!parsedLine->defaultInclude)
            {
                CT_SAFE_FREE_STRING(buffer);
                BAIL_ON_CENTERIS_ERROR(ceError = CTReadNextLine(loginMapFile,
                            &buffer, &endOfFile));
                if (endOfFile)
                {
                    break;
                }
                if (buffer[0] == '*')
                {
                    // This is the default entry
                    defaultInclude = strrchr(buffer, ':');
                    if (defaultInclude)
                    {
                        // Skip the :
                        defaultInclude++;
                        CTStripWhitespace(defaultInclude);
                        BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(
                                defaultInclude,
                                &parsedLine->defaultInclude));
                    }
                }
            }
            BAIL_ON_CENTERIS_ERROR(ceError = CTSafeCloseFile(&loginMapFile));
        }
    }
    BAIL_ON_CENTERIS_ERROR(ceError = CTSafeCloseFile(&file));

error:
    CTSafeCloseFile(&file);
    CTSafeCloseFile(&loginMapFile);
    CT_SAFE_FREE_STRING(fullPath);
    CT_SAFE_FREE_STRING(buffer);
    LW_SAFE_FREE_STRING(pCombinedLine);
    LW_SAFE_FREE_STRING(pNextLine);
    return ceError;
}

static
DWORD
ReadPamDirFile(
    IN OUT struct PamConf *conf,
    IN PCSTR pRootPrefix,
    IN PSTR pDirPath,
    IN PSTR pFileEntry
    )
{
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN bIsDir = FALSE;
    PSTR pNonPrefixedFilePath = NULL;
    PSTR pFilePath = NULL;
    PSTR pSymTarget = NULL;
    struct stat linkTargetStat = { 0 };
    struct stat pamDirectoryStat = { 0 };
    struct stat compareDirectoryStat = { 0 };
    PSTR pSymDirName = NULL;
    PSTR pSymBaseName = NULL;
    PSTR pAbsSymDirName = NULL;

    if (pFileEntry[0] == '.' || /*Ignore hidden files*/
       CTStrEndsWith(pFileEntry, ".bak") ||
       CTStrEndsWith(pFileEntry, ".new") ||
       CTStrEndsWith(pFileEntry, ".orig") ||
       CTStrEndsWith(pFileEntry, "~") ||
       CTStrStartsWith(pFileEntry, "#"))
    {
        goto error;
    }

    ceError = CTAllocateStringPrintf(
                    &pFilePath,
                    "%s%s/%s",
                    pRootPrefix,
                    pDirPath,
                    pFileEntry);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTCheckDirectoryExists(pFilePath, &bIsDir);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bIsDir)
    {
        goto error;
    }

    ceError = CTGetSymLinkTarget(pFilePath, &pSymTarget);
    if (ceError == LW_ERROR_INVALID_PARAMETER)
    {
        // It is not a symlink
        ceError = 0;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pSymTarget != NULL)
    {
        // Verify the symlink target exists
        while (stat(pFilePath, &linkTargetStat) < 0)
        {
            if (errno == EINTR)
                continue;
            if (errno == ENOENT)
            {
                DJ_LOG_WARNING("Pam file %s is a dangling symlink with target %s. Skipping", pFilePath, pSymTarget);
                goto error;
            }
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        // Only treat this symlink as an alias if it points to a file in the
        // same directory.

        pSymBaseName = basename(pSymTarget);
        pSymDirName = dirname(pSymTarget);

        while (stat(pDirPath, &pamDirectoryStat) < 0)
        {
            if (errno == EINTR)
                continue;
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        if (pSymDirName[0] != '/')
        {
            // If the path is relative, make it absolute
            ceError = CTAllocateStringPrintf(
                            &pAbsSymDirName,
                            "%s/%s",
                            pDirPath,
                            pSymDirName);
            BAIL_ON_CENTERIS_ERROR(ceError);
            pSymDirName = pAbsSymDirName;
        }
        while (stat(pSymDirName, &compareDirectoryStat) < 0)
        {
            if (errno == EINTR)
                continue;
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if (pamDirectoryStat.st_dev != compareDirectoryStat.st_dev ||
                pamDirectoryStat.st_ino != compareDirectoryStat.st_ino)
        {
            CT_SAFE_FREE_STRING(pSymTarget);
            pSymBaseName = NULL;
        }
    }

    if (pSymBaseName != NULL)
    {
        ceError = AddPamServiceAlias(
                conf,
                pFileEntry,
                pSymBaseName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = CTAllocateStringPrintf(
                        &pNonPrefixedFilePath,
                        "%s/%s",
                        pDirPath,
                        pFileEntry);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = ReadPamFile(conf, pRootPrefix, pNonPrefixedFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    CT_SAFE_FREE_STRING(pFilePath);
    CT_SAFE_FREE_STRING(pAbsSymDirName);
    CT_SAFE_FREE_STRING(pNonPrefixedFilePath);
    CT_SAFE_FREE_STRING(pSymTarget);
    return ceError;
}

/* On a real system, set the rootPrefix to "". When testing, set it to the
 * test directory that mirrors the target file system.
 */
static DWORD ReadPamConfiguration(const char *rootPrefix, struct PamConf *conf)
{
    DWORD ceError = ERROR_SUCCESS;
    DIR *dp = NULL;
    struct dirent *dirp;
    PSTR pszDirPath = NULL;

    memset(conf, 0, sizeof(struct PamConf));

    DJ_LOG_INFO("Reading pam configuration");

    ceError = CTAllocateStringPrintf(&pszDirPath, "%s/etc/pam.d", rootPrefix);
    BAIL_ON_CENTERIS_ERROR(ceError);

    dp = opendir(pszDirPath);
    if (dp != NULL)
    {
        while ((dirp = readdir(dp)) != NULL)
        {
            ceError = ReadPamDirFile(
                            conf,
                            rootPrefix,
                            "/etc/pam.d",
                            dirp->d_name);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    CT_SAFE_FREE_STRING(pszDirPath);

    if (dp != NULL)
    {
        closedir(dp);
    }

    ceError = CTAllocateStringPrintf(&pszDirPath, "%s/usr/local/etc/pam.d", rootPrefix);
    BAIL_ON_CENTERIS_ERROR(ceError);

    dp = opendir(pszDirPath);
    if (dp != NULL)
    {
        while ((dirp = readdir(dp)) != NULL)
        {
            ceError = ReadPamDirFile(
                            conf,
                            rootPrefix,
                            "/usr/local/etc/pam.d",
                            dirp->d_name);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    ceError = ReadPamFile(conf, rootPrefix, "/etc/pam.conf");

    if (ceError == ERROR_FILE_NOT_FOUND && conf->lineCount > 0)
    {
        ceError = ERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    conf->modified = 0;

error:

    CT_SAFE_FREE_STRING(pszDirPath);

    if(dp != NULL)
    {
        closedir(dp);
    }

    return ceError;
}

struct OpenFile
{
    const char *name;
    char *tempPath;
    FILE *file;
    BOOLEAN followSymlinks;
};

static DWORD FindFile(const char *rootPrefix, const char *filename, DynamicArray *openFiles, int *lastIndexed, FILE **file)
{
    DWORD ceError = ERROR_SUCCESS;
    struct OpenFile *files = (struct OpenFile *)openFiles->data;
    int i;
    struct OpenFile newFile;
    char *tempPath = NULL;
    char *prefixedPath = NULL;
    memset(&newFile, 0, sizeof(newFile));
    BOOLEAN followSymlinks = TRUE;

    if(*lastIndexed < openFiles->size &&
            !strcmp(filename, files[*lastIndexed].name))
    {
        *file = files[*lastIndexed].file;
        goto cleanup;
    }

    for(i = 0; i < openFiles->size; i++)
    {
        if(!strcmp(filename, files[i].name))
        {
            *lastIndexed = i;
            *file = files[*lastIndexed].file;
            goto cleanup;
        }
    }

    BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&prefixedPath, "%s%s", rootPrefix, filename));
    ceError = CTGetFileTempPath(
                        prefixedPath,
                        NULL,
                        &tempPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (i = 0; i < openFiles->size; i++)
    {
        if (!strcmp(tempPath, files[i].tempPath))
        {
            DJ_LOG_INFO("%s conflicts with %s - breaking symlinks to solve conflict", filename, files[i].name);
            followSymlinks = FALSE;
            files[i].followSymlinks = FALSE;
            break;
        }
    }
    if (!followSymlinks)
    {
        CT_SAFE_FREE_STRING(tempPath);
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&tempPath, "%s.lwidentity.new.conflicted", prefixedPath));
    }

    /* The file hasn't been opened yet. */
    newFile.name = filename;
    newFile.tempPath = tempPath;
    tempPath = NULL;
    newFile.followSymlinks = followSymlinks;

    ceError = CTOpenFile(newFile.tempPath, "w", &newFile.file);
    if(ceError)
    {
        DJ_LOG_ERROR("Unable to open '%s' for writing", tempPath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayAppend(openFiles, sizeof(struct OpenFile), &newFile, 1));
    files = (struct OpenFile *)openFiles->data;
    memset(&newFile, 0, sizeof(newFile));

    *lastIndexed = openFiles->size - 1;
    *file = files[*lastIndexed].file;

cleanup:
error:
    if(newFile.file != NULL)
        CTCloseFile(newFile.file);
    CT_SAFE_FREE_STRING(tempPath);
    CT_SAFE_FREE_STRING(newFile.tempPath);
    CT_SAFE_FREE_STRING(prefixedPath);
    return ceError;
}

static DWORD MoveFiles(const char *rootPrefix, DynamicArray *openFiles, PSTR *diff)
{
    DWORD ceError = ERROR_SUCCESS;
    int i;
    char *finalName = NULL;
    char *prefixedPath = NULL;
    PSTR oldDiff = NULL;
    PSTR fileDiff = NULL;
    BOOLEAN same;
    struct OpenFile *files = (struct OpenFile *)openFiles->data;

    for(i = 0; i < openFiles->size; i++)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&prefixedPath, "%s%s", rootPrefix, files[i].name));
        ceError = CTGetFileTempPath(
                            prefixedPath,
                            &finalName,
                            NULL);
        BAIL_ON_CENTERIS_ERROR(ceError);

        BAIL_ON_CENTERIS_ERROR(ceError = CTFileContentsSame(files[i].tempPath, finalName, &same));
        if(same)
        {
            DJ_LOG_INFO("Pam file %s unmodified", finalName);
            BAIL_ON_CENTERIS_ERROR(ceError = CTRemoveFile(files[i].tempPath));
        }
        else
        {
            DJ_LOG_INFO("Pam file %s modified", finalName);
            if(diff != NULL)
            {
                CT_SAFE_FREE_STRING(oldDiff);
                CT_SAFE_FREE_STRING(fileDiff);
                oldDiff = *diff;
                if(!oldDiff)
                    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup("", &oldDiff));
                else
                    *diff = NULL;
                BAIL_ON_CENTERIS_ERROR(ceError = CTGetFileDiff(finalName, files[i].tempPath, &fileDiff, FALSE));
                BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(diff, "%sChanges to %s:\n%s", oldDiff, files[i].name, fileDiff));
            }
            if (files[i].followSymlinks)
            {
                ceError = CTSafeReplaceFile(finalName, files[i].tempPath);
            }
            else
            {
                ceError = CTSafeReplaceFile(prefixedPath, files[i].tempPath);
            }
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        CT_SAFE_FREE_STRING(finalName);
        CT_SAFE_FREE_STRING(prefixedPath);
    }

error:
    CT_SAFE_FREE_STRING(finalName);
    CT_SAFE_FREE_STRING(prefixedPath);
    CT_SAFE_FREE_STRING(oldDiff);
    CT_SAFE_FREE_STRING(fileDiff);
    return ceError;
}

static DWORD CloseFiles(const char *rootPrefix, DynamicArray *openFiles)
{
    DWORD firstError = ERROR_SUCCESS, lastError;
    int i;
    struct OpenFile *files = (struct OpenFile *)openFiles->data;
    for(i = 0; i < openFiles->size; i++)
    {
        /* Close all of the files, but return the error from the first one that
         * failed. */
        if(files[i].file != NULL)
        {
            lastError = CTCloseFile(files[i].file);
            if(!firstError)
                firstError = lastError;
            files[i].file = NULL;
        }
    }

    return firstError;
}

static DWORD AppendToken(StringBuffer *save, CTParseToken *token)
{
    DWORD ceError = ERROR_SUCCESS;
    if(token != NULL)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppend(save, token->value));
        BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppend(save, token->trailingSeparator));
    }

error:
    return ceError;
}

//The string buffer must have already been constructed
static DWORD AppendPamLine(StringBuffer *save, struct PamLine *lineObj)
{
    DWORD ceError = ERROR_SUCCESS;
    int j;

    GCE(ceError = CTStringBufferAppend(save, lineObj->leadingWhiteSpace));
    if(lineObj->service != NULL && strcmp(Basename(lineObj->fromFile), lineObj->service->value))
    {
        /* This entry is from a pam.conf file */
        GCE(ceError = AppendToken(save, lineObj->service));
    }
    GCE(ceError = AppendToken(save, lineObj->phase));
    GCE(ceError = AppendToken(save, lineObj->control));
    GCE(ceError = AppendToken(save, lineObj->module));
    for(j = 0; j < lineObj->optionCount; j++)
    {
        GCE(ceError = AppendToken(save, &lineObj->options[j]));
    }
    if(lineObj->comment != NULL)
        GCE(ceError = CTStringBufferAppend(save, lineObj->comment));

cleanup:
    return ceError;
}

static DWORD WritePamConfiguration(const char *rootPrefix, struct PamConf *conf, PSTR *diff)
{
    DWORD ceError = ERROR_SUCCESS;
    DynamicArray openFiles;
    struct OpenFile *files = NULL;
    int lastIndexedFile = 0;
    int i = 0;
    StringBuffer buffer;
    memset(&openFiles, 0, sizeof(openFiles));
    BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferConstruct(&buffer));

    DJ_LOG_INFO("Writing pam configuration");

    for(i = 0; i < conf->lineCount; i++)
    {
        struct PamLine *lineObj = &conf->lines[i];
        FILE *file = NULL;
        BAIL_ON_CENTERIS_ERROR(ceError = FindFile(rootPrefix, lineObj->fromFile, &openFiles, &lastIndexedFile, &file));
        CTStringBufferClear(&buffer);
        BAIL_ON_CENTERIS_ERROR(ceError = AppendPamLine(&buffer, lineObj));
        BAIL_ON_CENTERIS_ERROR(ceError = CTFilePrintf(file, "%s\n", buffer.data));
    }

    BAIL_ON_CENTERIS_ERROR(ceError = CloseFiles(rootPrefix, &openFiles));
    BAIL_ON_CENTERIS_ERROR(ceError = MoveFiles(rootPrefix, &openFiles, diff));

error:
    files = (struct OpenFile *)openFiles.data;
    for (i = 0; i < openFiles.size; i++)
    {
        CT_SAFE_FREE_STRING(files[i].tempPath);
    }
    CloseFiles(rootPrefix, &openFiles);
    CTArrayFree(&openFiles);
    CTStringBufferDestroy(&buffer);
    return ceError;
}

static
void
FreePamServiceAliasContents(
    struct PamServiceAlias *pAlias
    )
{
    CT_SAFE_FREE_STRING(pAlias->pFromService);
    CT_SAFE_FREE_STRING(pAlias->pToService);
}

static void FreePamConfContents(struct PamConf *conf)
{
    int i;
    for(i = 0; i < conf->lineCount; i++)
    {
        FreePamLineContents(&conf->lines[i]);
    }
    CTArrayFree(&conf->private_lines);
    UpdatePublicLines(conf);

    for(i = 0; i < conf->private_aliases.size; i++)
    {
        FreePamServiceAliasContents(&((struct PamServiceAlias *)
                    conf->private_aliases.data)[i]);
    }
    CTArrayFree(&conf->private_aliases);
}

/* Strips known module paths off from the beginning of the module name, and strips off known extensions from the end of the module name.
 *
 * The result is stored in destName. destName is a buffer of size 'bufferSize'.
 *
 * If destName is not large enough, as much of srcName is copied as possible,
 * and false is returned.
 *
 * If bufferSize >= 1, then destName will always be null terminated.
 */
static BOOLEAN NormalizeModuleName( char *destName, const char *srcName, size_t bufferSize)
{
    size_t trimEnd = 0;
    size_t copyLen;
    PSTR ppPrefixes[] = {
        "/usr/lib/security/$ISA/",
        "/usr/lib/security/",
        "/lib64/security/",
        "/usr/lib64/security/",
        "/lib/security/hpux32/",
        "/lib/security/hpux64/",
        "/lib/security/$ISA/",
        "/lib/security/",
        "/usr/lib/vmware/lib/libpam.so.0/security/",
        "/usr/lib/pam/",
        "/usr/lib/security/sparcv9/",
        "/usr/lib/security/amd64/",
        "/usr/local/lib/",
        "/opt/SUNWut/lib/",
        "/opt/SUNWkio/lib/",
        NULL
    };
    DWORD index = 0;

    if(bufferSize < 1)
        return FALSE;

    for (index = 0; ppPrefixes[index] != NULL; index++)
    {
        if (CTStrStartsWith(srcName, ppPrefixes[index]))
        {
            srcName += strlen(ppPrefixes[index]);
            break;
        }
    }

    /* Remove any "lib" prefix so that libpam_foo == pam_foo.  HP-UX has a bunch
     * of libpam_foo.  If we need to distinguish libpam_foo vs pam_foo in the future,
     * we will need to add back libpam_X rules in various places. */
    if(CTStrStartsWith(srcName, "lib"))
        srcName += strlen("lib");

    if(CTStrEndsWith(srcName, ".sl"))
        trimEnd = strlen(".sl");
    else if(CTStrEndsWith(srcName, ".sl.1"))
        trimEnd = strlen(".sl.1");
    else if(CTStrEndsWith(srcName, ".so"))
        trimEnd = strlen(".so");
    else if(CTStrEndsWith(srcName, ".so.1"))
        trimEnd = strlen(".so.1");
    else if(CTStrEndsWith(srcName, ".1"))
        trimEnd = strlen(".1");

    if (srcName == NULL)
    {
        copyLen = 0;
    }
    else
    {
        copyLen = strlen(srcName) - trimEnd;
    }
    if(copyLen > bufferSize - 1)
        copyLen = bufferSize - 1;

    strncpy(destName, srcName, copyLen);
    destName[copyLen] = 0;
    return TRUE;
}

/* returns true if the pam module should be commented out when pam_lwidentity is enabled.
 */
static BOOLEAN PamModuleGrants( const char * phase, const char * module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));

    if(!strcmp(buffer, "pam_deny"))
        return FALSE;
    if(!strcmp(buffer, "pam_prohibit"))
        return FALSE;
    /* We use pam_sample on Solaris 8 to act like pam_deny. pam_sample could
     * actually act like something else depending on which options are passed,
     * but it is safe to assume that it always blocks.
     */
    if(!strcmp(buffer, "pam_sample"))
        return FALSE;

    /* Assume that the module returns success for someone */
    return TRUE;
}

static BOOLEAN PamModuleIsLwidentity(const char *phase, const char *module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));
    if(!strcmp(buffer, "pam_lsass"))
        return TRUE;
    if(!strcmp(buffer, "pam_lwidentity"))
        return TRUE;
    if(!strcmp(buffer, "libpam_lwidentity"))
        return TRUE;

    return FALSE;
}

static BOOLEAN PamModuleIsPwcheck(const char *phase, const char *module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));
    if(!strcmp(buffer, "pam_pwcheck"))
        return TRUE;

    return FALSE;
}

static BOOLEAN PamModuleIsOldCenteris(const char *phase, const char *module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));
    if(!strcmp(buffer, "pam_centeris"))
        return TRUE;

    return FALSE;
}

static BOOLEAN PamModuleIsLwiPassPolicy(const char *phase, const char *module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));
    if(!strcmp(buffer, "pam_lwipasspolicy"))
        return TRUE;

    return FALSE;
}

/* returns true if the pam module grants users access based on the caller's UID or GID, or maybe the machine the user is trying to connect from.
 *
 * Basically this returns true if the module does not prompt for a password, but can let the user login based on some kind of semi-intellegent check.
 */
static BOOLEAN PamModuleChecksCaller( const char * phase, const char * module)
{
    char buffer[256];

    NormalizeModuleName( buffer, module, sizeof(buffer));

    if(!strcmp(buffer, "pam_wheel"))
        return TRUE;
    if(!strcmp(buffer, "pam_rootok"))
        return TRUE;
    if(!strcmp(buffer, "pam_allowroot"))
        return TRUE;
    if(!strcmp(buffer, "pam_self"))
        return TRUE;
    if(!strcmp(buffer, "pam_rhosts_auth"))
        return TRUE;
    if(!strcmp(buffer, "pam_rhosts"))
        return TRUE;
    if(!strcmp(buffer, "pam_console"))
        return TRUE;
    if(!strcmp(buffer, "pam_timestamp"))
        return TRUE;
    if(!strcmp(buffer, "pam_krb5"))
        return TRUE;
    // This is the same as pam_krb5 except that it grabs an AFS token too
    if(!strcmp(buffer, "pam_krb5afs"))
        return TRUE;
    if(!strcmp(buffer, "pam_securid"))
        return TRUE;
    if(!strcmp(buffer, "pam_opie"))
        return TRUE;
    //Used by IBM Director. Found at Gap
    if(!strcmp(buffer, "pam_ve"))
        return TRUE;
    //Used by Opsware. Found at Gap
    if(!strcmp(buffer, "/opt/OPSWsshd/libexec/opsw_auth"))
        return TRUE;
    if(!strcmp(buffer, "pam_succeed_if"))
        return TRUE;
    // Used on Fedora Core 11. This is a Finger Print reader authentication module, does not prompt for password.
    if(!strcmp(buffer, "pam_fprintd"))
        return TRUE;
    // Used on Solaris. Automatically enabled by SunRay Server Software
    if(!strcmp(buffer, "pam_kiosk"))
        return TRUE;
    if(!strcmp(buffer, "pam_sunray_admingui"))
        return TRUE;
    if (!strcmp(buffer, "pam_sunray"))
    {
        return TRUE;
    }
    if (!strcmp(buffer, "pam_passwd_auth"))
    {
        return TRUE;
    }
    if (!strcmp(buffer, "pam_sunray_amgh"))
    {
        return TRUE;
    }

    return FALSE;
}

/* returns true if the pam module prompts for a password and stores it in the pam state.
 */
static BOOLEAN PamModulePrompts( const char * phase, const char * module)
{
    char buffer[256];

    /* Modules only prompt during the auth and account phase */
    if(strcmp(phase, "auth") && strcmp(phase, "password"))
        return FALSE;

    NormalizeModuleName( buffer, module, sizeof(buffer));

    if(!strcmp(buffer, "pam_unix"))
        return TRUE;
    /* On Redhat 2.1, 7.3, and 7.2 there is a pam_pwdb.so. It seems to do
       the same thing as pam_unix.so. For some reason, the distro includes
       both modules, and services use one or the other.
     */
    if(!strcmp(buffer, "pam_pwdb"))
        return TRUE;
    if(!strcmp(buffer, "pam_unix2"))
        return TRUE;
    if(!strcmp(buffer, "pam_authtok_get"))
        return TRUE;
    if(!strcmp(buffer, "pam_unix_auth"))
        return TRUE;
    /* Fix bug 5239. Upon further research it looks like pam_dhkeys does
     * check the user's password. However, the module will still return
     * PAM_IGNORE if the user isn't logging in from NIS.
     */
    if(!strcmp(buffer, "pam_dhkeys") && !strcmp(phase, "auth"))
        return TRUE;
    if(!strcmp(buffer, "pam_aix"))
        return TRUE;
    if (!strcmp(buffer, "pam_passwd_auth"))
    {
        /* This module appears on Solaris in the auth phase only for password
         * changes.
         *      For root, it will not prompt for the current password.
         *      For domain accounts, it will not prompt for the current
         *          password.
         *      For local accounts, it will prompt only if the username is the
         *          same as the one running the command.
         *
         * Unfortunately, if pam_passwd_auth detects that a password is stored
         * in the pam handle, it always return success.
         */
        return FALSE;
    }
    if(!strcmp(buffer, "pam_securityserver"))
        return TRUE;
    if(!strcmp(buffer, "pam_serialnumber"))
        return TRUE;
    if(!strcmp(buffer, "pam_winbind"))
        return TRUE;
    if(!strcmp(buffer, "pam_ldap"))
        return TRUE;
    // Part of freeradius. It authenticates the user by a radius server
    if (!strcmp(buffer, "pam_radius_auth"))
        return TRUE;
    //This was found on SLED10 SP1. It is used for Novell's E-Directory.
    if(!strcmp(buffer, "pam_nam"))
        return TRUE;
    //This was found on Ubuntu Gutsy. It is used for pure-ftpd
    if(!strcmp(buffer, "pam_ftp"))
        return TRUE;
    //Found on RHEL 2.1 AS. Fixes bug #5374
    if(!strcmp(buffer, "pam_smb_auth"))
        return TRUE;
    if(!strcmp(buffer, "pam_mysql"))
        return TRUE;
    if(!strcmp(buffer, "pam_gnome_keyring"))
        return TRUE;
    // This module is used on Max OS X 10.7
    if(!strcmp(buffer, "pam_ntlm"))
        return TRUE;
    // This module is used on Max OS X 10.6 - Snow Leopard
    if(!strcmp(buffer, "pam_opendirectory"))
        return TRUE;
    // This module is used on Max OS X 10.6 - Snow Leopard
    if(!strcmp(buffer, "pam_mount"))
        return TRUE;
    // This module is used on Max OS X 10.5 - Leopard
    if(!strcmp(buffer, "pam_afpmount"))
        return TRUE;
    // Used on Mandriva 2009 as an alternative to pam_unix
    if (!strcmp(buffer, "pam_tcb"))
        return TRUE;
    // From FoxT BoKS
    if (!strcmp(buffer, "pam_boks"))
        return TRUE;
    // From Parallels Plesk Panel for Linux
    if (!strcmp(buffer, "pam_plesk"))
        return TRUE;

    /* pam_lwidentity will only prompt for domain users during the password phase. All in all, it doesn't store passwords for subsequent modules in the password phase. */
    if(PamModuleIsLwidentity(phase, module) && !strcmp(phase, "auth"))
        return TRUE;
    if(PamModuleIsLwiPassPolicy(phase, module) && !strcmp(phase, "password"))
        return TRUE;
    if(!strcmp(phase, "password") && !strcmp(buffer, "pam_pwcheck"))
        return TRUE;
    if(!strcmp(phase, "password") && !strcmp(buffer, "pam_cracklib"))
        return TRUE;

    /* Assume that the module does not prompt */
    return FALSE;
}

static BOOLEAN PamModuleUnderstandsTryFirstPass( const char * phase, const char * module)
{
    const char *moduleBasename = strrchr(module, '/');
    char buffer[256];

    if (!moduleBasename)
    {
        moduleBasename = module;
    }
    else
    {
        moduleBasename++;
    }

    if(!PamModulePrompts(phase, module))
        return FALSE;

    NormalizeModuleName( buffer, module, sizeof(buffer));

    /* pam_aix treats try_first_pass as try the existing password if it is set.
     * If the existing password that was set is wrong, prompt again and put up
     * an obscure error message if the second password is correct.
     */
    if(!strcmp(buffer, "pam_aix"))
        return FALSE;
    /* libpam_unix.so.1 on HP-UX will prompt with "System Password:" if the
       existing password is incorrect. The module name will get normalized to
       pam_unix, which Linux systems also use (but it works on them). So the
       prefix of the unnormalized module name is also checked.
       */
    if(!strcmp(buffer, "pam_unix") && CTStrStartsWith(moduleBasename, "lib"))
        return FALSE;

    /* pam_unix2 on OpenSuse 11, and SLES 9.x is known to not support this
     * option. The man page also says it is not supported. No distros that use
     * pam_unix2 are currently known to suport it. */
    if (!strcmp(buffer, "pam_unix2"))
        return FALSE;

    if (!strcmp(buffer, "pam_boks"))
        return FALSE;

    return TRUE;
}

/* returns true if the pam module returns PAM_SUCCESS either all the time, or for users based on a password.
 */
static BOOLEAN PamModuleRemoveOnEnable( const char * phase, const char *control, const char * module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));

    /* Fix bug 5557. On HP-UX 11.31, libpam_hpsec.so.1 also blocks domain
     * users in the account phase. */
    if((!strcmp(phase, "auth") || !strcmp(phase, "account")) &&
            !strcmp(control, "required") &&
            !strcmp(buffer, "pam_hpsec"))
    {
        return TRUE;
    }

    return FALSE;
}

/* returns true if the pam module returns a pam error code either all the time, or for users based on a password.
 */
static BOOLEAN PamModuleDenies( const char * phase, const char * module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));

    if(!strcmp(buffer, "pam_authtok_get"))
        return FALSE;
    if(!strcmp(buffer, "pam_permit"))
        return FALSE;
    if(!strcmp(buffer, "pam_env"))
        return FALSE;

    /* Assume that the module sometimes denies logins */
    return TRUE;
}

/* returns true if the pam module will always return an error code for a domain user (assuming that a local account by the same name doesn't exist)
 */
static BOOLEAN PamModuleAlwaysDeniesDomainLogins( const char * phase, const char * module, const LwDistroInfo *distro)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));

    /* If it never denies login, it won't deny login for domain users */
    if(!PamModuleDenies(phase, module))
        return FALSE;

    /* If it always denies login, it will for domain users too */
    if(!PamModuleGrants(phase, module))
        return TRUE;

    if(PamModuleIsLwidentity(phase, module))
        return FALSE;

    /* The domain user will not be root */
    if(!strcmp(buffer, "pam_rootok"))
        return TRUE;
    if(!strcmp(buffer, "pam_allowroot"))
        return TRUE;
    if(!strcmp(buffer, "pam_self"))
        return TRUE;

    /* It would be too difficult to debug a POC where securid was blocking SSH
     * for domain users. */
    if(!strcmp(buffer, "pam_securid"))
        return TRUE;
    if(!strcmp(buffer, "pam_opie"))
        return TRUE;

    /* These modules return errors for users they don't know about */
    if(!strcmp(buffer, "pam_unix_account"))
        return TRUE;
    if(!strcmp(buffer, "pam_unix_session"))
        return TRUE;
    if(!strcmp(buffer, "pam_authtok_store"))
        return TRUE;

    /* We don't really want to use local password checks for domain users*/
    if(!strcmp(buffer, "pam_authtok_check"))
        return TRUE;
    if(!strcmp(buffer, "pam_cracklib"))
        return TRUE;
    if(!strcmp(buffer, "pam_pwcheck"))
        return TRUE;

    /* The pam_mount and pam_afpmount modules on OS X prompts,
     * but don't actually authenticate the user. */
    if(!strcmp(buffer, "pam_mount") || !strcmp(buffer, "pam_afpmount"))
        return FALSE;

    /* pam_hpsec on HP-UX blocks only for auth and session.  It is a no-op for
     * account and password.  The issue is that it returns unknown user because
     * it does not use nsswitch and only looks at local accounts. */
    if((!strcmp(phase, "auth") || !strcmp(phase, "session")) && !strcmp(buffer, "pam_hpsec"))
        return TRUE;

    /* We don't really want to use our own local password checks either */
    if(PamModuleIsLwiPassPolicy(phase, module))
        return TRUE;

    /* The password change prompts are out of order if our module is installed
     * before pam_authtok_get, which comes after pam_dhkeys on Solaris 8.
     */
    if(!strcmp(buffer, "pam_dhkeys") && !strcmp(phase, "password"))
        return FALSE;

    /* This allows pam_lsass to go after pam_unix or pam_unix2. This is
     * necessary so that pam_console, pam_foreground, or pam_resmgr gets run if
     * present.
     */
    if ((!strcmp(buffer, "pam_unix") || !strcmp(buffer, "pam_unix2")) &&
            !strcmp(phase, "session"))
    {
        if (distro->os == OS_HPUX)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    /* This module is listed as non-prompting because it does not prompt for
     * domain users, but it does deny domain users.
     */
    if (!strcmp(buffer, "pam_passwd_auth"))
    {
        return TRUE;
    }

    /* Assume that if it prompts for a password, it will complain about a
     * domain user
     */
    return PamModulePrompts("auth", module);
}

#if 0
static BOOLEAN PamModuleGivesLocalAccess(const char * phase, const char * module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));

    if(!strcmp(buffer, "pam_console") && !strcmp(phase, "session"))
        return TRUE;

    if(!strcmp(buffer, "pam_foreground") && !strcmp(phase, "session"))
        return TRUE;

    return FALSE;
}
#endif

/* returns true if the pam module at least sometimes returns success for AD users
 */
/*Currently unused
static BOOLEAN PamModuleGrantsDomainLogins( const char * phase, const char * module)
{
    if(PamModuleGrants(phase, module) && !PamModuleAlwaysDeniesDomainLogins(phase, module))
        return TRUE;

    return FALSE;
}
*/

static
DWORD
GetIncludeName(
    struct PamConf *conf,
    struct PamLine *lineObj,
    PSTR *ppIncludeService)
{
    DWORD ceError = 0;
    char buffer[256] = "";
    PCSTR pRawName = NULL;
    PSTR pIncludeService = NULL;

    if(lineObj->module != NULL)
        NormalizeModuleName( buffer, lineObj->module->value, sizeof(buffer));


    if(lineObj->phase != NULL &&
            !strcmp(lineObj->phase->value, "@include"))
    {
        pRawName = lineObj->control->value;
    }
    else if(lineObj->control != NULL &&
            !strcmp(lineObj->control->value, "include"))
    {
        pRawName = lineObj->module->value;
    }
    else if(lineObj->control != NULL &&
            !strcmp(lineObj->control->value, "substack"))
    {
        pRawName = lineObj->module->value;
    }
    else if(!strcmp(buffer, "pam_stack"))
    {
        int i;
        for(i = 0; i < lineObj->optionCount; i++)
        {
            if(CTStrStartsWith(lineObj->options[i].value, "service="))
            {
                pRawName = lineObj->options[i].value + strlen("service=");
                break;
            }
        }
    }
    if (pRawName == NULL &&
            !strcmp(buffer, "pam_per_user") && lineObj->defaultInclude)
    {
        pRawName = lineObj->defaultInclude;
    }

    if (pRawName != NULL)
    {
        ceError = CTStrdup(
                GetUnaliasedServiceName(conf, pRawName),
                &pIncludeService);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    *ppIncludeService = pIncludeService;

error:
    if (ceError)
    {
        CT_SAFE_FREE_STRING(pIncludeService);
        *ppIncludeService = NULL;
    }
    return ceError;
}

struct ConfigurePamModuleState
{
    /* Set to true if the requested module (lwidentity, lwipasspolicy, etc.) has either been disabled or enabled
     */
    BOOLEAN configuredRequestedModule;
    /* Set to true if a line has been passed that has "sufficient" as the control and a module that lets users in based off of a password. This will not be set to true for modules that use caller based authentication like pam_rootok.
     */
    BOOLEAN sawSufficientPromptingCheck;
    /* Set to true if a line has been passed that has a module that prompts for passwords (assuming that try_first_pass, etc. are not on the line), regardless of what the control is.
     */
    BOOLEAN sawPromptingModule;
    /* Set to true if a line has been passed with pam_pwcheck as its module.
     */
    BOOLEAN sawPwcheck;
    /* Set to true if a line has been passed that has "required" or "requisite" as its control and a module that checks an attribute of the caller without prompting for a password. pam_rhosts_auth is an example of such a module.
     */
    BOOLEAN sawCallerRequirementLine;
    /* Set to true if a line has been passed that has a module that atleast sometimes returns PAM_SUCCESS for domain users, regardless of what the control is onthat line.
     */
    BOOLEAN sawDomainUserGrantingLine;

    /* On Solaris, pam_lwidentity has to be called with 'set_default_repository' at the beginning of the auth stage in order for password changes to work. Later on pam_lwidentity has to also be installed without 'set_default_repository'.
     */
    BOOLEAN hasSetDefaultRepository;

    /* Set to true if a line is found that is not an include. This is used to
     * check if the pam stack is essentially empty for a service.
     */
    BOOLEAN sawNonincludeLine;

    /* Set to true if our smartcard prompt line has been added. */
    BOOLEAN hasAddedSmartCardPrompt;

    /* Whether a line like "required pam_deny.so" has been encountered */
    BOOLEAN sawRequiredDenyAll;

    /* Whether a line like "sufficient pam_rhosts.so" has been encountered */
    BOOLEAN sawCallerSufficientLine;

    /* Whether a line like "sufficient pam_passwd_auth.so.1 has been
     * encountered */
    BOOLEAN sawSufficientPasswordChange;

    int includeLevel;
};

static DWORD PamOldCenterisDisable(struct PamConf *conf, const char *service, const char * phase, struct ConfigurePamModuleState *state)
{
    DWORD ceError = ERROR_SUCCESS;
    int line = NextLineForService(conf, -1, service, phase);
    /* Do not free this variable */
    struct PamLine *lineObj;
    const char *module;
    const char *control;
    char *includeService = NULL;

    DJ_LOG_INFO("Disabling pam_centeris for pam service %s for phase %s", service, phase);

    while(line != -1)
    {
        lineObj = &conf->lines[line];
        GetModuleControl(lineObj, &module, &control);

        DJ_LOG_VERBOSE("Looking at entry %s %s %s %s", service, phase, control, module);

        CT_SAFE_FREE_STRING(includeService);
        ceError = GetIncludeName(
                        conf,
                        lineObj,
                        &includeService);
        BAIL_ON_CENTERIS_ERROR(ceError);
        if(includeService != NULL)
        {
            DJ_LOG_INFO("Including %s for service %s", includeService, service);
            ceError = PamOldCenterisDisable(conf, includeService, phase, state);
            if(ceError == LW_ERROR_PAM_MISSING_SERVICE)
                ceError = ERROR_SUCCESS;
            if(ceError == LW_ERROR_PAM_BAD_CONF)
                ceError = ERROR_SUCCESS;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if(PamModuleIsOldCenteris(phase, module))
        {
            DJ_LOG_INFO("Removing pam_centeris from service %s", service);
            BAIL_ON_CENTERIS_ERROR(ceError = RemoveLineAndUpdateSkips(conf, &line));
            state->configuredRequestedModule = TRUE;
            continue;
        }
        //Don't worry about try_first_pass. Our old configuration code didn't
        //get that right anyway.

        line = NextLineForService(conf, line, service, phase);
    }

error:
    CT_SAFE_FREE_STRING(includeService);
    return ceError;
}


void GetModuleControl(struct PamLine *lineObj, const char **module, const char **control)
{
    if(lineObj->module == NULL)
        *module = "";
    else
        *module = lineObj->module->value;

    if(lineObj->control == NULL)
        *control = "";
    else
        *control = lineObj->control->value;

    /* It is easier to treat "pam_lwidentity.so set_default_repository" as a
     * different module than pam_lwidentity.so because it acts completely
     * different.
     */
    if(lineObj->optionCount == 1 && !strcmp(lineObj->options[0].value, "set_default_repository"))
    {
        if(PamModuleIsLwidentity("auth", *module))
            *module = "pam_lwidentity_set_repo";
    }

    /* Ditto for "pam_lwidentity.so smartcard_prompt". */
    if((lineObj->optionCount == 1 || lineObj->optionCount == 2) && !strcmp(lineObj->options[0].value, "smartcard_prompt"))
    {
        if(PamModuleIsLwidentity("auth", *module))
            *module = "pam_lwidentity_smartcard_prompt";
    }
}

static DWORD PamLwidentityDisable(struct PamConf *conf, const char *service, const char * phase, const char *pam_lwidentity, struct ConfigurePamModuleState *state)
{
    DWORD ceError = ERROR_SUCCESS;
    int line;
    /* Do not free this variable */
    struct PamLine *lineObj;
    const char *module;
    const char *control;
    char *includeService = NULL;
    char *fromFileBackup = NULL;
    char *commentBackup = NULL;

    DJ_LOG_INFO("Disabling pam_lwidentity for pam service %s for phase %s", service, phase);

    DJ_LOG_INFO("Looking for lines commented out during the pam enable");
    for(line = 0; line < conf->lineCount; line++)
    {
        lineObj = &conf->lines[line];
        if(CTStrStartsWith(lineObj->comment, "#Commented out by lwidentity: "))
        {
            const char *original = lineObj->comment + strlen("#Commented out by lwidentity: ");
            DJ_LOG_INFO("Found line '%s' commented out by lwidentity", original);

            //Replace the lineObj object in place with its uncommented
            //version.
            CT_SAFE_FREE_STRING(fromFileBackup);
            CT_SAFE_FREE_STRING(commentBackup);
            fromFileBackup = lineObj->fromFile;
            lineObj->fromFile = NULL;
            commentBackup = lineObj->comment;
            lineObj->comment = NULL;
            FreePamLineContents(lineObj);

            BAIL_ON_CENTERIS_ERROR(ceError = ParsePamLine(lineObj, fromFileBackup, original, NULL));

            conf->modified = 1;
            continue;
        }
    }

    line = NextLineForService(conf, -1, service, phase);
    while(line != -1)
    {
        lineObj = &conf->lines[line];
        GetModuleControl(lineObj, &module, &control);

        DJ_LOG_VERBOSE("Looking at entry %s %s %s %s", service, phase, control, module);

        CT_SAFE_FREE_STRING(includeService);
        ceError = GetIncludeName(
                        conf,
                        lineObj,
                        &includeService);
        BAIL_ON_CENTERIS_ERROR(ceError);
        if(includeService != NULL)
        {
            DJ_LOG_INFO("Including %s for service %s", includeService, service);
            ceError = PamLwidentityDisable(conf, includeService, phase, pam_lwidentity, state);
            if(ceError == LW_ERROR_PAM_MISSING_SERVICE)
                ceError = ERROR_SUCCESS;
            if(ceError == LW_ERROR_PAM_BAD_CONF)
                ceError = ERROR_SUCCESS;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if(PamModuleIsLwidentity(phase, module) ||
                !strcmp(module, "pam_lwidentity_set_repo") ||
                !strcmp(module, "pam_lwidentity_smartcard_prompt"))
        {
            DJ_LOG_INFO("Removing pam_lwidentity from service %s", service);
            BAIL_ON_CENTERIS_ERROR(ceError = RemoveLineAndUpdateSkips(conf, &line));
            state->configuredRequestedModule = TRUE;
            continue;
        }

        if(PamModulePrompts(phase, module))
        {
            DJ_LOG_VERBOSE("The module prompts");
            if(state->configuredRequestedModule && !state->sawPromptingModule)
            {
                int found = 0;
                BAIL_ON_CENTERIS_ERROR(ceError = RemoveOption(conf, line, "try_first_pass", &found));
                if(found > 0)
                    DJ_LOG_INFO("Removed try_first_pass from module %s", module);

                BAIL_ON_CENTERIS_ERROR(ceError = RemoveOption(conf, line, "use_first_pass", &found));
                if(found > 0)
                    DJ_LOG_INFO("Removed use_first_pass from module %s", module);

                BAIL_ON_CENTERIS_ERROR(ceError = RemoveOption(conf, line, "use_authtok", &found));
                if(found > 0)
                    DJ_LOG_INFO("Removed use_authtok from module %s", module);

            }
            state->sawPromptingModule = TRUE;
        }

        line = NextLineForService(conf, line, service, phase);
    }

error:
    CT_SAFE_FREE_STRING(includeService);
    CT_SAFE_FREE_STRING(fromFileBackup);
    CT_SAFE_FREE_STRING(commentBackup);
    return ceError;
}

static DWORD SetPamTokenValue(CTParseToken **token, CTParseToken *prev, const char *value)
{
    DWORD ceError = ERROR_SUCCESS;

    if (value == NULL)
    {
        ceError = ERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if(prev != NULL && (prev->trailingSeparator == NULL ||
        strlen(prev->trailingSeparator) < 1))
    {
        CT_SAFE_FREE_STRING(prev->trailingSeparator);
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(" ", &prev->trailingSeparator));
    }
    if(*token == NULL)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken), (void **) (PVOID)token));
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup("", &(*token)->trailingSeparator));
    }
    else
        CT_SAFE_FREE_STRING((*token)->value);

    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(value, &(*token)->value));

error:
    return ceError;
}

static DWORD FindPamDenyLikeModule(const char *testPrefix, char **modulePath, char **moduleOption)
{
    DWORD ceError = ERROR_SUCCESS;
    LwDistroInfo distro;
    memset(&distro, 0, sizeof(distro));
    *modulePath = NULL;
    *moduleOption = NULL;

    ceError = FindModulePath(testPrefix, "pam_deny", modulePath, NULL);
    if(ceError == ERROR_MISSING_SYSTEMFILE)
    {
        ceError = FindModulePath(testPrefix, "pam_prohibit", modulePath, NULL);
    }
    if(ceError == ERROR_MISSING_SYSTEMFILE)
    {
        /*Solaris 8 does not have pam_deny. It does have pam_sample that has a
         * mode to act like pam_deny. pam_sample should be available on all
         * Solaris systems because it is provided by the same package that
         * provides pam_unix.
         *
         * Because pam_sample is a vague name, it will only be used on Solaris
         * systems.
         */
        ceError = DJGetDistroInfo(testPrefix, &distro);
        BAIL_ON_CENTERIS_ERROR(ceError);
        if(distro.os == OS_SUNOS)
        {
            ceError = FindModulePath(testPrefix, "pam_sample", modulePath, NULL);
            if(!ceError)
            {
                ceError = CTStrdup("always_fail", moduleOption);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
        else
        {
            ceError = ERROR_MISSING_SYSTEMFILE;
        }
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
error:
    if(ceError)
    {
        CT_SAFE_FREE_STRING(*modulePath);
        CT_SAFE_FREE_STRING(*moduleOption);
    }
    DJFreeDistroInfo(&distro);
    return ceError;
}

#if 0
static void MoveLine(struct PamConf *conf, int oldPos, int newPos)
{
    struct PamLine temp;

    //Make a copy of the line we're moving
    memcpy(&temp, &conf->lines[oldPos], sizeof(conf->lines[oldPos]));

    //Make room for the line in the new position
    if(newPos < oldPos)
    {
        //shift the area between newPos and oldPos forward
        memmove(&conf->lines[newPos + 1], &conf->lines[newPos],
                (char *)&conf->lines[oldPos] - (char *)&conf->lines[newPos]);
    }
    else
    {
        //shift the area between newPos and oldPos backward
        memmove(&conf->lines[oldPos], &conf->lines[oldPos + 1],
                (char *)&conf->lines[newPos] - (char *)&conf->lines[oldPos]);
    }

    //Put the line into its new position
    memcpy(&conf->lines[newPos], &temp, sizeof(conf->lines[newPos]));
    conf->modified = TRUE;
}
#endif

static void FixPromptingModule(struct PamConf *conf, const char *service, const char *phase, struct ConfigurePamModuleState *state, int line, LWException **exc)
{
    struct PamLine *lineObj;
    const char *module;
    const char *control;

    /* If pam_lwidentity is the first prompting module on the stack, the next module needs to have something like try_first_pass added.
     */
    DJ_LOG_VERBOSE("FixPromptingModule(%s, %s, %d)", service, phase, line);
    while(line != -1 && !state->sawPromptingModule)
    {
        lineObj = &conf->lines[line];
        GetModuleControl(lineObj, &module, &control);

        DJ_LOG_VERBOSE("Looking at entry %s %s %s %s", service, phase, control, module);
        if(PamModulePrompts(phase, module))
        {
            DJ_LOG_INFO("Making sure module %s uses the password stored by pam_lwidentity", module);
            if(!strcmp(phase, "auth"))
            {
                if(!ContainsOption(conf, line, "use_first_pass"))
                {
                    const char *optionName = "try_first_pass";
                    if(!PamModuleUnderstandsTryFirstPass(phase, module))
                        optionName = "use_first_pass";
                    LW_CLEANUP_CTERR(exc, AddOption(conf, line, optionName));
                }
            }
            /* Right now pam_lwidentity doesn't store the password for non-domain users, so we shouldn't do this for subsequent modules.
            else if(!strcmp(phase, "password"))
            {
                if(!ContainsOption(conf, line, "use_first_pass"))
                {
                    * Try the old and new password, if it has been prompted *
                    BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "try_first_pass"));
                }
                * Definitely use the new password *
                BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "use_authtok"));
            }
            */
            state->sawPromptingModule = TRUE;
        }
        line = NextLineForService(conf, line, service, phase);
    }
cleanup:
    DJ_LOG_VERBOSE("FixPromptingModule done: line now %d", line);
}

static
void
AddSmartCartLine(
    IN const LwDistroInfo *pDistro,
    IN OUT struct PamConf *pConf,
    IN const char *pPamLwidentity,
    IN OUT struct ConfigurePamModuleState *pState,
    IN int AddBeforeLine,
    OUT int *pLineMovedTo,
    OUT LWException **ppExc
    )
{
    struct PamLine *lineObj = NULL;

    LW_CLEANUP_CTERR(ppExc,
            CopyLineAndUpdateSkips(pConf, AddBeforeLine, pLineMovedTo));
    lineObj = &pConf->lines[AddBeforeLine];
    LW_CLEANUP_CTERR(ppExc,
            SetPamTokenValue(&lineObj->control, lineObj->phase, "requisite"));
    LW_CLEANUP_CTERR(ppExc,
            SetPamTokenValue(&lineObj->module, lineObj->control, pPamLwidentity));
    lineObj->optionCount = 0;
    LW_CLEANUP_CTERR(ppExc,
            AddOption(pConf, AddBeforeLine, "smartcard_prompt"));

    /*
     * RHEL4 doesn't clear the old password when retrying after a
     * failed password attempt, so we can't use try_first_pass
     * there.  On all other platforms this makes calling into
     * PAM with pre-populated username and password work properly.
     */
    if (pDistro->distro != DISTRO_RHEL || pDistro->version[0] > '4' ||
            pDistro->version[1] != '.')
    {
        LW_CLEANUP_CTERR(ppExc,
                AddOption(pConf, AddBeforeLine, "try_first_pass"));
    }

    pState->hasAddedSmartCardPrompt = TRUE;
    pState->sawPromptingModule = FALSE;
cleanup:
    ;
}

static void PamLwidentityEnable(const char *testPrefix, const LwDistroInfo *distro, struct PamConf *conf, const char *service, const char * phase, const char *pam_lwidentity, struct ConfigurePamModuleState *state, LWException **exc)
{
    int prevLine = -1;
    int line = NextLineForService(conf, -1, service, phase);
    int lwidentityLine = -1;
    int secondPasswd = -1;
    /* Do not free this variable */
    struct PamLine *lineObj;
    const char *module;
    const char *control;
    char *includeService = NULL;
    char *pam_deny_path = NULL;
    char *pam_deny_option = NULL;
    StringBuffer comment;
    BOOLEAN doingPasswdForAIX = FALSE;
    LWException *nestedException = NULL;
    PSTR newMessage = NULL;
    DWORD ceError = 0;

    DJ_LOG_INFO("Enabling pam_lwidentity for pam service %s for phase %s", service, phase);
    memset(&comment, 0, sizeof(comment));

    if(line == -1)
    {
        /* This means that this service is not defined for this phase. An
         * example is the session phase of passwd.  This function will return
         * an error, and the parent function should decide whether or not to
         * ignore the error.
         */
        if (!strcmp(service, "xrdp-sesman") &&
            !strcmp(phase, "session") &&
            (line = NextLineForService(conf, -1, service, "account")) != -1)
        {
            // This is for remote desktop logins. There is a bug on SLED 11
            // Nomad (an implementation of xrdp) where it does not contain a
            // session phase. In this case, pam_lsass still needs to be added
            // for home directory creation.

            DJ_LOG_INFO("Inserting pam_lwidentity for xrdp missing session");
            LW_CLEANUP_CTERR(exc, CopyLineAndUpdateSkips(conf, line, &lwidentityLine));

            /* Fill in the correct values for lwidentityLine */
            lineObj = &conf->lines[lwidentityLine];
            /* This is incase the phase is @include */
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->phase, lineObj->service, phase));
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->control, lineObj->phase, "sufficient"));
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->module, lineObj->control, pam_lwidentity));
            lineObj->optionCount = 0;
            goto cleanup;
        }
        LW_CLEANUP_CTERR(exc, LW_ERROR_PAM_MISSING_SERVICE);
    }

    /* Pam enable algorithm:

     Find the first pam line that would block our module from accepting
     logins, and insert our module in front of it. Along the way, note
     if any password prompting modules were passed.  (e.g., required
     pam_allowroot, sufficient pam_permit)

     If the blocking line blocks all logins (including local), do not
     add lwidentity to this service.  (For example, if the service only
     had pam_deny.  Note that a sufficent line above would still allow
     users in.)

     If there are no lines which block domain logins, add our module to
     the end of the stack.  If this is the auth phase, throw an error
     instead.

     For the auth and password phases, fix up the try_first_pass options
     afterwards.

    */
    while(line != -1)
    {
        lineObj = &conf->lines[line];
        GetModuleControl(lineObj, &module, &control);

        DJ_LOG_VERBOSE("Looking at entry %s %s %s %s", service, phase, control, module);

        CT_SAFE_FREE_STRING(includeService);
        ceError = GetIncludeName(
                        conf,
                        lineObj,
                        &includeService);
        LW_CLEANUP_CTERR(exc, ceError);
        if(includeService != NULL)
        {
            DJ_LOG_INFO("Including %s for service %s", includeService, service);
            state->includeLevel++;
            PamLwidentityEnable(testPrefix, distro, conf, includeService, phase, pam_lwidentity, state, &nestedException);
            state->includeLevel--;
            if(LW_IS_OK(nestedException))
            {
                LW_HANDLE(&nestedException);
                if(state->configuredRequestedModule)
                {
                    prevLine = line;
                    line = NextLineForService(conf, line, service, phase);
                    break;
                }
            }
            else if(nestedException->code == LW_ERROR_PAM_MISSING_SERVICE)
            {
                LW_HANDLE(&nestedException);
            }
            else
            {
                CTAllocateStringPrintf(&newMessage, "Encountered while processing %s service:\n%s", service, nestedException->longMsg);
                CT_SAFE_FREE_STRING(nestedException->longMsg);
                nestedException->longMsg = newMessage;
                newMessage = NULL;
            }
            LW_CLEANUP(exc, nestedException);
        }

        if(PamModuleRemoveOnEnable(phase, control, module))
        {
            char *fromFileBackup;
            char *leadingWhiteSpaceBackup;

            DJ_LOG_INFO("Commenting out '%s' module", module);
            CTStringBufferClear(&comment);
            LW_CLEANUP_CTERR(exc, CTStringBufferAppend(&comment, "#Commented out by lwidentity: "));
            LW_CLEANUP_CTERR(exc, AppendPamLine(&comment, lineObj));

            fromFileBackup = lineObj->fromFile;
            lineObj->fromFile = NULL;
            leadingWhiteSpaceBackup = lineObj->leadingWhiteSpace;
            lineObj->leadingWhiteSpace = NULL;
            FreePamLineContents(lineObj);
            lineObj->fromFile = fromFileBackup;
            lineObj->leadingWhiteSpace = leadingWhiteSpaceBackup;
            lineObj->comment = CTStringBufferFreeze(&comment);

            prevLine = line;
            line = NextLineForService(conf, line, service, phase);
            continue;
        }

        if( !strcmp(module, "pam_passwd_auth.so.1") &&
                (!strcmp(control, "required") ||
                !strcmp(control, "requisite")))
        {
            /*Solaris's password authentication module must be first on the stack, but it blocks our module. So we'll just rework their config a little bit.
             *
             * This changes:
             * auth required pam_passwd_auth.so.1
             * to:
             * auth sufficient pam_passwd_auth.so.1
             * auth required pam_deny.so.1
             */
            int newLine = -1;

            if(pam_deny_path == NULL)
            {
                LW_CLEANUP_CTERR(exc, FindPamDenyLikeModule(testPrefix, &pam_deny_path, &pam_deny_option));
            }

            LW_CLEANUP_CTERR(exc, CopyLineAndUpdateSkips(conf, line, &newLine));
            lineObj = &conf->lines[newLine];
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->module, lineObj->control, pam_deny_path));
            lineObj->optionCount = 0;
            if(pam_deny_option != NULL)
                LW_CLEANUP_CTERR(exc, AddOption(conf, newLine, pam_deny_option));

            lineObj = &conf->lines[line];
            CT_SAFE_FREE_STRING(lineObj->control->value);
            LW_CLEANUP_CTERR(exc, CTStrdup("sufficient", &lineObj->control->value));
            GetModuleControl(lineObj, &module, &control);
        }

        if(!strcmp(module, "pam_lwidentity_set_repo"))
            state->hasSetDefaultRepository = TRUE;

        if (distro->os == OS_SUNOS && !state->hasSetDefaultRepository &&
                !strcmp(phase, "auth") && (PamModulePrompts(phase, module) ||
                    !strcmp(module, "pam_passwd_auth.so.1")))
        {
            int newLine = -1;
            LW_CLEANUP_CTERR(exc, CopyLineAndUpdateSkips(conf, line, &newLine));
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->control, lineObj->phase, "requisite"));
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->module, lineObj->control, pam_lwidentity));
            lineObj->optionCount = 0;
            LW_CLEANUP_CTERR(exc, AddOption(conf, line, "set_default_repository"));

            line = newLine;
            lineObj = &conf->lines[newLine];
            GetModuleControl(lineObj, &module, &control);

            state->hasSetDefaultRepository = TRUE;
        }

        if (!strcmp(control, "sufficient") &&
            !strcmp(module, "pam_passwd_auth.so.1"))
        {
            state->sawSufficientPasswordChange = TRUE;
        }

        if(!strcmp(module, "pam_lwidentity_smartcard_prompt"))
            state->hasAddedSmartCardPrompt = TRUE;

        if(!state->hasAddedSmartCardPrompt && !strcmp(phase, "auth") &&
                PamModulePrompts(phase, module))
        {
            LW_TRY(exc, AddSmartCartLine(
                distro,
                conf,
                pam_lwidentity,
                state,
                line,
                &line,
                &LW_EXC));
            lineObj = &conf->lines[line];
            GetModuleControl(lineObj, &module, &control);
            LW_TRY(exc, FixPromptingModule(conf, service, phase, state, line, &LW_EXC));
        }

        if(PamModuleAlwaysDeniesDomainLogins(phase, module, distro) && (
                    !strcmp(control, "required") ||
                    !strcmp(control, "requisite")))
            break;

        if(PamModuleIsLwidentity(phase, module))
        {
            DJ_LOG_INFO("Found pam_lwidentity");
            /* Fix up the options incase this module was installed by a previous version of the product */
            if(state->sawPromptingModule)
            {
                if(!strcmp(phase, "auth") && !ContainsOption(conf, line, "try_first"))
                {
                    DJ_LOG_INFO("Fixing up pam_lwidentity line which must have come from a previous install");
                    /* Use the previously prompted current password */
                    LW_CLEANUP_CTERR(exc, AddOption(conf, line, "try_first_pass"));
                }
                else if(!strcmp(phase, "password") && (!ContainsOption(conf, line, "try_first_pass") || !ContainsOption(conf, line, "use_authtok")))
                {
                    DJ_LOG_INFO("Fixing up pam_lwidentity line which must have come from a previous install");
                    /* Try the old and new password, if it has been prompted */
                    LW_CLEANUP_CTERR(exc, AddOption(conf, line, "try_first_pass"));
                    /* Definitely use the new password */
                    LW_CLEANUP_CTERR(exc, AddOption(conf, line, "use_authtok"));
                }
            }
            state->configuredRequestedModule = TRUE;
            prevLine = line;
            line = NextLineForService(conf, line, service, phase);
            break;
        }

        if (!strcmp(control, "sufficient") &&
                PamModuleGrants(phase, module) &&
                !PamModulePrompts(phase, module) &&
                !PamModuleChecksCaller(phase, module))
        {
            /* This module seems to let anyone through */
            break;
        }

        if(includeService == NULL)
        {
            DJ_LOG_VERBOSE("It is not an include line");
            state->sawNonincludeLine = TRUE;
        }

        if(PamModulePrompts(phase, module))
        {
            DJ_LOG_VERBOSE("The module prompts");
            state->sawPromptingModule = TRUE;
        }

        /* Ubuntu 8.10 has this configuration:
         * auth [success=1 default=ignore] pam_unix.so nullok_secure
         * auth requisite pam_deny.so
         *
         * As a workaround for the above configuration, 'success=1' is treated
         * as sufficient, although this does not hold true in general.
         *
         * An updated version of Ubuntu 8.10 has this configuration:
         * account [success=1 new_authtok_reqd=done default=ignore]        pam_unix.so
         * account requisite                       pam_deny.so
         */
        if ((!strcmp(control, "sufficient") ||
                !strncmp(control, "[success=1 ", sizeof("[success=1 ") - 1)) &&
                PamModuleGrants(phase, module) &&
                PamModulePrompts(phase, module))
        {
            state->sawSufficientPromptingCheck = TRUE;
        }

        if( (!strcmp(control, "required") ||
                    !strcmp(control, "requisite")) &&
                PamModuleChecksCaller(phase, module))
        {
            state->sawCallerRequirementLine = TRUE;
        }

        if (!strcmp(control, "sufficient") &&
            PamModuleChecksCaller(phase, module))
        {
            state->sawCallerSufficientLine = TRUE;
        }

        if (!PamModuleAlwaysDeniesDomainLogins(phase, module, distro))
        {
            state->sawDomainUserGrantingLine = TRUE;
        }

        prevLine = line;
        line = NextLineForService(conf, line, service, phase);
    }

    if (line == -1)
    {
        if (state->includeLevel == 0)
        {
            DJ_LOG_INFO("Bottomed out of pam stack");
        }
        else
        {
            goto cleanup;
        }
    }

    if(!state->configuredRequestedModule)
    {
        if(!strcmp(phase, "auth") &&
                line == -1 &&
                !state->sawDomainUserGrantingLine &&
                state->sawNonincludeLine)
        {
            /* This tries to fix the user's pam configuration. The user has
             * something like:
             * auth sufficient pam_rootok.so
             *
             * Without a required line after it. This is a risky way to set
             * a pam configuration, but it works. We'll add this to the end of
             * the stack so the rest of our logic works:
             * auth required pam_deny.so
             *
             * Here are the services this is known to affect:
             * - groupadd, groupmod, groupdel, newusers, useradd,
             *   usermod, and userdel - on Ubuntu
             * - chage on Ubuntu
             * - quagga on Centos
             */
            LW_CLEANUP_CTERR(exc, CopyLineAndUpdateSkips(conf, prevLine, &line));
            lineObj = &conf->lines[line];
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->phase, lineObj->service, "auth"));
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->control, lineObj->phase, "required"));

            if(pam_deny_path == NULL)
            {
                LW_CLEANUP_CTERR(exc, FindPamDenyLikeModule(testPrefix, &pam_deny_path, &pam_deny_option));
            }
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->module, lineObj->control, pam_deny_path));
            lineObj->optionCount = 0;
            if(pam_deny_option != NULL)
                LW_CLEANUP_CTERR(exc, AddOption(conf, line, pam_deny_option));

            module = lineObj->module->value;
            control = lineObj->control->value;
        }
        if(!strcmp(phase, "auth") &&
                (line == -1 ||
                (!strcmp(control, "sufficient") &&
                PamModuleGrants(phase, module) &&
                !PamModulePrompts(phase, module) &&
                !PamModuleChecksCaller(phase, module))))
        {
            char normalizedService[256];

            if (state->sawCallerRequirementLine ||
                (state->sawRequiredDenyAll && state->sawCallerSufficientLine))
            {
                /* This service is protected by something other than a password
                 */
                goto cleanup;
            }

            /* Nothing checks the password for local users?
             */

            /* There are a few systems that are known to be broken. They
             * are listed here:
             *
             * - gdm-autologin doesn't prompt for a password
             * - passwd - doesn't use the auth phase on Ubuntu
             * - rsh - I can't figure out how rsh on Solaris works. It
             *   must just use rlogin
             * - shadow - Suse has something unprotected called
             *    /etc/pam.d/shadow. I can't figure out what uses it.
             * - runuser/runlevel-l - Requires root-ness to do its job.  So it only
             *   checks for that.  (Found on CentOS 5.)
             * - ekshell - this is a kerberized rsh. It first authenticates the
             *     user outside of pam.
             * - kshell - this is a kerberized rsh. It first authenticates the
             *     user outside of pam.
             *
             * kde-np is the passwordless login. It is the kde equivalent of
             * gdm-autologin.
             * xdm-np is the passwordless login. It is the xdm equivalent of
             * gdm-autologin.
             * - useradd on Suse 10.2 has sufficient pam_rootok followed by
             *   required pam_permit.so. The program is not setuid root though
             *
             * - gnome-screensaver-smartcard on Suse 10.3 includes
             *   common-auth-smartcard. If that file does not exist on the
             *   system, then gnome-screensaver-smartcard does not have any
             *   auth modules.
             */
            strncpy(normalizedService, service, sizeof(normalizedService));
            normalizedService[sizeof(normalizedService) - 1] = '\0';
            if(CTStrEndsWith(normalizedService, ".rpmnew"))
            {
                normalizedService[
                    strlen(normalizedService) - strlen(".rpmnew")] = '\0';
            }
            if(CTStrEndsWith(normalizedService, ".dpkg-new"))
            {
                normalizedService[
                    strlen(normalizedService) - strlen(".dpkg-new")] = '\0';
            }
            if(!strcmp(normalizedService, "gdm-autologin"))
                goto cleanup;
            if(!strcmp(normalizedService, "gdm-smartcard"))
                goto cleanup;
            if(!strcmp(normalizedService, "gdm-launch-environment"))
                goto cleanup;
            // Equivalent to gdm-autologin
            if(!strcmp(normalizedService, "gdm-welcome"))
                goto cleanup;
            if(!strcmp(normalizedService, "passwd"))
                goto cleanup;
            if(!strcmp(normalizedService, "chpasswd"))
                goto cleanup;
            if(!strcmp(normalizedService, "newusers"))
                goto cleanup;
            if(!strcmp(normalizedService, "rsh"))
                goto cleanup;
            if(!strcmp(normalizedService, "rshd"))
                goto cleanup;
            if(!strcmp(normalizedService, "shadow"))
                goto cleanup;
            if(!strcmp(normalizedService, "runuser"))
                goto cleanup;
            if(!strcmp(normalizedService, "runuser-l"))
                goto cleanup;
            if(!strcmp(normalizedService, "ekshell"))
                goto cleanup;
            if(!strcmp(normalizedService, "kshell"))
                goto cleanup;
            /* I'm not sure if this is a typo or not */
            if(!strcmp(normalizedService, "kde-np"))
                goto cleanup;
            /* Centos 5 has kdm-np as part of an optional package */
            if(!strcmp(normalizedService, "kdm-np"))
                goto cleanup;
            if(!strcmp(normalizedService, "xdm-np"))
                goto cleanup;
            if(!strcmp(normalizedService, "useradd"))
                goto cleanup;
            if(!strcmp(normalizedService, "gnome-screensaver-smartcard"))
                goto cleanup;
            /* The 5.4 update of RHEL 5 has "auth sufficient pam_env.so".  This
             * lets all users through, but it is only called by the cron daemon
             * when running jobs for the user. Only the setcred phase of auth
             * is called, so crond is unable to check passwords anyway.

             * The cron section is not used to determine who can edit their
             * crontab.
             */
            if (!strcmp(normalizedService, "crond"))
                goto cleanup;
            if (!strcmp(normalizedService, "utadmingui"))
            {
                goto cleanup;
            }

            DJ_LOG_ERROR("Nothing seems to be protecting logins for service %s", service);
            if(!strcmp(control, "sufficient"))
            {
                LW_RAISE_EX(exc, LW_ERROR_PAM_BAD_CONF, "Unknown PAM module", "The PowerBroker Identity Services PAM module cannot be configured for the %s service. This service uses the '%s' module, which is not in this program's list of known modules. Please email PowerBroker Identity Services technical support and include a copy of /etc/pam.conf or /etc/pam.d.", service, module);
            }
            //It is somewhat normal to not require a password in an included
            //pam file. It is up to the top-most parent to require the password.
            else if(state->includeLevel == 0)
            {
                LW_RAISE_EX(exc, LW_ERROR_PAM_BAD_CONF, "Unknown PAM configuration", "The PowerBroker Identity Services PAM module cannot be configured for the %s service.  Either this service is unprotected (does not require a valid password for access), or it is using a PAM module that this program is unfamiliar with.  Please email PowerBroker Identity Services technical support and include a copy of /etc/pam.conf or /etc/pam.d.", service);
            }
            goto cleanup;
        }

        if(line != -1)
        {
            char buffer[256] = "";
            NormalizeModuleName( buffer, module, sizeof(buffer));

            if( (!state->sawSufficientPromptingCheck || !strcmp(service, "runuser")) &&
                    !state->sawSufficientPasswordChange &&
                    (!PamModuleGrants(phase, module) || PamModuleChecksCaller(phase, module)) &&
                    (!strcmp(control, "required") ||
                    !strcmp(control, "requisite")))
            {
                /* I guess the user wants to block everyone from logging in, or
                 * authenticate users with something other than a password.  In
                 * this case, we won't let domain users in either, unless it is
                 * the password phase on OS X. OS X has an OS bug where it
                 * blocks password changes during login for all services except
                 * passwd and interactive logins.
                 */
                if( !(distro->os == OS_DARWIN && !strcmp(phase, "password")) )
                {
                    state->sawRequiredDenyAll = TRUE;
                    goto cleanup;
                }
            }
            /*Insert our module before the line*/
            DJ_LOG_INFO("Inserting pam_lwidentity before %s", module);
            if(!strcmp(phase, "password") && !strcmp(buffer, "pam_aix"))
                doingPasswdForAIX = TRUE;
            LW_CLEANUP_CTERR(exc, CopyLineAndUpdateSkips(conf, line, NULL));
            lwidentityLine = line;
        }
        else
        {
            if (!state->sawNonincludeLine)
            {
                LW_CLEANUP_CTERR(exc, LW_ERROR_PAM_MISSING_SERVICE);
            }
            /*There was nothing to block domain users. We'll add our module at the bottom of the stack.
             * prevLine != -1, because a noninclude line was seen.
             *
             * Since this is not an auth phase, our module is still added because we should do things like support password changes, make home directories, etc..
             */
            DJ_LOG_INFO("Inserting pam_lwidentity at the end of the pam stack");
            LW_CLEANUP_CTERR(exc, CopyLineAndUpdateSkips(conf, prevLine, &lwidentityLine));
        }

        if (!strcmp(phase, "auth") && !state->hasAddedSmartCardPrompt)
        {
            LW_TRY(exc, AddSmartCartLine(
                distro,
                conf,
                pam_lwidentity,
                state,
                lwidentityLine,
                &lwidentityLine,
                &LW_EXC));
            state->sawPromptingModule = TRUE;
        }

        /* Fill in the correct values for lwidentityLine */
        lineObj = &conf->lines[lwidentityLine];
        /* This is incase the phase is @include */
        LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->phase, lineObj->service, phase));
        LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->control, lineObj->phase, "sufficient"));
        LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->module, lineObj->control, pam_lwidentity));
        lineObj->optionCount = 0;

        if(!strcmp(phase, "account"))
        {
            /* Group membership checks, aka "allow local logins" is
             * normally enforced during the auth phase. However, logins
             * using ssh keys bypass entering a password, and the whole
             * auth phase. So pam_lwidentity can't block users with bad
             * group membership in that case.
            *
            * The account phase will still be called. Group membership
            * will be enforced in the account phase as well as the auth
            * phase. The following lines will allow local users to
            * continue down the stack (first line succeeds and second
            * fails).
            *
            * In the case of domain users, the first line will block
            * domain users who don't meet group membership
            * requirements. The second line will allow the rest of the
            * domain users into the system.
            *
            * account required pam_lwidentity unknown_ok
            * account sufficient pam_lwidentity
            */
            LW_CLEANUP_CTERR(exc, CopyLineAndUpdateSkips(conf, lwidentityLine, NULL));
            lineObj = &conf->lines[lwidentityLine];
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->control, lineObj->phase, "required"));
            LW_CLEANUP_CTERR(exc, AddOption(conf, lwidentityLine, "unknown_ok"));
        }
        else if(state->sawPromptingModule)
        {
            DJ_LOG_INFO("Telling pam_lwidentity to use the previously prompted password");
            if(!strcmp(phase, "auth"))
            {
                /* Use the previously prompted current password */
                LW_CLEANUP_CTERR(exc, AddOption(conf, lwidentityLine, "try_first_pass"));
            }
            else if(!strcmp(phase, "password"))
            {
                /* Try the old and new password, if it has been prompted */
                LW_CLEANUP_CTERR(exc, AddOption(conf, lwidentityLine, "try_first_pass"));
                /* Definitely use the new password */
                LW_CLEANUP_CTERR(exc, AddOption(conf, lwidentityLine, "use_authtok"));
            }
        }
        if(doingPasswdForAIX)
        {
            /* On AIX, first PAM based modules will have a chance to change
             * the password, then LAM based modules will have a chance. The
             * problem is that if the pam_lwidentity module is sufficient,
             * then we will first prompt during the PAM check. If the PAM check
             * fails (because they didn't enter the right password or
             * something), then the password will be reprompted for LAM. We
             * don't have much control over the LAM prompts, so they look bad.
             * We would like to avoid having the LAM prompts displayed.
             *
             * To do this correctly, we need a way of:
             * 1. exiting the PAM stack successfully if the password is
             * correctly changed for a domain
             * 2. exiting the PAM stack with an error if something went wrong
             * for a domain user.
             * 3. continuing down the PAM stack if the user is not a domain
             * user.
             *
             * This configuration will accomplish all of those tasks:
             * passwd  password  requisite     pam_lwidentity.so unknown_ok remember_chpass
             * passwd  password  sufficient    pam_lwidentity.so
             * passwd  password  required      /usr/lib/security/pam_aix
             */
            LW_CLEANUP_CTERR(exc, CopyLineAndUpdateSkips(conf, lwidentityLine, &secondPasswd));
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->control, lineObj->phase, "requisite"));
            LW_CLEANUP_CTERR(exc, AddOption(conf, lwidentityLine, "unknown_ok"));
            LW_CLEANUP_CTERR(exc, AddOption(conf, lwidentityLine, "remember_chpass"));
            LW_CLEANUP_CTERR(exc, AddOption(conf, secondPasswd, "use_first_pass"));
            lineObj = &conf->lines[lwidentityLine];
        }

        /* I don't think this code is necessary
        BOOLEAN understandsNewAuthtokReqd;
#ifdef __LWI_SOLARIS__
        understandsNewAuthtokReqd = FALSE;
#else
        understandsNewAuthtokReqd = TRUE;
#endif

        if(!strcmp(phase, "auth") && !understandsNewAuthtokReqd)
        {
            * This is for machines which do not correctly handle the PAM_NEW_AUTHTOK_REQD return code when the module is marked as sufficient. For these systems, we have to have our module run as required underneath the sufficient line.
             *
            BAIL_ON_CENTERIS_ERROR(ceError = CopyLine(conf, lwidentityLine, &lwidentityLine));
            lineObj = &conf->lines[lwidentityLine];
            CT_SAFE_FREE_STRING(lineObj->control->value);
            BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup("required", &lineObj->control->value));
            BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, lwidentityLine, "unknown_ok"));
            * This will only add the option if it hasn't been added so far *
            BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, lwidentityLine, "use_first_pass"));
            state->sawPromptingModule = TRUE;
        }*/
        line = NextLineForService(conf, lwidentityLine, service, phase);
        state->configuredRequestedModule = TRUE;
    }

    if (state->configuredRequestedModule)
        LW_TRY(exc, FixPromptingModule(conf, service, phase, state, line, &LW_EXC));
cleanup:
    CTStringBufferDestroy(&comment);
    CT_SAFE_FREE_STRING(includeService);
    CT_SAFE_FREE_STRING(pam_deny_path);
    CT_SAFE_FREE_STRING(pam_deny_option);
    CT_SAFE_FREE_STRING(newMessage);
    LW_HANDLE(&nestedException);
}

static DWORD PamLwiPassPolicyDisable(struct PamConf *conf, const char *service, const char * phase, const char *pam_lwipasspolicy, struct ConfigurePamModuleState *state)
{
    DWORD ceError = ERROR_SUCCESS;
    int line = NextLineForService(conf, -1, service, phase);
    /* Do not free this variable */
    struct PamLine *lineObj;
    const char *module;
    const char *control;
    char *includeService = NULL;

    if(strcmp(phase, "password"))
    {
        /*Only install the password policy for the password phase*/
        ceError = ERROR_SUCCESS;
        goto error;
    }

    DJ_LOG_INFO("Disabling password policy for pam service %s for phase %s", service, phase);

    while(line != -1)
    {
        lineObj = &conf->lines[line];
        GetModuleControl(lineObj, &module, &control);

        DJ_LOG_VERBOSE("Looking at entry %s %s %s %s", service, phase, control, module);

        CT_SAFE_FREE_STRING(includeService);
        ceError = GetIncludeName(
                        conf,
                        lineObj,
                        &includeService);
        BAIL_ON_CENTERIS_ERROR(ceError);
        if(includeService != NULL)
        {
            DJ_LOG_INFO("Including %s for service %s", includeService, service);
            ceError = PamLwiPassPolicyDisable(conf, includeService, phase, pam_lwipasspolicy, state);
            if(ceError == LW_ERROR_PAM_MISSING_SERVICE)
                ceError = ERROR_SUCCESS;
            if(ceError == LW_ERROR_PAM_BAD_CONF)
                ceError = ERROR_SUCCESS;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if(PamModuleIsLwiPassPolicy(phase, module))
        {
            DJ_LOG_INFO("Removing pam_lwipasspolicy from service %s", service);
            BAIL_ON_CENTERIS_ERROR(ceError = RemoveLineAndUpdateSkips(conf, &line));
            state->configuredRequestedModule = TRUE;
            continue;
        }

        if(PamModulePrompts(phase, module))
        {
            DJ_LOG_VERBOSE("The module prompts");
            if(state->configuredRequestedModule && !state->sawPromptingModule)
            {
                int found = 0;
                BAIL_ON_CENTERIS_ERROR(ceError = RemoveOption(conf, line, "try_first_pass", &found));
                if(found > 0)
                    DJ_LOG_INFO("Removed try_first_pass from module %s", module);

                BAIL_ON_CENTERIS_ERROR(ceError = RemoveOption(conf, line, "use_first_pass", &found));
                if(found > 0)
                    DJ_LOG_INFO("Removed use_first_pass from module %s", module);

                BAIL_ON_CENTERIS_ERROR(ceError = RemoveOption(conf, line, "use_authtok", &found));
                if(found > 0)
                    DJ_LOG_INFO("Removed use_authtok from module %s", module);

            }
            state->sawPromptingModule = TRUE;
        }

        line = NextLineForService(conf, line, service, phase);
    }

error:
    CT_SAFE_FREE_STRING(includeService);
    return ceError;
}

static DWORD PamLwiPassPolicyEnable(struct PamConf *conf, const char *service, const char * phase, const char *pam_lwipasspolicy, struct ConfigurePamModuleState *state)
{
    DWORD ceError = ERROR_SUCCESS;
    int line = NextLineForService(conf, -1, service, phase);
    /* Do not free this variable */
    struct PamLine *lineObj;
    const char *module;
    const char *control;
    char *includeService = NULL;

    if(strcmp(phase, "password"))
    {
        /*Only install the password policy for the password phase*/
        ceError = ERROR_SUCCESS;
        goto error;
    }

    DJ_LOG_INFO("Enabling password policy for pam service %s for phase %s", service, phase);

    if(line == -1)
    {
        /* This means that this service is not defined for this phase.
         * This function will return an error, and the parent function should decide whether or not to ignore the error.
         */
        BAIL_ON_CENTERIS_ERROR(ceError = LW_ERROR_PAM_MISSING_SERVICE);
    }

    /* Insert the password policy module before the first module that prompts and isn't pam_lwidentity. Add use_authtok to the prompting module if necessary. */
    while(line != -1)
    {
        lineObj = &conf->lines[line];
        GetModuleControl(lineObj, &module, &control);

        DJ_LOG_VERBOSE("Looking at entry %s %s %s %s", service, phase, control, module);

        CT_SAFE_FREE_STRING(includeService);
        ceError = GetIncludeName(
                        conf,
                        lineObj,
                        &includeService);
        BAIL_ON_CENTERIS_ERROR(ceError);
        if(includeService != NULL)
        {
            DJ_LOG_INFO("Including %s for service %s", includeService, service);
            ceError = PamLwiPassPolicyEnable(conf, includeService, phase, pam_lwipasspolicy, state);
            if(ceError == LW_ERROR_PAM_MISSING_SERVICE)
                ceError = ERROR_SUCCESS;
            if(ceError == LW_ERROR_PAM_BAD_CONF)
                ceError = ERROR_SUCCESS;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if(PamModuleIsLwiPassPolicy(phase, module))
        {
            DJ_LOG_INFO("Found pam_lwipasspolicy");
            state->configuredRequestedModule = TRUE;
        }

        if(includeService == NULL)
        {
            DJ_LOG_VERBOSE("It is not an include line");
            state->sawNonincludeLine = TRUE;
        }

        if(PamModulePrompts(phase, module))
        {
            DJ_LOG_VERBOSE("The module prompts");
            /* On Suse, we rely on pam_pwcheck to prompt for the old password,
             * so our module must come after it.
             */
            if(!state->configuredRequestedModule && !PamModuleIsLwidentity(phase, module) && !PamModuleIsPwcheck(phase, module))
            {
                DJ_LOG_INFO("Inserting %s before %s", pam_lwipasspolicy, module);
                BAIL_ON_CENTERIS_ERROR(ceError = CopyLineAndUpdateSkips(conf, line, NULL));
                /* Fill in the correct values for the password policy */
                lineObj = &conf->lines[line];

                BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(pam_lwipasspolicy, &lineObj->module->value));
                BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup("required", &lineObj->control->value));
                lineObj->optionCount = 0;

                if(state->sawPromptingModule)
                {
                    if(state->sawPwcheck)
                    {
                        /* Always use the old and new password */
                        BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "use_first_pass"));
                    }
                    else
                    {
                        /* Try the old and new password, if it has been prompted */
                        BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "try_first_pass"));
                    }
                    /* Definitely use the new password */
                    BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "use_authtok"));
                }
                state->configuredRequestedModule = TRUE;
            }
            else
            {
                if(!state->sawPromptingModule && state->configuredRequestedModule && !PamModuleIsLwiPassPolicy(phase, module))
                {
                    if(!ContainsOption(conf, line, "use_first_pass"))
                    {
                        /* Try the old and new password, if it has been prompted */
                        BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "try_first_pass"));
                    }
                    /* Definitely use the new password */
                    BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "use_authtok"));
                }
                state->sawPromptingModule = TRUE;
                if(PamModuleIsPwcheck(phase, module))
                    state->sawPwcheck = TRUE;
            }
        }

        line = NextLineForService(conf, line, service, phase);
    }

    if(!state->configuredRequestedModule && !state->sawNonincludeLine)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = LW_ERROR_PAM_MISSING_SERVICE);
    }

error:
    CT_SAFE_FREE_STRING(includeService);
    return ceError;
}

#define MODULE_FLAG_32BIT   1
#define MODULE_FLAG_64BIT   2

static DWORD FindModulePath(const char *testPrefix, const char *basename, char **destName, DWORD *moduleFlags)
{
    /*If you update this function, update NormalizeModuleName too */
    DWORD ceError = ERROR_SUCCESS;
    int i, j, k;
    int foundPath = -1;
    char *fullPath = NULL;
    BOOLEAN exists;
    const char *searchDirs[] = {
        "/lib/security",
        "/lib64/security",
        "/usr/lib/security",
        "/usr/lib64/security",
        "/lib/security/hpux32",
        "/lib/security/hpux64",
        "/usr/lib/pam",
        "/usr/lib/security/sparcv9",
        "/usr/lib/security/amd64",
        "/usr/local/lib",
        NULL
    };

    const char *searchNamePrefixes[] = {
        "",
        "lib",
        NULL
    };

    const char *searchNameSuffixes[] = {
        ".so",
        ".so.1",
        ".so.2",
        ".sl",
        ".sl.1",
        ".1",
        NULL
    };

    char *foundName = NULL;
    PSTR fileProgPath = NULL;
    PSTR fileType = NULL;

    if(moduleFlags != NULL)
    {
        *moduleFlags = 0;
        ceError = CTFindFileInPath("file", "/bin:/usr/bin:/usr/local/bin", &fileProgPath);
        if(ceError == ERROR_FILE_NOT_FOUND)
            ceError = ERROR_SUCCESS;
        GCE(ceError);
    }

    DJ_LOG_INFO("Searching for the system specific path of %s", basename);

    for(i = 0; searchDirs[i] != NULL; i++)
    {
        for(j = 0; searchNamePrefixes[j] != NULL; j++)
        {
            for(k = 0; searchNameSuffixes[k] != NULL; k++)
            {
                CT_SAFE_FREE_STRING(fullPath);
                GCE(ceError = CTAllocateStringPrintf(&fullPath, "%s%s/%s%s%s", testPrefix, searchDirs[i], searchNamePrefixes[j], basename, searchNameSuffixes[k]));
                DJ_LOG_VERBOSE("Checking if %s exists", fullPath);
                GCE(ceError = CTCheckFileOrLinkExists(fullPath, &exists));
                if(exists)
                {
                    DJ_LOG_INFO("Found pam module %s", fullPath);

                    if(fileProgPath != NULL)
                    {
                        CT_SAFE_FREE_STRING(fileType);
                        GCE(ceError = CTShell("%fileprog %filepath >%type",
                                CTSHELL_STRING(fileprog, fileProgPath),
                                CTSHELL_STRING(filepath, fullPath),
                                CTSHELL_BUFFER(type, &fileType)));

                        if(strstr(fileType, "64-bit"))
                        {
                            *moduleFlags |= MODULE_FLAG_64BIT;
                            DJ_LOG_INFO("Found it is 64bit");
                        }
                        if(strstr(fileType, "32-bit"))
                        {
                            *moduleFlags |= MODULE_FLAG_32BIT;
                            DJ_LOG_INFO("Found it is 32bit");
                        }
                    }
                    /* If the file is found in exactly one path, use the full path. If the file is found in more than one path, use the relative name. */
                    if(foundPath == i)
                    {
                        /* We already found something in this dir */
                    }
                    else if(foundPath == -1)
                    {
                        CT_SAFE_FREE_STRING(fullPath);
                        GCE(ceError = CTAllocateStringPrintf(&fullPath, "%s/%s%s%s", searchDirs[i], searchNamePrefixes[j], basename, searchNameSuffixes[k]));
                        foundName = fullPath;
                        fullPath = NULL;
                        foundPath = i;
                    }
                    else if(foundPath != -2)
                    {
                        /* This is the second directory that the file has been
                         * found in. Strip off the directory name
                         */
                        CT_SAFE_FREE_STRING(foundName);
                        GCE(ceError = CTAllocateStringPrintf(&fullPath, "%s%s%s", searchNamePrefixes[j], basename, searchNameSuffixes[k]));
                        foundName = fullPath;
                        fullPath = NULL;
                        foundPath = -2;
                    }
                    break;
                }
            }
        }
    }

    if(foundPath == -1)
    {
        DJ_LOG_INFO("Unable to find %s", basename);
        GCE(ceError = ERROR_MISSING_SYSTEMFILE);
    }
    else
    {
        DJ_LOG_INFO("Using module path '%s'", foundName);
    }

cleanup:
    CT_SAFE_FREE_STRING(fullPath);
    CT_SAFE_FREE_STRING(fileProgPath);
    CT_SAFE_FREE_STRING(fileType);
    if(destName != NULL)
        *destName = foundName;
    else
        CT_SAFE_FREE_STRING(foundName);
    return ceError;
}

static DWORD FindPamLwiPassPolicy(const char *testPrefix, char **destName)
{
    return FindModulePath(testPrefix, "pam_lwipasspolicy", destName, NULL);
}

static DWORD FindPamLwidentity(const char *testPrefix, char **destName, DWORD *flags)
{
    DWORD ceError;
    ceError = FindModulePath(testPrefix, "pam_lsass", destName, flags);
    if(ceError)
    {
        DJ_LOG_ERROR("Unable to find pam_lsass");
    }
    return ceError;
}

void DJUpdatePamConf(const char *testPrefix,
        struct PamConf *conf,
        JoinProcessOptions *options,
        WarningFunction warning,
        BOOLEAN enable,
        LWException **exc)
{
    DWORD ceError = ERROR_SUCCESS;
    struct ConfigurePamModuleState state;
    char *pam_lwidentity = NULL;
    char *pam_lwipasspolicy = NULL;
    int i,j;
    const char *phases[] = {"auth", "account", "password", "session", NULL};
    int serviceCount;
    char **services = NULL;
    LwDistroInfo distro;
    LWException *nestedException = NULL;
    DWORD moduleFlags = 0;

    memset(&distro, 0, sizeof(distro));
    if(testPrefix == NULL)
        testPrefix = "";
    LW_CLEANUP_CTERR(exc, DJGetDistroInfo(testPrefix, &distro));
    LW_CLEANUP_CTERR(exc, ListServices(conf, &services, &serviceCount));

    if(enable)
    {
        LW_CLEANUP_CTERR(exc, FindPamLwidentity(testPrefix, &pam_lwidentity,
                    &moduleFlags));
        ceError = FindPamLwiPassPolicy(testPrefix, &pam_lwipasspolicy);
        /* Ignore the password policy on systems that don't have it */
        if(ceError == ERROR_MISSING_SYSTEMFILE)
            ceError = ERROR_SUCCESS;
        LW_CLEANUP_CTERR(exc, ceError);
    }

    for(i = 0; i < serviceCount; i++)
    {
        if(!strcmp(services[i], "common-pammount"))
        {
            /* eeepc has a pam file called common-pammount which is only
             * intended to be used as an include file in conjunction with
             * other include files. All of our changes should go in
             * common-auth instead.
             */
            DJ_LOG_INFO("Ignoring pam service common-pammount");
            continue;
        }
        if(!strcmp(services[i], "system-auth"))
        {
            /* Centos uses system-auth only as an include file. We should
             * not directly try to enable lsass for this entry point because
             * we want to insert "session sufficient pam_lsass" in
             * gdm instead.
             */
            DJ_LOG_INFO("Not directly enabling pam entry point 'system-auth'");
            continue;
        }
        if(!strcmp(services[i], "system-auth-ac"))
        {
            /* Include file on Fedora 14.
             */
            DJ_LOG_INFO("Not directly enabling pam entry point 'system-auth-ac'");
            continue;
        }
        if(!strcmp(services[i], "common-session"))
        {
            /* Sled uses system-auth only as an include file. We should
             * not directly try to enable lsass for this entry point because
             * we want to insert "session sufficient pam_lsass" in
             * gdm instead.
             */
            DJ_LOG_INFO("Not directly enabling pam entry point 'common-session'");
            continue;
        }
        if(!strcmp(services[i], "vmware-authd") && enable &&
                (moduleFlags & MODULE_FLAG_32BIT) != MODULE_FLAG_32BIT)
        {
            /* Vmware is always a 32bit program, even on 64bit systems. Vmware
             * has its own copy of libpam. Ubuntu by default does not have
             * 32bit pam libraries, and /lib/security/pam_* are actually the
             * 64bit pam libraries. If we try to install a 64bit pam library
             * into vmware-authd, the vmware will not be able to load the
             * module and it will fail to authenticate anyone.
             */
            DJ_LOG_WARNING("Ignoring pam service vmware-authd because 32bit pam libraries are not available on this system");
            continue;
        }
        for(j = 0; phases[j] != NULL; j++)
        {
            memset(&state, 0, sizeof(state));
            if(enable)
            {
                LW_HANDLE(&nestedException);
                PamLwidentityEnable(testPrefix, &distro, conf, services[i], phases[j], pam_lwidentity, &state, &nestedException);
                if(LW_IS_OK(nestedException) &&
                        !state.configuredRequestedModule &&
                        !strcmp(phases[j], "auth"))
                {
                    /* The only way this could happen is if the module lets the users through based on something other than passwords (like whether the user is on a system console). If pam_lwidentity was not installed for the auth phase, it shouldn't be installed for the other phases either.
                     */
                    break;
                }
                if(!LW_IS_OK(nestedException))
                {
                    if(nestedException->code ==
                            LW_ERROR_PAM_MISSING_SERVICE)
                    {
                        LW_HANDLE(&nestedException);
                    }
                    else if(warning != NULL &&
                            !IsRequiredService(services[i], conf))
                    {
                        //Show all errors for this service as warnings and continue
                        warning(options, nestedException->shortMsg, nestedException->longMsg);
                        DJLogException(LOG_LEVEL_WARNING, nestedException);
                        LW_HANDLE(&nestedException);
                    }
                }
                LW_CLEANUP(exc, nestedException);
            }
            else
            {
                ceError = PamLwidentityDisable(conf, services[i], phases[j], pam_lwidentity, &state);
                if(ceError == LW_ERROR_PAM_MISSING_SERVICE)
                    ceError = ERROR_SUCCESS;
                LW_CLEANUP_CTERR(exc, ceError);
            }

            memset(&state, 0, sizeof(state));
            ceError = PamOldCenterisDisable(conf, services[i], phases[j], &state);
            if(ceError == LW_ERROR_PAM_MISSING_SERVICE)
                ceError = ERROR_SUCCESS;
            LW_CLEANUP_CTERR(exc, ceError);
        }
        memset(&state, 0, sizeof(state));
        if(enable && pam_lwipasspolicy != NULL)
        {
            ceError = PamLwiPassPolicyEnable(conf, services[i], "password", pam_lwipasspolicy, &state);
        }
        else
        {
            ceError = PamLwiPassPolicyDisable(conf, services[i], "password", pam_lwipasspolicy, &state);
        }
        if(ceError == LW_ERROR_PAM_MISSING_SERVICE)
            ceError = ERROR_SUCCESS;
        LW_CLEANUP_CTERR(exc, ceError);
    }

cleanup:
    LW_HANDLE(&nestedException);
    DJFreeDistroInfo(&distro);
    if(services == NULL)
        FreeServicesList(services);
    CT_SAFE_FREE_STRING(pam_lwidentity);
    CT_SAFE_FREE_STRING(pam_lwipasspolicy);
}

static DWORD
AddMissingAIXServices(struct PamConf *conf)
{
    DWORD ceError = ERROR_SUCCESS;
    /*AIX does not use PAM by default. It's default pam.conf is actually broken for a few services.
     *
     * During the install we switch it over to pam mode, so we have to fix up its default services
     */
    if(NextLineForService(conf, -1, "sshd", "auth") == -1)
    {
        /* Fixes bug 3799 */
        DJ_LOG_INFO("Adding pam entries for sshd");
        BAIL_ON_CENTERIS_ERROR(ceError = CopyService(conf, "telnet", "sshd"));
    }

    if(NextLineForService(conf, -1, "sudo", "auth") == -1)
    {
        DJ_LOG_INFO("Adding pam entries for sudo");
        BAIL_ON_CENTERIS_ERROR(ceError = CopyService(conf, "telnet", "sudo"));
    }

    if(NextLineForService(conf, -1, "dtsession", "auth") == -1)
    {
        /* Fixes bug 4200 */
        DJ_LOG_INFO("Adding pam entries for dtsession");
        BAIL_ON_CENTERIS_ERROR(ceError = CopyService(conf, "sshd", "dtsession"));
    }

    if(NextLineForService(conf, -1, "dtlogin", "auth") == -1)
    {
        /* Fixes bug 4906 */
        DJ_LOG_INFO("Adding pam entries for dtlogin");
        BAIL_ON_CENTERIS_ERROR(ceError = CopyService(conf, "telnet", "dtlogin"));
    }

error:
    return ceError;
}

static void
CheckForPamAuthUpdate(
    const char *testPrefix,
    BOOLEAN *pbUsesPamAuthUpdate,
    BOOLEAN *pbUsedPamAuthUpdate,
    LWException **exc
    )
{
    char *pamauthupdate = NULL;
    BOOLEAN pamauthupdateExists = FALSE;
    char *pamauthupdatedir = NULL;
    BOOLEAN pamauthupdatedirExists = FALSE;
    char *pamauthupdatedirconf = NULL;
    BOOLEAN pamauthupdatedirconfExists = FALSE;
    char *pamauthupdateconf = NULL;
    BOOLEAN pamauthupdateconfExists = FALSE;

    *pbUsesPamAuthUpdate = FALSE;

    if (testPrefix == NULL)
        testPrefix="";

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
         &pamauthupdate, "%s%s", testPrefix, "/usr/sbin/pam-auth-update"));
    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(pamauthupdate, &pamauthupdateExists));

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
        &pamauthupdatedir, "%s%s", testPrefix, "/usr/share/pam-configs"));
    LW_CLEANUP_CTERR(exc, CTCheckDirectoryExists(pamauthupdatedir, &pamauthupdatedirExists));

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
        &pamauthupdatedirconf, "%s%s", testPrefix, "/usr/share/pam-configs/pbis"));
    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(pamauthupdatedirconf, &pamauthupdatedirconfExists));

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
        &pamauthupdateconf, "%s%s", testPrefix, "/opt/pbis/share/pbis.pam-auth-update"));
    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(pamauthupdateconf, &pamauthupdateconfExists));

    if (pamauthupdateExists && pamauthupdatedirExists && pamauthupdateconfExists)
    {
        *pbUsesPamAuthUpdate = TRUE;
        if (pamauthupdatedirconfExists)
            *pbUsedPamAuthUpdate = TRUE;
    }

cleanup:
    CT_SAFE_FREE_STRING(pamauthupdate);
    CT_SAFE_FREE_STRING(pamauthupdatedir);
    CT_SAFE_FREE_STRING(pamauthupdatedirconf);
    CT_SAFE_FREE_STRING(pamauthupdateconf);
}

static void
EnablePamAuthUpdate(
    const char *testPrefix,
    LWException **exc
    )
{
    char *pamauthupdate = NULL;
    char *pamauthupdateconf = NULL;
    char *pamauthupdatedirconf = NULL;
    BOOLEAN pamauthupdatedirconfExists = FALSE;

    if (testPrefix == NULL)
        testPrefix="";

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
         &pamauthupdate, "%s%s", testPrefix, "/usr/sbin/pam-auth-update --package pbis"));

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
        &pamauthupdateconf, "%s%s", testPrefix, "/opt/pbis/share/pbis.pam-auth-update"));

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
        &pamauthupdatedirconf, "%s%s", testPrefix, "/usr/share/pam-configs/pbis"));
    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(pamauthupdatedirconf, &pamauthupdatedirconfExists));

    if (!pamauthupdatedirconfExists)
        LW_CLEANUP_CTERR(exc, CTCopyFileWithOriginalPerms(pamauthupdateconf, pamauthupdatedirconf));

    LW_CLEANUP_CTERR(exc, CTRunCommand(pamauthupdate));

cleanup:
    CT_SAFE_FREE_STRING(pamauthupdate);
    CT_SAFE_FREE_STRING(pamauthupdateconf);
    CT_SAFE_FREE_STRING(pamauthupdatedirconf);
}

static void
DisablePamAuthUpdate(
    const char *testPrefix,
    LWException **exc
    )
{
    char *pamauthupdate = NULL;
    char *pamauthupdatedirconf = NULL;

    if (testPrefix == NULL)
        testPrefix="";

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
         &pamauthupdate, "%s%s", testPrefix, "/usr/sbin/pam-auth-update --package --remove pbis"));

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
         &pamauthupdatedirconf, "%s%s", testPrefix, "/usr/share/pam-configs/pbis"));

    LW_CLEANUP_CTERR(exc, CTRunCommand(pamauthupdate));

    unlink(pamauthupdatedirconf);

cleanup:
    CT_SAFE_FREE_STRING(pamauthupdate);
    CT_SAFE_FREE_STRING(pamauthupdatedirconf);
}

DWORD
DJAddMissingAIXServices(PCSTR rootPrefix)
{
    DWORD ceError = ERROR_SUCCESS;
    struct PamConf conf;
    memset(&conf, 0, sizeof(conf));

    if(rootPrefix == NULL)
        rootPrefix = "";

    GCE(ceError = ReadPamConfiguration(rootPrefix, &conf));
    GCE(ceError = AddMissingAIXServices(&conf));
    if(conf.modified)
    {
        GCE(ceError = WritePamConfiguration(rootPrefix, &conf, NULL));
    }

cleanup:
    FreePamConfContents(&conf);
    return ceError;
}

void DJNewConfigurePamForADLogin(
    const char * testPrefix,
    JoinProcessOptions *options,
    WarningFunction warning,
    BOOLEAN enable,
    LWException **exc
    )
{
    DWORD ceError = ERROR_SUCCESS;
    struct PamConf conf;
    char *pam_lwidentityconf = NULL;
    LwDistroInfo distro;
    BOOLEAN bPamAuthUpdateSupported = FALSE;
    BOOLEAN bPamAuthUpdateLikewiseEnabled = FALSE;
    memset(&conf, 0, sizeof(conf));
    memset(&distro, 0, sizeof(distro));

    ceError = DJGetDistroInfo("", &distro);
    LW_CLEANUP_CTERR(exc, ceError);
    DJ_LOG_INFO("Distro Version %s", distro.version);
    /* Special case Mac OS X
       This operating system provides a wrapper PAM module that redirects PAM calls to
       all of the registered DirectoryService plugins. pam_opendirectory.so is already
       configured in 10.6, and we can therefore skip registration of pam_lsass.so. We
       only need to install our daemons and either LWEDSPlugIn.dsplug or LWIDSPlugIn.dsplug. */
    if (distro.os == OS_DARWIN && (
            !strncmp(distro.version, "10.8", strlen("10.8")) ||
            !strncmp(distro.version, "10.7", strlen("10.7")) ||
            !strncmp(distro.version, "10.6", strlen("10.6")) ||
            !strncmp(distro.version, "10.5", strlen("10.5")))
       )
    {
        DJ_LOG_INFO("Ignoring pam configuration phase of domainjoin utility for this OS. Mac OS X 10.5 or higher uses a common PAM module for all authentication plugins registered with DirectoryService (pam_opendirectory.so). Therefore no action is needed for this join module.");
        goto cleanup;
    }

    if(testPrefix == NULL)
        testPrefix = "";
    ceError = ReadPamConfiguration(testPrefix, &conf);
#ifdef __LWI_AIX__
    if(ceError == ERROR_FILE_NOT_FOUND)
    {
        /* This is an AIX 5.2 machine that doesn't have a pam.conf */
        ceError = ERROR_SUCCESS;
        goto cleanup;
    }
#endif
    LW_CLEANUP_CTERR(exc, ceError);

#ifdef __LWI_AIX__
    if(enable)
    {
        AddMissingAIXServices(&conf);
    }
#endif

    if(enable)
    {
        BOOLEAN confExists;
        DJ_LOG_INFO("Making sure that try_first_pass is not on in pam_lwidentity.conf");
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
            &pam_lwidentityconf, "%s%s", testPrefix,
            "/etc/security/pam_lwidentity.conf"));
        LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(pam_lwidentityconf, &confExists));
        if(confExists)
        {
            LW_CLEANUP_CTERR(exc, CTRunSedOnFile(pam_lwidentityconf, pam_lwidentityconf,
                FALSE, "s/^\\([ \t]*try_first_pass[ \t]*=.*\\)$/# \\1/"));
        }
    }

    /* On systems that support it, we want to use 'pam-auth-update'.
       But we are adding support rather late and the pam configuration may have
       already been edited by domainjoin -- so try to get the system to the
       state where pam-auth-update can work.
       */
    LW_TRY(exc, CheckForPamAuthUpdate(testPrefix, &bPamAuthUpdateSupported, &bPamAuthUpdateLikewiseEnabled, &LW_EXC));
    if (bPamAuthUpdateSupported)
    {
        if (enable)
        {
            if (!bPamAuthUpdateLikewiseEnabled)
            {
                LW_TRY(exc, DJUpdatePamConf(testPrefix, &conf, options, warning, FALSE, &LW_EXC));
                if(conf.modified)
                    LW_CLEANUP_CTERR(exc, WritePamConfiguration(testPrefix, &conf, NULL));
            }
            else
            {
                // The pam-auth-update file *may* have changed.
                LW_TRY(exc, DisablePamAuthUpdate(testPrefix, &LW_EXC));
            }
            LW_TRY(exc, EnablePamAuthUpdate(testPrefix, &LW_EXC));
        }
        else if (bPamAuthUpdateLikewiseEnabled)
        {
            LW_TRY(exc, DisablePamAuthUpdate(testPrefix, &LW_EXC));
        }
    }
    else
    {
        LW_TRY(exc, DJUpdatePamConf(testPrefix, &conf, options, warning, enable, &LW_EXC));

        if(conf.modified)
            LW_CLEANUP_CTERR(exc, WritePamConfiguration(testPrefix, &conf, NULL));
        else
            DJ_LOG_INFO("Pam configuration not modified");
    }

cleanup:
    FreePamConfContents(&conf);
    DJFreeDistroInfo(&distro);
    CT_SAFE_FREE_STRING(pam_lwidentityconf);
}


static DWORD IsLwidentityEnabled(struct PamConf *conf, const char *service, const char * phase, BOOLEAN *configured)
{
    DWORD ceError = ERROR_SUCCESS;
    int line = NextLineForService(conf, -1, service, phase);
    /* Do not free this variable */
    struct PamLine *lineObj;
    const char *module;
    const char *control;
    char *includeService = NULL;
    BOOLEAN sawNonincludeLine = FALSE;
    LwDistroInfo distro;

    memset(&distro, 0, sizeof(distro));
    *configured = FALSE;

    ceError = DJGetDistroInfo("", &distro);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(line == -1)
    {
        /* This means that this service is not defined for this phase. An example is the session phase of passwd.
         * This function will return an error, and the parent function should decide whether or not to ignore the error.
         */
        BAIL_ON_CENTERIS_ERROR(ceError = LW_ERROR_PAM_MISSING_SERVICE);
    }

    while(line != -1)
    {
        lineObj = &conf->lines[line];
        GetModuleControl(lineObj, &module, &control);

        CT_SAFE_FREE_STRING(includeService);
        ceError = GetIncludeName(
                        conf,
                        lineObj,
                        &includeService);
        BAIL_ON_CENTERIS_ERROR(ceError);
        if(includeService != NULL)
        {
            ceError = IsLwidentityEnabled(conf, includeService, phase, configured);
            if(!ceError && *configured)
                goto error;
            if(ceError == LW_ERROR_PAM_MISSING_SERVICE)
                ceError = ERROR_SUCCESS;
            else if(ceError == LW_ERROR_PAM_BAD_CONF)
                ceError = ERROR_SUCCESS;
            else
                sawNonincludeLine = TRUE;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else
            sawNonincludeLine = TRUE;

        if(PamModuleAlwaysDeniesDomainLogins(phase, module, &distro) && (
                    !strcmp(control, "required") ||
                    !strcmp(control, "requisite")))
            break;

        if(PamModuleIsLwidentity(phase, module))
        {
            DJ_LOG_INFO("Found pam_lwidentity");
            *configured = TRUE;
            break;
        }

        if(!strcmp(control, "sufficient") &&
                PamModuleGrants(phase, module) &&
                !PamModulePrompts(phase, module) &&
                !PamModuleChecksCaller(phase, module))
        {
            /* This module seems to let anyone through */
            break;
        }

        line = NextLineForService(conf, line, service, phase);
    }

    if(!sawNonincludeLine)
        BAIL_ON_CENTERIS_ERROR(ceError = LW_ERROR_PAM_MISSING_SERVICE);

error:
    CT_SAFE_FREE_STRING(includeService);
    DJFreeDistroInfo(&distro);
    return ceError;
}

DWORD
DJCopyPamToRootDir(
        const char *srcPrefix,
        const char *destPrefix
        )
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR srcPath = NULL;
    PSTR destPath = NULL;
    BOOLEAN exists;

    if(srcPrefix == NULL)
        srcPrefix = "";
    if(destPrefix == NULL)
        destPrefix = "";

    CT_SAFE_FREE_STRING(srcPath);
    GCE(ceError = CTAllocateStringPrintf(&srcPath, "%s/etc", srcPrefix));
    GCE(ceError = CTCheckDirectoryExists(srcPath, &exists));
    if(exists)
    {
        CT_SAFE_FREE_STRING(destPath);
        GCE(ceError = CTAllocateStringPrintf(&destPath, "%s/etc", destPrefix));
        GCE(ceError = CTCreateDirectory(destPath, 0700));
    }

    CT_SAFE_FREE_STRING(srcPath);
    GCE(ceError = CTAllocateStringPrintf(&srcPath, "%s/etc/pam.d", srcPrefix));
    GCE(ceError = CTCheckDirectoryExists(srcPath, &exists));
    if(exists)
    {
        CT_SAFE_FREE_STRING(destPath);
        GCE(ceError = CTAllocateStringPrintf(&destPath, "%s/etc/pam.d", destPrefix));
        GCE(ceError = CTCopyDirectory(srcPath, destPath));
    }

    CT_SAFE_FREE_STRING(srcPath);
    GCE(ceError = CTAllocateStringPrintf(&srcPath, "%s/etc/pam.conf", srcPrefix));
    GCE(ceError = CTCheckFileOrLinkExists(srcPath, &exists));
    if(exists)
    {
        CT_SAFE_FREE_STRING(destPath);
        GCE(ceError = CTAllocateStringPrintf(&destPath, "%s/etc/pam.conf", destPrefix));
        GCE(ceError = CTCopyFileWithOriginalPerms(srcPath, destPath));
    }

cleanup:
    CT_SAFE_FREE_STRING(srcPath);
    CT_SAFE_FREE_STRING(destPath);
    return ceError;
}

BOOLEAN IsRequiredService(PCSTR service, const struct PamConf *conf)
{
    int i;
    PCSTR requiredServices[] = {"ssh", "login", "su"};
    if(NextLineForService(conf, 0, "ssh", "auth") == -1)
    {
        requiredServices[0] = "sshd";
    }
    for(i = 0; i < sizeof(requiredServices)/sizeof(requiredServices[0]); i++)
    {
        if(NextLineForService(conf, 0, requiredServices[i], "auth") == -1)
        {
            requiredServices[i] = "other";
        }
    }
    for(i = 0; i < sizeof(requiredServices)/sizeof(requiredServices[0]); i++)
    {
        if(!strcmp(service, requiredServices[i]))
        {
            return TRUE;
        }
    }
    return FALSE;
}

static QueryResult QueryPam(const JoinProcessOptions *options, LWException **exc)
{
    DWORD ceError = ERROR_SUCCESS;
    LWException *nestedException = NULL;
    QueryResult result = NotConfigured;
    PSTR tempDir = NULL;
    struct PamConf conf;
    BOOLEAN configured;
    PCSTR services[] = {"ssh", "sshd", "login", "su", "other"};
    int i;
    LwDistroInfo distro;
    BOOLEAN bPamAuthUpdateSupported = FALSE;
    BOOLEAN bPamAuthUpdateLikewiseEnabled = FALSE;

    memset(&distro, 0, sizeof(distro));
    memset(&conf, 0, sizeof(conf));

    ceError = DJGetDistroInfo("", &distro);
    BAIL_ON_CENTERIS_ERROR(ceError);

    DJ_LOG_INFO("Distro Version %s", distro.version);
    /* Special case Mac OS X
       This operating system provides a wrapper PAM module that redirects PAM calls to
       all of the registered DirectoryService plugins. pam_opendirectory.so is already
       configured in 10.5, and we can therefore skip registration of pam_lsass.so. We
       only need to install our daemons and either LWEDSPlugIn.dsplug or LWIDSPlugIn.dsplug. */
    if (distro.os == OS_DARWIN && (
            !strncmp(distro.version, "10.8", strlen("10.8")) ||
            !strncmp(distro.version, "10.7", strlen("10.7")) ||
            !strncmp(distro.version, "10.6", strlen("10.6")) ||
            !strncmp(distro.version, "10.5", strlen("10.5")))
       )
    {
        DJ_LOG_INFO("No action is needed for PAM join module on Mac OS X 10.5 or higher. Returning module result of FullyConfigured.");
        result = FullyConfigured;
        goto cleanup;
    }

    if ( options->ignorePam || options->enableMultipleJoins )
    {
        result = NotApplicable;
        goto cleanup;
    }

    LW_TRY(exc, CheckForPamAuthUpdate(NULL, &bPamAuthUpdateSupported, &bPamAuthUpdateLikewiseEnabled, &LW_EXC));
    if (bPamAuthUpdateSupported)
    {
        if ((options->joiningDomain && bPamAuthUpdateLikewiseEnabled) ||
            (!options->joiningDomain && !bPamAuthUpdateLikewiseEnabled))
        {
            result = FullyConfigured;
        }
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, CTCreateTempDirectory(&tempDir));
    LW_CLEANUP_CTERR(exc, DJCopyPamToRootDir(NULL, tempDir));
    ceError = ReadPamConfiguration(tempDir, &conf);
    if(ceError == ERROR_FILE_NOT_FOUND)
    {
        result = NotApplicable;
        goto cleanup;
    }
    LW_CLEANUP_CTERR(exc, ceError);

    if(!options->joiningDomain)
    {
        if (distro.os == OS_SUNOS)
        {
            // The "pam_lsass.so set_default_repository" line is necessary when
            // lsass appears in nsswitch.conf for password changes to continue
            // working.
            DJ_LOG_INFO("Pam cannot be deconfigured without deconfiguring nsswitch on Solaris.");
            result = FullyConfigured;
            goto cleanup;
        }

        LW_TRY(exc, DJUpdatePamConf(NULL, &conf, NULL, NULL, options->joiningDomain,
                &LW_EXC));
        if(conf.modified)
            goto cleanup;
        result = FullyConfigured;
        goto cleanup;
    }

    for(i = 0; i < sizeof(services)/sizeof(services[0]); i++)
    {
        PCSTR service = services[i];
        if(!IsRequiredService(service, &conf))
            continue;
        ceError = IsLwidentityEnabled(&conf, service, "auth", &configured);
        if(ceError == LW_ERROR_PAM_MISSING_SERVICE)
        {
            configured = TRUE;
            ceError = ERROR_SUCCESS;
        }
        LW_CLEANUP_CTERR(exc, ceError);
        if(!configured)
            goto cleanup;

        ceError = IsLwidentityEnabled(&conf,
                    service, "account", &configured);
        if(ceError == LW_ERROR_PAM_MISSING_SERVICE)
        {
            configured = TRUE;
            ceError = ERROR_SUCCESS;
        }
        LW_CLEANUP_CTERR(exc, ceError);
        if(!configured)
            goto cleanup;

        ceError = IsLwidentityEnabled(&conf,
                    service, "password", &configured);
        if(ceError == LW_ERROR_PAM_MISSING_SERVICE)
        {
            configured = TRUE;
            ceError = ERROR_SUCCESS;
        }
        LW_CLEANUP_CTERR(exc, ceError);
        if(!configured)
            goto cleanup;
    }

    result = SufficientlyConfigured;
    DJUpdatePamConf(NULL, &conf, NULL, NULL, TRUE,
            &nestedException);
    if(!LW_IS_OK(nestedException) &&
            nestedException->code == LW_ERROR_PAM_BAD_CONF)
    {
        //Eat this error because the configuration is sufficient
        conf.modified = TRUE;
        LW_HANDLE(&nestedException);
    }
    LW_CLEANUP(exc, nestedException);
    if(conf.modified)
        goto cleanup;

    result = FullyConfigured;

cleanup:
    if(tempDir != NULL)
    {
        CTRemoveDirectory(tempDir);
        CT_SAFE_FREE_STRING(tempDir);
    }
    FreePamConfContents(&conf);
    DJFreeDistroInfo(&distro);
    LWHandle(&nestedException);
    return result;

error:
    goto cleanup;
}

static void DoPam(JoinProcessOptions *options, LWException **exc)
{
    DJNewConfigurePamForADLogin(NULL, options, options->warningCallback,
                options->joiningDomain, exc);
}

static PSTR GetPamDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR tempDir = NULL;
    PSTR ret = NULL;
    PSTR diff = NULL;
    struct PamConf conf;
    BOOLEAN bPamAuthUpdateSupported = FALSE;
    BOOLEAN bPamAuthUpdateLikewiseEnabled = FALSE;

    memset(&conf, 0, sizeof(conf));
    
    LW_TRY(exc, CheckForPamAuthUpdate(NULL, &bPamAuthUpdateSupported, &bPamAuthUpdateLikewiseEnabled, &LW_EXC));
    if (bPamAuthUpdateSupported)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
            &ret, "PAM is managed by the system utility pam-auth-update."));

        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, CTCreateTempDirectory(&tempDir));
    LW_CLEANUP_CTERR(exc, DJCopyPamToRootDir(NULL, tempDir));

    LW_CLEANUP_CTERR(exc, ReadPamConfiguration(tempDir, &conf));

    LW_TRY(exc, DJUpdatePamConf(NULL, &conf, (JoinProcessOptions *)options, options->warningCallback,
        options->joiningDomain, &LW_EXC));

    if(conf.modified)
        WritePamConfiguration(tempDir, &conf, &diff);

    if(diff == NULL || strlen(diff) < 1)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
            &ret, "Fully configured"));
    }
    else
    {
        if(options->joiningDomain)
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf( &ret,
"The ssh, su, and login services must list pam_lwidentity in their auth, account, and password phases. If this step is performed automatically, the local password policy module will be installed, and pam_lwidentity will be enabled for all services and all phases. Here is a list of the changes that would be made to the files if this configuration is performed automatically:\n%s", diff));
        }
        else
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf( &ret,
"All references to PowerBroker Identity Services PAM modules must be removed from pam.conf/pam.d. Otherwise, logins will break if these file are later removed from the system. Here is a list of changes that will be performed:\n%s", diff));
        }
    }

cleanup:
    if(tempDir != NULL)
    {
        CTRemoveDirectory(tempDir);
        CT_SAFE_FREE_STRING(tempDir);
    }
    CT_SAFE_FREE_STRING(diff);
    return ret;
}

const JoinModule DJPamModule = { TRUE, "pam", "configure pam.d/pam.conf", QueryPam, DoPam, GetPamDescription };
