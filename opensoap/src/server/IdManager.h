/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: IdManager.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ID_MANAGER_H
#define ID_MANAGER_H

#include <string>

namespace OpenSOAP {

  class IdManager {
  public:
    IdManager();
    ~IdManager();

    void run();

  private:
    std::string getNewId();

#if defined(WIN32)
    int
#else
    std::string
#endif
      socketAddr_;
#if !defined(HAVE_UUID_H) && !defined(HAVE_UUID_UUID_H)
    std::string prevId_;
    int idSequence_;
#endif
  };
}

#endif /* ID_MANAGER_H */
