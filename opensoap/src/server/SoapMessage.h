/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SoapMessage.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef SoapMessage_H
#define SoapMessage_H

#include <string>
#include "XmlDoc.h"
#include "SoapException.h"

namespace OpenSOAP {

    class SoapMessage : public XmlDoc {
    public:
        SoapMessage();
        virtual ~SoapMessage();
        
        void setMessageId(const std::string& mid);
        const std::string& getMessageId() const;

        virtual std::string createSoapFault(const std::string& faultCode,
                                            const std::string& faultString,
                                            const std::string& faultActor,
                                            const std::string& detail) const;
        virtual std::string createSoapFault(const SoapException& se) const;

    protected:
        virtual void parse();
        std::string messageId;
    };
}

#endif //SoapMessage_H
