OpenSOAP DCOM Bridging Service Usage Description


1. DCOM Component Registration
   The DCOM component MUST be registered on both the client and server machines.

1.1 Required Files

1.1.1 For the DLL Server :
      (1) ***.dll
          Files providing class information
      (2) ***ps.dll
          Files providing information about the proxy server interface

1.1.2 For the EXE Server :
      (1) ***.exe
          Files providing class information
      (2) ***ps.dll
          Files providing information about the proxy server interface

1.2 Registration of Class and Proxy Interface

1.2.1 For the DLL Server :
      (1) Class Registration
           regsvr32 ***.dll
      (2) Proxy Interface Registration
           regsvr32 ***ps.dll

1.2.2 For the EXE Server :
      (1) Class Registration
           ***.exe /regserver
      (2) Proxy Interface Registration
           regsvr32 ***ps.dll

1.3 Registration Verification
    Run the OLE/COM Object Viewer(OleView.exe). Select [Expert Mode] in the [View] menu.
    Open the [All Objects] tree, and verify that [*** Class] has been registered.

2. Creating the OpenSOAP DCOM bBridging Service
2.1 Required Files
    (1) ***.tlb
        DCOM component object type library files

2.2 Creating the Service
    The OpenSOAP bridging service is one type of OpenSOAP service and because it is used at the DCOM component
    side, the OpenSOAP DCOM bridging service is located at the DCOM client side. When creating, import the "***.tlb"
    files, and when using call the DCOM object on the remote machine with the function below.
    CoCreateInstanceEx()

2.3 Sample(C Language)
    Please refer to the DCOM bridging sample for details.

    For example, implementation of a simple arithmetic subtration service .

#define _WIN32_DCOM

#import "SubExe.tlb"

#include <iostream>

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

	try{
		SUBEXELib::ISubtractExePtr subService;
		COSERVERINFO serverInfo;
		MULTI_QI multiQi;

		serverInfo.dwReserved1 = NULL;
		serverInfo.pwszName = L"MIHARU";/* name of remote DCOM server machine */
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

		SUBEXELib::ISubtractExe* pInt =
			 static_cast<SUBEXELib::ISubtractExe*>(multiQi.pItf);
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

3. Configuration to run the DLL Server
3.1 Surrogate Process
    Using DllHost.exe it is possible to remotely access the DLL server.

3.2 Surrogate Process Configuration
    Run the OLE/COM Object Viewer(OleView.exe). Enable [Expert Mode] in the [View] menu.
    Next, open the [All Objects] tree, and find the [*** Class].
    Select the class in the tree view, and on the right hand side window select [Implementation] and [Inproc Server].
    Click on the [Use Surrogate Process] box. However, if the object listed in [Path to Custom Surrogate] cannot
    be found, the DllSurrogate cannot be added using Object Viewer. In this case, after inserting a single space in
    [Path to Custom Surrogate] delete it. In this way, the server will be connected to the DllHost.exe surrogate process.
    Clicking on any other object updates the registry.

4. Configuration for running the EXE Server
    There are no special  requirements.

5. DCOM Operation Verification

5.1 For the DLL Server :
    Open the Task Manager on the DCOM EXE machine. Verify that the DllHost.exe process is runs when the DCOM Object in
    the DCOM Server is called by the OpenSOAP DCOM bridging service.

5.2 For the EXE Server :
    Open the Task Manager on the DCOM EXE machine. Verify that the ***.exe process is runs when the DCOM Object in
    the DCOM Server is called by the OpenSOAP DCOM bridging service.

