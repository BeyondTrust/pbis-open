/*-----------------------------------------------------------------------------
 * $RCSfile: DCOMDllGEtDSService.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

/* DCOM Test Begin */
#define _WIN32_DCOM

#import "ServerInfo.tlb"

#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlimpl.cpp>

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Service.h>


#include <LMCONS.H>
#include <iostream>
using namespace std;

/* DCOM Test End */


#if !defined(CONNECT_TYPE)
//# define CONNECT_TYPE "stdio"
# define CONNECT_TYPE "cgi"
#endif /* CONNECT_TYPE */

static
const   char
CALC_METHOD_NAMESPACE_URI[] =
"http://namespaces.opensoap.jp";
static
const   char
CALC_METHOD_NAMESPACE_PREFIX[] =
"m";

static
const char
SERVICE_REQUEST_NAME[] = "GetServerDiskSize";

static
const char
SERVICE_OPERAND_NAME[] = "ServerName";

static
const char
SERVICE_RESPONSE_RESULT_NAME[] = "GetServerDiskSizeResponse";

static
const char
SERVICE_RESULT_NAME[] = "ServerDiskSize";



static
int
DCOMDllGetDSServiceGetParameter(OpenSOAPEnvelopePtr /* [in] */ request,
                                OpenSOAPStringPtr * /* [out] */ servername) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (request && servername) {
        OpenSOAPBlockPtr body_block = NULL;
        ret = OpenSOAPEnvelopeGetBodyBlockMB(request,
                                             SERVICE_REQUEST_NAME,
                                             &body_block);
        if (OPENSOAP_SUCCEEDED(ret)) {
            int is_same_ns = 0;
            ret = OpenSOAPBlockIsSameNamespaceMB(body_block,
                                                 CALC_METHOD_NAMESPACE_URI,
                                                 &is_same_ns);
            if (OPENSOAP_SUCCEEDED(ret) && is_same_ns) {
				ret = OpenSOAPBlockGetChildValueMB(body_block,
												   SERVICE_OPERAND_NAME,
												   "string",
												   servername);
			}
        }
    }

    return ret;
}

static
int
DCOMDllGetDSServiceCreateResponse(const char * /* [in] */ response_name,
								  long /* [in] */ diskspace,
                                  OpenSOAPEnvelopePtr * /* [out] */ response) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (response_name && response) {
        ret = OpenSOAPEnvelopeCreateMB(NULL, NULL, response);
        if (OPENSOAP_SUCCEEDED(ret)) {
            OpenSOAPBlockPtr body_block = NULL;
            ret = OpenSOAPEnvelopeAddBodyBlockMB(*response,
                                                 response_name,
                                                 &body_block);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPBlockSetNamespaceMB(body_block,
                                                  CALC_METHOD_NAMESPACE_URI,
                                                  CALC_METHOD_NAMESPACE_PREFIX);
				if(OPENSOAP_SUCCEEDED(ret)) {
					ret = OpenSOAPBlockSetChildValueMB(body_block,
													   SERVICE_RESULT_NAME,
													   "int",
													   &diskspace);
				}
            }
        }
    }

    return ret;
}


static
int 
GetDiskSize( wchar_t * servername, long * disksize )
{
    int ret = OPENSOAP_PARAMETER_BADVALUE;

	HRESULT hr = CoInitialize(NULL);

	try
	{
		HRESULT hr = S_OK;
		SERVERINFOLib::IDiskInfoPtr pDiskInfo;

		COSERVERINFO csi = {0, servername, NULL, 0};
		MULTI_QI qi = {&__uuidof(SERVERINFOLib::IDiskInfo), NULL, S_OK};
		hr = CoCreateInstanceEx(__uuidof(SERVERINFOLib::DiskInfo), NULL,
								CLSCTX_REMOTE_SERVER, &csi, 1, &qi);
		if(FAILED(hr))
		{
			_com_issue_error(hr);
		}
		SERVERINFOLib::IDiskInfo* pInt = static_cast<SERVERINFOLib::IDiskInfo*>(qi.pItf);
		pDiskInfo.Attach(pInt);

		hyper hypDiskSize;
		pDiskInfo->GetFreeDiskSpace(L"C:\\", &hypDiskSize);
		hypDiskSize /= 1024;
		hypDiskSize /= 1024;
		*disksize = (long)hypDiskSize;
	}
	catch( const _com_error & e)
	{
		_tprintf(_T("There is an error (%08x): %s\n" ),
			e.Error(), e.ErrorMessage() );
	}

	CoUninitialize();

	ret = OPENSOAP_NO_ERROR;
    return ret;
}

static
int
DCOMDllGetDSServiceFunc(OpenSOAPEnvelopePtr /* [in] */ request,
                        OpenSOAPEnvelopePtr * /* [out] */ response,
                        void * /* [in, out] */ opt) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
	size_t servernameLen = 0;
	wchar_t * servername = NULL;

    if (1) {
        OpenSOAPStringPtr opensoap_servername = NULL;
		fprintf(stderr, "DCOMDllGetDSService\n");
        ret = DCOMDllGetDSServiceGetParameter(request,
                                              &opensoap_servername);
        if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPStringGetLengthWC( opensoap_servername,
											 &servernameLen);
			if(OPENSOAP_SUCCEEDED(ret)) {
				servername = (wchar_t *)malloc( (servernameLen + 1)*sizeof(wchar_t) );
				if( servername == NULL ) {
					ret = OPENSOAP_MEM_BADALLOC;
					return ret;
				}
				++servernameLen;
				ret = OpenSOAPStringGetStringWC( opensoap_servername,
												 &servernameLen,
												 servername );
				if( OPENSOAP_SUCCEEDED(ret) ) {
					long disksize = 0;
					ret = GetDiskSize( servername,
										&disksize );
					if (OPENSOAP_SUCCEEDED(ret)) {
						ret = DCOMDllGetDSServiceCreateResponse(SERVICE_RESPONSE_RESULT_NAME,
														        disksize,
														        response);
					}
				}
			}
        }
    }

    return ret;
}


int
main(void) {
    int ret = 0;
    OpenSOAPServicePtr DCOMDllGetDS_service = NULL;
    int error_code
        = OpenSOAPInitialize(NULL);
    if (OPENSOAP_SUCCEEDED(error_code)) {
        error_code 
            = OpenSOAPServiceCreateMB(&DCOMDllGetDS_service,
                                      "DCOMDllGetDSService",
                                      CONNECT_TYPE,
                                      0);
        if (OPENSOAP_SUCCEEDED(error_code)) {
            error_code
                = OpenSOAPServiceRegisterMB(DCOMDllGetDS_service,
                                            "GetServerDiskSize",
                                            DCOMDllGetDSServiceFunc,
                                            NULL);
        }
        if (OPENSOAP_SUCCEEDED(error_code)) {
            error_code 
                = OpenSOAPServiceRun(DCOMDllGetDS_service);
        }

        OpenSOAPServiceRelease(DCOMDllGetDS_service);

        OpenSOAPUltimate();
    }

    return ret;
}
