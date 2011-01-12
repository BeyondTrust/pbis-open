// MultiplyExe.cpp : CMultiplyExe のインプリメンテーション
#include "stdafx.h"
#include "MultiExe.h"
#include "MultiplyExe.h"

/////////////////////////////////////////////////////////////////////////////
// CMultiplyExe


STDMETHODIMP CMultiplyExe::Multiply(double a, double b, double *result)
{
	// TODO: この位置にインプリメント用のコードを追加してください
	HRESULT ret = S_FALSE;

	if( result ) {
		*result = a * b;
		ret = S_OK;
	}

	return S_OK;
}
