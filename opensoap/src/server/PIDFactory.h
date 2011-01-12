/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: PIDFactory.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef PID_FACTORY_H
#define PID_FACTORY_H

#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <fstream>

using namespace std;

namespace OpenSOAP {

  class PIDFactory {
  public:
    PIDFactory();
    virtual ~PIDFactory();

    bool makePID(const std::string& processName);
    int checkPID(const std::string& processName);

    bool makeFile(const std::string& fileName);
    bool existPIDFile(const std::string& fileName);
    void closeFile(void);
    bool isFileOpen(void);
    void deleteFile(void);
    bool setLock(void);
    bool isLocked(void);
    pid_t getLockPid(void);

    int fd;
    pid_t lock_pid;

    std::string theName;
  };
}

#endif /* PID_FACTORY_H */
