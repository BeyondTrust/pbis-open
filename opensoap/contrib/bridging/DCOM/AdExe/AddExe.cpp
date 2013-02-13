// AddExe.cpp : CAddExe のインプリメンテーション
#include "stdafx.h"
#include "AdExe.h"
#include "AddExe.h"

/////////////////////////////////////////////////////////////////////////////
// CAddExe


STDMETHODIMP CAddExe::Add(double a, double b, double *result)
{
	// TODO: この位置にインプリメント用のコードを追加してください
	HRESULT ret = S_FALSE;

	if( result ) {
		*result = a + b;
		ret = S_OK;
	}

	return ret;
}
