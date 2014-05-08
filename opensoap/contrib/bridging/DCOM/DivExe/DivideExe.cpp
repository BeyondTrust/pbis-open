// DivideExe.cpp : CDivideExe のインプリメンテーション
#include "stdafx.h"
#include "DivExe.h"
#include "DivideExe.h"

/////////////////////////////////////////////////////////////////////////////
// CDivideExe


STDMETHODIMP CDivideExe::Divide(double a, double b, double *result)
{
	// TODO: この位置にインプリメント用のコードを追加してください
	HRESULT ret = S_FALSE;

	if( result && b ) {
		*result = a / b;
		ret = S_OK;
	}

	return ret;
}
