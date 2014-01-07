#define _WIN32_DCOM

#import "SubExe.tlb"
//using namespace SUBEXELib;

#include <iostream>
using namespace std;

#define REMOTE 0
int
main(void) {
	HRESULT hr = CoInitialize(NULL);

	if(FAILED(hr)) {
		cerr << "COM Initialize Failed." << endl;
		return hr;
	}

#if REMOTE
	try{
		SUBEXELib::ISubtractExePtr subService;
		hr = subService.CreateInstance(__uuidof(SUBEXELib::SubtractExe));
		if(FAILED(hr)) {
			_com_issue_error(hr);
		}

		double a = 5.4;
		double b = 3.2;
		double result = SUBEXELib::subService->Subtract(a,b);
		cout
			<< a << " - " << b << " = " << result << endl;
	}

#else
	try{
		SUBEXELib::ISubtractExePtr subService;
		COSERVERINFO serverInfo;
		MULTI_QI multiQi;

		serverInfo.dwReserved1 = NULL;
		serverInfo.pwszName = L"JD-NOTE";
//		serverInfo.pwszName = L"MIHARU";
		serverInfo.pAuthInfo = NULL;
		serverInfo.dwReserved2 = NULL;

		multiQi.pIID = &__uuidof(SUBEXELib::ISubtractExe);
		multiQi.pItf = NULL;
		multiQi.hr = S_OK;

		hr = CoCreateInstanceEx(__uuidof(SUBEXELib::SubtractExe),
								NULL,
								CLSCTX_REMOTE_SERVER,
								&serverInfo,
								1,
								&multiQi);
		if(FAILED(hr)) {
			_com_issue_error(hr);
		}

		SUBEXELib::ISubtractExe* pInt = static_cast<SUBEXELib::ISubtractExe*>(multiQi.pItf);
		subService.Attach(pInt);

		double a = 5.4;
		double b = 3.2;
		double result = subService->Subtract(a, b);

		cout
			<< a << " - " << b << " = " << result << endl;
	}
#endif

	catch(const _com_error & err ) {
		hr = err.Error();
		cerr
			<< err.ErrorMessage()
			<< "0x"
			<< hex
			<< hr
			<< endl;
	}

	CoUninitialize();
	return hr;
}
								