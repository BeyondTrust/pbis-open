/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: RuleBase.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef RuleBase_H
#define RuleBase_H

#include <string>
#include "SoapMessage.h"

namespace OpenSOAP {

    class SrvConf;
    
    class RuleBase {
    public:
        RuleBase(SrvConf& aSrvConf) : srvConf(aSrvConf) {}
        virtual ~RuleBase() {}
        virtual void initSession() {}
        virtual void termSession() {}
        virtual void invoke(SoapMessage& request, SoapMessage& response){}
        
    protected:
        virtual void parse(SoapMessage& request,
                           std::string& method,
                           std::string& ns,
                           int& type) =0;
        SrvConf& srvConf;
    };
}

#endif //RuleBase_H

