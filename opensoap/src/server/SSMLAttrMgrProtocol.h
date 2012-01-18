/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SSMLAttrMgrProtocol.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SSMLAttrMgrProtocol_H
#define SSMLAttrMgrProtocol_H

#include <string>

namespace OpenSOAP {
//プロトコル定義
  
  //メッセージデリミタ
  //static const std::string ITEM_DELMIT = ":";
  static const std::string ITEM_DELMIT = "|";
  static const std::string SUB_ITEM_DELMIT = "!";
  static const std::string VAL_DELMIT = "=";
  static const std::string NODE_DELMIT = "/";
  //static const std::string ATTR_DELMIT = ":";
  static const std::string ATTR_DELMIT = ",";

  //汎用メッセージ作成キーワード
  static const std::string QUERY = "QUERY";
  static const std::string QUESTION_MARK = "?";
  static const std::string MULTI_QUESTION_MARK = "??";

  
  //処理命令
  static const std::string CMD = "CMD";
  //-------------------------------------------------
  static const std::string RELOAD_XML = "RELOAD_XML";
  static const std::string SEARCH = "SEARCH";
  //-------------------------------------------------

  static const std::string RESULT = "RESULT";
  static const std::string SUCCESS = "SUCCESS";
  static const std::string FAILURE = "FAILURE";
  static const std::string RET_CODE = "RET_CODE";
  static const std::string ANSWER = "ANSWER";

#if 1
    //extend 2004/01/04
    typedef enum eSSMLType {
        EXTERNAL_SERVICES,
        INTERNAL_SERVICES,
        REPLY_TO,
    } SSMLType;
    //SSML Type keywork
    static const std::string SSML_TYPE = "SSML_TYPE";
    static const std::string SSML_TYPE_EXTERNAL = "EXT";
    static const std::string SSML_TYPE_INTERNAL = "IN";
    static const std::string SSML_TYPE_REPLYTO = "REPLYTO";

#endif

} // end namespace OpenSOAP

#endif /* SSMLAttrMgrProtocol_H */
