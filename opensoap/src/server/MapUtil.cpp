/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MapUtil.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <iostream>

#include "MapUtil.h"
#include "ServerCommon.h"
#include "AppLogger.h"

using namespace OpenSOAP;
using namespace std;

MapUtil::MapUtil(const std::string& itemDelimit, 
		 const std::string& keyValDelimit)
  : itemDelimit_(itemDelimit)
  , keyValDelimit_(keyValDelimit)
{
}

MapUtil::~MapUtil()
{
}

bool 
MapUtil::exists(const std::string& key) const
{
    map<string, string>::const_iterator itr = dataMap.find(key);
    return itr == dataMap.end() ? false : true;
}

string 
MapUtil::toString() const
{
    string retStr;
    map<string, string>::const_iterator mpos;
    for (mpos = dataMap.begin(); mpos != dataMap.end(); ++mpos) {
        if (!retStr.empty()) {
            retStr += itemDelimit_;
        }
        retStr += (*mpos).first;
        string val = (*mpos).second;
        if (!val.empty()) {
            retStr += keyValDelimit_;
        }
        retStr += val;
    }
    return retStr;
}

void 
MapUtil::insertItem(const string& data)
{
    string::size_type begIdx = 0;
    string::size_type endIdx = 0;
    for (;;) {
        begIdx = data.find_first_not_of(itemDelimit_, endIdx);
        if (begIdx == string::npos) {
            break;
        }
        endIdx = data.find_first_of(itemDelimit_, begIdx);
        if (endIdx == string::npos) {
            endIdx = data.length();
        }
        string token = data.substr(begIdx, (endIdx-begIdx));

        insertMap(token);
    }
}

void 
MapUtil::insertMap(const string& item)
{
    static char METHOD_LABEL[] = "MapUtil::insertMap: ";

    string::size_type begIdx = 0;
    string::size_type endIdx = 0;
    
    begIdx = item.find_first_not_of(keyValDelimit_);
    if (begIdx == string::npos) {
        AppLogger::Write(APLOG_WARN,"%s%s",METHOD_LABEL,"no key");
        return;
    }
    endIdx = item.find_first_of(keyValDelimit_, begIdx);
    if (endIdx == string::npos) {
        endIdx = item.length();
    }
    string key = item.substr(begIdx, (endIdx-begIdx));
    
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL,"key",key.c_str());
#endif /* DEBUG */
    
    string val;
    
    begIdx = item.find_first_not_of(keyValDelimit_, endIdx);
    if (begIdx == string::npos) {
        LOGPRINT(TAG_WRN)
            << METHOD_LABEL
            << "no value" 
            << endl;
        //return;
        val = "";
    }
    else {
        // edit 2001/08/26
        // AAA=BBB=CCC processed as AAA : BBB=CCC
        val = item.substr(begIdx, (item.length()-begIdx));
    }
    
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL,"val",val.c_str());
#endif /* DEBUG */
    
    dataMap.insert(make_pair(key, val));
}


void 
MapUtil::spy() const
{
    cerr << "### MapUtil::spy() ###" << endl;
    cerr << " [KEY]:[VALUE]" << endl;
    cerr << " -------------" << endl;
    map<string, string>::const_iterator mpos;
    for (mpos = dataMap.begin(); mpos != dataMap.end(); ++mpos) {
        cerr << " [" << (*mpos).first << "]:[" 
             << (*mpos).second << "]" << endl;
    }
    cerr << "######################" << endl;
}

void
MapUtil::insert(string& fst, string& snd)
{
    dataMap.insert(make_pair(fst,snd));
}

void
MapUtil::insert(pair<string, string> insData)
{
    dataMap.insert(insData);
}

string
MapUtil::operator [](const string& key)
{
    return dataMap[key];
}

// End of MapUtil.cpp

