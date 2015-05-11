/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Rule.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef Rule_H
#define Rule_H

#include "RuleBase.h"

using namespace OpenSOAP;

namespace OpenSOAPv1_2_00 {
    class Rule : public RuleBase {
    public:
        Rule(SrvConf& aSrvConf);
        virtual ~Rule();
        virtual void initSession();
        virtual void termSession();
        virtual void invoke(SoapMessage& request, SoapMessage& response);
    protected:
        virtual void parse(SoapMessage& request,
                           std::string& method,
                           std::string& ns,
                           int& type);

        bool checkMyUrlAndPopForwardPathStack(SoapMessage& request, 
                                              bool& modified);
        bool attachReceivedPath(SoapMessage& request);
        bool isIncludedMyUrlInReceivedPath() const;
        bool checkIsSameAsMyUrl(const std::string& forwardPath) const;
        bool checkAndModifiedForwarderHopcount(SoapMessage& request);
        bool attachOrUpdateBackwardPath(SoapMessage& request);
    };
}

#endif //Rule_H
