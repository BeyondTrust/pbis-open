// AddDll.cpp : CAddDll のインプリメンテーション
#include "stdafx.h"
#include "AdDll.h"
#include "AddDll.h"

/////////////////////////////////////////////////////////////////////////////
// CAddDll


STDMETHODIMP CAddDll::Add(double a, double b, double *result)
{
	// TODO: この位置にインプリメント用のコードを追加してください
	HRESULT ret = S_FALSE;

	if( result ) {
		*result = a + b;
		ret = S_OK;
	}

	return ret;
}
