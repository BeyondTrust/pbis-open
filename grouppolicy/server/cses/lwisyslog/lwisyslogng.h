#ifndef __LWISYSLOGNG_H__
#define __LWISYSLOGNG_H__

CENTERROR 
WriteToNGFileFromList (
    FILE *fp, 
    PSYSLOGNODE pNode,
    DWORD dwFilterCnt
    );

BOOLEAN
UpdateApparmorProfile (
    PSTR pszFileName
    );

CENTERROR
PrepareListFromNGFile(
    PCSTR pszOrigFile,
    PSYSLOGNODE *ppMerged
    );

CENTERROR
AppendNGEntryToList( 
    PSTR pszKey, 
    PSTR pszValue,
    PSYSLOGNODE* ppMasterList 
    );

#endif /* __LWISYSLOGNG_H__ */
