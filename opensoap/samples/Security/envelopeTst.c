/*-----------------------------------------------------------------------------
 * $RCSfile: envelopeTst.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <stdio.h>
#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Envelope.h>
#include <OpenSOAP/Security.h>
#include "tstcmn.h"

/***************************************************************************** 
    Function      : 暗号化＆署名＆保存
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.26 *****/
static int encAndSave
(const char* szNameIn,          /* (i)  入力ファイル名 */
 const char* szPubKName,        /* (i)  公開鍵ファイル名 */
 const char* szPrivKName,       /* (i)  秘密鍵ファイル名 */
 const char* szNameOut)         /* (o)  出力ファイル名 */
{
    int nRet;
    OpenSOAPEnvelopePtr env = NULL;
    /* OpenSOAP API 初期化 */
    nRet = OpenSOAPInitialize(NULL);
    if (!OPENSOAP_SUCCEEDED(nRet)) {
        goto FuncEnd;
    }
    /* Envelopeをファイルよりロード */
    nRet = LoadEnvelope(szNameIn, &env);
    if (!OPENSOAP_SUCCEEDED(nRet)) {
        goto FuncEnd;
    }
    /* 暗号化 */
    nRet = OpenSOAPSecEncWithFile(env, szPubKName);
    if (!OPENSOAP_SUCCEEDED(nRet)) {
        goto FuncEnd;
    }
    /* 署名付加 */
    nRet = OpenSOAPSecAddSignWithFile(env,
                                      OPENSOAP_HA_SHA,
                                      szPrivKName,
                                      NULL);
    if (!OPENSOAP_SUCCEEDED(nRet)) {
        goto FuncEnd;
    }
    /* 暗号化したものを保存 */
    nRet = SaveEnvelope(env, szNameOut);
    if (!OPENSOAP_SUCCEEDED(nRet)) {
        goto FuncEnd;
    }
FuncEnd:
    OpenSOAPUltimate();
    return nRet;
}
/***************************************************************************** 
    Function      : 署名検証＆復号化＆保存
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.26 *****/
static int decAndSave
(const char* szNameIn,          /* (i)  入力ファイル名 */
 const char* szPubKName,        /* (i)  公開鍵ファイル名 */
 const char* szPrivKName,       /* (i)  秘密鍵ファイル名 */
 const char* szNameOut)         /* (i)  出力ファイル名 */
{
    int nRet;
    OpenSOAPEnvelopePtr env = NULL;
    /* OpenSOAP API 初期化 */
    nRet = OpenSOAPInitialize(NULL);
    if (!OPENSOAP_SUCCEEDED(nRet)) {
        goto FuncEnd;
    }
    /* Envelopeをファイルよりロード */
    nRet = LoadEnvelope(szNameIn, &env);
    if (!OPENSOAP_SUCCEEDED(nRet)) {
        goto FuncEnd;
    }
    /* 署名検証 */
    nRet = OpenSOAPSecVerifySignWithFile(env,
                                         szPubKName);
    if (!OPENSOAP_SUCCEEDED(nRet)) {
        goto FuncEnd;
    }
    /* 復号化 */
    nRet = OpenSOAPSecDecWithFile(env, szPrivKName);
    if (!OPENSOAP_SUCCEEDED(nRet)) {
        goto FuncEnd;
    }
    /* 復号化したものを保存 */
    nRet = SaveEnvelope(env, szNameOut);
    if (!OPENSOAP_SUCCEEDED(nRet)) {
        goto FuncEnd;
    }
FuncEnd:
    OpenSOAPUltimate();
    return nRet;
}
/***************************************************************************** 
    Function      : 使用方法の表示
    Return        : void
 ************************************************ Yuji Yamawaki 01.11.26 *****/
static void usage
(const char* szProg)
{
    fprintf(stderr, "Usage: %s envName PubKeyFName PrivKeyFName\n", szProg);
}
/***************************************************************************** 
    Function      : メイン
    Return        :int
 ************************************************ Yuji Yamawaki 01.11.26 *****/
int main(int argc, char** argv)
{
    int         nRet = 0;
    const char* szEncName = "encrypt.xml"; /* 暗号化された Envelope 名称 */
    /* 引数の数チェック */
    if (argc < 4) {
        usage(argv[0]);
        nRet = 1;
        goto FuncEnd;
    }
    /* 暗号化テスト */
    nRet = encAndSave(argv[1], argv[2], argv[3], szEncName);
    if (!OPENSOAP_SUCCEEDED(nRet)) {
        fprintf(stderr, "encAndSave() Error [%08X].\n", nRet);
        nRet = -1;
        goto FuncEnd;
    }
    /* 復号化テスト */
    nRet = decAndSave(szEncName, argv[2], argv[3], "decrypt.xml");
    if (!OPENSOAP_SUCCEEDED(nRet)) {
        fprintf(stderr, "decAndSave() Error [%08X].\n", nRet);
        nRet = -1;
        goto FuncEnd;
    }
FuncEnd:
    return nRet;
}
