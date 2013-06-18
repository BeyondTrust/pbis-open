/*-----------------------------------------------------------------------------
 * $RCSfile: fio.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include "fio.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(WIN32)
#else
#include <unistd.h>
#endif
#include <iostream>
#include <stdlib.h>
#include "AppLogger.h"
#include "SrvErrorCode.h"

using namespace std;

static const char* s_szCharSet = "UTF-8";

/***************************************************************************** 
    Function      : Load File to Memory
    Return        : unsigned char*(call free() afer use)
 ************************************************ Yuji Yamawaki 01.07.27 *****/
unsigned char* LoadFile
(const char*    szFileName, /* (i)  File Name */
 unsigned long* pulSize)    /* (o)  File Size(Bytes) */
{
    unsigned char* szNew = NULL;
    FILE*          fp = NULL;
#if defined(WIN32)
    struct _stat    statBuf;
#else
    struct stat    statBuf;
#endif
    /* Check Arguments */
    if (szFileName == NULL || pulSize == NULL)
        return NULL;
    /* Get File Size */
#if defined(WIN32)
    if (_stat(szFileName, &statBuf) != 0) {
#else
    if (stat(szFileName, &statBuf) != 0) {
#endif
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s(%s)"
								,"File Not Found"
								, szFileName);
        goto FuncEnd;
    }
    *pulSize = (unsigned long)statBuf.st_size;
    if (*pulSize == 0) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s = %lu"
								,"File Size"
								, *pulSize);
        goto FuncEnd;
    }
    /* Allocate Area */
    if ((szNew = (unsigned char*) malloc(*pulSize)) == NULL) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s(size:%lu)"
								,"Memory Allocate Error"
								, *pulSize);
        goto FuncEnd;
    }
    /* Open and Load File */
    if ((fp = fopen(szFileName, "rb")) == NULL) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s(%s)"
								,"File Open Error"
								,szFileName);
        free(szNew);
        szNew = NULL;
        goto FuncEnd;
    }
    if (fread(szNew, *pulSize, 1, fp) != 1) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s(%s)"
								,"File Read Error"
								,szFileName);
        free(szNew);
        szNew = NULL;
        goto FuncEnd;
    }
    fclose(fp);
    fp = NULL;
FuncEnd:
    if (fp != NULL)
        fclose(fp);
    return szNew;
}
/***************************************************************************** 
    Function      : Save Memory To File
    Return        : int (0: OK, Others: NG)
 ************************************************ Yuji Yamawaki 01.07.27 *****/
int SaveFile
(unsigned char* szMemData,      /* (i)  Data In Memory */
 unsigned long  ulMemSize,      /* (i)  Data Size(Bytes) */
 const char*    szFileName)     /* (i)  File Name */
{
    FILE* fp = NULL;
    int   iRet = 0;
    /* Check Arguments */
    if (szMemData == NULL || ulMemSize == 0 || szFileName == NULL)
        return -1;
    /* Open File */
    if ((fp = fopen(szFileName, "wb")) == NULL) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s(%s)"
								,"File Open Error"
								,szFileName);
        iRet = -1;
        goto FuncEnd;
    }
    /* Write */
    if (fwrite(szMemData, ulMemSize, 1, fp) != 1) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s(%s)"
								,"File Write Error"
								,szFileName);
        iRet = -1;
        goto FuncEnd;
    }
    /* End */
    fclose(fp);
    fp = NULL;

FuncEnd:
    if (fp != NULL)
        fclose(fp);
    return iRet;
}
/***************************************************************************** 
    Function      : Save Memory To File With New Line
    Return        : int (0: OK, Others: NG)
 ************************************************ Yuji Yamawaki 01.07.27 *****/
int SaveFileWithNewLine
(unsigned char* szMemData,      /* (i)  Data In Memory */
 unsigned long  ulMemSize,      /* (i)  Data Size(Bytes) */
 int            nCharPerLine,   /* (i)  Character Count Per Line */
 const char*    szFileName)     /* (i)  File Name */
{
    FILE* fp = NULL;
    int   iRet = 0;
    unsigned long  ulRest;
    unsigned char* szCur;
    unsigned long  ulWriteSize;

    /* Check Arguments */
    if (szMemData == NULL || ulMemSize == 0 ||
        nCharPerLine <= 0 || szFileName == NULL)
        return -1;
    /* Open File */
    if ((fp = fopen(szFileName, "wb")) == NULL) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s(%s)"
								,"File Open Error"
								,szFileName);
        iRet = -1;
        goto FuncEnd;
    }
    /* Write */
    ulRest = ulMemSize;
    szCur = szMemData;
    while (ulRest > 0) {
        if (ulRest > (unsigned long)nCharPerLine) {
            ulWriteSize = (unsigned long)nCharPerLine;
        } else {
            ulWriteSize = ulRest;
        }
        if (fwrite(szCur, ulWriteSize, 1, fp) != 1) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s(%s)"
								,"File Write Error"
								,szFileName);
            iRet = -1;
            goto FuncEnd;
        }
        fputc('\n', fp);
        szCur += ulWriteSize;
        ulRest -= ulWriteSize;
    }
    /* End */
    fclose(fp);
    fp = NULL;

FuncEnd:
    if (fp != NULL)
        fclose(fp);
    return iRet;
}
/***************************************************************************** 
    Function      : SOAP Envelope のセーブ
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.26 *****/
int SaveEnvelope
(const OpenSOAPEnvelopePtr soap_env,  /* (i)  SOAP Envelope */
 const char*               szFName)   /* (i)  File Name */
{
    int iRet = OPENSOAP_PARAMETER_BADVALUE;
    OpenSOAPByteArrayPtr soap_env_data = NULL;
    FILE* fp = NULL;
    size_t sizData;
    const unsigned char* szDataTop;

    if (!soap_env || !szFName) {
        /* Bad Argument */
        goto FuncEnd;
    }
    iRet = OpenSOAPByteArrayCreate(&soap_env_data);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    /* Envelope -> ByteArray */
    iRet = OpenSOAPEnvelopeGetCharEncodingString(soap_env,
                                                 s_szCharSet,
                                                 soap_env_data);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    /* Get Byte Array Size */
    iRet = OpenSOAPByteArrayGetSize(soap_env_data,
                                    &sizData);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    /* Get Byte Array Top */
    iRet = OpenSOAPByteArrayBeginConst(soap_env_data,
                                       &szDataTop);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    /* Save To File */
    fp = fopen(szFName, "wb");
    if (fp == NULL) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s[%s]"
								,"Cannot Open File "
								,szFName);
        iRet = OPENSOAP_PARAMETER_BADVALUE;
        goto FuncEnd;
    }
    if (fwrite(szDataTop, 1, sizData, fp) != sizData) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s[%s]"
								,"Write Error File "
								,szFName);
        iRet = OPENSOAP_PARAMETER_BADVALUE;
        goto FuncEnd;
    }
FuncEnd:
    OpenSOAPByteArrayRelease(soap_env_data);
    if (fp != NULL) {
        fclose(fp);
    }
    if (OPENSOAP_FAILED(iRet)) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s %s 0x%04x"
								,"saveEnvelope() Error."
								,"OpenSOAP Error Code: "
								,iRet);
    }
    return iRet;
}

/***************************************************************************** 
    Function      : SOAP Envelope のセーブ
    Return        : int
 ************************************************ Yuichi SATO *****/
int SaveEnvelope
(const OpenSOAPEnvelopePtr soap_env,  /* (i)  SOAP Envelope */
 std::string&              szString)   /* (i)  File Name */
{
    int iRet = OPENSOAP_PARAMETER_BADVALUE;
    OpenSOAPByteArrayPtr soap_env_data = NULL;
    FILE* fp = NULL;
    size_t sizData;
    const unsigned char* szDataTop;

    if (!soap_env) {
        /* Bad Argument */
        goto FuncEnd;
    }
    iRet = OpenSOAPByteArrayCreate(&soap_env_data);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    /* Envelope -> ByteArray */
    iRet = OpenSOAPEnvelopeGetCharEncodingString(soap_env,
                                                 s_szCharSet,
                                                 soap_env_data);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    /* Get Byte Array Size */
    iRet = OpenSOAPByteArrayGetSize(soap_env_data,
                                    &sizData);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    /* Get Byte Array Top */
    iRet = OpenSOAPByteArrayBeginConst(soap_env_data,
                                       &szDataTop);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    /* Copy to String */
    szString = std::string((char*)szDataTop);
    
FuncEnd:
    OpenSOAPByteArrayRelease(soap_env_data);
    if (fp != NULL) {
        fclose(fp);
    }
    if (OPENSOAP_FAILED(iRet)) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s %s 0x%04x"
								,"saveEnvelope() Error."
								,"OpenSOAP Error Code: "
								,iRet);
    }
    return iRet;
}

/***************************************************************************** 
    Function      : SOAP Envelope のロード
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.26 *****/
int LoadEnvelope
(const char*          szFName,   /* (i)  Envelopeファイル名 */
 OpenSOAPEnvelopePtr* pEnv)      /* (o)  Envelope */
{
    int                  iRet = OPENSOAP_PARAMETER_BADVALUE;
    unsigned char*       szData = NULL; /* Envelopeデータ */
    unsigned long        ulSize;
    OpenSOAPByteArrayPtr barray = NULL;

	unsigned int i = 0;

    /* 引数のチェック */
    if (szFName == NULL || pEnv == NULL) {
        goto FuncEnd;
    }
    /* Envelopeファイルのロード */
    szData = LoadFile(szFName, &ulSize);
    if (szData == NULL) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"Insufficient Memory.\n");
        goto FuncEnd;
    }

#ifdef DEBUG
    cerr << "szData = [" << endl;
    for(i = 0; i < ulSize; i++) {
      cerr << szData[i];
    }
    cerr << "]" << endl;
#endif //DEBUG
    
    /* OpenSOAPByteArrayPtrに変換 */
    iRet = OpenSOAPByteArrayCreateWithData(szData,
                                           (size_t)ulSize,
                                           &barray);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
    if (szData != NULL) { /* 元データはもう不要なので削除 */
        free(szData);
        szData = NULL;
    }
    /* 取り込み */
    iRet = OpenSOAPEnvelopeCreateCharEncoding(s_szCharSet,
                                              barray,
                                              pEnv);
    if (!OPENSOAP_SUCCEEDED(iRet)) {
        goto FuncEnd;
    }
FuncEnd:
    if (barray != NULL) {
      OpenSOAPByteArrayRelease(barray);
    }
    if (szData != NULL) {
      free(szData);
    }
    return iRet;
}

/***************************************************************************** 
    Function      : SOAP Envelope のロード
    Return        : int
 ************************************************ Yuichi SATO *****/
int LoadEnvelope
(const std::string          szString,   /* (i)  Envelope文字列 */
 OpenSOAPEnvelopePtr* pEnv)      /* (o)  Envelope */
{
  int                  iRet = OPENSOAP_PARAMETER_BADVALUE;
  unsigned char*       szData = NULL; /* Envelopeデータ */
  unsigned long        ulSize;
  OpenSOAPByteArrayPtr barray = NULL;
  
  unsigned int i = 0;

  /* 引数のチェック */
  if (szString.size() == 0 || pEnv == NULL) {
    goto FuncEnd;
  }
  ulSize = szString.size();    
  szData = (unsigned char*)malloc(sizeof(unsigned char) * ulSize);
  if(szData == NULL) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"LoadEnvelop(): malloc fail.");
    goto FuncEnd;
  }
  
  memcpy(szData, szString.c_str(), ulSize);
  
#ifdef DEBUG
  cerr << "szData = [" << endl;
  for(i = 0; i < ulSize; i++) {
    cerr << szData[i];
  }
  cerr << "]" << endl;
#endif //DEBUG
  
  /* OpenSOAPByteArrayPtrに変換 */
  iRet = OpenSOAPByteArrayCreateWithData(szData,
					 (size_t)ulSize,
					 &barray);
  if (!OPENSOAP_SUCCEEDED(iRet)) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s%s"
								,"LoadEnvelop(): "
								,"OpenSOAPByteArrayCreateWithData fail."
								);
    goto FuncEnd;
  }
  
  if (szData != NULL) { /* 元データはもう不要なので削除 */
    free(szData);
    szData = NULL;
  }
  /* 取り込み */
  iRet = OpenSOAPEnvelopeCreateCharEncoding(s_szCharSet,
					    barray,
					    pEnv);
  if (!OPENSOAP_SUCCEEDED(iRet)) {
		OpenSOAP::AppLogger::Write(ERR_ERROR,"%s%s"
								,"LoadEnvelop(): "
								,"OpenSOAPEnvelopeCreateCharEncoding fail."
								);
    goto FuncEnd;
  }
  
 FuncEnd:
  if (barray != NULL) {
    OpenSOAPByteArrayRelease(barray);
  }
  if (szData != NULL) {
    free(szData);
  }
  return iRet;
}
