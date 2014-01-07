/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue Jan 29 10:57:24 2002
 */
/* Compiler settings for C:\Djin\NEDO\Program\DCOM\MultiDll\MultiDll.idl:
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

#ifndef __MultiDll_h__
#define __MultiDll_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IMultiplyDll_FWD_DEFINED__
#define __IMultiplyDll_FWD_DEFINED__
typedef interface IMultiplyDll IMultiplyDll;
#endif 	/* __IMultiplyDll_FWD_DEFINED__ */


#ifndef __MultiplyDll_FWD_DEFINED__
#define __MultiplyDll_FWD_DEFINED__

#ifdef __cplusplus
typedef class MultiplyDll MultiplyDll;
#else
typedef struct MultiplyDll MultiplyDll;
#endif /* __cplusplus */

#endif 	/* __MultiplyDll_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IMultiplyDll_INTERFACE_DEFINED__
#define __IMultiplyDll_INTERFACE_DEFINED__

/* interface IMultiplyDll */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IMultiplyDll;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C9E20848-FE2D-49B4-836F-1159E612E4FD")
    IMultiplyDll : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Multiply( 
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMultiplyDllVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMultiplyDll __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMultiplyDll __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMultiplyDll __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Multiply )( 
            IMultiplyDll __RPC_FAR * This,
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result);
        
        END_INTERFACE
    } IMultiplyDllVtbl;

    interface IMultiplyDll
    {
        CONST_VTBL struct IMultiplyDllVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMultiplyDll_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMultiplyDll_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMultiplyDll_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMultiplyDll_Multiply(This,a,b,result)	\
    (This)->lpVtbl -> Multiply(This,a,b,result)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMultiplyDll_Multiply_Proxy( 
    IMultiplyDll __RPC_FAR * This,
    /* [in] */ double a,
    /* [in] */ double b,
    /* [retval][out] */ double __RPC_FAR *result);


void __RPC_STUB IMultiplyDll_Multiply_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMultiplyDll_INTERFACE_DEFINED__ */



#ifndef __MULTIDLLLib_LIBRARY_DEFINED__
#define __MULTIDLLLib_LIBRARY_DEFINED__

/* library MULTIDLLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_MULTIDLLLib;

EXTERN_C const CLSID CLSID_MultiplyDll;

#ifdef __cplusplus

class DECLSPEC_UUID("5A231398-BFF1-43DA-8E38-502F7161830E")
MultiplyDll;
#endif
#endif /* __MULTIDLLLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
