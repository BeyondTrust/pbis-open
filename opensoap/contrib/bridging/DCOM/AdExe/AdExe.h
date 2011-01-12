/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Jan 31 11:01:22 2002
 */
/* Compiler settings for C:\Djin\NEDO\Program\DCOM\AdExe\AdExe.idl:
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

#ifndef __AdExe_h__
#define __AdExe_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IAddExe_FWD_DEFINED__
#define __IAddExe_FWD_DEFINED__
typedef interface IAddExe IAddExe;
#endif 	/* __IAddExe_FWD_DEFINED__ */


#ifndef __AddExe_FWD_DEFINED__
#define __AddExe_FWD_DEFINED__

#ifdef __cplusplus
typedef class AddExe AddExe;
#else
typedef struct AddExe AddExe;
#endif /* __cplusplus */

#endif 	/* __AddExe_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IAddExe_INTERFACE_DEFINED__
#define __IAddExe_INTERFACE_DEFINED__

/* interface IAddExe */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAddExe;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("50D98FB1-6166-4C68-9540-F2FDF56A4EA7")
    IAddExe : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAddExeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAddExe __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAddExe __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAddExe __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IAddExe __RPC_FAR * This,
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result);
        
        END_INTERFACE
    } IAddExeVtbl;

    interface IAddExe
    {
        CONST_VTBL struct IAddExeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAddExe_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAddExe_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAddExe_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAddExe_Add(This,a,b,result)	\
    (This)->lpVtbl -> Add(This,a,b,result)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAddExe_Add_Proxy( 
    IAddExe __RPC_FAR * This,
    /* [in] */ double a,
    /* [in] */ double b,
    /* [retval][out] */ double __RPC_FAR *result);


void __RPC_STUB IAddExe_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAddExe_INTERFACE_DEFINED__ */



#ifndef __ADEXELib_LIBRARY_DEFINED__
#define __ADEXELib_LIBRARY_DEFINED__

/* library ADEXELib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ADEXELib;

EXTERN_C const CLSID CLSID_AddExe;

#ifdef __cplusplus

class DECLSPEC_UUID("7DC6AAAA-DDF1-481B-A35F-5D2AEA432728")
AddExe;
#endif
#endif /* __ADEXELib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
