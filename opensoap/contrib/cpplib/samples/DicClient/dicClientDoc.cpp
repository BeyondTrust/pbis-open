/*-----------------------------------------------------------------------------
 * $RCSfile: dicClientDoc.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
// dicClientDoc.cpp : CDicClientDoc クラスの動作の定義を行います。
//

#include "stdafx.h"
#include "dicClient.h"

#include "dicClientDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDicClientDoc

IMPLEMENT_DYNCREATE(CDicClientDoc, CDocument)

BEGIN_MESSAGE_MAP(CDicClientDoc, CDocument)
	//{{AFX_MSG_MAP(CDicClientDoc)
		// メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
		//        この位置に生成されるコードを編集しないでください。
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDicClientDoc クラスの構築/消滅

CDicClientDoc::CDicClientDoc()
{
	// TODO: この位置に１度だけ呼ばれる構築用のコードを追加してください。

}

CDicClientDoc::~CDicClientDoc()
{
}

BOOL CDicClientDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: この位置に再初期化処理を追加してください。
	// (SDI ドキュメントはこのドキュメントを再利用します。)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CDicClientDoc シリアライゼーション

void CDicClientDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: この位置に保存用のコードを追加してください。
	}
	else
	{
		// TODO: この位置に読み込み用のコードを追加してください。
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDicClientDoc クラスの診断

#ifdef _DEBUG
void CDicClientDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CDicClientDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDicClientDoc コマンド
