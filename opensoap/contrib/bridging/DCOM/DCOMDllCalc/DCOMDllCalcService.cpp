/*-----------------------------------------------------------------------------
 * $RCSfile: CalcService.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

/* DCOM Test Begin */
#define _WIN32_DCOM

#include <stdio.h>
#include <string.h>

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Service.h>

#import "AdDll.tlb"
#import "SubDll.tlb"
#import "MultiDll.tlb"
#import "DivDll.tlb"

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
"http://tempuri.org/message/";
static
const   char
CALC_METHOD_NAMESPACE_PREFIX[] =
"m";

static
const char
SERVICE_OPERAND_A_NAME[] = "A";
static
const char
SERVICE_OPERAND_B_NAME[] = "B";

static
const char
SERVICE_RESPONSE_RESULT_NAME[] = "Result";


typedef 
int
(*CalcFunctionType)(double a, double b, double *r);

static
int
Add(double a,
    double b,
    double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

	HRESULT hr = CoInitialize(NULL);

	if (FAILED(hr)) {
		std::cerr << "COM Initialize failed" << std::endl;
		ret = OPENSOAP_MEM_ERROR;
		return ret;
	}

	try {
		ADDLLLib::IAddDllPtr addService;
		COSERVERINFO serverInfo;
		MULTI_QI multiQi;

		serverInfo.dwReserved1 = NULL;
		serverInfo.pwszName = L"JD-NOTE";
		serverInfo.pAuthInfo = NULL;
		serverInfo.dwReserved2 = NULL;

		multiQi.pIID = NULL;
		multiQi.pIID = &__uuidof(ADDLLLib::IAddDll);
		if( multiQi.pIID == NULL ){
			ret = OPENSOAP_NOT_CATEGORIZE_ERROR;
			return ret;
		}
		multiQi.pItf = NULL;
		multiQi.hr = S_OK;

		hr = CoCreateInstanceEx(__uuidof(ADDLLLib::AddDll), NULL, 
								CLSCTX_REMOTE_SERVER,
								&serverInfo, 1, &multiQi );

		TCHAR szUserName[UNLEN + 1];
		DWORD dwUserNameLen = UNLEN + 1;
		if (GetUserName(szUserName, &dwUserNameLen)) {
			cerr
				<< "UserName: "
				<< szUserName
				<< endl;
		}
		if(FAILED(hr))
		{
			TCHAR szUserName[UNLEN + 1];
			DWORD dwUserNameLen = UNLEN + 1;
			if (GetUserName(szUserName, &dwUserNameLen)) {
				cerr
					<< "UserName: "
					<< szUserName
					<< endl;
			}
			_com_error e(hr);
			if (e.ErrorMessage()) {
				cerr <<
				e.ErrorMessage() << endl;
			}
			ret = OPENSOAP_IO_ERROR;
			return ret;
		}

		ADDLLLib::IAddDll* pInt = static_cast<ADDLLLib::IAddDll*>(multiQi.pItf);
		addService.Attach(pInt);

		*r = addService->Add(a, b);
	}

	catch (const _com_error & err) {
		hr = err.Error();
		cerr
			<< err.ErrorMessage()
			<< "0x"
			<<	hex
			<<	hr
			<<	endl;
	}

	CoUninitialize();

	ret = OPENSOAP_NO_ERROR;
    return ret;
}

static
int
Subtract(double a,
         double b,
         double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

	HRESULT hr = CoInitialize(NULL);

	if (FAILED(hr)) {
		std::cerr << "COM Initialize failed" << std::endl;
		ret = OPENSOAP_MEM_ERROR;
		return ret;
	}

	try {
		SUBDLLLib::ISubtractDllPtr subService;
		COSERVERINFO serverInfo;
		MULTI_QI multiQi;

		serverInfo.dwReserved1 = NULL;
		serverInfo.pwszName = L"JD-NOTE";
		serverInfo.pAuthInfo = NULL;
		serverInfo.dwReserved2 = NULL;

		multiQi.pIID = NULL;
		multiQi.pIID = &__uuidof(SUBDLLLib::ISubtractDll);
		if( multiQi.pIID == NULL ){
			ret = OPENSOAP_NOT_CATEGORIZE_ERROR;
			return ret;
		}
		multiQi.pItf = NULL;
		multiQi.hr = S_OK;

		hr = CoCreateInstanceEx(__uuidof(SUBDLLLib::SubtractDll), NULL, 
								CLSCTX_REMOTE_SERVER,
								&serverInfo, 1, &multiQi );

		SUBDLLLib::ISubtractDll* pInt = static_cast<SUBDLLLib::ISubtractDll*>(multiQi.pItf);
		subService.Attach(pInt);

		*r = subService->Subtract(a, b);
	}

	catch (const _com_error & err) {
		hr = err.Error();
		cerr
			<< err.ErrorMessage()
			<< "0x"
			<<	hex
			<<	hr
			<<	endl;
	}

	CoUninitialize();

	ret = OPENSOAP_NO_ERROR;
    return ret;
}

static
int
Multiply(double a,
         double b,
         double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

	HRESULT hr = CoInitialize(NULL);

	if (FAILED(hr)) {
		std::cerr << "COM Initialize failed" << std::endl;
		ret = OPENSOAP_MEM_ERROR;
		return ret;
	}

	try {
		MULTIDLLLib::IMultiplyDllPtr multiService;
		COSERVERINFO serverInfo;
		MULTI_QI multiQi;

		serverInfo.dwReserved1 = NULL;
		serverInfo.pwszName = L"JD-NOTE";
		serverInfo.pAuthInfo = NULL;
		serverInfo.dwReserved2 = NULL;

		multiQi.pIID = NULL;
		multiQi.pIID = &__uuidof(MULTIDLLLib::IMultiplyDll);
		if( multiQi.pIID == NULL ){
			ret = OPENSOAP_NOT_CATEGORIZE_ERROR;
			return ret;
		}
		multiQi.pItf = NULL;
		multiQi.hr = S_OK;

		hr = CoCreateInstanceEx(__uuidof(MULTIDLLLib::MultiplyDll), NULL, 
								CLSCTX_REMOTE_SERVER,
								&serverInfo, 1, &multiQi );

		MULTIDLLLib::IMultiplyDll* pInt = static_cast<MULTIDLLLib::IMultiplyDll*>(multiQi.pItf);
		multiService.Attach(pInt);

		*r = multiService->Multiply(a, b);
	}

	catch (const _com_error & err) {
		hr = err.Error();
		cerr
			<< err.ErrorMessage()
			<< "0x"
			<<	hex
			<<	hr
			<<	endl;
	}

	CoUninitialize();

	ret = OPENSOAP_NO_ERROR;
    return ret;
}

static
int
Divide(double a,
       double b,
       double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

	if( !r || !b ) {
		return ret;
	}

	HRESULT hr = CoInitialize(NULL);

	if (FAILED(hr)) {
		std::cerr << "COM Initialize failed" << std::endl;
		ret = OPENSOAP_MEM_ERROR;
		return ret;
	}

	try {
		DIVDLLLib::IDivideDllPtr divService;
		COSERVERINFO serverInfo;
		MULTI_QI multiQi;

		serverInfo.dwReserved1 = NULL;
		serverInfo.pwszName = L"JD-NOTE";
		serverInfo.pAuthInfo = NULL;
		serverInfo.dwReserved2 = NULL;

		multiQi.pIID = NULL;
		multiQi.pIID = &__uuidof(DIVDLLLib::IDivideDll);
		if( multiQi.pIID == NULL ){
			ret = OPENSOAP_NOT_CATEGORIZE_ERROR;
			return ret;
		}
		multiQi.pItf = NULL;
		multiQi.hr = S_OK;

		hr = CoCreateInstanceEx(__uuidof(DIVDLLLib::DivideDll), NULL, 
								CLSCTX_REMOTE_SERVER,
								&serverInfo, 1, &multiQi );

		DIVDLLLib::IDivideDll* pInt = static_cast<DIVDLLLib::IDivideDll*>(multiQi.pItf);
		divService.Attach(pInt);

		*r = divService->Divide(a, b);
	}

	catch (const _com_error & err) {
		hr = err.Error();
		cerr
			<< err.ErrorMessage()
			<< "0x"
			<<	hex
			<<	hr
			<<	endl;
	}

	CoUninitialize();

	ret = OPENSOAP_NO_ERROR;
    return ret;
}

typedef struct {
    const char *requestName;
    const char *responseName;
    CalcFunctionType calcFunc;
} CalcServiceMethodMapItem;

static
int
CalcServiceGetParameter(OpenSOAPEnvelopePtr /* [in] */ request,
                        const char * /* [in] */ request_name,
                        double * /* [out] */ a,
                        double * /* [out] */ b) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (a && b) {
        OpenSOAPBlockPtr body_block = NULL;
        ret = OpenSOAPEnvelopeGetBodyBlockMB(request,
                                             request_name,
                                             &body_block);
        if (OPENSOAP_SUCCEEDED(ret)) {
            int is_same_ns = 0;
            ret = OpenSOAPBlockIsSameNamespaceMB(body_block,
                                                 CALC_METHOD_NAMESPACE_URI,
                                                 &is_same_ns);
            if (OPENSOAP_SUCCEEDED(ret) && is_same_ns) {
                ret = OpenSOAPBlockGetChildValueMB(body_block,
                                                   SERVICE_OPERAND_A_NAME,
                                                   "double",
                                                   a);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPBlockGetChildValueMB(body_block,
                                                       SERVICE_OPERAND_B_NAME,
                                                       "double",
                                                       b);
                }
            }
        }
    }

    return ret;
}

static
int
CalcServiceCreateResponse(const char * /* [in] */ response_name,
                          double /* [in] */ r,
                          OpenSOAPEnvelopePtr * /* [out] */ response) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (response_name && *response_name) {
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
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPBlockSetChildValueMB(body_block,
                                                       SERVICE_RESPONSE_RESULT_NAME,
                                                       "double",
                                                       &r);
                }
            }
        }
    }

    return ret;
}

static
int
CalcServiceFunc(OpenSOAPEnvelopePtr /* [in] */ request,
                OpenSOAPEnvelopePtr * /* [out] */ response,
                void * /* [in, out] */ opt) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    CalcServiceMethodMapItem *m_map = (CalcServiceMethodMapItem *)opt;

    if (m_map) {
        double a = 0;
        double b = 0;
		fprintf(stderr, "DCOMDllCalcService\n");
        ret = CalcServiceGetParameter(request,
                                      m_map->requestName,
                                      &a,
                                      &b);
        if (OPENSOAP_SUCCEEDED(ret)) {
            double r = 0;
            ret = (m_map->calcFunc)(a, b, &r);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = CalcServiceCreateResponse(m_map->responseName,
                                                r,
                                                response);
            }
        }
    }

    return ret;
}

static
const CalcServiceMethodMapItem
CalcServiceMethodMap[] = {
    {"Add", "AddResponse", Add},
    {"Subtract", "SubtractResponse", Subtract},
    {"Multiply", "MultiplyResponse", Multiply},
    {"Divide", "DivideResponse", Divide},
    {NULL, NULL, NULL}
};

/*
 */
int
main(void) {
    int ret = 0;
    OpenSOAPServicePtr calc_service = NULL;
    int error_code
        = OpenSOAPInitialize(NULL);
    if (OPENSOAP_SUCCEEDED(error_code)) {
        const CalcServiceMethodMapItem *m_map_i = CalcServiceMethodMap;
        error_code 
            = OpenSOAPServiceCreateMB(&calc_service,
                                      "CalcService",
                                      CONNECT_TYPE,
                                      0);
        for (; OPENSOAP_SUCCEEDED(error_code) && m_map_i->requestName;
            ++m_map_i) {
            error_code
                = OpenSOAPServiceRegisterMB(calc_service,
                                            m_map_i->requestName,
                                            CalcServiceFunc,
                                            (void *)m_map_i);
        }
        if (OPENSOAP_SUCCEEDED(error_code)) {
            error_code 
                = OpenSOAPServiceRun(calc_service);
        }

        OpenSOAPServiceRelease(calc_service);

        OpenSOAPUltimate();
    }

    return ret;
}
