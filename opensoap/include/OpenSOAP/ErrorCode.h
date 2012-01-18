/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ErrorCode.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_ErrorCode_H
#define OpenSOAP_ErrorCode_H

/**
 * @file OpenSOAP/ErrorCode.h
 * @brief OpenSOAP API Error Code Definitions
 * @author
 *    OpenSOAP Development Team
 */

/*
=begin
= OpenSOAP Error code definitions.
=end
 */

/**
 * @def OPENSOAP_NO_ERROR (0)
 * @brief No Error
 */
#define OPENSOAP_NO_ERROR (0)

/**
 * @def OPENSOAP_NOT_CATEGORIZE_ERROR   (0xffffffffL)
 * @brief Undefined Error
 */
#define OPENSOAP_NOT_CATEGORIZE_ERROR   (0xffffffffL)

/**
 * @def OPENSOAP_IMPLEMENTATION_ERROR   (0x40000000L)
 * @brief Implementation Error
 */
#define OPENSOAP_IMPLEMENTATION_ERROR   (0x40000000L)

/**
 * @def OPENSOAP_YET_IMPLEMENTATION     (0x40000001L)
 * @brief Not Yet Implemented Error
 */
#define OPENSOAP_YET_IMPLEMENTATION     (0x40000001L)

/**
 * @def OPENSOAP_UNSUPPORT_PROTOCOL     (0x40000002L)
 * @brief Protocol not supported (including ssl without HAVE_SSL)
 */
#define OPENSOAP_UNSUPPORT_PROTOCOL     (0x40000002L)

/**
 * @def OPENSOAP_PARAMETER_BADVALUE     (0x20000001L)
 * @brief Bad Parameter Value Error
 */
#define OPENSOAP_PARAMETER_BADVALUE     (0x20000001L)

/**
 * @def OPENSOAP_USERDEFINE_ERROR       (0x10000000L)
 * @brief User Defined Error
 */
#define OPENSOAP_USERDEFINE_ERROR       (0x10000000L)

/**
 * @def OPENSOAP_MEM_ERROR      (0x00010000L)
 * @brief Memory Error
 */
#define OPENSOAP_MEM_ERROR      (0x00010000L)

/**
 * @def OPENSOAP_MEM_BADALLOC   (0x00010001L)
 * @brief Memory Allocation Error
 */
#define OPENSOAP_MEM_BADALLOC   (0x00010001L)

/**
 * @def OPENSOAP_MEM_OUTOFRANGE (0x00010002L)
 * @brief Memory Out-of-range Error
 */
#define OPENSOAP_MEM_OUTOFRANGE (0x00010002L)



/**
 * @def OPENSOAP_CHAR_ERROR             (0x00020000L)
 * @brief Char Conversion Error
 */
#define OPENSOAP_CHAR_ERROR             (0x00020000L)

/**
 * @def OPENSOAP_ICONV_NOT_IMPL         (0x00020001L)
 * @brief iconv Not Implemented Error
 */
#define OPENSOAP_ICONV_NOT_IMPL         (0x00020001L)

/**
 * @def OPENSOAP_INVALID_MB_SEQUENCE    (0x00020002L)
 * @brief Multi-byte Sequence Invalid Error
 */
#define OPENSOAP_INVALID_MB_SEQUENCE    (0x00020002L)

/**
 * @def OPENSOAP_INCOMPLETE_MB_SEQUENCE (0x00020003L)
 * @brief Multi-byte Sequence Incomplete Error
 */
#define OPENSOAP_INCOMPLETE_MB_SEQUENCE (0x00020003L)

/**
 * @def OPENSOAP_UNKNOWN_CHARENCODE     (0x00020004L)
 * @brief Unknown Character Encoding Error
 */
#define OPENSOAP_UNKNOWN_CHARENCODE     (0x00020004L)

/**
 * @def OPENSOAP_SETLOCALEFAILURE       (0x00020005L)
 * @brief Set Locale Failure Error
 */
#define OPENSOAP_SETLOCALEFAILURE       (0x00020005L)



/**
 * @def OPENSOAP_IO_ERROR       (0x00040000L)
 * @brief I/O Error
 */
#define OPENSOAP_IO_ERROR       (0x00040000L)

/**
 * @def OPENSOAP_IO_READ_ERROR	(0x00040001L)
 * @brief Read Error
 */
#define OPENSOAP_IO_READ_ERROR	(0x00040001L)

/**
 * @def OPENSOAP_IO_WRITE_ERROR	(0x00040002L)
 * @brief Write Error
 */
#define OPENSOAP_IO_WRITE_ERROR	(0x00040002L)

/**
 * @def OPENSOAP_FILE_ERROR		(0x00048000L)
 * @brief File I/O Error
 */
#define	OPENSOAP_FILE_ERROR		(0x00048000L)

/**
 * @def OPENSOAP_FILEOPEN_ERROR	(0x00048001L)
 * @brief File Open Error
 */
#define	OPENSOAP_FILEOPEN_ERROR	(0x00048001L)



/**
 * @def OPENSOAP_XML_ERROR				(0x00080000L)
 * @brief XML Manipulation Error
 */
#define OPENSOAP_XML_ERROR				(0x00080000L)

/**
 * @def OPENSOAP_XMLNODE_NOT_FOUND		(0x00080001L)
 * @brief XML Node Not Found Error
 */
#define OPENSOAP_XMLNODE_NOT_FOUND		(0x00080001L)

/**
 * @def OPENSOAP_XML_BADNAMESPACE		(0x00080002L)
 * @brief Bad Namespace Is Used Error
 */
#define OPENSOAP_XML_BADNAMESPACE		(0x00080002L)

/**
 * @def OPENSOAP_XML_NOHEADERBODY		(0x00080003L)
 * @brief Header And/Or Body Element Not Found Error
 */
#define OPENSOAP_XML_NOHEADERBODY		(0x00080003L)

/**
 * @def OPENSOAP_XML_BADDOCUMENTTYPE	(0x00080004L)
 * @brief XML Document Root Element Is Not Envelope Error
 */
#define OPENSOAP_XML_BADDOCUMENTTYPE	(0x00080004L)

/**
 * @def OPENSOAP_XML_BADMAKEDOCUMENT	(0x00080005L)
 * @brief XML Document Creation Failure Error
 */
#define OPENSOAP_XML_BADMAKEDOCUMENT	(0x00080005L)

/**
 * @def OPENSOAP_XML_EMPTYDOCUMENT		(0x00080006L)
 * @brief XML Document Empty Error
 */
#define OPENSOAP_XML_EMPTYDOCUMENT		(0x00080006L)

/**
 * @def OPENSOAP_XML_NOTXMLDOCUMENT		(0x00080007L)
 * @brief Not XML Document Error
 */
#define OPENSOAP_XML_NOTXMLDOCUMENT		(0x00080007L)

/**
 * @def OPENSOAP_XML_NS_URI_UNMATCHED	(0x00080008L)
 * @brief XML Namespace Prefix Matched, But URI Unmatched Error
 */
#define OPENSOAP_XML_NS_URI_UNMATCHED	(0x00080008L)



/**
 * @def OPENSOAP_SEC_ERROR				(0x00100000L)
 * @brief Security Error
 */
#define OPENSOAP_SEC_ERROR				(0x00100000L)

/**
 * @def OPENSOAP_SEC_KEYGEN_ERROR		(0x00100001L)
 * @brief Key Generation Error
 */
#define OPENSOAP_SEC_KEYGEN_ERROR		(0x00100001L)

/**
 * @def OPENSOAP_SEC_SIGNGEN_ERROR		(0x00100002L)
 * @brief Signature Generation Error
 */
#define OPENSOAP_SEC_SIGNGEN_ERROR		(0x00100002L)

/**
 * @def OPENSOAP_SEC_SIGNVERIFY_ERROR	(0x00100003L)
 * @brief Signature Verification Error
 */
#define OPENSOAP_SEC_SIGNVERIFY_ERROR	(0x00100003L)

/**
 * @def OPENSOAP_SEC_ENCRYPT_ERROR		(0x00100004L)
 * @brief Encryption Error
 */
#define OPENSOAP_SEC_ENCRYPT_ERROR		(0x00100004L)

/**
 * @def OPENSOAP_SEC_DECRYPT_ERROR		(0x00100005L)
 * @brief Decryption Error
 */
#define OPENSOAP_SEC_DECRYPT_ERROR		(0x00100005L)

/**
 * @def OPENSOAP_TRANSPORT_ERROR		(0x00200000L)
 * @brief Transport Error
 */
#define OPENSOAP_TRANSPORT_ERROR		(0x00200000L)

/**
 * @def OPENSOAP_TRANSPORT_INVOKE_ERROR	(0x00210000L)
 * @brief Transport Invoke Error
 */
#define OPENSOAP_TRANSPORT_INVOKE_ERROR	(0x00210000L)

/**
 * @def OPENSOAP_TRANSPORT_HOST_NOT_FOUND	(0x00210001L)
 * @brief Transport Error - Host Not Found
 *        Maybe DNS error. ---- ADDRINFO ? SOCKET ?
 */
#define OPENSOAP_TRANSPORT_HOST_NOT_FOUND	(0x00210001L)

/**
 * @def OPENSOAP_TRANSPORT_CONNECTION_REFUSED	(0x00210002L)
 * @brief Transport Error - Connection Refused
 *        No one listening on the remote address
 */
#define OPENSOAP_TRANSPORT_CONNECTION_REFUSED	(0x00210002L)

/**
 * @def OPENSOAP_TRANSPORT_CONNECTION_TIMEOUT	(0x00210003L)
 * @brief Transport Error - Connection Timeout
 */
#define OPENSOAP_TRANSPORT_CONNECTION_TIMEOUT	(0x00210003L)

/**
 * @def OPENSOAP_TRANSPORT_NETWORK_UNREACH	(0x00210004L)
 * @brief Transport Error - Network is unreachable
 */
#define OPENSOAP_TRANSPORT_NETWORK_UNREACH	(0x00210004L)

/**
 * @def OPENSOAP_TRANSPORT_HOST_UNREACH	(0x00210005L)
 * @brief Transport Error - Host is unreachable
 */
#define OPENSOAP_TRANSPORT_HOST_UNREACH	(0x00210005L)

/**
 * @def OPENSOAP_TRANSPORT_HTTP_ERROR	(0x00220000L)
 * @brief Transport HTTP Error
 */
#define OPENSOAP_TRANSPORT_HTTP_ERROR	(0x00220000L)

/**
 * @def OPENSOAP_TRANSPORT_IS_HTTP_ERROR(x)	((x) & (OPENSOAP_TRANSPORT_HTTP_ERROR))
 * @brief Non-0 if Transport Error code is an HTTP_ERROR
 */
#define OPENSOAP_TRANSPORT_IS_HTTP_ERROR(x)	(((x) & (OPENSOAP_TRANSPORT_HTTP_ERROR)) == (OPENSOAP_TRANSPORT_HTTP_ERROR))

/**
 * @def OPENSOAP_TRANSPORT_GET_HTTP_ERROR(x) ((x) & ~(OPENSOAP_TRANSPORT_HTTP_ERROR))
 * @brief Calculate actual HTTP Status (Error) number from OpenSOAP Transport Error
 */
#define OPENSOAP_TRANSPORT_GET_HTTP_ERROR(x) \
        ((x) & ~(OPENSOAP_TRANSPORT_HTTP_ERROR))

/**
 * @def OPENSOAP_TRANSPORT_SET_HTTP_ERROR(x) ((x) | (OPENSOAP_TRANSPORT_HTTP_ERROR))
 * @brief Calculate OpenSOAP Transport Error number from actual HTTP Status
 */
#define OPENSOAP_TRANSPORT_SET_HTTP_ERROR(x) \
        ((x) | (OPENSOAP_TRANSPORT_HTTP_ERROR))

/**
 * @def OPENSOAP_TRANSPORT_SSL_ERROR	(0x00240000L)
 * @brief Transport SSL Error
 */
#define OPENSOAP_TRANSPORT_SSL_ERROR	(0x00240000L)

/**
 * @def OPENSOAP_TRANSPORT_SSL_VERSION_ERROR	(0x00240001L)
 * @brief Transport - SSL Version Invalid
 */
#define OPENSOAP_TRANSPORT_SSL_VERSION_ERROR	(0x00240001L)

/**
 * @def OPENSOAP_TRANSPORT_SSL_CERTFILE_ERROR	(0x00240002L)
 * @brief Transport - SSL Certification File Error
 */
#define OPENSOAP_TRANSPORT_SSL_CERTFILE_ERROR	(0x00240002L)

/*
=begin
= OpenSOAP Error code judge macro.
=end
 */

/**
 * @def OPENSOAP_SUCCEEDED(err) (!(err))
 * @brief OpenSOAP Success Macro Error
 */
#define OPENSOAP_SUCCEEDED(err) (!(err))

/**
 * @def OPENSOAP_FAILED(err)  (err)
 * @brief OpenSOAP Failure Macro Error
 */
#define OPENSOAP_FAILED(err)  (err)

#endif  /* OpenSOAP_ErrorCode_H */
