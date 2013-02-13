/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ChannelManager.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ChannelManager_H
#define ChannelManager_H

#include <sys/types.h>
#include <string>

#include <OpenSOAP/Defines.h>
#include "ThreadDef.h"

namespace OpenSOAP {

    //prototype def.
    class ChannelDescriptor;
    
    class OPENSOAP_CLASS ChannelManager {
        
    public:
        ChannelManager();
        virtual ~ChannelManager();
        
    public:
        
        //socket address 指定 (run()実行前条件)
#if defined(WIN32)
        bool setSocketAddr(const int addr);
#else
        bool setSocketAddr(const std::string& addr);
#endif
        
        // Socket 作成の際のマスクの指定
#if defined(WIN32)
        void setMask(int newMask);
#else
        void setMask(::mode_t newMask);
#endif
        //受信待ち実行開始
        void run();
        
    protected:
#if defined(WIN32)
	int
#else
        std::string 
#endif
        socketAddr_;
#if defined(WIN32)
	int
#else
	::mode_t 
#endif
        newMask_, curMask;
        bool setMaskFlag;
        //通信毎のサブスレッド
        //※スレッド関数の仕様によりC＋＋クラスでは
        //  スタティックメソッドである必要がある．
	static
        ThrFuncReturnType connectionThread(ThrFuncArgType arg);
        
        //このメソッドをサブクラス毎に実装する．
        virtual void doProc(int sockfd);

        typedef struct ThrDataTag {
            ChannelManager* that;
            int sockfd;
        } ThrData;

    };

} // end of namespace OpenSOAP


#endif /* ChannelManager_H */
