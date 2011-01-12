/*-----------------------------------------------------------------------------
 * $RCSfile: dicClient.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
// dicClient.h : DICCLIENT アプリケーションのメイン ヘッダー ファイル
//

#if !defined(AFX_DICCLIENT_H__B833339E_18A2_45AD_8A5B_6D3C3484A269__INCLUDED_)
#define AFX_DICCLIENT_H__B833339E_18A2_45AD_8A5B_6D3C3484A269__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // メイン シンボル

/////////////////////////////////////////////////////////////////////////////
// CDicClientApp:
// このクラスの動作の定義に関しては dicClient.cpp ファイルを参照してください。
//

class CDicClientApp : public CWinApp
{
public:
	CDicClientApp();

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CDicClientApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// インプリメンテーション
	//{{AFX_MSG(CDicClientApp)
	afx_msg void OnAppAbout();
		// メモ - ClassWizard はこの位置にメンバ関数を追加または削除します。
		//        この位置に生成されるコードを編集しないでください。
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_DICCLIENT_H__B833339E_18A2_45AD_8A5B_6D3C3484A269__INCLUDED_)
