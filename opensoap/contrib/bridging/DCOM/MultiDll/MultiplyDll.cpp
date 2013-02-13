// MultiplyDll.cpp : CMultiplyDll のインプリメンテーション
#include "stdafx.h"
#include "MultiDll.h"
#include "MultiplyDll.h"

/////////////////////////////////////////////////////////////////////////////
// CMultiplyDll


STDMETHODIMP CMultiplyDll::Multiply(double a, double b, double *result)
{
	// TODO: この位置にインプリメント用のコードを追加してください
	HRESULT ret = S_FALSE;

	if( result ) {
		*result = a * b;
		ret = S_OK;
	}

	return ret;
}
