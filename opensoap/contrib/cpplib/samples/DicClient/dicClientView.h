/*-----------------------------------------------------------------------------
 * $RCSfile: dicClientView.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
// dicClientView.h : CDicClientView クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_DICCLIENTVIEW_H__EA8A3F3B_6EBC_48AD_A02A_851C3244EC66__INCLUDED_)
#define AFX_DICCLIENTVIEW_H__EA8A3F3B_6EBC_48AD_A02A_851C3244EC66__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "OpenSOAPInitializer.h"
#include "dicMethod.h"
#include <vector>

class CDicClientView : public CHtmlView
{
protected: // シリアライズ機能のみから作成します。
	CDicClientView();
	DECLARE_DYNCREATE(CDicClientView)

// アトリビュート
public:
	CDicClientDoc* GetDocument();

// オペレーション
public:

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CDicClientView)
	public:
	virtual void OnDraw(CDC* pDC);  // このビューを描画する際にオーバーライドされます。
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // 構築後の最初の１度だけ呼び出されます。
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	virtual ~CDicClientView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CDicClientView)
	afx_msg void OnSearch();
	afx_msg void OnDblclkItemlist();
	afx_msg void OnSelchangeItemlist();
	afx_msg void OnSelchangeDiclist();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void ChangeStatusMessage(const string& mes);
	CWnd* GetDlgItem(int nID);
	void GetDic();
	void SearchItem(const string& keyword);

//	std::vector<std::string> gdiList;
	SearchDicItem sdi;
	GetDicList gdl;

	Initializer initializer;
};

#ifndef _DEBUG  // dicClientView.cpp ファイルがデバッグ環境の時使用されます。
inline CDicClientDoc* CDicClientView::GetDocument()
   { return (CDicClientDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_DICCLIENTVIEW_H__EA8A3F3B_6EBC_48AD_A02A_851C3244EC66__INCLUDED_)
