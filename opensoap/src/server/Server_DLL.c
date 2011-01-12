/*-----------------------------------------------------------------------------
 * $RCSfile: Server_DLL.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static    const	char	CVS_ID[]	=	"$Id: Server_DLL.c,v 1.1.12.1 2004/06/04 07:23:22 okada Exp $";
#endif  /* _DEBUG */

#include <windows.h>

/*
 Entry Point
*/
BOOL
WINAPI
DllMain(HINSTANCE	hDLL,
	DWORD		dwReason,
	LPVOID		lpReserved) {
    switch (dwReason) {
      case DLL_PROCESS_ATTACH:
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:
	  break;
    }

    return TRUE;
}
