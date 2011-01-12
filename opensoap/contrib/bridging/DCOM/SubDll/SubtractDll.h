// SubtractDll.h : CSubtractDll ÇÃêÈåæ

#ifndef __SUBTRACTDLL_H_
#define __SUBTRACTDLL_H_

#include "resource.h"       // ÉÅÉCÉì ÉVÉìÉ{Éã

/////////////////////////////////////////////////////////////////////////////
// CSubtractDll
class ATL_NO_VTABLE CSubtractDll : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSubtractDll, &CLSID_SubtractDll>,
	public ISubtractDll
{
public:
	CSubtractDll()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SUBTRACTDLL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSubtractDll)
	COM_INTERFACE_ENTRY(ISubtractDll)
END_COM_MAP()

// ISubtractDll
public:
	STDMETHOD(Subtract)(/*[in]*/ double a, /*[in]*/ double b, /*[out,retval]*/ double *result);
};

#endif //__SUBTRACTDLL_H_
