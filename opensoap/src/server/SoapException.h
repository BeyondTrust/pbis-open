/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SoapException.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef SoapException_H
#define SoapException_H

#include "Exception.h"
//#include "SoapMessage.h"

namespace OpenSOAP {

    class SoapMessage;
    class SoapException : public Exception {
    protected:
        class Detail;
    public:
        //SoapException();
        //SoapException(const SoapMessage& baseSoapMessage);
        SoapException(const Exception& e);
        SoapException(const SoapException& se);
        //SoapException(int errcode ,int detailcode,int level);
        SoapException(int errcode 
                      ,int detailcode
                      ,int level
                      ,const std::string& modname
                      ,int id
            );
        SoapException(int errcode 
                      ,int detailcode
                      ,int level
                      ,const std::string& modname
                      ,int id
                      ,const SoapMessage* soapMessage
                      //,const SoapMessage& soapMessage
            );
        virtual ~SoapException();
        
        //soap-header referance
        //void setSoapHeaderRef(const SoapMessage& soapMessage);
        void setSoapHeaderRef(const SoapMessage* soapMessage);

        //code setter
        void setFaultCode(const std::string& aFaultCode);
        void setFaultString(const std::string& aFaultString);
        void setFaultActor(const std::string& aFaultActor);
        void setDetail(const Detail& aDetail);
        void setDetail(const std::string& aDetailStr) {
            detail = aDetailStr;
        }
        void setHttpStatusCode(int code) {httpStatusCode = code;}

        //code getter
        const std::string& getFaultCode() const;
        const std::string& getFaultString() const;
        const std::string& getFaultActor() const;
        const Detail& getDetail() const;
        const int getHttpStatusCode() const {return httpStatusCode;}

        //static method
        static void initMyUri();
        static const std::string& getMyUri() {return myUri;}
        
        std::string createSoapFault() const;

      protected:

        //internal class
        class Detail {
          public:
            Detail() {}
            Detail(const std::string& aDetailString) 
                : detailString(aDetailString) {}
            Detail(const Detail& aDetail) {
                detailString = aDetail.detailString;
            }
            virtual ~Detail() {}

            void setDetail(const std::string& aDetail) {
                detailString = aDetail;
            }
            const std::string& toString() const {
                return detailString;
            }
          protected:
            std::string detailString;
        };

        std::string faultCode;
        std::string faultString;
        std::string faultActor;
        Detail detail;
        int httpStatusCode;

        static void setMyUri(const std::string& uri) { myUri = uri; }
        static std::string myUri;

        //used for soap-header copy
        const SoapMessage* soapHeaderRef;


    }; // class Exception
    
} // namespace OpenSOAP

#endif // SoapException_H
