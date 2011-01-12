/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Jan 31 11:49:20 2002
 */
/* Compiler settings for C:\Djin\NEDO\Program\DCOM\AdDll\AdDll.idl:
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

#ifndef __AdDll_h__
#define __AdDll_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IAddDll_FWD_DEFINED__
#define __IAddDll_FWD_DEFINED__
typedef interface IAddDll IAddDll;
#endif 	/* __IAddDll_FWD_DEFINED__ */


#ifndef __AddDll_FWD_DEFINED__
#define __AddDll_FWD_DEFINED__

#ifdef __cplusplus
typedef class AddDll AddDll;
#else
typedef struct AddDll AddDll;
#endif /* __cplusplus */

#endif 	/* __AddDll_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IAddDll_INTERFACE_DEFINED__
#define __IAddDll_INTERFACE_DEFINED__

/* interface IAddDll */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAddDll;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E40DC641-2D09-46CF-8554-9BC97ADD0F57")
    IAddDll : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAddDllVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAddDll __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAddDll __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAddDll __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IAddDll __RPC_FAR * This,
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result);
        
        END_INTERFACE
    } IAddDllVtbl;

    interface IAddDll
    {
        CONST_VTBL struct IAddDllVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAddDll_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAddDll_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAddDll_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAddDll_Add(This,a,b,result)	\
    (This)->lpVtbl -> Add(This,a,b,result)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAddDll_Add_Proxy( 
    IAddDll __RPC_FAR * This,
    /* [in] */ double a,
    /* [in] */ double b,
    /* [retval][out] */ double __RPC_FAR *result);


void __RPC_STUB IAddDll_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAddDll_INTERFACE_DEFINED__ */



#ifndef __ADDLLLib_LIBRARY_DEFINED__
#define __ADDLLLib_LIBRARY_DEFINED__

/* library ADDLLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ADDLLLib;

EXTERN_C const CLSID CLSID_AddDll;

#ifdef __cplusplus

class DECLSPEC_UUID("9422BEA4-2572-42FA-A5A3-B8D0B73D8292")
AddDll;
#endif
#endif /* __ADDLLLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
