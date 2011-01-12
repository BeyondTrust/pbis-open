/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Dec 03 10:58:42 2001
 */
/* Compiler settings for C:\Djin\NEDO\Program\DCOM\ServerInfo\ServerInfo.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ServerInfo_h__
#define __ServerInfo_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IDiskInfo_FWD_DEFINED__
#define __IDiskInfo_FWD_DEFINED__
typedef interface IDiskInfo IDiskInfo;
#endif 	/* __IDiskInfo_FWD_DEFINED__ */


#ifndef __DiskInfo_FWD_DEFINED__
#define __DiskInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class DiskInfo DiskInfo;
#else
typedef struct DiskInfo DiskInfo;
#endif /* __cplusplus */

#endif 	/* __DiskInfo_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IDiskInfo_INTERFACE_DEFINED__
#define __IDiskInfo_INTERFACE_DEFINED__

/* interface IDiskInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDiskInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A5442D0D-D20C-4902-94B5-E9922E9DEA53")
    IDiskInfo : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetFreeDiskSpace( 
            /* [string][in] */ const wchar_t __RPC_FAR *wszDrive,
            /* [out] */ hyper __RPC_FAR *hypFreeBytes) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiskInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDiskInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDiskInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDiskInfo __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFreeDiskSpace )( 
            IDiskInfo __RPC_FAR * This,
            /* [string][in] */ const wchar_t __RPC_FAR *wszDrive,
            /* [out] */ hyper __RPC_FAR *hypFreeBytes);
        
        END_INTERFACE
    } IDiskInfoVtbl;

    interface IDiskInfo
    {
        CONST_VTBL struct IDiskInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiskInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiskInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiskInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiskInfo_GetFreeDiskSpace(This,wszDrive,hypFreeBytes)	\
    (This)->lpVtbl -> GetFreeDiskSpace(This,wszDrive,hypFreeBytes)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiskInfo_GetFreeDiskSpace_Proxy( 
    IDiskInfo __RPC_FAR * This,
    /* [string][in] */ const wchar_t __RPC_FAR *wszDrive,
    /* [out] */ hyper __RPC_FAR *hypFreeBytes);


void __RPC_STUB IDiskInfo_GetFreeDiskSpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiskInfo_INTERFACE_DEFINED__ */



#ifndef __SERVERINFOLib_LIBRARY_DEFINED__
#define __SERVERINFOLib_LIBRARY_DEFINED__

/* library SERVERINFOLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_SERVERINFOLib;

EXTERN_C const CLSID CLSID_DiskInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("192613E1-373D-4CCA-9A36-633AE18E2B3F")
DiskInfo;
#endif
#endif /* __SERVERINFOLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
