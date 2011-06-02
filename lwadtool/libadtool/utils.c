/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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

/*
 * Module Name:
 *
 *        utils.c
 *
 * Abstract:
 *
 *        Utility methods.
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 17, 2010
 *
 */

#include "includes.h"

/**
 * Check if comma is present in the string.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
static inline BOOL IsCommaPresent(IN PSTR s) {
    INT i;

    for(i = 0; s && s[i]; ++i) {
        if(s[i] == ',') {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Check if '=' sign is present in the string.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
static inline BOOL IsEqPresent(IN PSTR s) {
    INT i;

    for(i = 0; s && s[i]; ++i) {
        if(s[i] == '=') {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Check if '@' sign is present in the string.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
static inline BOOL IsAtPresent(IN PSTR s) {
    INT i;

    for(i = 0; s && s[i]; ++i) {
        if(s[i] == '@') {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Check if dot is present in the string.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
inline BOOL IsDotPresent(IN PSTR s) {
    INT i;

    for(i = 0; s && s[i]; ++i) {
        if(s[i] == '.') {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Check if '\' sign is present in the string.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
inline BOOL IsBackSlashPresent(IN PSTR s) {
    INT i;

    for(i = 0; s && s[i]; ++i) {
        if(s[i] == '\\') {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Check if '/' sign is present in the string.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
inline BOOL IsSlashPresent(IN PSTR s) {
    INT i;

    for(i = 0; s && s[i]; ++i) {
        if(s[i] == '/') {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Check if the string is a name-value pair.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
inline BOOL IsNVP(IN PSTR s) {
    return IsEqPresent(s);
}

/**
 * Check if the string is DN component.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
inline BOOL IsDNComp(IN PSTR s) {
    return IsNVP(s) && IsCommaPresent(s);
}

/**
 * Check if the string is UPN.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
inline BOOL IsUPN(IN PSTR s) {
    return !IsNVP(s) && !IsCommaPresent(s) && IsAtPresent(s);
}

/**
 * Check if the string is SPN (service principal name).
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
inline BOOL IsSPN(IN PSTR s) {
    return DoesStrStartWith(s, "HOST/", 1);
}

/**
 * Check if domain component is specified in the name or it is a UPN.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
inline BOOL IsFullOrUPN(IN PSTR s) {
    return IsUPN(s) || IsBackSlashPresent(s);
}

/**
 * Get name component from UPN, Domain\User string, or NAME=VALUE pair.
 *
 * @param logonName Name to parse.
 * @return Name component (Dynamically allocated.)
 */
PSTR GetNameComp(IN PSTR logonName) {
    DWORD dwError = 0;
    PSTR name = NULL;
    int i;

    if(!logonName) {
        goto cleanup;
    }

    for(i = 0; i < strlen(logonName); ++i) {
        if(logonName[i] == '\\') {
            dwError = LwStrDupOrNull(logonName + i + 1, &name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            goto cleanup;
        }
    }

    for(i = 0; i < strlen(logonName); ++i) {
        if(logonName[i] == '@') {
            dwError = LwStrndup(logonName, i, &name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            goto cleanup;
        }
    }

    for(i = 0; i < strlen(logonName); ++i) {
        if(logonName[i] == '=') {
            dwError = LwStrndup(logonName + i + 1, strlen(logonName + i + 1), &name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            goto cleanup;
        }
    }

    for(i = 0; i < strlen(logonName); ++i) {
        if(logonName[i] == '/') {
            dwError = LwAllocateMemory(sizeof(CHAR) * (i + 1), OUT_PPVOID(&name));
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            strncpy(name, (PCSTR) logonName, i);
            goto cleanup;
        }
    }

    dwError = LwStrDupOrNull(logonName, &name);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    cleanup:
        return name;

    error:
        goto cleanup;
}

/**
 * Get NETBIOS domain component from UPN, or Domain\User string.
 *
 * @param logonName Name to parse.
 * @return Name component (Dynamically allocated.)
 */
PSTR GetDomainCompFromUserName(IN PSTR logonName) {
    DWORD dwError = 0;
    PSTR name = NULL;
    int i;

    if(!logonName) {
        goto cleanup;
    }

    for(i = 0; i < strlen(logonName); ++i) {
        if(logonName[i] == '\\') {
            dwError = LwAllocateMemory(sizeof(CHAR) * (i + 1), OUT_PPVOID(&name));
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            strncpy(name, (PCSTR) logonName, i);
            goto cleanup;
        }
    }

    for(i = 0; i < strlen(logonName); ++i) {
        if(logonName[i] == '@') {
            dwError = LwAllocateMemory(sizeof(CHAR) * (i + 1), OUT_PPVOID(&name));
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            strncpy(name, (PCSTR) logonName, i);
            goto cleanup;
        }
    }

    for(i = 0; i < strlen(logonName); ++i) {
        if(logonName[i] == '.') {
            dwError = LwAllocateMemory(sizeof(CHAR) * (i + 1), OUT_PPVOID(&name));
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            strncpy(name, (PCSTR) logonName, i);
            goto cleanup;
        }
    }

    cleanup:
        return name;

    error:
        goto cleanup;
}

/**
 * Get realm component from UPN..
 *
 * @param logonName Name to parse.
 * @return Realm component (Dynamically allocated.)
 */
PSTR GetRealmComp(IN PSTR logonName) {
    DWORD dwError = 0;
    PSTR name = NULL;
    int i;

    if(!logonName) {
        goto cleanup;
    }

    for(i = 0; i < strlen(logonName); ++i) {
        if(logonName[i] == '@') {
            dwError = LwStrDupOrNull(logonName + i + 1, &name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            goto cleanup;
        }
    }

    for(i = 0; i < strlen(logonName); ++i) {
        if(logonName[i] == '/') {
            dwError = LwAllocateMemory(sizeof(CHAR) * (i + 1), OUT_PPVOID(&name));
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            strncpy(name, (PCSTR) logonName, i);
            goto cleanup;
        }
    }

    cleanup:
        return name;

    error:
        goto cleanup;
}

/**
 * Check if str starts with the prefix.
 *
 * @param str String to check.
 * @param prefix Prefix.
 * @param ignoreCase Should be set to one if the case must be ignored during comparison.
 * @return TRUE if str starts with the prefix; FALSE - otherwise.
 */
BOOL DoesStrStartWith(IN PSTR str, IN PSTR prefix, IN INT ignoreCase)
{
    INT i;

    if(!str || !prefix || (strlen(prefix) > strlen(str))) {
        return FALSE;
    }

    for(i = 0; prefix[i]; ++i) {
        if(ignoreCase) {
            if(tolower(prefix[i]) != tolower(str[i])) {
                return FALSE;
            }
        }
        else {
            if(prefix[i] != str[i]) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/**
 * Check if str and str2 are equal.
 *
 * @param str String to compare.
 * @param str2 String to compare.
 * @param ignoreCase Should be set to one if the case must be ignored during comparison.
 * @return TRUE if str and str2 are equal; FALSE - otherwise.
 */
BOOL IsEqual(IN PSTR str, IN PSTR str2, IN INT ignoreCase)
{
    INT i;

    if(strlen(str2) != strlen(str)) {
        return FALSE;
    }

    for(i = 0; str2[i]; ++i) {
        if(ignoreCase) {
            if(tolower(str2[i]) != tolower(str[i])) {
                return FALSE;
            }
        }
        else {
            if(str2[i] != str[i]) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/**
 * Get domain from DN. E.g. if passed OU=Users,DC=corpqa,DC=centeris,DC=com,
 * it will return corpqa.centeris.com.
 *
 * @param dn Distinguished name.
 * @param domain Domain
 * @return 0 on success; error code on failure.
 */
DWORD GetDomainFromDN(IN PSTR dn, OUT PSTR *domain)
{
    DWORD dwError = 0;
    PSTR buf = NULL;
    PSTR bufp = 0;
    PSTR dcp = NULL;
    PSTR commap = NULL;
    PSTR ndn = NULL;
    int len = 0;

    dwError = LwStrDupOrNull(dn, &ndn);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    LwStrToLower(ndn);

    dwError = LwAllocateMemory(sizeof(CHAR) * (strlen(ndn) + 1), OUT_PPVOID(&buf));
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    bufp = buf;
    commap = ndn;
    dcp = ndn;

    while(dcp && (dcp = strstr((PCSTR) dcp, "dc="))) {
        if(!dcp) {
            break;
        }

        dcp += 3;

        if(*dcp == '\0') {
            break;
        }

        commap = strstr((PCSTR) dcp, ",");

        if(commap == NULL) {
            len = strlen(dcp);
        }
        else {
            len = commap - dcp;
        }

        if(bufp != buf) {
            strcpy(bufp, ".");
            ++bufp;
        }

        strncpy(bufp, (PCSTR) dcp, len);
        bufp += len;
        dcp += len;
    }

    if(bufp == buf) {
        dwError = ADT_ERR_INVALID_ARG;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = LwStrDupOrNull(buf, domain);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    cleanup:
        LW_SAFE_FREE_MEMORY(buf);
        LW_SAFE_FREE_MEMORY(ndn);
        return dwError;

    error:
        goto cleanup;
}

/**
 * Check if str ends with the suffix.
 *
 * @param str String to check.
 * @param suffix Suffix.
 * @param ignoreCase Should be set to one if the case must be ignored during comparison.
 * @return TRUE if str ends with the suffix; FALSE - otherwise.
 */
BOOL DoesStrEndWith(IN PSTR str, IN PSTR suffix, IN INT ignoreCase)
{
    INT i, j;

    if(!str || !suffix || (strlen(suffix) > strlen(str))) {
        return FALSE;
    }

    for(i = strlen(suffix) - 1, j = strlen(str) - 1; i >= 0; --i, --j) {
        if(suffix[i] == '\n') {
            ++j;
            continue;
        }

        if(str[j] == '\n') {
            ++i;
            continue;
        }

        if(ignoreCase) {
            if(tolower(suffix[i]) != tolower(str[j])) {
                return FALSE;
            }
        }
        else {
            if(suffix[i] != str[j]) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/**
 * Get index of the specified character in a string.
 *
 * @param str String to check.
 * @param ch Character to find the index of.
 * @param ignoreCase Should be set to one if the case must be ignored during comparison.
 * @return The index or -1 if the string does not contain the character.
 */
INT GetFirstIndexOfChar(IN PSTR str, IN CHAR ch, IN INT ignoreCase)
{
    INT i;

    for(i = 0; str && str[i]; ++i) {
        if(ignoreCase) {
            if(tolower(str[i]) == tolower(ch)) {
                return i;
            }
        }
        else {
            if(str[i] == ch) {
                return i;
            }
        }
    }

    return -1;
}

/**
 * Check if all characters in the string are printable.
 *
 * @param str String to check.
 * @return TRUE is the string does not contain non-printable characters; FALSE otherwise.
 */
BOOL IsPrintable(IN PSTR str)
{
    INT i;

    for(i = 0; str && str[i]; ++i) {
        if(!isprint((int) (str[i]))) {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * Get parent DN. This method dynamically allocates memory, which must be freed.
 *
 * @param str DN to parse.
 * @param out parent DN.
 * @return 0 on success; error code on failure.
 */
DWORD GetParentDN(IN PSTR str, OUT PSTR *out)
{
    DWORD dwError = 0;
    INT len = 0;
    INT ind = 0;

    if(!str || !str[0]) {
        *out = NULL;
        goto cleanup;
    }

    if(!IsCommaPresent(str)) {
        dwError = LwStrDupOrNull("", out);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        goto cleanup;
    }

    ind = GetFirstIndexOfChar(str, ',', 1);
    len = strlen(str) - ind;

    dwError = LwAllocateMemory(sizeof(CHAR) * (len + 1), (PVOID *) out);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    strncpy(*out, str + ind + 1, len - 1);

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Get RDN. This method dynamically allocates memory, which must be freed.
 *
 * @param str DN to parse.
 * @param out RDN component.
 * @return 0 on success; error code on failure.
 */
DWORD GetRDN(IN PSTR str, OUT PSTR *out)
{
    DWORD dwError = 0;
    INT len = 0;

    if(!str || !str[0]) {
        *out = NULL;
        goto cleanup;
    }

    if(!IsCommaPresent(str)) {
        dwError = LwStrDupOrNull(str, out);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        goto cleanup;
    }

    len = GetFirstIndexOfChar(str, ',', 1);

    dwError = LwAllocateMemory(sizeof(CHAR) * (len + 1), (PVOID *) out);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    strncpy(*out, str, len);

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Get Net BIOS domain name from a fully qualified name.
 *
 * @param str Name to parse.
 * @param out Domain component.
 * @return 0 on success; error code on failure.
 */
DWORD GetDomainComp(IN PSTR str, OUT PSTR *out)
{
    DWORD dwError = 0;
    INT len = 0;

    if(!str || !str[0]) {
        *out = NULL;
        goto cleanup;
    }

    if(!IsDotPresent(str)) {
        dwError = LwStrDupOrNull(str, out);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        goto cleanup;
    }

    len = GetFirstIndexOfChar(str, '.', 1);

    dwError = LwAllocateMemory(sizeof(CHAR) * (len + 1), (PVOID *) out);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    strncpy(*out, str, len);

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Spit the string using the specified separator. This method dynamically
 * allocates memory, which must be freed.
 *
 * @param s String to parse.
 * @param separator Separator character.
 * @param out Array of substrings.
 * @return 0 on success; error code on failure.
 */
DWORD SplitStr(IN PSTR s, IN CHAR separator, OUT PSTR **out)
{
    DWORD dwError = 0;
    INT count;
    INT i, j, k;
    INT *len = NULL;
    INT ind;

    if(!s || !s[0]) {
        *out = NULL;
        goto cleanup;
    }

    for(count = 0, i = 0; s[i] != 0; ++i) {
        if(s[i] == separator) {
            ++count;
        }
    }

    dwError = LwAllocateMemory(sizeof(PSTR) * (count + 2), OUT_PPVOID(out));
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    dwError = LwAllocateMemory(sizeof(INT) * (count + 2), OUT_PPVOID(&len));
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    for(ind = 0, i = 0, j = 0; s[i] != 0; ++i) {
        if(s[i] == separator) {
            len[ind++] = i - j;
            j = i + 1;
        }
    }

    len[ind] = i - j;

    dwError = LwAllocateMemory(sizeof(CHAR) * (len[0] + 1),
                               (PVOID *) &((*out)[0]));

    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    for (i = 0, k = 0, count = 0; s[i] != 0; ++i, ++k) {
        if (s[i] == separator) {
            (*out)[count++][k] = '\0';
            k = -1;

            dwError = LwAllocateMemory(sizeof(CHAR) * (len[count] + 1),
                                       (PVOID *) (&(*out)[count]));
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            continue;
        }

        (*out)[count][k] = s[i];
    }

    (*out)[count++][k] = '\0';
    (*out)[count] = NULL;

    cleanup:
        LW_SAFE_FREE_MEMORY(len);
        return dwError;

    error:
        goto cleanup;
}

/**
 * Concatenate the array of strings with an additional string. This method
 * dynamically allocates memory, which must be freed.
 *
 * @param s Array of strings.
 * @param strToAdd String to add.
 * @param out Reference to the resulting string.
 * @return 0 on success; error code on failure.
 */
DWORD StrArray2Str(IN PSTR *s, IN PSTR strToAdd, OUT PSTR *out)
{
    DWORD dwError = 0;
    INT i, j, k;
    INT len = 0;

    if(!s && !strToAdd) {
       out = NULL;
       goto cleanup;
    }

    for(i = 0; s && s[i]; ++i) {
        len += strlen(s[i]) + 1;
    }

    if(strToAdd) {
        len += strlen(strToAdd) + 1;
    }

    dwError = LwAllocateMemory(sizeof(CHAR) * len, (PVOID *) out);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    for(i = 0, k = 0; s && s[i]; ++i) {
        for(j = 0; s[i][j]; ++j) {
            (*out)[k++] = s[i][j];
        }

        (*out)[k++] = ';';
    }

    if(!strToAdd) {
        (*out)[--k] = '\0';
    }
    else {
        for(i = 0; strToAdd[i]; ++i) {
            (*out)[k++] = strToAdd[i];
        }

        (*out)[k] = '\0';
    }

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Concatenate the array of strings excluding the string specified by index.
 * This method dynamically allocates memory, which must be freed.
 *
 * @param s Array of strings.
 * @param ind Index of the string to exclude.
 * @param out Reference to the resulting string.
 * @return 0 on success; error code on failure.
 */
DWORD StrArray2StrExcluding(IN PSTR *s, IN INT ind, OUT PSTR *out)
{
    DWORD dwError = 0;
    INT i, j, k;
    INT len = 0;
    INT count;

    if(!s) {
       out = NULL;
       goto cleanup;
    }

    for(i = 0, count = 0; s && s[i]; ++i) {
        if(i == ind) {
            continue;
        }

        len += strlen(s[i]) + 1;
        ++count;
    }

    if(count == 0) {
        out = NULL;
        goto cleanup;
    }

    ++len;

    dwError = LwAllocateMemory(sizeof(CHAR) * len, OUT_PPVOID(out));
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    for(i = 0, k = 0; s && s[i]; ++i) {
        if(i == ind) {
            continue;
        }

        for(j = 0; s[i][j]; ++j) {
            (*out)[k++] = s[i][j];
        }

        (*out)[k++] = ';';
    }

    (*out)[--k] = '\0';

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Check if the specified string is in the string array and return its index.
 *
 * @param s Array of strings.
 * @param str String to check.
 * @return index of the string in the array or -1 if the string is not found.
 */
INT IndexOfStrInArray(IN PSTR *s, IN PSTR str)
{
    BOOL ret = -1;
    INT i, j;

    for(i = 0; s && s[i]; ++i) {
        if(strlen(s[i]) != strlen(str)) {
            continue;
        }

        for(j = 0; s[i][j]; ++j) {
            if(s[i][j] != str[j]) {
                break;
            }
        }

        if(!str[j]) {
            ret = i;
            break;
        }
    }

    return ret;
}

/**
 * Process option with a "-" value.
 *
 * @param str Password option address.
 * @return 0 on success; error code on failure.
 */
DWORD ProcessDash(IN PSTR * str) {
    DWORD dwError = 0;
    PSTR buf = NULL;
    INT len = 129;
    INT nLen = 0;

    if(str && *str && !strcmp((PCSTR) *str, "-")) {
        LW_SAFE_FREE_MEMORY(*str);

        dwError = LwAllocateMemory(len, OUT_PPVOID(&buf));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

        *str = fgets(buf, len - 1, stdin);
        
        nLen = strlen(*str);
        
        if((nLen > 1) && ((*str)[nLen - 1] == '\n')) {
            (*str)[nLen - 1] = (char) 0;
        }
    }

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Print a message.
 *
 * @param appContext Application context reference.
 * @param dst Can be stdout, stderr, or NULL - execution result string.
 * @param level Log level.
 * @param str The formated content will be added to this string. Can be NULL.
 * @param format Format
 * @param msgList Arguments
 */
static VOID
PrintMessage(IN AppContextTP appContext, IN FILE *dst, IN LogLevelT level, IN PCSTR format, va_list msgList)
{
    DWORD dwError = 0;
    PSTR str = NULL;
    PSTR addStr = NULL;
    PSTR retStr = NULL;
    INT  len = 0;
    PSTR res = NULL;

    if(appContext == NULL) {
        vfprintf(stderr, format, msgList);
        return;
    }

    if ((dst != 0) && (level > appContext->gopts.logLevel)) {
        return;
    }

    if((level == LogLevelError) && (appContext->gopts.logLevel < LogLevelVerbose)) {
        return; // Do not print detailed error messages unless we are tracing a problem
    }

    if(format == NULL) {
        return;
    }

    if(appContext->outputMode == AdtOutputModeStdout) {
        vfprintf((dst == 0) ? stdout : dst, format, msgList);
        return;
    }

    if(dst == stdout) {
        str = appContext->stdoutStr;
    }
    else {
        if (dst == stderr) {
            str = appContext->stderrStr;
        }
        else {
            str = appContext->execResult.base.resultStr;
        }
    }

    dwError = LwAllocateStringPrintfV(&addStr, format, msgList);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    if (str == NULL) {
        res = addStr;
    }
    else {
        len += strlen(addStr);
        len += strlen(str);

        dwError = LwAllocateMemory(len + 1, OUT_PPVOID(&retStr));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

        res = retStr;
        strcpy(retStr, str);
        strcpy(retStr + strlen(str), addStr);
        LW_SAFE_FREE_MEMORY(addStr);
        LW_SAFE_FREE_MEMORY(str);
    }

    if(dst == stdout) {
        appContext->stdoutStr = res;
    }
    else {
        if (dst == stderr) {
            appContext->stderrStr = res;
        }
        else {
            str = appContext->execResult.base.resultStr = res;
        }
    }
    return;

    error:
        fprintf(stderr, "%s at %s:%d\n", AdtGetErrorMsg(ADT_ERR_FAILED_ALLOC), __FILE__, __LINE__);
}

/**
 * Get string presentation of the log level.
 *
 * @param level Log level
 * @return String presentation of the log level or an empty string if the
 * level is none.
 */
PCSTR LogLevel2Str(IN LogLevelT level)
{
    PCSTR ret = NULL;

    switch(level) {
        case LogLevelError:
            ret = "ERROR";
            break;

        case LogLevelWarning:
            ret = "WARN";
            break;

        case LogLevelInfo:
            ret = "INFO";
            break;

        case LogLevelVerbose:
            ret = "VERBOSE";
            break;

        case LogLevelTrace:
            ret = "TRACE";
            break;

        case LogLevelNone:
        default:
            break;
    }

    return ret;
}

/**
 * Print a message to stdout.
 *
 * @param appContext Application context reference.
 * @param level Log level.
 * @param str The formated content will be added to this string. Can be NULL.
 * @param format Format
 * @param ... Arguments
 */
VOID PrintStdout(IN AppContextTP appContext, IN LogLevelT level, IN PCSTR format, ...)
{
    va_list msgList;
    va_start(msgList, format);

    PrintMessage(appContext, stdout, level, format, msgList);

    va_end(msgList);
}

/**
 * Print a message to stderr.
 *
 * @param appContext Application context reference.
 * @param level Log level.
 * @param str The formated content will be added to this string. Can be NULL.
 * @param format Format
 * @param ... Arguments
 */
static VOID _PrintStderr(IN AppContextTP appContext, IN LogLevelT level, IN PCSTR format, ...)
{
    va_list msgList;
    va_start(msgList, format);

    PrintMessage(appContext, stderr, level, format, msgList);

    va_end(msgList);
}

/**
 * Print a message to stderr.
 *
 * @param appContext Application context reference.
 * @param level Log level.
 * @param str The formated content will be added to this string. Can be NULL.
 * @param format Format
 * @param ... Arguments
 */
VOID PrintStderr(IN AppContextTP appContext, IN LogLevelT level, IN PCSTR format, ...)
{
    _PrintStderr(appContext, level, "%s:", LogLevel2Str(level));

    va_list msgList;
    va_start(msgList, format);

    PrintMessage(appContext, stderr, level, format, msgList);

    va_end(msgList);
}

/**
 * Print a message to the action execution result string or stdout.
 *
 * @param appContext Application context reference.
 * @param level Log level.
 * @param str The formated content will be added to this string. Can be NULL.
 * @param format Format
 * @param ... Arguments
 */
VOID PrintResult(IN AppContextTP appContext, IN LogLevelT level, IN PCSTR format, ...)
{
    va_list msgList;
    va_start(msgList, format);

    PrintMessage(appContext, 0, level, format, msgList);

    va_end(msgList);
}

/**
 * Get current NT time.
 * The resulting string is dynamically allocated. Must be freed.
 *
 * @return NT time.
 */
PSTR GetCurrentNtTime()
{
    DWORD dwError = 0;
    time_t utime = 0;
    UINT64 result = 0;
    PSTR resultStr = NULL;

    utime = time(NULL);

    result = (utime + 11644473600LL) * 10000000LL;

    dwError = LwAllocateStringPrintf(&resultStr, "%lld", result);

    if(dwError) {
        return NULL;
    }

    return resultStr;
}

/*
 * Convert a string to unsigned long long int.
 *
 * @param pStr Pointer to a string to convert
 * @param base Convertion base
 * @param res  Converted value (result).
 * @return 0 on success; error code on error.
 */
DWORD
Str2Ull(IN PCSTR pStr, IN register INT base, OUT unsigned long long int *res)
{
    DWORD dwError = 0;
    register INT negative, any;
    register UCHAR c;
    register PCSTR s = pStr;

    s = pStr;

    do {
        c = *s++;
    }
    while (isspace(c));

    if (c == '-') {
        c = *s++;
        negative = 1;
    }
    else {
        if (c == '+') {
            c = *s++;
        }

        negative = 0;
    }

    if ((base == 0 || base == 16) && (c == '0') && ((*s == 'x') || (*s == 'X'))) {
        c = s[1];
        s += 2;
        base = 16;
    }

    if (base == 0) {
        base = c == '0' ? 8 : 10;
    }

    register unsigned long long int aBase = (UINT) base;
    register unsigned long long int cut = (unsigned long long int) ULLONG_MAX / aBase;
    register INT cutLimit = (unsigned long long int) ULLONG_MAX % aBase;

    for (*res = 0, any = 0;; c = *s++) {
        if (!isascii(c)) {
            break;
        }

        if (isdigit(c)) {
            c -= '0';
        }
        else {
            if (isalpha(c)) {
                c -= isupper(c) ? 'A' - 10 : 'a' - 10;
            }
            else {
                break;
            }
        }

        if (c >= base) {
            break;
        }

        if ((any < 0) || (*res > cut) || ((*res == cut) && (c > cutLimit))) {
            any = -1;
        }
        else {
            *res *= aBase;
            *res += c;
            any = 1;
        }
    }

    if (any < 0) {
        dwError = ERANGE;
        goto error;
    }
    else {
        if (negative) {
            *res *= -1;
        }
    }

    cleanup:
        return dwError;

    error:
        goto cleanup;
}
