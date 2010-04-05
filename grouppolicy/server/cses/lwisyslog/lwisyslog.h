#ifndef __LWISYSLOG_H__
#define __LWISYSLOG_H__

CENTERROR
AppendEntryToList(
    PSTR pszKey,
    PSTR pszValue,
    PSYSLOGNODE* ppListFromAD
    );

CENTERROR
PrepareListFromFile(
    PCSTR pszOrigFile,
    PSYSLOGNODE *ppMerged
    );

CENTERROR 
ComputeSyslogKey( 
    PSTR pszKeyString,
    PSTR *ppszKey
    );

CENTERROR 
ComputeSyslogValue( 
    PSTR pszValueString,
    PSTR *ppszValue
    );

CENTERROR
FormatString(
    PSTR pszStr,
    PSTR pszNewStr
    );

#endif /* __LWISYSLOG_H__ */
