/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri Jan 25 12:24:28 2002
 */
/* Compiler settings for C:\Djin\NEDO\Program\DCOM\SubExe\SubExe.idl:
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

#ifndef __SubExe_h__
#define __SubExe_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ISubtractExe_FWD_DEFINED__
#define __ISubtractExe_FWD_DEFINED__
typedef interface ISubtractExe ISubtractExe;
#endif 	/* __ISubtractExe_FWD_DEFINED__ */


#ifndef __SubtractExe_FWD_DEFINED__
#define __SubtractExe_FWD_DEFINED__

#ifdef __cplusplus
typedef class SubtractExe SubtractExe;
#else
typedef struct SubtractExe SubtractExe;
#endif /* __cplusplus */

#endif 	/* __SubtractExe_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ISubtractExe_INTERFACE_DEFINED__
#define __ISubtractExe_INTERFACE_DEFINED__

/* interface ISubtractExe */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISubtractExe;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BB54CDDC-5F54-48C5-A952-B3AC363111B8")
    ISubtractExe : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Subtract( 
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISubtractExeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISubtractExe __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISubtractExe __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISubtractExe __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Subtract )( 
            ISubtractExe __RPC_FAR * This,
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result);
        
        END_INTERFACE
    } ISubtractExeVtbl;

    interface ISubtractExe
    {
        CONST_VTBL struct ISubtractExeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISubtractExe_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISubtractExe_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISubtractExe_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISubtractExe_Subtract(This,a,b,result)	\
    (This)->lpVtbl -> Subtract(This,a,b,result)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISubtractExe_Subtract_Proxy( 
    ISubtractExe __RPC_FAR * This,
    /* [in] */ double a,
    /* [in] */ double b,
    /* [retval][out] */ double __RPC_FAR *result);


void __RPC_STUB ISubtractExe_Subtract_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISubtractExe_INTERFACE_DEFINED__ */



#ifndef __SUBEXELib_LIBRARY_DEFINED__
#define __SUBEXELib_LIBRARY_DEFINED__

/* library SUBEXELib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_SUBEXELib;

EXTERN_C const CLSID CLSID_SubtractExe;

#ifdef __cplusplus

class DECLSPEC_UUID("AE1166EE-04DD-41DF-804C-54D82919A1C6")
SubtractExe;
#endif
#endif /* __SUBEXELib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
