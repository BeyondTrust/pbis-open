// SubtractDll.cpp : CSubtractDll のインプリメンテーション
#include "stdafx.h"
#include "SubDll.h"
#include "SubtractDll.h"

/////////////////////////////////////////////////////////////////////////////
// CSubtractDll


STDMETHODIMP CSubtractDll::Subtract(double a, double b, double *result)
{
	// TODO: この位置にインプリメント用のコードを追加してください
	HRESULT ret = S_FALSE;

	if(result) {
		*result = a - b;
		ret = S_OK;
	}

	return ret;
}
