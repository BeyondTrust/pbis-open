// DiskInfo.h : CDiskInfo ÇÃêÈåæ

#ifndef __DISKINFO_H_
#define __DISKINFO_H_

#include "resource.h"       // ÉÅÉCÉì ÉVÉìÉ{Éã

/////////////////////////////////////////////////////////////////////////////
// CDiskInfo
class ATL_NO_VTABLE CDiskInfo : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDiskInfo, &CLSID_DiskInfo>,
	public IDiskInfo
{
public:
	CDiskInfo()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_DISKINFO)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDiskInfo)
	COM_INTERFACE_ENTRY(IDiskInfo)
END_COM_MAP()

// IDiskInfo
public:
	STDMETHOD(GetFreeDiskSpace)(const wchar_t* wszDrive, /*[out]*/ hyper* hypFreeBytes);
};

#endif //__DISKINFO_H_
