#define _WIN32_DCOM

#import "DivDll.tlb"
//using namespace DIVDLLLib;

#include <iostream>
using namespace std;

int
main(void) {
	HRESULT hr = CoInitialize(NULL);

	if(FAILED(hr)) {
		cerr << "COM Initialize Failed." << endl;
		return hr;
	}

	try{
		DIVDLLLib::IDivideDllPtr divService;
		COSERVERINFO serverInfo;
		MULTI_QI multiQi;

		serverInfo.dwReserved1 = NULL;
		serverInfo.pwszName = L"JD-NOTE";
//		serverInfo.pwszName = L"MIHARU";
		serverInfo.pAuthInfo = NULL;
		serverInfo.dwReserved2 = NULL;

		multiQi.pIID = &__uuidof(DIVDLLLib::IDivideDll);
		multiQi.pItf = NULL;
		multiQi.hr = S_OK;

		hr = CoCreateInstanceEx(__uuidof(DIVDLLLib::DivideDll),
								NULL,
								CLSCTX_REMOTE_SERVER,
								&serverInfo,
								1,
								&multiQi);
		if(FAILED(hr)) {
			_com_issue_error(hr);
		}

		DIVDLLLib::IDivideDll* pInt = static_cast<DIVDLLLib::IDivideDll*>(multiQi.pItf);
		divService.Attach(pInt);

		double a = 5.4;
		double b = 2.1;
		double result = divService->Divide(a, b);

		cout
			<< a << " / " << b << " = " << result << endl;
	}

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
								