/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: PIDFactory.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <iostream>

#include <unistd.h>
#include <fstream>

#include "PIDFactory.h"
#include "SrvConf.h"
#include "AppLogger.h"

using namespace OpenSOAP;
using namespace std;

//for LOG
static const std::string CLASS_SIG = "PIDFactory";

PIDFactory::PIDFactory()
{

  theName = "";
  fd = 0;
  lock_pid = 0;

}

PIDFactory::~PIDFactory()
{

  if(lock_pid == getpid()){
    closeFile();
    deleteFile();
  }

}

bool
PIDFactory::makePID(const string& processName)
{
  SrvConf srvConf;
  string pidOutputFile = srvConf.getPIDPath();
  pidOutputFile += processName;

  if (!makeFile(pidOutputFile)) {
    AppLogger::Write(APLOG_ERROR,"%s:%s[%s]%s","PIDFactory::makePID"
					,"pid file",pidOutputFile.c_str(),"cannot open!");
    return false;
  }

  if (isLocked()){
    closeFile();
    AppLogger::Write(APLOG_ERROR,"PIDFactory::makePID: pid file already locked");
    return false;
  }
  else{
    if (!setLock()){
      closeFile();
      AppLogger::Write(APLOG_ERROR,"PIDFactory::makePID: unable to lock pid file");
      return false;
    }
  }
  
  return true;
}

int
PIDFactory::checkPID(const string& processName)
{
  int		lockPid;
  SrvConf srvConf;
  string pidOutputFile = srvConf.getPIDPath();
  pidOutputFile += processName;

  lockPid = 0;
  if (makeFile(pidOutputFile)) {
    if (isLocked()){
      lockPid = getLockPid();
    }
    closeFile();
  }

  return lockPid;
}

bool
PIDFactory::makeFile(const string& fileName)
{

//  theFile = new ofstream();
//  theFile->open(fileName.c_str(), ios::in | ios::out);

  theName = "";
  theName += fileName;
  if ((fd = open(fileName.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) <= 0){
	  return false;
  }

  return true;
}

bool
PIDFactory::existPIDFile(const string& fileName)
{
  SrvConf		srvConf;
  struct stat		fStat;
  int			ret;

  string pidOutputFile = srvConf.getPIDPath();
  pidOutputFile += fileName;
  
  if ((ret = stat(pidOutputFile.c_str(), &fStat)) == -1){
    return false;
  }

  return true;
}

void
PIDFactory::closeFile(void)
{

//  theFile->close();
  close(fd);

  return;
}

bool
PIDFactory::isFileOpen(void)
{

//  return theFile->is_open();
  if (fd > 0){
    return true;
  }
  else{
    return false;
  }
}

void
PIDFactory::deleteFile(void)
{

  unlink(theName.c_str());

  return;
}

bool
PIDFactory::setLock(void)
{
  struct flock		lock_str;
  int			ret;
  char			pid_str[32];

  lock_str.l_start = 0;
  lock_str.l_len = 0;
  lock_str.l_pid = getpid();
  lock_str.l_type = F_WRLCK;
  lock_str.l_whence = 0;

  if ((ret = fcntl(fd, F_SETLK, &lock_str)) == -1){
    switch(errno){
      case EAGAIN :
        break;
      default :
        break;
    }
    return false;
  }

  lock_pid = lock_str.l_pid;

//  *theFile << lock_pid << "                " << flush;

  sprintf(pid_str, "%d                ", lock_pid);
  if ((ret = write(fd, (char *)pid_str, strlen(pid_str))) != strlen(pid_str)){
    return false;
  }

  return true;
}

bool
PIDFactory::isLocked(void)
{
  struct flock		lock_str;
  int			ret;

  lock_str.l_start = 0;
  lock_str.l_len = 0;
  lock_str.l_pid = getpid();
  lock_str.l_type = F_WRLCK;
  lock_str.l_whence = 0;

  if ((ret = fcntl(fd, F_GETLK, &lock_str)) == -1){
    switch(errno){
      case EAGAIN :
        break;
      default :
        break;
    }
    return true;
  }

  if (lock_str.l_type == F_UNLCK){
    return false;
  }

  return true;
}

pid_t
PIDFactory::getLockPid(void)
{
  struct flock		lock_str;
  int			ret;

  lock_str.l_start = 0;
  lock_str.l_len = 0;
  lock_str.l_pid = getpid();
  lock_str.l_type = F_WRLCK;
  lock_str.l_whence = 0;

  if ((ret = fcntl(fd, F_GETLK, &lock_str)) == -1){
    switch(errno){
      case EAGAIN :
        break;
      default :
        break;
    }
    return 0;
  }

  if (lock_str.l_type == F_UNLCK){
    return 0;
  }

  return lock_str.l_pid;
}
