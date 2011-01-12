/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ChannelDescriptor.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ChannelDescriptor_H
#define ChannelDescriptor_H

#include <string>

#include <OpenSOAP/Defines.h>

namespace OpenSOAP {

    //内部プロセス間通信用ファイルディスクリプタクラス
    class OPENSOAP_CLASS ChannelDescriptor {
        
        friend class ChannelSelector;
        friend class ChannelManager;
        
    public:
        ChannelDescriptor();
        ChannelDescriptor(const ChannelDescriptor& aDesc);
        virtual ~ChannelDescriptor();
        
        int read(std::string& aReadData);
        int write(const std::string& aWriteData);
        
        enum WAIT_OPT { 
            WAIT_NOLIMIT = -1, // max 180sec.
            NO_WAIT      = 0 
        };
        
    protected:
        ChannelDescriptor(const std::string& aId);
        
        const std::string& getId() const { return id_; }
        void setId(const std::string& id);
        
    public:
        void setReadFd(const int fd);
        void setWriteFd(const int fd);
        int& getReadFd();
        int& getWriteFd();

    protected:
        //public:
        std::string id_;
        int readFd_;
        int writeFd_;
        
    };
    
} // end of namespace OpenSOAP


#endif /* ChannelDescriptor_H */
