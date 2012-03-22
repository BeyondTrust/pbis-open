/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: StringUtil.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef StringUtil_toString_H
#define StringUtil_toString_H

#include <iostream>
#include <string>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#if defined(HAVE_SSTREAM) || defined(WIN32)
#include <sstream>
#else
#include <strstream.h>
#endif /* HAVE_SSTREAM */

namespace StringUtil {
    template <typename SrcType>
    std::string
    toString(const SrcType &src) {

#if defined(HAVE_SSTREAM) || defined(WIN32)
        std::ostringstream    ost;
	ost
            <<  src;
#else
        ostrstream  ost;
        ost
            <<  src
	    <<  std::ends; 
	//	    <<  /* std:: */ends; 
#endif /* HAVE_SSTREAM */

        return ost.str();
        
    }
  
  template <typename TargetType>
  void
  fromString(const std::string& src, TargetType& target) {
#if defined(HAVE_SSTREAM) || defined(WIN32)
    std::istringstream    ist(src);
#else
    istrstream  ist(src.c_str());
#endif /* HAVE_SSTREAM */
    ist >> target;
  }
  
}

#endif  /* StringUtil_toString_H */
