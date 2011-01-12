/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ChannelDescriptor.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "ServerCommon.h"
#include "ChannelDescriptor.h"
#include "AppLogger.h"

using namespace OpenSOAP;
using namespace std;

//==== ChannelDescriptor ====
//ファイルディスクリプタクラス


ChannelDescriptor::ChannelDescriptor()
  : readFd_(-1)
  , writeFd_(-1)
{
  AppLogger::Write(APLOG_DEBUG9,"ChannelDescriptor::ChannelDescriptor()");
}

ChannelDescriptor::ChannelDescriptor(const ChannelDescriptor&
						 aDesc)
  : id_(aDesc.id_)
  , readFd_(aDesc.readFd_)
  , writeFd_(aDesc.writeFd_)
{
  AppLogger::Write(APLOG_DEBUG9,"ChannelDescriptor::ChannelDescriptor(copy)");
}

ChannelDescriptor::ChannelDescriptor(const std::string& id)
  : id_(id)
  , readFd_(-1)
  , writeFd_(-1)
{
  AppLogger::Write(APLOG_DEBUG9,"ChannelDescriptor::ChannelDescriptor(%s)"
  					,id.c_str());
}


ChannelDescriptor::~ChannelDescriptor()
{
  AppLogger::Write(APLOG_DEBUG9,"ChannelDescriptor::~ChannelDescriptor()");
}


void ChannelDescriptor::setId(const std::string& id)
{
  id_ = id;
}

void ChannelDescriptor::setReadFd(const int fd)
{
  readFd_ = fd;
}

void ChannelDescriptor::setWriteFd(const int fd)
{
  writeFd_ = fd;
}

int& ChannelDescriptor::getReadFd()
{
  return readFd_;
}

int& ChannelDescriptor::getWriteFd()
{
  return writeFd_;
}


int ChannelDescriptor::read(std::string& data)
{
    //check select()
    fd_set  rfds;
    while(1) {
        FD_ZERO(&rfds);
        FD_SET(readFd_,&rfds);
        select(FD_SETSIZE,&rfds,NULL,NULL,NULL);


        if(FD_ISSET(readFd_,&rfds)) {
            return OpenSOAP::read(readFd_, data);
        }
    }

    //OpenSOAP::read call.
    //return OpenSOAP::read(readFd_, data);
}

int ChannelDescriptor::write(const std::string& data)
{
    //check select()
    fd_set  wfds;
    while(1) {
        FD_ZERO(&wfds);
        FD_SET(writeFd_,&wfds);
        select(FD_SETSIZE,NULL,&wfds,NULL,NULL);
        
        if(FD_ISSET(writeFd_,&wfds)) {
            return OpenSOAP::write(writeFd_, data);
        }
    }

    //OpenSOAP::write call.
    //return OpenSOAP::write(writeFd_, data);
}

// End of ChannelDescriptor.cpp
