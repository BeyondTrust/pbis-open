#ifndef __LWISYSLOG_WRITE_H__
#define __LWISYSLOG_WRITE_H__

BOOLEAN
InsertFileName (
    PSTR pszFileName
    );

void 
WriteNGOptions( 
    FILE *sngfp
    );

void 
WriteNGDefSrc(
    FILE *sngfp
    );

CENTERROR 
WriteToNGFile (
    FILE *fp, 
    PSTR facilities,
    PSTR dstPath,
    DWORD dwCnt
    );

CENTERROR
WriteToFile( 
    FILE *pHandle,
    PSTR pszKeyString,
    PSTR pszValueString
    );

#endif /* __LWISYSLOG_WRITE_H__ */
