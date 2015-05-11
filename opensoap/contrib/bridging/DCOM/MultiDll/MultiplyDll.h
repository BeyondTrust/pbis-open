// MultiplyDll.h : CMultiplyDll ÇÃêÈåæ

#ifndef __MULTIPLYDLL_H_
#define __MULTIPLYDLL_H_

#include "resource.h"       // ÉÅÉCÉì ÉVÉìÉ{Éã

/////////////////////////////////////////////////////////////////////////////
// CMultiplyDll
class ATL_NO_VTABLE CMultiplyDll : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMultiplyDll, &CLSID_MultiplyDll>,
	public IMultiplyDll
{
public:
	CMultiplyDll()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MULTIPLYDLL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMultiplyDll)
	COM_INTERFACE_ENTRY(IMultiplyDll)
END_COM_MAP()

// IMultiplyDll
public:
	STDMETHOD(Multiply)(/*[in]*/ double a, /*[in]*/ double b, /*[out,retval]*/ double *result);
};

#endif //__MULTIPLYDLL_H_
