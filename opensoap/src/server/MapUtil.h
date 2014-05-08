/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MapUtil.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef MapUtil_H
#define MapUtil_H

#include <string>
#include <map>
#include <utility>

#include <OpenSOAP/Defines.h>

namespace OpenSOAP {

    class OPENSOAP_CLASS MapUtil {
    private:
        std::map<std::string, std::string> dataMap;
    public:
        void insert(std::string& fst, std::string& snd);
        void insert(std::pair<std::string, std::string> insData);
        std::string operator[](const std::string& key);
    public:
        MapUtil(const std::string& itemDelimit, 
                const std::string& keyValDelimit);
        ~MapUtil();
        
        void insertItem(const std::string& data);
        bool exists(const std::string& key) const;
        std::string toString() const;
        
        void spy() const;
            
    private:
        void insertMap(const std::string& item);
        
        std::string itemDelimit_;
        std::string keyValDelimit_;
    };
        
} // end of namespace OpenSOAP

#endif /* MapUtil_H */
