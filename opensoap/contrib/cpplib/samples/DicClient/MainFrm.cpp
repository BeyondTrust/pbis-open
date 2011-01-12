/*-----------------------------------------------------------------------------
 * $RCSfile: MainFrm.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
// MainFrm.cpp : CMainFrame クラスの動作の定義を行います。
//

#include "stdafx.h"
#include "dicClient.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
		// メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
		//        この位置に生成されるコードを編集しないでください。
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // ステータス ライン インジケータ
	ID_INDICATOR_KANA,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame クラスの構築/消滅

CMainFrame::CMainFrame()
{
	// TODO: この位置にメンバの初期化処理コードを追加してください。
	
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

/*	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // 作成に失敗
	}
*/
	if (!m_wndDialogBar.Create(this, IDD_DIALOGBAR, CBRS_LEFT
		| CBRS_TOOLTIPS | CBRS_FLYBY, 0))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;      // 作成に失敗
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // 作成に失敗
	}

	// TODO: ツール バーをドッキング可能にしない場合は以下の３行を削除
	//       してください。
	m_wndDialogBar.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndDialogBar);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: この位置で CREATESTRUCT cs を修正して、Window クラスやスタイルを
	//       修正してください。

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame クラスの診断

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame メッセージ ハンドラ


CDialogBar* CMainFrame::GetDialogBar()
{
	return &m_wndDialogBar;
}

void CMainFrame::ChangeStatusMessage(char *mes)
{
	m_wndStatusBar.SetWindowText(mes);
}
