/*-----------------------------------------------------------------------------
 * $RCSfile: dicClientDoc.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
// dicClientDoc.h : CDicClientDoc クラスの宣言およびインターフェイスの定義をします。
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_DICCLIENTDOC_H__BE71A0E8_D79D_4DBA_809A_EF825546F112__INCLUDED_)
#define AFX_DICCLIENTDOC_H__BE71A0E8_D79D_4DBA_809A_EF825546F112__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CDicClientDoc : public CDocument
{
protected: // シリアライズ機能のみから作成します。
	CDicClientDoc();
	DECLARE_DYNCREATE(CDicClientDoc)

// アトリビュート
public:

// オペレーション
public:

//オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CDicClientDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	virtual ~CDicClientDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成されたメッセージ マップ関数
protected:
	//{{AFX_MSG(CDicClientDoc)
		// メモ - ClassWizard はこの位置にメンバ関数を追加または削除します。
		//        この位置に生成されるコードを編集しないでください。
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_DICCLIENTDOC_H__BE71A0E8_D79D_4DBA_809A_EF825546F112__INCLUDED_)
