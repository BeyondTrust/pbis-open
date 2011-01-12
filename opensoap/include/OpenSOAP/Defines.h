/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Defines.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_Defines_H
#define OpenSOAP_Defines_H
/**
 * @file OpenSOAP/Defines.h
 * @brief OpenSOAP API definition header file
 * @author
 *	OpenSOAP Development Team
 */
/**
 * @def OPENSOAP_API
 * @brief OpenSOAP API declare/definition prefix macro.
 *	
 */
/**
 * @def OPENSOAP_VAR
 * @brief OpenSOAP API variable declare/definition prefix macro.
 *	
 */
/**
 * @def OPENSOAP_CLASS
 * @brief OpenSOAP API C++ class definition prefix macro.
 *
 * This macro is used when you shuld be exported all member functions.
 * i.e.
 * @code
 *	class OPENSOAP_CLASS TwoChannel {
 *	public:
 *		TwoChannel();
 *		const std::string &getOne() const;
 *	};
 * @endcode
 */

/* for Visual C++ */
#if defined(_MSC_VER) && !defined(OPENSOAP_STATIC)
#  if defined(OPENSOAP_BUILD_DLL)
#    define OPENSOAP_API __declspec(dllexport) __stdcall
#    define OPENSOAP_VAR __declspec(dllexport)
#    define OPENSOAP_CLASS __declspec(dllexport)
#  else
#    define OPENSOAP_API __declspec(dllimport) __stdcall
#    define OPENSOAP_VAR __declspec(dllimport)
#    define OPENSOAP_CLASS __declspec(dllimport)
#  endif /* OPENSOAP_BUILD_DLL */
#else /* OPENSOAP_STATIC || !_MSC_VER */
#  if defined(_MSC_VER)
#    define OPENSOAP_API __stdcall
#  else
#    define OPENSOAP_API
#  endif /* !_MSC_VER */
#  define OPENSOAP_VAR
#  define OPENSOAP_CLASS
#endif /* defined(_MSC_VER) && !defined(OPENSOAP_STATIC) */

#endif  /* OpenSOAP_Defines_H */

