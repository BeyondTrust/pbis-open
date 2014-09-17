/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: QueueAdopter.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef QUEUE_ADOPTER_H
#define QUEUE_ADOPTER_H

#include <string>

namespace OpenSOAP
{
    //declare prototype
    class ChannelSelector;
    class ChannelDescriptor;

    class QueueAdopter {

        typedef enum {
            PUSH,
            EXPIRE,
            POP,
        } AccessType;

    public:
        typedef enum {
            LOCAL,
            FORWARD,
        } QueueType;


    public:
        QueueAdopter();
        QueueAdopter(const QueueType& type);
        ~QueueAdopter();

        //regist entry
        std::string push(const std::string& fileId);
        //extract entry
        std::string pop(const std::string& fileId);
        //delete entry
        std::string expire(const std::string& fileId);

    protected:

        QueueType qType;

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

#endif //QUEUE_ADOPTER_H

