#define _WIN32_DCOM

#import "MultiExe.tlb"
//using namespace MULTIEXELib;

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
		MULTIEXELib::IMultiplyExePtr multiService;
		COSERVERINFO serverInfo;
		MULTI_QI multiQi;

		serverInfo.dwReserved1 = NULL;
		serverInfo.pwszName = L"JD-NOTE";
//		serverInfo.pwszName = L"MIHARU";
		serverInfo.pAuthInfo = NULL;
		serverInfo.dwReserved2 = NULL;

		multiQi.pIID = &__uuidof(MULTIEXELib::IMultiplyExe);
		multiQi.pItf = NULL;
		multiQi.hr = S_OK;

		hr = CoCreateInstanceEx(__uuidof(MULTIEXELib::MultiplyExe),
								NULL,
								CLSCTX_REMOTE_SERVER,
								&serverInfo,
								1,
								&multiQi);
		if(FAILED(hr)) {
			_com_issue_error(hr);
		}

		MULTIEXELib::IMultiplyExe* pInt = static_cast<MULTIEXELib::IMultiplyExe*>(multiQi.pItf);
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
								