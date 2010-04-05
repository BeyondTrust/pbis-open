#ifndef __LWISYSLOG_UTILS_H__
#define __LWISYSLOG_UTILS_H__

BOOLEAN
IsRsyslogSupported();

BOOLEAN
IsSELinuxSupported();

CENTERROR
AddSELinuxExceptionForSyslog();

BOOLEAN
IsSysLogNG();

BOOLEAN
IsApparmorSupported();

CENTERROR
SendHUPToSyslog();

BOOLEAN
StringStartsWithStr(
    PCSTR pszString,
    PCSTR pszKey
    );

BOOLEAN
StringStartsWithChar(
    PCSTR pszString,
    CHAR  cKey
    );

VOID
StripTabspace(
    PSTR pszString
    );

VOID
FreeSyslogList(
    PSYSLOGNODE* ppMerged
    );

CENTERROR
CompareAndAppendNode(
    PSYSLOGNODE *ppMerged,
    PSYSLOGNODE pNewNode
    );

CENTERROR
CreateNewNode( 
    PSYSLOGNODE *pNewNode, 
    PSTR pszKey, 
    PSTR pszValue
    );

CENTERROR
AppendNodeToList(
    PSYSLOGNODE *ppMerged,
    PSYSLOGNODE pNewNode
    );

VOID
GetLastWord(
    PSTR pszString,
    PSTR pszTemp
    );

VOID
IsDuplicateEntry(
    PSYSLOGNODE *ppMasterList, 
    PSTR pszKey,
    PSTR pszValue,
    BOOLEAN* pbDuplicate
    );

VOID
FindClosingSymbol(
    PSTR pszStartPos, 
    PSTR *ppszPos
    );

VOID
StripStartAndEndChar(
    PSTR pszFirst,
    CHAR cStripChar,
    DWORD* pdwLen
    );

CENTERROR
MergeFileListAndADList(
    PSYSLOGNODE *ppListFromAD,
    PSYSLOGNODE *ppListFromFile
    );

#endif /* __LWISYSLOG_UTILS_H__ */
