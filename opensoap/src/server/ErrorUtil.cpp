#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string>
#include "ErrorUtil.h"
#include "SrvErrorCode.h"
using namespace std;
using namespace OpenSOAP;

static const std::string CLASS_SIG = "ErrorUtil::";

typedef struct _strmap {
	int code;
	const char *str;
} StrMap;

static const char * UNDEFINED_ERR_MSG="undefined error !";
const StrMap ErrStrList[]={
//OpenSOAP API Error 
	{OPENSOAP_NOT_CATEGORIZE_ERROR,
		"Undefind error !"},
	{OPENSOAP_IMPLEMENTATION_ERROR,"Implementation error !"},
	{OPENSOAP_YET_IMPLEMENTATION,"Not yet implementation error !"},
	{OPENSOAP_UNSUPPORT_PROTOCOL,"Protocol not supported error !"},
	{OPENSOAP_PARAMETER_BADVALUE,"Program error(Bad Parameter Value) !"},
	{OPENSOAP_MEM_ERROR,"Memory error !"},
	{OPENSOAP_MEM_BADALLOC,"Memory allocation error !"},
	{OPENSOAP_MEM_OUTOFRANGE,"Memory Out-of-range error !"},
	{OPENSOAP_CHAR_ERROR,"Char conversion error !"},
	{OPENSOAP_ICONV_NOT_IMPL,"iconv not implemented error !"},
	{OPENSOAP_INVALID_MB_SEQUENCE,"Multi-byte sequence invalid error !"},
	{OPENSOAP_INCOMPLETE_MB_SEQUENCE,"Multi-byte sequence incomplete error !"},
	{OPENSOAP_UNKNOWN_CHARENCODE,"Unknown character encoding error !"},
	{OPENSOAP_SETLOCALEFAILURE,"Set locale failure error !"},
	{OPENSOAP_IO_ERROR,"I/O error !"},
	{OPENSOAP_IO_READ_ERROR,"I/O Read error !"},
	{OPENSOAP_IO_WRITE_ERROR,"I/O Write error !"},
	{OPENSOAP_FILE_ERROR,"File I/O error !"},
	{OPENSOAP_FILEOPEN_ERROR,"File open error !"},
	{OPENSOAP_XML_ERROR,"XML Manipulation error !"},
	{OPENSOAP_XMLNODE_NOT_FOUND,"XML Node not found error !"},
	{OPENSOAP_XML_BADNAMESPACE,"Bad Namespace is used error !"},
	{OPENSOAP_XML_NOHEADERBODY,
		"Header and/or body element not found error !"},
	{OPENSOAP_XML_BADDOCUMENTTYPE,
		"XML document root element is not envelope error !"},
	{OPENSOAP_XML_BADMAKEDOCUMENT,"XML document creation failure error !"},
	{OPENSOAP_XML_EMPTYDOCUMENT,"XML document empty error !"},
	{OPENSOAP_XML_NOTXMLDOCUMENT,"Not XML document error !"},
	{OPENSOAP_XML_NS_URI_UNMATCHED,
		"XML namespace prefix matched, but URI unmatched error !"},
	{OPENSOAP_SEC_ERROR,"Security error !"},
	{OPENSOAP_SEC_KEYGEN_ERROR,"Key generation error !"},
	{OPENSOAP_SEC_SIGNGEN_ERROR,"Signature generation error !"},
	{OPENSOAP_SEC_SIGNVERIFY_ERROR,"Signature verification error !"},
	{OPENSOAP_SEC_ENCRYPT_ERROR,"Encryption error !"},
	{OPENSOAP_SEC_DECRYPT_ERROR,"Decryption error !"},
	{OPENSOAP_TRANSPORT_ERROR,"Transport error !"},
	{OPENSOAP_TRANSPORT_INVOKE_ERROR,"Transport invoke error !"},
	{OPENSOAP_TRANSPORT_HOST_NOT_FOUND,
		"Transport-Host not found! maybe DNS error. -- ADDRINFO? SOCKET?"},
	{OPENSOAP_TRANSPORT_CONNECTION_REFUSED,
		"Transport-Connection refused! No one listening on the remote address"},
	{OPENSOAP_TRANSPORT_CONNECTION_TIMEOUT,
		"Transport-Connection timeout!"},
	{OPENSOAP_TRANSPORT_NETWORK_UNREACH,
		"Transport-Network is unreachable!"},
	{OPENSOAP_TRANSPORT_HOST_UNREACH,"Transport-Host is unreachable!"},
	{OPENSOAP_TRANSPORT_HTTP_ERROR,"Transport-HTTP error !"},
	{OPENSOAP_TRANSPORT_SSL_ERROR,"Transport-SSL error !"},
	{OPENSOAP_TRANSPORT_SSL_VERSION_ERROR,
		"Transport-SSL version invalid error !"},
	{OPENSOAP_TRANSPORT_SSL_CERTFILE_ERROR,
		"Transport-SSL SSL certification file error !"},
// OpenSOAP Server extention
	{OPENSOAPSERVER_THREAD_ERROR,"Thread error !"},
	{OPENSOAPSERVER_THREAD_CREATE_ERROR,"Thread create error !"},
	{OPENSOAPSERVER_THREAD_EXIT_ERROR,"Thread exit error !"},
	{OPENSOAPSERVER_THREAD_LOCK_ERROR,"Thread lock error !"},
	{OPENSOAPSERVER_THREAD_UNLOCK_ERROR,"Thread unlock error !"},
	{OPENSOAPSERVER_NETWORK_ERROR,"Network error !"},
	{OPENSOAPSERVER_NETWORK_SOCKET_CREATE_ERROR,"Network socket create error!"},
	{OPENSOAPSERVER_NETWORK_SOCKET_OPEN_ERROR,"Network socket open error !"},
	{OPENSOAPSERVER_NETWORK_SOCKET_CLOSE_ERROR,"Network socket close error !"},
	{OPENSOAPSERVER_NETWORK_SOCKET_WRITE_ERROR,"Network write error !"},
	{OPENSOAPSERVER_NETWORK_SOCKET_READ_ERROR,"Network read error !"},
	{OPENSOAPSERVER_NETWORK_SOCKET_SEQUENCE_ERROR,"Network sequence error !"},
	{OPENSOAPSERVER_NETWORK_SOCKET_BIND_ERROR,"Network socket bind error !"},
	{OPENSOAPSERVER_NETWORK_SOCKET_ACCEPT_ERROR,"Network socket accept error !"},
	{OPENSOAPSERVER_NETWORK_SOCKET_LISTEN_ERROR,"Network socket listen error !"},
	{OPENSOAPSERVER_NETWORK_SOCKET_CONNECT_ERROR,"Network socket connect error !"},
	{OPENSOAPSERVER_NETWORK_SOCKET_SELECT_ERROR,"Network socket select error !"},
	{OPENSOAPSERVER_SEM_ERROR,"Semaphor error !"},
	{OPENSOAPSERVER_SEM_CTL_ERROR,"Semaphor control error !"},
	{OPENSOAPSERVER_SEM_GET_ERROR,"Semaphor get error !"},
	{OPENSOAPSERVER_SEM_OP_ERROR,"Semaphor operation error !"},
	{OPENSOAPSERVER_SHM_ERROR,"Shared memory error !"},
	{OPENSOAPSERVER_SHM_CTL_ERROR,"Shared memory control error !"},
	{OPENSOAPSERVER_SHM_GET_ERROR,"Shared memory get error !"},
	{OPENSOAPSERVER_SHM_AT_ERROR,"Shared memory attach error !"},
	{OPENSOAPSERVER_SHM_DT_ERROR,"Shared memory detach error !"},
	{OPENSOAPSERVER_CONF_ERROR,"Opensoap server configuration error !"},
	{OPENSOAPSERVER_SERVERCONF_ERROR,"server.conf open/read error !"},
	{OPENSOAPSERVER_SSML_ERROR,"SSML file open/read error !"},
	{OPENSOAPSERVER_UNKNOWN_ERROR,"Unkown error !"},
	{OPENSOAPSERVER_EXEC_ERROR,"Program execute error !"},
	{OPENSOAPSERVER_FORK_ERROR,"Fork error !"},
	{OPENSOAPSERVER_RUNTIME_ERROR,"Runtime Error !"},
	{OPENSOAPSERVER_LOGIC_ERROR,"Logic Error!"},
	{OPENSOAPSERVER_TIMEOUT_ERROR,"Timeout Error!"},
	{0xFFFFFFFF,NULL}
	};


std::string ErrorUtil::toString(int id){
	int i;
	std::string str;
	for (i=0;i<100&&ErrStrList[i].code!=id&&ErrStrList[i].str!=NULL;i++);
	str=(ErrStrList[i].str!=NULL)? ErrStrList[i].str : UNDEFINED_ERR_MSG;

	return str;
}

const char * ErrorUtil::toConstChar(int id){
	int i;
	const char * str;

	for (i=0;ErrStrList[i].code!=id&&ErrStrList[i].str!=NULL;i++);
	str=(ErrStrList[i].str!=NULL)? ErrStrList[i].str : UNDEFINED_ERR_MSG;

	return str;
}

