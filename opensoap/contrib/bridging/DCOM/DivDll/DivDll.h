/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue Jan 29 14:37:24 2002
 */
/* Compiler settings for C:\Djin\NEDO\Program\DCOM\DivDll\DivDll.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
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

#ifndef __DivDll_h__
#define __DivDll_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IDivideDll_FWD_DEFINED__
#define __IDivideDll_FWD_DEFINED__
typedef interface IDivideDll IDivideDll;
#endif 	/* __IDivideDll_FWD_DEFINED__ */


#ifndef __DivideDll_FWD_DEFINED__
#define __DivideDll_FWD_DEFINED__

#ifdef __cplusplus
typedef class DivideDll DivideDll;
#else
typedef struct DivideDll DivideDll;
#endif /* __cplusplus */

#endif 	/* __DivideDll_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IDivideDll_INTERFACE_DEFINED__
#define __IDivideDll_INTERFACE_DEFINED__

/* interface IDivideDll */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDivideDll;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("219B17CF-1D60-46C4-9991-1376B7C9CA16")
    IDivideDll : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Divide( 
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDivideDllVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDivideDll __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDivideDll __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDivideDll __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Divide )( 
            IDivideDll __RPC_FAR * This,
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result);
        
        END_INTERFACE
    } IDivideDllVtbl;

    interface IDivideDll
    {
        CONST_VTBL struct IDivideDllVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDivideDll_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDivideDll_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDivideDll_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDivideDll_Divide(This,a,b,result)	\
    (This)->lpVtbl -> Divide(This,a,b,result)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDivideDll_Divide_Proxy( 
    IDivideDll __RPC_FAR * This,
    /* [in] */ double a,
    /* [in] */ double b,
    /* [retval][out] */ double __RPC_FAR *result);


void __RPC_STUB IDivideDll_Divide_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDivideDll_INTERFACE_DEFINED__ */



#ifndef __DIVDLLLib_LIBRARY_DEFINED__
#define __DIVDLLLib_LIBRARY_DEFINED__

/* library DIVDLLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DIVDLLLib;

EXTERN_C const CLSID CLSID_DivideDll;

#ifdef __cplusplus

class DECLSPEC_UUID("E20519F0-35E5-4FD4-AEDA-64079ED8A952")
DivideDll;
#endif
#endif /* __DIVDLLLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
