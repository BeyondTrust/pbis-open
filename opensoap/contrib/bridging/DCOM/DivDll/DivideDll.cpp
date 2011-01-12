// DivideDll.cpp : CDivideDll のインプリメンテーション
#include "stdafx.h"
#include "DivDll.h"
#include "DivideDll.h"

/////////////////////////////////////////////////////////////////////////////
// CDivideDll


STDMETHODIMP CDivideDll::Divide(double a, double b, double *result)
{
	// TODO: この位置にインプリメント用のコードを追加してください

	HRESULT ret = S_FALSE;

	if( result && b ) {
		*result = a / b;
		ret = S_OK;
	}

	return ret;
}
