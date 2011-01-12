/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: XmlQuery2.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef XMLQuery2_H
#define XMLQuery2_H

#include <iostream>
#include <string>
#include <vector>
#include <libxml/tree.h>

namespace OpenSOAP {

  //DOM解析
  class XmlQuery2 {

  public:
    typedef struct NameSpaceDefTag {
      std::string href;
      std::string prefix;
    } NameSpaceDef;

  public:
    XmlQuery2(const std::string& query);
    virtual ~XmlQuery2();
    
    //値の取得
    //取得できればtrueを返す
    //※RootQuery に RootNodeを渡す
    //int getValue(xmlDocPtr& doc, xmlNodePtr node);
    int getValue(xmlDocPtr& doc, xmlNodePtr node, 
		 std::vector<std::string>& values);

    int attach(xmlDocPtr& doc, 
	       xmlNodePtr node,
	       xmlNodePtr parent,
	       const std::string& value,
	       bool updateFlg,
	       bool useNs,
	       const NameSpaceDef* nsDef,
	       int insPos = -1); //-1=prev, 1=next

    int del(xmlDocPtr& doc, 
	    xmlNodePtr node);
    //xmlNodePtr parent,
    //const std::string& value);

    void spy() const;

  protected:

    class DataSet {
    public:
      DataSet() : value(NULL) {}
      DataSet(const DataSet& ds) : value(NULL) {
	setName(ds.name);
	if (ds.value) { setValue(*(ds.value)); }
      } 

      ~DataSet() { if (value) { delete value; } }

      // get method
      const std::string& getName() const { return name; }
      //const std::string* getValue() const { return value; }
      const std::string getValue() const { return isNullValue() ? "" : *value; }

      // set method
      void setName(const std::string& aName) { name = aName; }
      //値の設定(条件値として)
      //※最終的な取得目的値はXmlQuery2::valueの要素として
      //  設定される
      void setValue(const std::string& aValue) { 
	if (!value) { 
	  value = new std::string(aValue); 
	}
	else {
	  *value = aValue;
	}
      }
      //値の初期化
      void clearValue() {
	if (value) {
	  delete value;
	  value = NULL;
	}
      }
      //値がNULLかどうか。
      //NULLでなければ条件値。NULLなら取得したい目的値として処理する。
      bool isNullValue() const { return NULL == value ? true : false; }

      void spy() const {
          std::cerr << " name=[" << name << "]" << std::endl;
          std::cerr << " value=[" 
                    << (NULL == value ? "<null>" : *value) << "]" << std::endl;
      }

    protected:
      std::string name;
      std::string* value; //NULLでなければ条件値。NULLなら取得目的値
    };

    //子インスタンスポインタ
    XmlQuery2* next_;

    //std::string nodeName_; //条件(必須)
    DataSet node_;

    //このノードの要素値が目的かどうか。
    //要素名は必須であるため、要素が有りかつ値が未設定(NULL)という
    //条件では目的値かどうか判断できないため。
    bool isTarget_; 

    //複数の値を要求しているかどうか。(??が複数、?は１つのみ)
    //していない場合は初めに見つかった結果が返される。
    bool isMulti_;

    //属性
    std::vector<DataSet> attributes_;

    void addName(const std::string& nameToken);
    void addAttributes(const std::string& attrToken);

  };

} // end of namespace OpenSOAP


#endif /* XMLQuery2_H */
