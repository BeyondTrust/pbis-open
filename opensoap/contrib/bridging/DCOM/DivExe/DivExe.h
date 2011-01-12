/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri Jan 25 16:29:15 2002
 */
/* Compiler settings for C:\Djin\NEDO\Program\DCOM\DivExe\DivExe.idl:
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

#ifndef __DivExe_h__
#define __DivExe_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IDivideExe_FWD_DEFINED__
#define __IDivideExe_FWD_DEFINED__
typedef interface IDivideExe IDivideExe;
#endif 	/* __IDivideExe_FWD_DEFINED__ */


#ifndef __DivideExe_FWD_DEFINED__
#define __DivideExe_FWD_DEFINED__

#ifdef __cplusplus
typedef class DivideExe DivideExe;
#else
typedef struct DivideExe DivideExe;
#endif /* __cplusplus */

#endif 	/* __DivideExe_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IDivideExe_INTERFACE_DEFINED__
#define __IDivideExe_INTERFACE_DEFINED__

/* interface IDivideExe */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDivideExe;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("13736884-10E2-4B00-8297-A6F4E1856A33")
    IDivideExe : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Divide( 
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDivideExeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDivideExe __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDivideExe __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDivideExe __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Divide )( 
            IDivideExe __RPC_FAR * This,
            /* [in] */ double a,
            /* [in] */ double b,
            /* [retval][out] */ double __RPC_FAR *result);
        
        END_INTERFACE
    } IDivideExeVtbl;

    interface IDivideExe
    {
        CONST_VTBL struct IDivideExeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDivideExe_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDivideExe_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDivideExe_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDivideExe_Divide(This,a,b,result)	\
    (This)->lpVtbl -> Divide(This,a,b,result)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDivideExe_Divide_Proxy( 
    IDivideExe __RPC_FAR * This,
    /* [in] */ double a,
    /* [in] */ double b,
    /* [retval][out] */ double __RPC_FAR *result);


void __RPC_STUB IDivideExe_Divide_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDivideExe_INTERFACE_DEFINED__ */



#ifndef __DIVEXELib_LIBRARY_DEFINED__
#define __DIVEXELib_LIBRARY_DEFINED__

/* library DIVEXELib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DIVEXELib;

EXTERN_C const CLSID CLSID_DivideExe;

#ifdef __cplusplus

class DECLSPEC_UUID("B3980D3B-02DD-4A30-9103-8B49E6618F30")
DivideExe;
#endif
#endif /* __DIVEXELib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
