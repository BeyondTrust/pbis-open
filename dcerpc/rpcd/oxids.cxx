/* Import, Cache and Export OXIDs:
 * 
 * The ORPC/DCOM Object Exporter
 * */

#if ENABLE_DCOM
extern "C"	{
#include <commonp.h>
#include <com.h>

#include <dce/objex.h>
};

error_status_t objex_ResolveOxid(
    /* [in] */ handle_t hRpc,
    /* [in] */ OXID *pOxid,
    /* [in] */ idl_ushort_int cRequestedProtseqs,
    /* [in] */ idl_ushort_int arRequestedProtseqs[],
    /* [out] */ DUALSTRINGARRAY **psaOxidBindings,
    /* [out] */ IPID *pipidRemUnknown,
    /* [out] */ DWORD *pAuthnHint
)
{
	/* Brain-dead version */
	return rpc_s_ok;
}

error_status_t objex_SimplePing(
    /* [in] */ handle_t hRpc,
    /* [in] */ SETID *pSetId
)
{
	return rpc_s_ok;

}


error_status_t objex_ComplexPing(
    /* [in] */ handle_t hRpc,
    /* [in, out] */ SETID *pSetId,
    /* [in] */ idl_ushort_int SequenceNum,
    /* [in] */ idl_ushort_int cAddToSet,
    /* [in] */ idl_ushort_int cDelFromSet,
    /* [in] */ OID AddToSet[],
    /* [in] */ OID DelFromSet[],
    /* [out] */ idl_ushort_int *pPingBackoffFactor
)
{
	return rpc_s_ok;

}


error_status_t objex_ServerAlive(
    /* [in] */ handle_t hRpc
)
{
	return rpc_s_ok;
}


error_status_t objex_ResolveOxid2(
    /* [in] */ handle_t hRpc,
    /* [in] */ OXID *pOxid,
    /* [in] */ idl_ushort_int cRequestedProtseqs,
    /* [in] */ idl_ushort_int arRequestedProtseqs[],
    /* [out] */ DUALSTRINGARRAY **ppdsaOxidBindings,
    /* [out] */ IPID *pipidRemUnknown,
    /* [out] */ DWORD *pAuthnHint,
    /* [out] */ COMVERSION *pComVersion
)
{
	return rpc_s_ok;
}


GLOBAL IObjectExporter_v0_0_epv_t objex_mgr_epv	=
{
	objex_ResolveOxid,
	objex_SimplePing,
	objex_ComplexPing,
	objex_ServerAlive,
	objex_ResolveOxid2
};


#else
/* ANSI requires this */
int objex_dummy_var;
#endif
