#define _WIN32_DCOM

#import "MultiDll.tlb"
//using namespace MULTIDLLLib;

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
		MULTIDLLLib::IMultiplyDllPtr multiService;
		COSERVERINFO serverInfo;
		MULTI_QI multiQi;

		serverInfo.dwReserved1 = NULL;
		serverInfo.pwszName = L"JD-NOTE";
//		serverInfo.pwszName = L"MIHARU";
		serverInfo.pAuthInfo = NULL;
		serverInfo.dwReserved2 = NULL;

		multiQi.pIID = &__uuidof(MULTIDLLLib::IMultiplyDll);
		multiQi.pItf = NULL;
		multiQi.hr = S_OK;

		hr = CoCreateInstanceEx(__uuidof(MULTIDLLLib::MultiplyDll),
								NULL,
								CLSCTX_REMOTE_SERVER,
								&serverInfo,
								1,
								&multiQi);
		if(FAILED(hr)) {
			_com_issue_error(hr);
		}

		MULTIDLLLib::IMultiplyDll* pInt = static_cast<MULTIDLLLib::IMultiplyDll*>(multiQi.pItf);
		multiService.Attach(pInt);

		double a = 5.4;
		double b = 3.2;
		double result = multiService->Multiply(a, b);

		cout
			<< a << " * " << b << " = " << result << endl;
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
								