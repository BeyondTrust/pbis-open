/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: fio.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#if !defined(FIO_H)
#define FIO_H

#include <OpenSOAP/Envelope.h>
#include <string>

//  #if defined  __cplusplus
//  extern "C" {
//  #endif

    /* Load File To Memory */
    extern unsigned char* LoadFile
    (const char*    szFileName, /* (i)  File Name */
     unsigned long* pulSize);   /* (o)  File Size(Bytes) */

    /* Save Memory To File */
    extern int SaveFile
    (unsigned char* szMemData,      /* (i)  Data In Memory */
     unsigned long  ulMemSize,      /* (i)  Data Size(Bytes) */
     const char*    szFileName);    /* (i)  File Name */

    /* Save Memory To File With New Line */
    extern int SaveFileWithNewLine
    (unsigned char* szMemData,      /* (i)  Data In Memory */
     unsigned long  ulMemSize,      /* (i)  Data Size(Bytes) */
     int            nCharPerLine,   /* (i)  Character Count Per Line */
     const char*    szFileName);    /* (i)  File Name */

    /* Save SOAP Envelope */
    int SaveEnvelope
    (const OpenSOAPEnvelopePtr soap_env,  /* (i)  SOAP Envelope */
     const char*               szFName);  /* (i)  File Name */

    /* Save SOAP Envelope */
    int SaveEnvelope
    (const OpenSOAPEnvelopePtr soap_env,   /* (i)  SOAP Envelope */
     std::string&              szString);  /* (i)  Envelop String */

    /* Load SOAP Envelope */
    int LoadEnvelope
    (const char*          szFName,   /* (i)  Envelope File Nmae */
     OpenSOAPEnvelopePtr* pEnv);     /* (o)  Envelope */

    /* Load SOAP Envelope */
    int LoadEnvelope
    (const std::string    szString,  /* (i)  Envelope String */
     OpenSOAPEnvelopePtr* pEnv);     /* (o)  Envelope */

//  #if defined  __cplusplus
//  }
//  #endif
#endif



