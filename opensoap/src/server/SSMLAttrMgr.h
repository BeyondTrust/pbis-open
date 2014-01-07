/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SSMLAttrMgr.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SSMLAttrMgr_H
#define SSMLAttrMgr_H

#include <pthread.h>
#include <string>
#include <list>
#include <libxml/tree.h>

#include "ThreadDef.h"
#include "SSMLAttrMgrProtocol.h"

namespace OpenSOAP {

    class SSMLAttrMgr {

        static const std::string SSML_FILE_SUFFIX; // = "ssml";

    public:
        SSMLAttrMgr();
        virtual ~SSMLAttrMgr();
        
        //main communication proc.
        int run();

    private:
        //lock for data reload
        ThrMutexHandle loadXmlLock_;

        //UNIXdomainsocket
        std::string 

        socketAddr_;
        
        //target xml directory
        std::string xmlDir_;
        //extend 2003/12/11
        //extend for internal services
        std::string intrnlSrvcDir;
        //extend for replyTo-url
        std::string replyToDir;

        //xml document list
        std::list<xmlDocPtr> xmlList_;

        //extend 2003/12/11
        //extend for internal services
        std::list<xmlDocPtr> intrnlSrvcList;
        //extend for replyTo-url
        std::list<xmlDocPtr> replyToList;
        
        //read from xml files
        //modified 2004/01/04
        int loadXml();
        int loadXml(const std::string& dirPath, std::list<xmlDocPtr>& xmlList);
        bool addXmlList(const std::string& ssmlFilePath,
                        std::list<xmlDocPtr>& xmlList);

        //communication proc.
        static 
        ThrFuncReturnType
        connectionThread(ThrFuncArgType arg);
        void connectProc(int sockfd);

        //for shm buffer
        void clearShm();
        void clearShm(const SSMLType& ssmlType);
        void addShm(const std::string& query, const std::string& value,
                    const SSMLType& ssmlType);
        int semid_; // SEM ID

        std::string queryXml(const std::string& query, 
                             const SSMLType = EXTERNAL_SERVICES);

        typedef struct ThrDataTag {
            SSMLAttrMgr* that;
            int sockfd;
        } ThrData;
    };

} // end of namespace OpenSOAP


#endif /* SSMLAttrMgr_H */
