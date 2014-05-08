// DivideDll.h : CDivideDll ÇÃêÈåæ

#ifndef __DIVIDEDLL_H_
#define __DIVIDEDLL_H_

#include "resource.h"       // ÉÅÉCÉì ÉVÉìÉ{Éã

/////////////////////////////////////////////////////////////////////////////
// CDivideDll
class ATL_NO_VTABLE CDivideDll : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDivideDll, &CLSID_DivideDll>,
	public IDivideDll
{
public:
	CDivideDll()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_DIVIDEDLL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDivideDll)
	COM_INTERFACE_ENTRY(IDivideDll)
END_COM_MAP()

// IDivideDll
public:
	STDMETHOD(Divide)(/*[in]*/ double a, /*[in]*/ double b, /*[out,retval]*/ double *result);
};

#endif //__DIVIDEDLL_H_
