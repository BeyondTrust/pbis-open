// MultiplyExe.h : CMultiplyExe ÇÃêÈåæ

#ifndef __MULTIPLYEXE_H_
#define __MULTIPLYEXE_H_

#include "resource.h"       // ÉÅÉCÉì ÉVÉìÉ{Éã

/////////////////////////////////////////////////////////////////////////////
// CMultiplyExe
class ATL_NO_VTABLE CMultiplyExe : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMultiplyExe, &CLSID_MultiplyExe>,
	public IMultiplyExe
{
public:
	CMultiplyExe()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MULTIPLYEXE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMultiplyExe)
	COM_INTERFACE_ENTRY(IMultiplyExe)
END_COM_MAP()

// IMultiplyExe
public:
	STDMETHOD(Multiply)(/*[in]*/ double a, /*[in]*/ double b, /*[out,retval]*/ double *result);
};

#endif //__MULTIPLYEXE_H_
