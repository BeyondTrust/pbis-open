// DivideExe.h : CDivideExe ÇÃêÈåæ

#ifndef __DIVIDEEXE_H_
#define __DIVIDEEXE_H_

#include "resource.h"       // ÉÅÉCÉì ÉVÉìÉ{Éã

/////////////////////////////////////////////////////////////////////////////
// CDivideExe
class ATL_NO_VTABLE CDivideExe : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDivideExe, &CLSID_DivideExe>,
	public IDivideExe
{
public:
	CDivideExe()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_DIVIDEEXE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDivideExe)
	COM_INTERFACE_ENTRY(IDivideExe)
END_COM_MAP()

// IDivideExe
public:
	STDMETHOD(Divide)(/*[in]*/ double a, /*[in]*/ double b, /*[out,retval]*/ double *result);
};

#endif //__DIVIDEEXE_H_
