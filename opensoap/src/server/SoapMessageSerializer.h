/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SoapMessageSerializer.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef SoapMessageSerializer_H
#define SoapMessageSerializer_H

#include <string>
#include <vector>
#include <map>
#include <iostream>

//for libxml2
#include <libxml/tree.h>

#include "SoapMessage.h"

//depend on DataRetainer I/F

namespace OpenSOAP {
    
    class SoapMessageSerializer {
    public:
        SoapMessageSerializer();
        virtual ~SoapMessageSerializer();

        static void setSoapMessageSpoolPath(const std::string& path) {
            soapMessageSpoolPath = path;
        }

        //deserialize
        virtual void deserialize(SoapMessage& soapMessage) const;
        virtual void deserialize(SoapMessage& soapMessage,
                                 const std::string& id) const;
        virtual void deserialize(SoapMessage& soapMessage,
                                 const std::string& id,
                                 std::map<std::string,std::string>& hdrElem) const;

        //serialize
        virtual void serialize(SoapMessage& soapMessage) const;
        virtual void serialize(SoapMessage& soapMessage,
                               const SoapException& se) const;
        virtual void serialize(SoapMessage& soapMessage, 
                               const std::string& soapEnvelopeStr) const;
        virtual void serialize(SoapMessage& soapMessage, 
                               const std::string& soapEnvelopeStr,
                               const std::map<std::string,std::string>& hdrElem) const;

/*
        void serialize(const SoapMessage& xmlDoc, std::string& str);
        void serialize(const SoapMessage& xmlDoc, std::vector<char>& bytearray);
        void serialize(const SoapMessage& xmlDoc, std::ostream& ost);
        void deserialize(SoapMessage& xmlDoc, const std::string& str);
        void deserialize(SoapMessage& xmlDoc, const std::vector<char>& bytearray);
        void deserialize(SoapMessage& xmlDoc, const std::istream& ist);
*/      
        //private:
    protected:
        static std::string soapMessageSpoolPath;
    };
}

#endif //SoapMessageSerializer_H
