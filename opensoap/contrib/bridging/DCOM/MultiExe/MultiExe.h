/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri Jan 25 15:29:38 2002
 */
/* Compiler settings for C:\Djin\NEDO\Program\DCOM\MultiExe\MultiExe.idl:
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

#ifndef __MultiExe_h__
#define __MultiExe_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IMultiplyExe_FWD_DEFINED__
#define __IMultiplyExe_FWD_DEFINED__
typedef interface IMultiplyExe IMultiplyExe;
#endif 	/* __IMultiplyExe_FWD_DEFINED__ */


#ifndef __MultiplyExe_FWD_DEFINED__
#define __MultiplyExe_FWD_DEFINED__

#ifdef __cplusplus
typedef class MultiplyExe MultiplyExe;
#else
typedef struct MultiplyExe MultiplyExe;
#endif /* __cplusplus */

#endif 	/* __MultiplyExe_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IMultiplyExe_INTERFACE_DEFINED__
#define __IMultiplyExe_INTERFACE_DEFINED__

/* interface IMultiplyExe */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IMultiplyExe;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6B364159-B879-4FF7-B2DF-1B54F38EBA04")
    IMultiplyExe : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Multiply( 
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMultiplyExeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMultiplyExe __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMultiplyExe __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMultiplyExe __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Multiply )( 
            IMultiplyExe __RPC_FAR * This,
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result);
        
        END_INTERFACE
    } IMultiplyExeVtbl;

    interface IMultiplyExe
    {
        CONST_VTBL struct IMultiplyExeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMultiplyExe_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMultiplyExe_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMultiplyExe_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMultiplyExe_Multiply(This,a,b,result)	\
    (This)->lpVtbl -> Multiply(This,a,b,result)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMultiplyExe_Multiply_Proxy( 
    IMultiplyExe __RPC_FAR * This,
    /* [in] */ double a,
    /* [in] */ double b,
    /* [retval][out] */ double __RPC_FAR *result);


void __RPC_STUB IMultiplyExe_Multiply_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMultiplyExe_INTERFACE_DEFINED__ */



#ifndef __MULTIEXELib_LIBRARY_DEFINED__
#define __MULTIEXELib_LIBRARY_DEFINED__

/* library MULTIEXELib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_MULTIEXELib;

EXTERN_C const CLSID CLSID_MultiplyExe;

#ifdef __cplusplus

class DECLSPEC_UUID("A4101D61-F356-4F3F-BE52-F04D3023E177")
MultiplyExe;
#endif
#endif /* __MULTIEXELib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
