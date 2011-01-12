/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: XmlQuery.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef XMLQuery_H
#define XMLQuery_H

#include <iostream>
#include <string>
#include <vector>
#include <libxml/tree.h>

#include <OpenSOAP/Defines.h>

namespace OpenSOAP {

    //analyze DOM
    class OPENSOAP_CLASS XmlQuery {
    public:
        XmlQuery(const std::string& query);
        ~XmlQuery();
    
        //値の取得
        //取得できればtrueを返す
        //※RootQuery に RootNodeを渡す
        //int getValue(xmlDocPtr& doc, xmlNodePtr node);
        int getValue(xmlDocPtr& doc, xmlNodePtr node, 
                     std::vector<std::string>& values);

        //値の格納場所
        //複数得られる場合有り
        //can't use object in Win32 DLL interface, so declare as pointer.
        //std::vector<std::string> value;
        //std::vector<std::string>* values_;

        bool isMulti() const;
        
        void spy() const;
        
    protected:
        
        class OPENSOAP_CLASS DataSet {
        public:
            DataSet() : value_(NULL) {}
            DataSet(const DataSet& ds) : value_(NULL) {
                setName(ds.name);
                if (ds.value_) { setValue(*(ds.value_)); }
            } 
            
            ~DataSet() { if (value_) { delete value_; } }
            
            // get method
            const std::string& getName() const { return name; }
            //const std::string* getValue() const { return value; }
            const std::string getValue() const { 
                return isNullValue() ? "" : *value_; }
            
            // set method
            void setName(const std::string& aName) { name = aName; }
            //値の設定(条件値として)
            //※最終的な取得目的値はXmlQuery::valueの要素として
            //  設定される
            void setValue(const std::string& aValue);

            //値の初期化
            void clearValue() {
                if (value_) {
                    delete value_;
                    value_ = NULL;
                }
            }
            //値がNULLかどうか。
            //NULLでなければ条件値。NULLなら取得したい目的値として処理する。
            bool isNullValue() const { return NULL == value_ ? true : false; }
            
            void spy() const {
                std::cerr << " name=[" << name << "]" << std::endl;
                std::cerr << " value_=[" 
                          << (NULL == value_ ? "<null>" : *value_) 
                          << "]" << std::endl;
            }

        protected:
            std::string name;
            std::string* value_; //NULLでなければ条件値。NULLなら取得目的値
        };
        
        //子インスタンスポインタ
        XmlQuery* next_;
        
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
        //can't use object in Win32 DLL interface, so declare as pointer.
        //std::vector<DataSet> attributes_;
        std::vector<DataSet>* attributes_;
        
        void addName(const std::string& nameToken);
        void addAttributes(const std::string& attrToken);
        
        std::string::size_type findDelmitPos(const std::string& s,
                                             std::string::size_type beginPos,
                                             const std::string del);
    };

} // end of namespace OpenSOAP


#endif /* XMLQuery_H */
