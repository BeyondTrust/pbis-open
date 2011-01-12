// SubtractExe.h : CSubtractExe ÇÃêÈåæ

#ifndef __SUBTRACTEXE_H_
#define __SUBTRACTEXE_H_

#include "resource.h"       // ÉÅÉCÉì ÉVÉìÉ{Éã

/////////////////////////////////////////////////////////////////////////////
// CSubtractExe
class ATL_NO_VTABLE CSubtractExe : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSubtractExe, &CLSID_SubtractExe>,
	public ISubtractExe
{
public:
	CSubtractExe()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SUBTRACTEXE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSubtractExe)
	COM_INTERFACE_ENTRY(ISubtractExe)
END_COM_MAP()

// ISubtractExe
public:
	STDMETHOD(Subtract)(/*[in]*/ double a, /*[in]*/ double b, /*[out,retval]*/ double *result);
};

#endif //__SUBTRACTEXE_H_
