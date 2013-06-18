// AddExe.h : CAddExe ÇÃêÈåæ

#ifndef __ADDEXE_H_
#define __ADDEXE_H_

#include "resource.h"       // ÉÅÉCÉì ÉVÉìÉ{Éã

/////////////////////////////////////////////////////////////////////////////
// CAddExe
class ATL_NO_VTABLE CAddExe : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAddExe, &CLSID_AddExe>,
	public IAddExe
{
public:
	CAddExe()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_ADDEXE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAddExe)
	COM_INTERFACE_ENTRY(IAddExe)
END_COM_MAP()

// IAddExe
public:
	STDMETHOD(Add)(/*[in]*/ double a, /*[in]*/ double b, /*[out,retval]*/ double *result);
};

#endif //__ADDEXE_H_
