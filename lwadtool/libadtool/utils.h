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
 *        utils.h
 *
 * Abstract:
 *        Misc methods.
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Apr 14, 2010
 *
 */

#ifndef _AD_TOOL_UTILS_H_
#define _AD_TOOL_UTILS_H_

/**
 * Get name component from UPN or Domain\User string.
 *
 * @param logonName Name to parse.
 * @return Name component (Dynamically allocated.)
 */
extern PSTR GetNameComp(IN PSTR logonName);

/**
 * Get NETBIOS domain component from UPN, or Domain\User string.
 *
 * @param logonName Name to parse.
 * @return Name component (Dynamically allocated.)
 */
extern PSTR GetDomainCompFromUserName(IN PSTR logonName);

/**
 * Get realn component from UPN..
 *
 * @param logonName Name to parse.
 * @return Realm component (Dynamically allocated.)
 */
extern PSTR GetRealmComp(IN PSTR logonName);

/**
 * Check if str starts with the prefix.
 *
 * @param str String to check.
 * @param prefix Prefix.
 * @param ignoreCase Should be set to one if the case must be ignored during comparison.
 * @return TRUE if str starts with the prefix; FALSE - otherwise.
 */
extern BOOL DoesStrStartWith(IN PSTR str, IN PSTR prefix, IN INT ignoreCase);

/**
 * Check if str and str2 are equal.
 *
 * @param str String to compare.
 * @param str2 String to compare.
 * @param ignoreCase Should be set to one if the case must be ignored during comparison.
 * @return TRUE if str and str2 are equal; FALSE - otherwise.
 */
extern BOOL IsEqual(IN PSTR str, IN PSTR str2, IN INT ignoreCase);

/**
 * Check if str ends with the suffix.
 *
 * @param str String to check.
 * @param suffix Suffix.
 * @param ignoreCase Should be set to one if the case must be ignored during comparison.
 * @return TRUE if str ends with the suffix; FALSE - otherwise.
 */
extern BOOL DoesStrEndWith(IN PSTR str, IN PSTR suffix, IN INT ignoreCase);

/**
 * Get domain from DN. E.g. if passed OU=Users,DC=corpqa,DC=centeris,DC=com,
 * it will return corpqa.centeris.com.
 *
 * @param dn Distinguished name.
 * @param domain Domain
 * @return 0 on success; error code on failure.
 */
extern DWORD GetDomainFromDN(IN PSTR dn, OUT PSTR *domain);

/**
 * Get index of the specified character in a string.
 *
 * @param str String to check.
 * @param ch Character to find the index of.
 * @param ignoreCase Should be set to one if the case must be ignored during comparison.
 * @return The index or -1 if the string does not contain the character.
 */
extern INT GetFirstIndexOfChar(IN PSTR str, IN CHAR ch, IN INT ignoreCase);

/**
 * Read password from tty.
 *
 * @param passRet Read password.
 * @return 0 on success; error code on failure.
*/
extern DWORD ReadPasswdTty(IN PSTR* passRet);

/**
 * Process option with a "-" value.
 *
 * @param str Password option address.
 * @return 0 on success; error code on failure.
 */
extern DWORD ProcessDash(IN PSTR * str);

/**
 * Check if the string is a name-value pair.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
extern BOOL IsNVP(IN PSTR s);

/**
 * Check if '\' sign is present in the string.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
extern BOOL IsBackSlashPresent(IN PSTR s);

/**
 * Check if '/' sign is present in the string.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
extern inline BOOL IsSlashPresent(IN PSTR s);

/**
 * Check if dot is present in the string.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
extern inline BOOL IsDotPresent(IN PSTR s);

/**
 * Check if the string is DN component.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
extern BOOL IsDNComp(IN PSTR s);

/**
 * Check if the string is UPN.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
extern BOOL IsUPN(IN PSTR s);

/**
 * Check if the string is SPN (service principal name).
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
extern BOOL IsSPN(IN PSTR s);

/**
 * Check if domain component is specified in user name or it is a UPN.
 *
 * @param s String to check.
 * @return TRUE or FALSE.
 */
extern BOOL IsFullOrUPN(IN PSTR s);

/**
 * Check if all characters in the string are printable.
 *
 * @param str String to check.
 * @return TRUE is the string does not contain non-printable characters; FALSE otherwise.
 */
extern BOOL IsPrintable(IN PSTR str);

/**
 * Get parent DN. This method dynamically allocates memory, which must be freed.
 *
 * @param str DN to parse.
 * @param out parent DN.
 * @return 0 on success; error code on failure.
 */
extern DWORD GetParentDN(IN PSTR str, OUT PSTR *out);

/**
 * Get RDN. This method dynamically allocates memory, which must be freed.
 *
 * @param str DN to parse.
 * @param out RDN component.
 * @return 0 on success; error code on failure.
 */
extern DWORD GetRDN(IN PSTR str, OUT PSTR *out);

/**
 * Get Net BIOS domain name from a fully qualified name.
 *
 * @param str Name to parse.
 * @param out Domain component.
 * @return 0 on success; error code on failure.
 */
extern DWORD GetDomainComp(IN PSTR str, OUT PSTR *out);

/**
 * Spit the string using the specified separator. This method dynamically
 * allocates memory, which must be freed.
 *
 * @param s String to parse.
 * @param separator Separator character.
 * @param out Array of substrings.
 * @return 0 on success; error code on failure.
 */
extern DWORD SplitStr(IN PSTR s, IN CHAR separator, OUT PSTR **out);

/**
 * Concatenate the array of strings with an additional string. This method
 * dynamically allocates memory, which must be freed.
 *
 * @param s Array of strings.
 * @param strToAdd String to add.
 * @param out Reference to the resulting string.
 * @return 0 on success; error code on failure.
 */
extern DWORD StrArray2Str(IN PSTR *s, IN PSTR strToAdd, OUT PSTR *out);

/**
 * Check if the specified string is in the string array and return its index.
 *
 * @param s Array of strings.
 * @param str String to check.
 * @return index of the string in the array or -1 if the string is not found.
 */
extern INT IndexOfStrInArray(IN PSTR *s, IN PSTR str);

/**
 * Concatenate the array of strings excluding the string specified by index.
 * This method dynamically allocates memory, which must be freed.
 *
 * @param s Array of strings.
 * @param ind Index of the string to exclude.
 * @param out Reference to the resulting string.
 * @return 0 on success; error code on failure.
 */
extern DWORD StrArray2StrExcluding(IN PSTR *s, IN INT ind, OUT PSTR *out);

/**
 * Convert a string to unsigned long long int.
 *
 * @param pStr Pointer to a string to convert
 * @param base Convertion base
 * @param res  Converted value (result).
 * @return 0 on success; error code on error.
 */
extern DWORD Str2Ull(IN PCSTR pStr, IN register INT base, OUT unsigned long long int *res);

#endif /* _AD_TOOL_UTILS_H_ */
