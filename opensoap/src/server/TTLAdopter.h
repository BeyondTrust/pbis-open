/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: TTLAdopter.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef TTL_ADOPTER_H
#define TTL_ADOPTER_H

#include <string>

namespace OpenSOAP
{
    //declare prototype
    class ChannelSelector;
    class ChannelDescriptor;

    class TTLAdopter {

    public:
        TTLAdopter();
        ~TTLAdopter();

        //regist entry
        std::string push(const std::string& fileId);
        //check existance of entry and extract backward_path
        //  entry not exists: return false
        //  entry exists: return true and set backwardPath(=url or empty)
        bool ref(const std::string& messageId, std::string& backwardPath);

    protected:
        typedef enum {
            PUSH, 
            REF,
        } AccessType;

        ChannelSelector* chnl_;
        ChannelDescriptor* chnlDesc_;

        //create connector and open connection
        void openChnl(AccessType type);
        //close connection and destroy connector
        void closeChnl();
        //data send and recv
        void invoke(const std::string& sendData, std::string& recvData);

    };
}

#endif //TTL_ADOPTER_H

