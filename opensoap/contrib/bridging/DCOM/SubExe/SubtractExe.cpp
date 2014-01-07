// SubtractExe.cpp : CSubtractExe のインプリメンテーション
#include "stdafx.h"
#include "SubExe.h"
#include "SubtractExe.h"

/////////////////////////////////////////////////////////////////////////////
// CSubtractExe


STDMETHODIMP CSubtractExe::Subtract(double a, double b, double *result)
{
	// TODO: この位置にインプリメント用のコードを追加してください

	HRESULT ret = S_FALSE;

	if(result) {
		*result = a - b;
		ret = S_OK;
	}

	return ret;
}
