/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Jan 28 11:27:22 2002
 */
/* Compiler settings for C:\Djin\NEDO\Program\DCOM\SubDll\SubDll.idl:
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

#ifndef __SubDll_h__
#define __SubDll_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ISubtractDll_FWD_DEFINED__
#define __ISubtractDll_FWD_DEFINED__
typedef interface ISubtractDll ISubtractDll;
#endif 	/* __ISubtractDll_FWD_DEFINED__ */


#ifndef __SubtractDll_FWD_DEFINED__
#define __SubtractDll_FWD_DEFINED__

#ifdef __cplusplus
typedef class SubtractDll SubtractDll;
#else
typedef struct SubtractDll SubtractDll;
#endif /* __cplusplus */

#endif 	/* __SubtractDll_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ISubtractDll_INTERFACE_DEFINED__
#define __ISubtractDll_INTERFACE_DEFINED__

/* interface ISubtractDll */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISubtractDll;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7764B9B5-1607-42EE-8295-139268E6FFF7")
    ISubtractDll : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Subtract( 
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISubtractDllVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISubtractDll __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISubtractDll __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISubtractDll __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Subtract )( 
            ISubtractDll __RPC_FAR * This,
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result);
        
        END_INTERFACE
    } ISubtractDllVtbl;

    interface ISubtractDll
    {
        CONST_VTBL struct ISubtractDllVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISubtractDll_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISubtractDll_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISubtractDll_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISubtractDll_Subtract(This,a,b,result)	\
    (This)->lpVtbl -> Subtract(This,a,b,result)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISubtractDll_Subtract_Proxy( 
    ISubtractDll __RPC_FAR * This,
    /* [in] */ double a,
    /* [in] */ double b,
    /* [retval][out] */ double __RPC_FAR *result);


void __RPC_STUB ISubtractDll_Subtract_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISubtractDll_INTERFACE_DEFINED__ */



#ifndef __SUBDLLLib_LIBRARY_DEFINED__
#define __SUBDLLLib_LIBRARY_DEFINED__

/* library SUBDLLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_SUBDLLLib;

EXTERN_C const CLSID CLSID_SubtractDll;

#ifdef __cplusplus

class DECLSPEC_UUID("FA5ED1D1-91A1-4018-9391-C272F5BA5518")
SubtractDll;
#endif
#endif /* __SUBDLLLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
