/*-----------------------------------------------------------------------------
 * $RCSfile: Service_DLL.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static    const	char	CVS_ID[]	=	"$Id: Service_DLL.c,v 1.4 2002/10/17 08:40:23 bandou Exp $";
#endif  /* _DEBUG */

#include <windows.h>

/*
=begin
= Service_DLL.c
OpenSOAPService.DLL Entry Point.
=end 
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
