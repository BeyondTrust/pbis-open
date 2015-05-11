/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SpoolAdopter.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef SPOOL_ADOPTER_H
#define SPOOL_ADOPTER_H

#include <string>
#include <vector>

#include <OpenSOAP/Defines.h>

namespace OpenSOAP
{
    //declare prototype
    class ChannelSelector;
    class ChannelDescriptor;

    class OPENSOAP_CLASS SpoolAdopter {

    public:
        SpoolAdopter();
        ~SpoolAdopter();

        //regist entry
        std::string push(const std::string& fileId);
        //check existance of entry and extract fileId of response
        //  entry not exists: return false
        //  entry exists: return true and set responseFileId
        //                if send msg not contained <undelete> tag,
        //                delete entry from spool table
        bool pop(const std::string& fileId, std::string& responseFileId);

        //check existance of entry
        //  entry not exists: return false
        //  entry exists: return true
        //bool ref(const std::string& messageId);

        //expire record by ttl-manager
        std::string expire(const std::string& messageId);

    protected:
        typedef enum {
            PUSH, 
            POP,
            REF,
            EXPIRE,
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

#endif //SPOOL_ADOPTER_H

