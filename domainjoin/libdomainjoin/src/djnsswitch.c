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
#include "djstr.h"
#include "djdistroinfo.h"

#define NSSWITCH_CONF_PATH "/etc/nsswitch.conf"
#define NSSWITCH_LWIDEFAULTS "/etc/nsswitch.lwi_defaults"

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

typedef struct
{
    char *leadingWhiteSpace;
    CTParseToken name;
    DynamicArray modules;
    char *comment;
} NsswitchEntry;

typedef struct
{
    char *filename;
    /*Holds NsswitchEntry*/
    DynamicArray lines;
    BOOLEAN modified;
} NsswitchConf;

static const NsswitchEntry * GetEntry(const NsswitchConf *conf, size_t index)
{
    if(index >= conf->lines.size)
        return NULL;
    return ((NsswitchEntry *)conf->lines.data) + index;
}

static CTParseToken * GetEntryModule(NsswitchEntry *entry, size_t index)
{
    if(index >= entry->modules.size)
        return NULL;
    return ((CTParseToken *)entry->modules.data) + index;
}

static void FreeNsswitchEntryContents(NsswitchEntry *entry)
{
    size_t i;
    for(i = 0; i < entry->modules.size; i++)
    {
        CTFreeParseTokenContents(GetEntryModule(entry, i));
    }
    CTArrayFree(&entry->modules);
    CT_SAFE_FREE_STRING(entry->leadingWhiteSpace);
    CT_SAFE_FREE_STRING(entry->comment);
    CTFreeParseTokenContents(&entry->name);
}

static void FreeNsswitchConfContents(NsswitchConf *conf)
{
    size_t i;
    for(i = 0; i < conf->lines.size; i++)
    {
        FreeNsswitchEntryContents(((NsswitchEntry *)conf->lines.data) + i);
    }
}

/* Get the printed form of a line from the parsed form by concatenating all of the strings together */
static DWORD GetPrintedLine(DynamicArray *dest, NsswitchConf *conf, int line)
{
    DWORD ceError = ERROR_SUCCESS;
    size_t len = 0;
    char *pos;
    int i;
    const NsswitchEntry *lineObj = GetEntry(conf, line);

    len += CTGetTokenLen(&lineObj->name);
    for(i = 0; i < lineObj->modules.size; i++)
    {
        len += CTGetTokenLen(&((CTParseToken *)lineObj->modules.data)[i]);
    }
    if(lineObj->comment != NULL)
        len += strlen(lineObj->comment);

    //For the terminating NULL
    len++;

    if(len > dest->capacity)
        GCE(ceError = CTSetCapacity(dest, 1, len));
    pos = dest->data;
    CTAppendTokenString(&pos, &lineObj->name);
    for(i = 0; i < lineObj->modules.size; i++)
    {
        CTAppendTokenString(&pos, &((CTParseToken *)lineObj->modules.data)[i]);
    }
    if(lineObj->comment != NULL)
    {
        memcpy(pos, lineObj->comment, strlen(lineObj->comment));
        pos += strlen(lineObj->comment);
    }
    *pos = '\0';
    dest->size = len;

cleanup:
    return ceError;
}

static DWORD AddFormattedLine(NsswitchConf *conf, const char *filename, const char *linestr, const char **endptr)
{
    DWORD ceError = ERROR_SUCCESS;
    NsswitchEntry lineObj;
    const char *pos = linestr;
    const char *token_start = NULL;
    CTParseToken token;

    memset(&lineObj, 0, sizeof(lineObj));
    memset(&token, 0, sizeof(token));

    /* Find the leading whitespace in the line */
    token_start = pos;
    while(isblank(*pos)) pos++;
    GCE(ceError = CTStrndup(token_start, pos - token_start, &lineObj.leadingWhiteSpace));

    /* Read the name of the entry and attach its trailing : or = */
    GCE(ceError = CTReadToken(&pos, &lineObj.name, "=: \t", ";#\r\n", ""));

    /* Create an array of the modules for this entry */
    while(strchr("\r\n;#", *pos) == NULL)
    {
        GCE(ceError = CTReadToken(&pos, &token, ", \t", ";#\r\n", ""));
        GCE(ceError = CTArrayAppend(&lineObj.modules, sizeof(CTParseToken), &token, 1));
        memset(&token, 0, sizeof(token));
    }

    /*Read the comment, if there is one*/
    token_start = pos;
    while(strchr("\r\n", *pos) == NULL) pos++;

    if(pos != token_start)
        GCE(ceError = CTStrndup(token_start, pos - token_start, &lineObj.comment));

    GCE(ceError = CTArrayAppend(&conf->lines, sizeof(lineObj), &lineObj, 1));
    memset(&lineObj, 0, sizeof(lineObj));

    if(endptr != NULL)
        *endptr = pos;

cleanup:
    FreeNsswitchEntryContents(&lineObj);
    CTFreeParseTokenContents(&token);

    return ceError;
}

static DWORD ReadNsswitchFile(NsswitchConf *conf, const char *rootPrefix, const char *filename)
{
    DWORD ceError = ERROR_SUCCESS;
    FILE *file = NULL;
    PSTR buffer = NULL;
    char *fullPath = NULL;
    BOOLEAN endOfFile = FALSE;
    BOOLEAN exists;

    if(rootPrefix == NULL)
        rootPrefix = "";

    GCE(ceError = CTAllocateStringPrintf(
            &fullPath, "%s%s", rootPrefix, filename));
    DJ_LOG_INFO("Reading nsswitch file %s", fullPath);
    GCE(ceError = CTCheckFileOrLinkExists(fullPath, &exists));
    if(!exists)
    {
        DJ_LOG_INFO("File %s does not exist", fullPath);
        ceError = ERROR_FILE_NOT_FOUND;
        goto cleanup;
    }

    GCE(ceError = CTStrdup(filename,
        &conf->filename));
    GCE(ceError = CTOpenFile(fullPath, "r", &file));
    CT_SAFE_FREE_STRING(fullPath);
    while(TRUE)
    {
        CT_SAFE_FREE_STRING(buffer);
        GCE(ceError = CTReadNextLine(file, &buffer, &endOfFile));
        if(endOfFile)
            break;
        GCE(ceError = AddFormattedLine(conf, filename, buffer, NULL));
    }

    conf->modified = FALSE;

cleanup:
    CT_SAFE_FREE_STRING(buffer);
    if(file != NULL)
        CTCloseFile(file);
    CT_SAFE_FREE_STRING(fullPath);
    if(ceError)
        FreeNsswitchConfContents(conf);
    return ceError;
}

static DWORD WriteNsswitchConfiguration(const char *rootPrefix, NsswitchConf *conf)
{
    DWORD ceError = ERROR_SUCCESS;
    DynamicArray printedLine;
    int i;
    char *tempName = NULL;
    char *finalName = NULL;
    char *prefixedPath = NULL;
    FILE *file = NULL;
    memset(&printedLine, 0, sizeof(printedLine));

    GCE(ceError = CTAllocateStringPrintf(&prefixedPath, "%s%s", rootPrefix, conf->filename));

    GCE(ceError = CTGetFileTempPath(
                        prefixedPath,
                        &finalName,
                        &tempName));

    DJ_LOG_INFO("Writing nsswitch configuration for %s", finalName);

    ceError = CTOpenFile(tempName, "w", &file);
    if(ceError)
    {
        DJ_LOG_ERROR("Unable to open '%s' for writing", tempName);
        GCE(ceError);
    }

    for(i = 0; i < conf->lines.size; i++)
    {
        GCE(ceError = GetPrintedLine(&printedLine, conf, i));
        GCE(ceError = CTFilePrintf(file, "%s\n", printedLine.data));
    }

    GCE(ceError = CTCloseFile(file));
    file = NULL;

    GCE(ceError = CTSafeReplaceFile(
            finalName,
            tempName));
    DJ_LOG_INFO("File moved into place");

cleanup:
    if(file != NULL)
        CTCloseFile(file);
    CTArrayFree(&printedLine);
    CT_SAFE_FREE_STRING(tempName);
    CT_SAFE_FREE_STRING(finalName);
    CT_SAFE_FREE_STRING(prefixedPath);
    return ceError;
}

static int FindEntry(const NsswitchConf *conf, int startLine, const char *name)
{
    int i;
    if(startLine == -1)
        return -1;
    for(i = startLine; i < conf->lines.size; i++)
    {
        if(GetEntry(conf, i)->name.value != NULL &&
                !strcmp(GetEntry(conf, i)->name.value, name))
        {
            return i;
        }
    }
    return -1;
}

static DWORD AddEntry(NsswitchConf *conf, const LwDistroInfo *distro,
        int *addedIndex, const char *name)
{
    DWORD ceError = ERROR_SUCCESS;
    int line = -1;
    NsswitchEntry lineObj;
    const NsswitchEntry *copy;

    memset(&lineObj, 0, sizeof(lineObj));
    GCE(ceError = CTStrdup(name, &lineObj.name.value));

    for(line = 0; line < conf->lines.size; line++)
    {
        copy = GetEntry(conf, line);
        if(copy->name.value != NULL)
        {
            GCE(ceError = CTStrdup(copy->name.trailingSeparator, &lineObj.name.trailingSeparator));
            break;
        }
    }

    if(lineObj.name.trailingSeparator == NULL)
    {
        //Couldn't find an existing line to copy the separator from. We'll
        //have to guess based on the OS
        if(distro->os == OS_AIX)
            GCE(ceError = CTStrdup(" = ", &lineObj.name.trailingSeparator));
        else
            GCE(ceError = CTStrdup(": ", &lineObj.name.trailingSeparator));
    }

    GCE(ceError = CTArrayAppend(&conf->lines,
                sizeof(NsswitchEntry), &lineObj, 1));
    memset(&lineObj, 0, sizeof(lineObj));
    conf->modified = 1;
    if(addedIndex != NULL)
        *addedIndex = conf->lines.size - 1;

cleanup:
    FreeNsswitchEntryContents(&lineObj);
    return ceError;
}

const char * GetModuleSeparator(NsswitchConf *conf, const LwDistroInfo *distro)
{
    int line;
    const NsswitchEntry *copy;

    for(line = 0; line < conf->lines.size; line++)
    {
        copy = GetEntry(conf, line);
        if(copy->modules.size > 1)
        {
            /*This line has at least two modules in it. There will be a
             * separator after the first module.
             */
            return ((CTParseToken *)copy->modules.data)[0].trailingSeparator;
        }
    }
    /* We have to guess based on the OS */
    if(distro->os == OS_AIX)
    {
        return ", ";
    }
    return " ";
}

static int FindModuleOnLine(const NsswitchConf *conf, int line, const char *name)
{
    const NsswitchEntry *lineObj;
    int i;

    if(line < 0)
        return -1;
    lineObj = GetEntry(conf, line);
    for(i = 0; i < lineObj->modules.size; i++)
    {
        if(!strcmp(((CTParseToken *)lineObj->modules.data)[i].value, name))
            return i;
    }
    return -1;
}

static DWORD InsertModule(NsswitchConf *conf, const LwDistroInfo *distro,
        int line, int insertIndex, const char *name)
{
    DWORD ceError = ERROR_SUCCESS;
    NsswitchEntry *lineObj = (NsswitchEntry *)GetEntry(conf, line);
    CTParseToken *beforeModule = NULL, *afterModule = NULL;
    CTParseToken addModule;

    memset(&addModule, 0, sizeof(addModule));
    if(insertIndex == -1)
        insertIndex = lineObj->modules.size;

    GCE(ceError = CTStrdup(name, &addModule.value));

    if(insertIndex - 1 >= 0)
        beforeModule = (CTParseToken *)lineObj->modules.data + insertIndex - 1;
    if(insertIndex < lineObj->modules.size)
        afterModule = (CTParseToken *)lineObj->modules.data + insertIndex;

    if(beforeModule != NULL)
    {
        /* Copy the separator from the previous module */
        GCE(ceError = CTDupOrNullStr(beforeModule->trailingSeparator,
                &addModule.trailingSeparator));
        if(afterModule == NULL)
        {
            /*This is the last module.  Put in the correct separator after the
             * previous module */
            CT_SAFE_FREE_STRING(beforeModule->trailingSeparator);
            GCE(ceError = CTStrdup(GetModuleSeparator(conf, distro),
                    &beforeModule->trailingSeparator));
        }
    }
    else
    {
        if(afterModule == NULL)
        {
            //This is the last module
            if(lineObj->comment == NULL)
            {
                //Leave the trailingSeparator as NULL
            }
            else
            {
                GCE(ceError = CTStrdup(" ", &addModule.trailingSeparator));
            }
        }
        else
        {
            //This is the first module. Add the appropriate separator to
            //distinguish it from the next module.
            GCE(ceError = CTStrdup(GetModuleSeparator(conf, distro),
                    &addModule.trailingSeparator));
        }
    }

    GCE(ceError = CTArrayInsert(&lineObj->modules, insertIndex,
                sizeof(addModule), &addModule, 1));
    memset(&addModule, 0, sizeof(addModule));
    conf->modified = 1;

cleanup:
    CTFreeParseTokenContents(&addModule);
    return ceError;
}

static DWORD RemoveModule(NsswitchConf *conf, 
        int line, int moduleIndex)
{
    DWORD ceError = ERROR_SUCCESS;
    NsswitchEntry *lineObj = (NsswitchEntry *)GetEntry(conf, line);
    CTParseToken *beforeModule = NULL, *afterModule = NULL;
    CTParseToken *removeModule;

    removeModule = (CTParseToken *)lineObj->modules.data + moduleIndex;

    if(moduleIndex - 1 >= 0)
        beforeModule = (CTParseToken *)lineObj->modules.data + moduleIndex - 1;
    if(moduleIndex + 1 < lineObj->modules.size)
        afterModule = (CTParseToken *)lineObj->modules.data + moduleIndex + 1;

    if(afterModule == NULL && beforeModule != NULL)
    {
        /* Since the last module is being removed, move the trailingSeparator
         * to the previous module */
        CT_SAFE_FREE_STRING(beforeModule->trailingSeparator);
        beforeModule->trailingSeparator = removeModule->trailingSeparator;
        removeModule->trailingSeparator = NULL;
    }
    CTFreeParseTokenContents(removeModule);

    GCE(ceError = CTArrayRemove(&lineObj->modules, moduleIndex, sizeof(CTParseToken), 1));
    conf->modified = 1;

cleanup:
    return ceError;
}

static DWORD RemoveLine(NsswitchConf *conf, int line)
{
    DWORD ceError = ERROR_SUCCESS;
    NsswitchEntry *lineObj = (NsswitchEntry *)GetEntry(conf, line);

    if(lineObj == NULL)
    {
        GCE(ceError = ERROR_INVALID_PARAMETER);
    }

    FreeNsswitchEntryContents(lineObj);
    GCE(ceError = CTArrayRemove(&conf->lines, line, sizeof(NsswitchEntry), 1));

    conf->modified = 1;

cleanup:
    return ceError;
}

DWORD
ReadNsswitchConf(NsswitchConf *conf, const char *testPrefix,
        BOOLEAN allowFileCreate)
{
    PSTR copyDestPath = NULL;
    PSTR defaultFilePath = NULL;
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;
    memset(conf, 0, sizeof(*conf));
    
    //Keep trying to read different filenames until one of them is found
    if(!bFileExists)
    {
        bFileExists = TRUE;
        ceError = ReadNsswitchFile(conf, testPrefix, "/etc/nsswitch.conf");
        if(ceError == ERROR_FILE_NOT_FOUND)
        {
            bFileExists = FALSE;
            ceError = ERROR_SUCCESS;
        }
        GCE(ceError);
    }

    if(!bFileExists)
    {
        bFileExists = TRUE;
        ceError = ReadNsswitchFile(conf, testPrefix, "/etc/netsvc.conf");
        if(ceError == ERROR_FILE_NOT_FOUND)
        {
            bFileExists = FALSE;
            ceError = ERROR_SUCCESS;
        }
        GCE(ceError);
    }

    /* HP-UX 11.xx does not appear to have an nsswitch file in
       place by default. If we don't find on already installed,
       use our own */

    if(!bFileExists)
    {
        GCE(ceError = CTAllocateStringPrintf(
          &defaultFilePath, "%s%s", testPrefix, NSSWITCH_LWIDEFAULTS));
        GCE(ceError = CTCheckFileExists(defaultFilePath, &bFileExists));
      
        if (bFileExists) {
            ceError = ReadNsswitchFile(conf, testPrefix, NSSWITCH_LWIDEFAULTS);
            GCE(ceError);
            CT_SAFE_FREE_STRING(conf->filename);
            conf->modified = TRUE;

            if(allowFileCreate)
            {
                GCE(ceError = CTStrdup(NSSWITCH_CONF_PATH, &conf->filename));

                /* Copy over the original file. This way the user can more
                 * clearly see what we changed by comparing nsswitch.conf with
                 * nsswitch.conf.lwidentity.orig. Also, the permissions will be
                 * correct this way when the file is written out.
                 */
                DJ_LOG_INFO("Copying default nsswitch file");
                GCE(ceError = CTAllocateStringPrintf(
                    &copyDestPath, "%s%s", testPrefix, NSSWITCH_CONF_PATH));
                ceError = CTCopyFileWithOriginalPerms(defaultFilePath, copyDestPath);
                GCE(ceError);
            }
            else
            {
                GCE(ceError = CTStrdup(NSSWITCH_LWIDEFAULTS, &conf->filename));
            }
        }
    }

    if(!bFileExists)
    {
        GCE(ceError = ERROR_FILE_NOT_FOUND);
    }

cleanup:
    CT_SAFE_FREE_STRING(copyDestPath);
    CT_SAFE_FREE_STRING(defaultFilePath);

    return ceError;
}

static QueryResult RemoveCompat(NsswitchConf *conf, PSTR *description, LWException **exc)
{
    LwDistroInfo distro;
    int compatLine;
    int noncompatLine;
    int compatModIndex;
    BOOLEAN passwdNeedUpdate = FALSE;
    BOOLEAN groupNeedUpdate = FALSE;
    QueryResult result = FullyConfigured;

    memset(&distro, 0, sizeof(distro));

    /* The default configuration on FreeBSD is:
     * passwd: compat
     * passwd_compat: nis
     * group: compat
     * group_compat: nis
     *
     * The nsswitch man page says that compat must be the only module on the
     * line if it is used. Unfortunately, if a module is listed on the compat
     * line, it goes through a different interface which LSASS does not
     * understand. So this configuration must first be transformed into:
     *
     * passwd: files nis
     * group: files nis
     *
     * If the user is using compat mode with a non-default configuration, show
     * an error message instead.
     */

    LW_CLEANUP_CTERR(exc, DJGetDistroInfo(NULL, &distro));

    compatLine = FindEntry(conf, 0, "passwd_compat");
    if(compatLine != -1)
    {
        const NsswitchEntry *lineEntry = GetEntry(conf, compatLine);

        passwdNeedUpdate = TRUE;

        if(lineEntry->modules.size != 1)
        {
            result = CannotConfigure;
            goto done_configuring;
        }
        if(FindModuleOnLine(conf, compatLine, "nis") == -1)
        {
            result = CannotConfigure;
            goto done_configuring;
        }
        noncompatLine = FindEntry(conf, 0, "passwd");
        if(noncompatLine == -1)
        {
            result = CannotConfigure;
            goto done_configuring;
        }
        lineEntry = GetEntry(conf, noncompatLine);
        if(lineEntry->modules.size != 1)
        {
            result = CannotConfigure;
            goto done_configuring;
        }
        compatModIndex = FindModuleOnLine(conf, noncompatLine, "compat");
        if(compatModIndex == -1)
        {
            result = CannotConfigure;
            goto done_configuring;
        }

        result = NotConfigured;

        LW_CLEANUP_CTERR(exc, InsertModule(conf, &distro, noncompatLine, -1, "files"));
        LW_CLEANUP_CTERR(exc, InsertModule(conf, &distro, noncompatLine, -1, "nis"));
        LW_CLEANUP_CTERR(exc, RemoveModule(conf, noncompatLine, compatModIndex));
        LW_CLEANUP_CTERR(exc, RemoveLine(conf, compatLine));
    }

    compatLine = FindEntry(conf, 0, "group_compat");
    if(compatLine != -1)
    {
        const NsswitchEntry *lineEntry = GetEntry(conf, compatLine);

        groupNeedUpdate = TRUE;

        if(lineEntry->modules.size != 1)
        {
            result = CannotConfigure;
            goto done_configuring;
        }
        if(FindModuleOnLine(conf, compatLine, "nis") == -1)
        {
            result = CannotConfigure;
            goto done_configuring;
        }
        noncompatLine = FindEntry(conf, 0, "group");
        if(noncompatLine == -1)
        {
            result = CannotConfigure;
            goto done_configuring;
        }
        lineEntry = GetEntry(conf, noncompatLine);
        if(lineEntry->modules.size != 1)
        {
            result = CannotConfigure;
            goto done_configuring;
        }
        compatModIndex = FindModuleOnLine(conf, noncompatLine, "compat");
        if(compatModIndex == -1)
        {
            result = CannotConfigure;
            goto done_configuring;
        }

        result = NotConfigured;

        LW_CLEANUP_CTERR(exc, InsertModule(conf, &distro, noncompatLine, -1, "files"));
        LW_CLEANUP_CTERR(exc, InsertModule(conf, &distro, noncompatLine, -1, "nis"));
        LW_CLEANUP_CTERR(exc, RemoveModule(conf, noncompatLine, compatModIndex));
        LW_CLEANUP_CTERR(exc, RemoveLine(conf, compatLine));
    }

done_configuring:
    if(result == CannotConfigure && description != NULL)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup("Remove the passwd_compat and/or group_compat lines and use passwd and group instead. This cannot be done automatically because your system has a non-default nsswitch configuration.\n", description));
    }
    else if((passwdNeedUpdate || groupNeedUpdate) && description != NULL)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup("Remove the passwd_compat and/or group_compat lines and use passwd and group instead.\n", description));
        result = NotConfigured;
    }
    else if(description != NULL)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup("Fully Configured.", description));
    }

cleanup:
    DJFreeDistroInfo(&distro);

    return result;
}

DWORD
UpdateNsswitchConf(NsswitchConf *conf, BOOLEAN enable)
{
    DWORD ceError = ERROR_SUCCESS;
    LwDistroInfo distro;
    int line;
    int lwiIndex;
    static const char* moduleName = "lsass";
    static const char* oldModule = "lwidentity";

    GCE(ceError = DJGetDistroInfo(NULL, &distro));

    line = FindEntry(conf, 0, "passwd");
    if(enable && line == -1)
    {
        DJ_LOG_INFO("Adding passwd line");
        GCE(ceError = AddEntry(conf, &distro, &line, "passwd"));
        GCE(ceError = InsertModule(conf, &distro, line, -1, "files"));
    }
    lwiIndex = FindModuleOnLine(conf, line, moduleName);
    if(enable && lwiIndex == -1)
    {
        GCE(ceError = InsertModule(conf, &distro, line, -1, moduleName));
    }
    if(!enable && lwiIndex != -1)
    {
        GCE(ceError = RemoveModule(conf, line, lwiIndex));
    }
    lwiIndex = FindModuleOnLine(conf, line, oldModule);
    if(lwiIndex != -1)
    {
        GCE(ceError = RemoveModule(conf, line, lwiIndex));
    }

    // If lwidentity was the only entry
    // and we removed that now, don't write
    // an empty entry into the file
    if(!enable && line != -1 && GetEntry(conf, line)->modules.size == 0)
    {
        GCE(ceError = InsertModule(conf, &distro, line, -1, "files"));
    }

    line = FindEntry(conf, 0, "group");
    if(line == -1)
    {
        line = FindEntry(conf, 0, "groups");
    }
    if(enable && line == -1)
    {
        /* The nsswitch file doesn't have an existing groups line. We have to
         * guess based on platform whether it uses 'group' or 'groups'.
         */
        const char *groupName = "group";
        if(distro.os == OS_AIX || distro.os == OS_DARWIN)
        {
            groupName = "groups";
        }
        DJ_LOG_INFO("Adding %s line", groupName);
        GCE(ceError = AddEntry(conf, &distro, &line, groupName));
        GCE(ceError = InsertModule(conf, &distro, line, -1, "files"));
    }
    lwiIndex = FindModuleOnLine(conf, line, moduleName);
    if(enable && lwiIndex == -1)
    {
        GCE(ceError = InsertModule(conf, &distro, line, -1, moduleName));
    }
    if(!enable && lwiIndex != -1)
    {
        GCE(ceError = RemoveModule(conf, line, lwiIndex));
    }
    lwiIndex = FindModuleOnLine(conf, line, oldModule);
    if(lwiIndex != -1)
    {
        GCE(ceError = RemoveModule(conf, line, lwiIndex));
    }

    // If lwidentity was the only entry
    // and we removed that now, don't write
    // an empty entry into the file
    if(!enable && line != -1 && GetEntry(conf, line)->modules.size == 0)
    {
        GCE(ceError = InsertModule(conf, &distro, line, -1, "files"));
    }

cleanup:
    DJFreeDistroInfo(&distro);

    return ceError;
}

DWORD
DJConfigureNameServiceSwitch(const char *testPrefix, BOOLEAN enable)
{
    DWORD ceError = ERROR_SUCCESS;
    NsswitchConf conf;

    if(testPrefix == NULL)
        testPrefix = "";

    ceError = ReadNsswitchConf(&conf, testPrefix, TRUE);
    if(ceError == ERROR_FILE_NOT_FOUND)
    {
        ceError = ERROR_SUCCESS;
        DJ_LOG_WARNING("Warning: Could not find nsswitch file");
        goto cleanup;
    }
    GCE(ceError);

    ceError = UpdateNsswitchConf(&conf, enable);

    if(conf.modified)
        WriteNsswitchConfiguration(testPrefix, &conf);
    else
        DJ_LOG_INFO("nsswitch not modified");

cleanup:
    FreeNsswitchConfContents(&conf);

    return ceError;
}

const char *GetNameOfHostsByFile(const NsswitchConf *conf, const LwDistroInfo *distro)
{
    int line = FindEntry(conf, 0, "hosts");

    if (FindModuleOnLine(conf, line, "local") != -1)
        return "local";

    if (FindModuleOnLine(conf, line, "files") != -1)
        return "files";

    if(distro->os == OS_AIX)
        return "local";
    else
        return "files";
}

const char *GetNameOfHostsByDns(const NsswitchConf *conf, const LwDistroInfo *distro)
{
    int line = FindEntry(conf, 0, "hosts");

    if (FindModuleOnLine(conf, line, "dns") != -1)
        return "dns";

    if (FindModuleOnLine(conf, line, "bind") != -1)
        return "bind";

    if(distro->os == OS_AIX)
        return "bind";
    else
        return "dns";
}

//Does the platform-specific equivalent of this in nsswitch.conf:
// hosts: files dns
DWORD
DJConfigureHostsEntry(const char *testPrefix)
{
    DWORD ceError = ERROR_SUCCESS;
    NsswitchConf conf;
    LwDistroInfo distro;
    int line;
    const char *hostsByFile;
    const char *hostsByDns;
    int moduleIndex;

    if(testPrefix == NULL)
        testPrefix = "";

    memset(&distro, 0, sizeof(distro));
    memset(&conf, 0, sizeof(conf));

    GCE(ceError = DJGetDistroInfo(testPrefix, &distro));

    ceError = ReadNsswitchConf(&conf, testPrefix, TRUE);
    GCE(ceError);

    hostsByFile = GetNameOfHostsByFile(&conf, &distro);
    hostsByDns = GetNameOfHostsByDns(&conf, &distro);

    line = FindEntry(&conf, 0, "hosts");
    if(line == -1)
    {
        DJ_LOG_INFO("Adding hosts line");
        GCE(ceError = AddEntry(&conf, &distro, &line, "hosts"));
        GCE(ceError = InsertModule(&conf, &distro, line, 0, hostsByDns));
        GCE(ceError = InsertModule(&conf, &distro, line, 0, hostsByFile));
    }
    moduleIndex = FindModuleOnLine(&conf, line, hostsByFile);
    if(moduleIndex > 0)
    {
        /* The local module exists on the line, but it is not the first
         * entry. */
        GCE(ceError = RemoveModule(&conf, line, moduleIndex));
    }
    if(moduleIndex != 0)
    {
        GCE(ceError = InsertModule(&conf, &distro, line, 0, hostsByFile));
    }

    if(conf.modified)
        WriteNsswitchConfiguration(testPrefix, &conf);
    else
        DJ_LOG_INFO("nsswitch not modified");

cleanup:
    FreeNsswitchConfContents(&conf);
    DJFreeDistroInfo(&distro);

    return ceError;
}

#define APPARMOR_NSSWITCH "/etc/apparmor.d/abstractions/nameservice"

static DWORD
HasApparmor(BOOLEAN *hasApparmor)
{
    return CTCheckFileOrLinkExists(APPARMOR_NSSWITCH,
                hasApparmor);
}

static DWORD
IsApparmorConfigured(BOOLEAN *configured)
{
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN hasApparmor;

    *configured = FALSE;

    GCE(ceError = HasApparmor(&hasApparmor));
    if(hasApparmor)
    {
        GCE(ceError = CTCheckFileHoldsPattern(APPARMOR_NSSWITCH,
                    "pbis", configured));
    }
    else
    {
        *configured = TRUE;
    }

cleanup:

    return ceError;
}

static void ConfigureApparmor(BOOLEAN enable, LWException **exc)
{
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN hasApparmor;
    BOOLEAN configured;
    BOOLEAN usingMr;
    BOOLEAN removeLikewise = FALSE;
    FILE *file = NULL;
    PCSTR addString;
    PSTR restartPath = NULL;
    PSTR restartCommand = NULL;
    char *tempName = NULL;
    char *finalName = NULL;

    LW_CLEANUP_CTERR(exc, IsApparmorConfigured(&configured));
    if(configured == enable)
        goto cleanup;

    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(APPARMOR_NSSWITCH,
                &hasApparmor));
    if(!hasApparmor)
        goto cleanup;

    GCE(ceError = CTGetFileTempPath(
                        APPARMOR_NSSWITCH,
                        &finalName,
                        &tempName));

    LW_CLEANUP_CTERR(exc, CTCheckFileHoldsPattern(finalName,
                "mr,", &usingMr));

    if(usingMr)
        addString = 
PREFIXDIR "/lib/*.so*            mr,\n"
PREFIXDIR "/lib64/*.so*          mr,\n"
LOCALSTATEDIR "/lib/pbis/.lsassd  rw,\n";
    else
        addString =
PREFIXDIR "/lib/*.so*            r,\n"
PREFIXDIR "/lib64/*.so*          r,\n"
LOCALSTATEDIR "/lib/pbis/.lsassd  rw,\n";


    if(enable)
    {
        LW_CLEANUP_CTERR(exc, CTCopyFileWithOriginalPerms(finalName, tempName));
        LW_CLEANUP_CTERR(exc, CTOpenFile(tempName, "a", &file));
        LW_CLEANUP_CTERR(exc, CTFilePrintf(file, "\n# pbis\n%s# end pbis\n",
                    addString));

        CTSafeCloseFile(&file);

        LW_CLEANUP_CTERR(exc, CTSafeReplaceFile(finalName, tempName));
    }
    else
    {
        LW_CLEANUP_CTERR(exc, CTRunSedOnFile(finalName, finalName, FALSE, "/^[ \t]*#[ \t]*pbis[ \t]*$/,/^[ \t]*#[ \t]*end pbis[ \t]*$/d"));
        LW_CLEANUP_CTERR(exc, CTCheckFileHoldsPattern(finalName,
                    "end likewise", &removeLikewise));
        if (removeLikewise)
        {
            LW_CLEANUP_CTERR(exc, CTRunSedOnFile(finalName, finalName, FALSE, "/^[ \t]*#[ \t]*likewise[ \t]*$/,/^[ \t]*#[ \t]*end likewise[ \t]*$/d"));
        }
        LW_CLEANUP_CTERR(exc, CTRunSedOnFile(finalName, finalName, FALSE, "/^[ \t]*#[ \t]*centeris[ \t]*$/,/^[ \t]*#[ \t]*end centeris[ \t]*$/d"));
    }


    ceError = CTFindFileInPath("rcapparmor", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", &restartPath);
    if(ceError == ERROR_FILE_NOT_FOUND)
    {
        ceError = CTFindFileInPath("apparmor", "/etc/init.d/apparmor", &restartPath);
    }
    
    if(ceError == ERROR_FILE_NOT_FOUND)
    {
        ceError = ERROR_SUCCESS;
    }
    else if(!ceError)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&restartCommand,
                    "%s restart", restartPath));
        LW_TRY(exc, CTCaptureOutputToExc(restartCommand, &LW_EXC));
    }
    LW_CLEANUP_CTERR(exc, ceError);

cleanup:
    if(file != NULL)
    {
        CTCloseFile(file);
        CTRemoveFile(tempName);
    }
    CT_SAFE_FREE_STRING(restartPath);
    CT_SAFE_FREE_STRING(restartCommand);
    CT_SAFE_FREE_STRING(tempName);
    CT_SAFE_FREE_STRING(finalName);
}

static DWORD UnsupportedSeLinuxEnabled(BOOLEAN *hasBadSeLinux)
{
    BOOLEAN hasSeLinux;
    DWORD ceError = ERROR_SUCCESS;
    PSTR output = NULL;
    LwDistroInfo distro;

    *hasBadSeLinux = FALSE;
    memset(&distro, 0, sizeof(distro));

    GCE(ceError = CTCheckFileOrLinkExists("/usr/sbin/selinuxenabled", &hasSeLinux));
    if(!hasSeLinux)
        goto cleanup;

    GCE(ceError = CTCheckFileOrLinkExists("/usr/sbin/getenforce", &hasSeLinux));
    if(!hasSeLinux)
        goto cleanup;

    ceError = CTRunCommand("/usr/sbin/selinuxenabled >/dev/null 2>&1");
    if(ceError == ERROR_BAD_COMMAND)
    {
        //selinux is not enabled
        ceError = ERROR_SUCCESS;
        goto cleanup;
    }
    GCE(ceError);

    GCE(ceError = CTCaptureOutput("/usr/sbin/getenforce", &output));
    CTStripWhitespace(output);
    if(!strcmp(output, "Permissive"))
    {
        goto cleanup;
    }

    DJ_LOG_INFO("Selinux found to be present, enabled, and enforcing.");

    GCE(ceError = DJGetDistroInfo("", &distro));

    switch(distro.distro)
    {
        case DISTRO_CENTOS:
        case DISTRO_RHEL:
            if(distro.version[0] < '5')
            {
                DJ_LOG_INFO("Safe version of RHEL");
                goto cleanup;
            }
            break;
        case DISTRO_FEDORA:
            if(atol(distro.version) < 6)
            {
                DJ_LOG_INFO("Safe version of Fedora");
                goto cleanup;
            }
            break;
        default:
            goto cleanup;
    }
    *hasBadSeLinux = TRUE;

cleanup:
    if(ceError)
        *hasBadSeLinux = TRUE;

    CT_SAFE_FREE_STRING(output);
    DJFreeDistroInfo(&distro);

    return ceError;
}

static QueryResult QueryNsswitch(const JoinProcessOptions *options, LWException **exc)
{
    QueryResult result = FullyConfigured;
    BOOLEAN configured;
    BOOLEAN exists;
    BOOLEAN hasBadSeLinux;
    NsswitchConf conf;
    DWORD ceError = ERROR_SUCCESS;
    uid_t uid = 0;
    gid_t gid = 0;
    mode_t mode = 0;

    memset(&conf, 0, sizeof(conf));

    if (options->enableMultipleJoins)
    {
        result = NotApplicable;
        goto cleanup;
    }

    if (options->joiningDomain)
    {
        ceError = ReadNsswitchConf(&conf, NULL, FALSE);
        if(ceError == ERROR_FILE_NOT_FOUND)
        {
            ceError = ERROR_SUCCESS;
            DJ_LOG_WARNING("Warning: Could not find nsswitch file");
            goto cleanup;
        }
        LW_CLEANUP_CTERR(exc, ceError);
        
        LW_TRY(exc, result = RemoveCompat(&conf, NULL, &LW_EXC));
        if(result == CannotConfigure || result == NotConfigured)
        {
            goto cleanup;
        }

        LW_CLEANUP_CTERR(exc, CTGetOwnerAndPermissions(
            conf.filename, &uid, &gid, &mode));

        if ((mode & 0444) != 0444)
        {
            // The user has to fix the permissions
            result = CannotConfigure;
            goto cleanup;
        }

        LW_CLEANUP_CTERR(exc, UpdateNsswitchConf(&conf, TRUE));
        if(conf.modified)
        {
            LW_CLEANUP_CTERR(exc, UnsupportedSeLinuxEnabled(&hasBadSeLinux));
            if(hasBadSeLinux)
                result = CannotConfigure;
            else
                result = NotConfigured;
            goto cleanup;
        }
        
        LW_CLEANUP_CTERR(exc, DJHasMethodsCfg(&exists));

        if(exists)
        {
            LW_CLEANUP_CTERR(exc, DJIsMethodsCfgConfigured(&configured));

            if(!configured)
            {
                result = NotConfigured;
                goto cleanup;
            }
        }

        LW_CLEANUP_CTERR(exc, IsApparmorConfigured(&configured));

        if(!configured)
        {
            result = NotConfigured;
            goto cleanup;
        }
    }
    else
    {
        LW_CLEANUP_CTERR(exc, DJHasMethodsCfg(&exists));

        if(exists)
        {
            LW_CLEANUP_CTERR(exc, DJIsMethodsCfgConfigured(&configured));

            if(configured)
            {
                result = NotConfigured;
                goto cleanup;
            }
        }
    }

cleanup:

    FreeNsswitchConfContents(&conf);

    return result;
}

static void RestartDtloginIfRunning(JoinProcessOptions *options, LWException **exc)
{
    BOOLEAN doRestart;
    BOOLEAN inX;
    LWException *inner = NULL;
    LwDistroInfo distro;

    memset(&distro, 0, sizeof(distro));

    DJGetDaemonStatus("dtlogin", &doRestart, &inner);
    if(!LW_IS_OK(inner) && inner->code == ERROR_SERVICE_NOT_FOUND)
    {
        LW_HANDLE(&inner);
        goto cleanup;
    }
    LW_CLEANUP(exc, inner);

    if(doRestart)
    {
        /* Dtlogin will only be restarted if no one is logged in. */
        LW_CLEANUP_CTERR(exc, CTIsUserInX(&inX));

        if(inX)
        {
            doRestart = FALSE;
            /* If we're disabling domain logins, it isn't critical that
             * dtlogin is restarted. Without lwiauthd running, domain
             * users won't be able to log in anyway. */
            if(options->joiningDomain)
            {
                LW_CLEANUP_CTERR(exc, DJGetDistroInfo(NULL, &distro));
                if(distro.os == OS_SUNOS)
                {
                    if (options->warningCallback != NULL)
                    {
                        options->warningCallback(options, "Unable to restart dtlogin",
                            "The dtlogin process needs to be restarted for domain users to interactively login graphically, but it cannot be restarted at this time because a user is currently logged in. After the user exits, please run these commands as root, outside of an Xwindows session:\n\n"
                            "/etc/init.d/dtlogin stop\n"
                            "/etc/init.d/dtlogin start");
                    }
                }
                else if(distro.os == OS_HPUX)
                {
                    if (options->warningCallback != NULL)
                    {
                        options->warningCallback(options, "Unable to restart dtlogin",
                            "The dtlogin process needs to be restarted for domain users to interactively login graphically, but it cannot be restarted at this time because a user is currently logged in. After the user exits, please run these commands as root, outside of an Xwindows session:\n\n"
                            "/sbin/init.d/dtlogin.rc stop\n"
                            "/sbin/init.d/dtlogin.rc start");
                    }
                }
                else if(distro.os == OS_AIX)
                {
                    if (options->warningCallback != NULL)
                    {
                        options->warningCallback(options, "Unable to restart dtlogin",
                            "The dtlogin process needs to be restarted for domain users to interactively login graphically, but it cannot be restarted at this time because a user is currently logged in. After the user exits, please run these commands as root, outside of an Xwindows session:\n\n"
                            "kill `cat /var/dt/Xpid`\n"
                            "/etc/rc.dt");
                    }
                }
                else
                {
                    if (options->warningCallback != NULL)
                    {
                        options->warningCallback(options, "Unable to restart dtlogin",
                            "The dtlogin process needs to be restarted for domain users to interactively login graphically, but it cannot be restarted at this time because a user is currently logged in. After the user exits, please restart dtlogin.");
                    }
                }
            }
        }
    }
    if(doRestart)
    {
        LW_TRY(exc, DJStartStopDaemon("dtlogin", FALSE, &LW_EXC));
        LW_TRY(exc, DJStartStopDaemon("dtlogin", TRUE, &LW_EXC));
    }
cleanup:
    LW_HANDLE(&inner);
    DJFreeDistroInfo(&distro);
}

void
DoNsswitch(
    JoinProcessOptions *options,
    LWException **exc
    )
{
    LWException *restartException = NULL;
    NsswitchConf conf;
    DWORD ceError = ERROR_SUCCESS;

    memset(&conf, 0, sizeof(conf));

    LW_TRY(exc, ConfigureApparmor(options->joiningDomain, &LW_EXC));

    if(options->joiningDomain)
        LW_CLEANUP_CTERR(exc, DJFixMethodsConfigFile());
    else
        LW_CLEANUP_CTERR(exc, DJUnconfigMethodsConfigFile());

    ceError = ReadNsswitchConf(&conf, "", TRUE);
    if(ceError == ERROR_FILE_NOT_FOUND)
    {
        ceError = ERROR_SUCCESS;
        if (options->warningCallback != NULL)
        {
            options->warningCallback(options, "Could not find file", "Could not find nsswitch file");
        }
        goto cleanup;
    }
    LW_CLEANUP_CTERR(exc, ceError);

    if(options->joiningDomain)
    {
        LW_TRY(exc, RemoveCompat(&conf, NULL, &LW_EXC));
    }

    LW_CLEANUP_CTERR(exc, UpdateNsswitchConf(&conf, options->joiningDomain));

    if(conf.modified)
        LW_CLEANUP_CTERR(exc, WriteNsswitchConfiguration("", &conf));
    else
        DJ_LOG_INFO("nsswitch not modified");

    CTCaptureOutputToExc( SCRIPTDIR "/ConfigureLogin nsswitch_restart",
            &restartException);
    if(restartException != NULL && restartException->code == ERROR_BAD_COMMAND)
    {
        if (options->warningCallback)
        {
            options->warningCallback(options, "Some services require manual restart", restartException->longMsg);
        }
        LW_HANDLE(&restartException);
    }
    LW_CLEANUP(exc, restartException);

    LW_TRY(exc, RestartDtloginIfRunning(options, &LW_EXC));

    if (options->joiningDomain && options->warningCallback != NULL)
    {
        options->warningCallback(options,
                                 "System restart required",
                                 "Your system has been configured to authenticate to "
                                 "Active Directory for the first time.  It is recommended "
                                 "that you restart your system to ensure that all applications "
                                 "recognize the new settings.");
    }

cleanup:

    FreeNsswitchConfContents(&conf);
}

static PSTR GetNsswitchDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    PCSTR configureSteps;
    BOOLEAN hasBadSeLinux;
    QueryResult compatResult = FullyConfigured;
    PSTR compatDescription = NULL;
    NsswitchConf conf;
    DWORD ceError = ERROR_SUCCESS;

    memset(&conf, 0, sizeof(conf));

    LW_CLEANUP_CTERR(exc, UnsupportedSeLinuxEnabled(&hasBadSeLinux));
    if(hasBadSeLinux)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&ret,
"Your machine is using an unsupported SeLinux policy. This must be disabled before nsswitch can be modified to allow active directory users. Please run '/usr/sbin/setenforce Permissive' and then re-run this program."));
        goto cleanup;
    }

    ceError = ReadNsswitchConf(&conf, "", TRUE);
    if(ceError == ERROR_FILE_NOT_FOUND)
    {
        ceError = ERROR_SUCCESS;
        if (options->warningCallback != NULL)
        {
            options->warningCallback(options, "Could not find file", "Could not find nsswitch file");
        }
        goto cleanup;
    }
    LW_CLEANUP_CTERR(exc, ceError);

    if(options->joiningDomain)
    {
        LW_TRY(exc, compatResult = RemoveCompat(&conf, &compatDescription, &LW_EXC));
    }
    if(compatResult == FullyConfigured)
    {
        CT_SAFE_FREE_STRING(compatDescription);
        LW_CLEANUP_CTERR(exc, CTStrdup("", &compatDescription));
    }

    if (options->joiningDomain)
    {
        uid_t uid = 0;
        gid_t gid = 0;
        mode_t mode = 0;
        LW_CLEANUP_CTERR(exc, CTGetOwnerAndPermissions(
            conf.filename, &uid, &gid, &mode));

        if ((mode & 0444) != 0444)
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&ret,
"The permissions of 0%03o on %s are invalid. All users must have at least read permission for the file. You can fix this by running 'chmod a+r %s'.", (int)(mode&0777), conf.filename, conf.filename));
            goto cleanup;
        }
    }

    conf.modified = FALSE;
    LW_CLEANUP_CTERR(exc, UpdateNsswitchConf(&conf, options->joiningDomain));

    if(options->joiningDomain && conf.modified)
        configureSteps = 
"The following steps are required and can be performed automatically:\n"
"\t* Edit nsswitch apparmor profile to allow libraries in the " PREFIXDIR "/lib  and " PREFIXDIR "/lib64 directories\n"
"\t* List lwidentity module in /usr/lib/security/methods.cfg (AIX only)\n"
"\t* Add lwidentity to passwd and group/groups line /etc/nsswitch.conf or /etc/netsvc.conf\n";
    else if(conf.modified)
        configureSteps = 
"The following steps are required and can be performed automatically:\n"
"\t* Remove lwidentity module from /usr/lib/security/methods.cfg (AIX only)\n"
"\t* Remove lwidentity from passwd and group/groups line /etc/nsswitch.conf or /etc/netsvc.conf\n"
"The following step is optional:\n"
"\t* Remove apparmor exception for pbis nsswitch libraries\n";
    else
        configureSteps = "";

    if(strlen(compatDescription) || strlen(configureSteps))
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&ret,
"%s%sIf any changes are performed, then the following services must be restarted:\n"
"\t* GDM\n"
"\t* XDM\n"
"\t* Cron\n"
"\t* Dbus\n"
"\t* Nscd", compatDescription, configureSteps));
    }
    else
        LW_CLEANUP_CTERR(exc, CTStrdup("Fully Configured", &ret));

cleanup:
    CT_SAFE_FREE_STRING(compatDescription);
    FreeNsswitchConfContents(&conf);
    return ret;
}

const JoinModule DJNsswitchModule = { TRUE, "nsswitch", "enable/disable PowerBroker Identity Services nsswitch module", QueryNsswitch, DoNsswitch, GetNsswitchDescription };
