/*-----------------------------------------------------------------------------
 * $RCSfile: dicClientView.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
// dicClientView.cpp : CDicClientView クラスの動作の定義を行います。
//

#include "stdafx.h"
#include <direct.h>

#include "dicClient.h"
#include "MainFrm.h"
#include "dicClientDoc.h"
#include "dicClientView.h"

#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDicClientView

IMPLEMENT_DYNCREATE(CDicClientView, CHtmlView)

BEGIN_MESSAGE_MAP(CDicClientView, CHtmlView)
	//{{AFX_MSG_MAP(CDicClientView)
	ON_BN_CLICKED(IDC_SEARCH, OnSearch)
	ON_LBN_DBLCLK(IDC_ITEMLIST, OnDblclkItemlist)
	ON_LBN_SELCHANGE(IDC_ITEMLIST, OnSelchangeItemlist)
	ON_CBN_SELCHANGE(IDC_DICLIST, OnSelchangeDiclist)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDicClientView クラスの構築/消滅

CDicClientView::CDicClientView()
:initializer(0)
{
}

CDicClientView::~CDicClientView()
{
}

BOOL CDicClientView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: この位置で CREATESTRUCT cs を修正して Window クラスまたはスタイルを
	//  修正してください。

	return CHtmlView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CDicClientView クラスの描画

void CDicClientView::OnDraw(CDC* pDC)
{
	CDicClientDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: この場所にネイティブ データ用の描画コードを追加します。
}

void CDicClientView::OnInitialUpdate()
{
	CHtmlView::OnInitialUpdate();

	// TODO: このコードはポピュラーなウェッブ サイトへナビゲートします
	// 任意のウェッブ サイトに変更してください
	Navigate2(_T("http://www.btonic.com/ws/index.html"),NULL,NULL);

	GetDic();
}

/////////////////////////////////////////////////////////////////////////////
// CDicClientView クラスの診断

#ifdef _DEBUG
void CDicClientView::AssertValid() const
{
	CHtmlView::AssertValid();
}

void CDicClientView::Dump(CDumpContext& dc) const
{
	CHtmlView::Dump(dc);
}

CDicClientDoc* CDicClientView::GetDocument() 
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDicClientDoc)));
	return (CDicClientDoc*)m_pDocument;
}
#endif //_DEBUG

void CDicClientView::OnSearch() 
{
	CDicClientApp* pApp = static_cast<CDicClientApp*>(AfxGetApp());
	CMainFrame* pFrame = static_cast<CMainFrame*>(pApp->GetMainWnd());

	CEdit* keyword = static_cast<CEdit*>(pFrame->GetDialogBar()->GetDlgItem(IDC_KEYWORD));

	CString str;
	keyword->GetWindowText(str);

	if(str.GetLength() == 0) 
		return;

	SearchItem((LPCTSTR)str);

}

void CDicClientView::GetDic()
{
	CComboBox* diclist = static_cast<CComboBox*>(GetDlgItem(IDC_DICLIST));
	CComboBox* soption = static_cast<CComboBox*>(GetDlgItem(IDC_SEARCHOPTION));
	
	/* create a request message */
	gdl.SetEndpoint("http://btonic.est.co.jp/NetDic/NetDicv06.asmx");

	/* invoke service */
	gdl.Invoke();

	vector<DicInfo>::iterator di = gdl.out.DicInfoList.begin();

	// get a search option
	vector<SearchOption>::iterator it;

	for(it = di->SearchOptionList.begin(); it < di->SearchOptionList.end(); it++) {
		int index = soption->AddString(it->Name.c_str());
		soption->SetItemDataPtr(index, it);
	}
	soption->SetCurSel(0);

	// get dictionaly information
	while(di < gdl.out.DicInfoList.end()) {
		int index = diclist->AddString(di->FullName.c_str());
		diclist->SetItemDataPtr(index, (void*)&(*di));
		
		di++;
	}
	diclist->SetCurSel(0);
	OnSelchangeDiclist();
}

void CDicClientView::SearchItem(const string& keyword)
{
	CComboBox* diclist = static_cast<CComboBox*>(GetDlgItem(IDC_DICLIST));
	string DicID = static_cast<DicInfo*>(diclist->GetItemDataPtr(diclist->GetCurSel()))->DicID;

	CComboBox* soption = static_cast<CComboBox*>(GetDlgItem(IDC_SEARCHOPTION));
	SearchOption* so = static_cast<SearchOption*>(soption->GetItemDataPtr(soption->GetCurSel()));

	CListBox* itemlist = static_cast<CListBox*>(GetDlgItem(IDC_ITEMLIST));

	sdi.in.DicID = DicID;
	sdi.in.QueryString    = keyword;
	sdi.in.ScopeOption    = so->ScopeOption;
	sdi.in.MatchOption    = so->MatchOption;
	sdi.in.FormatOption   = "HTML";
	sdi.in.ResourceOption = "URI";
	sdi.in.CharsetOption  = "UNICODE";
    sdi.in.ReqItemIndex   = 0;
	sdi.in.ReqItemTitleCount = 20;
	sdi.in.ReqItemContentCount = 20;

	ChangeStatusMessage("WEBサービスに接続中");
	sdi.SetEndpoint("http://btonic.est.co.jp/NetDic/NetDicv06.asmx");
	sdi.Invoke();

	itemlist->ResetContent();
	vector<DicItem>::iterator di = sdi.out.DicItemList.begin();
	while(di < sdi.out.DicItemList.end()){
		int index = itemlist->AddString(di->Title.c_str());
		itemlist->SetItemDataPtr(index, di);

		di ++;
	}

	if(sdi.out.ItemCountTotal == 0)
		ChangeStatusMessage("No item matches．");
	else {
		String buf;
		buf.Format("%d item(s) match(es).", sdi.out.ItemCountTotal);
		ChangeStatusMessage(buf.GetString());
	}
}

CWnd* CDicClientView::GetDlgItem(int nID)
{
	CDicClientApp* pApp = static_cast<CDicClientApp*>(AfxGetApp());
	CMainFrame* pFrame = static_cast<CMainFrame*>(pApp->GetMainWnd());
	return (pFrame->GetDialogBar()->GetDlgItem(nID));
}

void CDicClientView::OnDblclkItemlist() 
{
	CListBox* itemlist = static_cast<CListBox*>(GetDlgItem(IDC_ITEMLIST));
	DicItem* di = static_cast<DicItem*>(itemlist->GetItemDataPtr(itemlist->GetCurSel()));

	GetDocument()->SetTitle(di->Title.c_str());
	{
		ofstream of("out.html");

		of << "<html>" << di->Head;
		of << di->Body << "</html>";
	}
	char dir[256+10];
	getcwd(dir, sizeof(dir));
	strcat(dir, "\\out.html");

	Navigate2(_T(dir),NULL,NULL); 
}

void CDicClientView::OnSelchangeItemlist() 
{
	OnDblclkItemlist();
}

void CDicClientView::ChangeStatusMessage(const string& mes)
{
	CDicClientApp* pApp = static_cast<CDicClientApp*>(AfxGetApp());
	CMainFrame* pFrame = static_cast<CMainFrame*>(pApp->GetMainWnd());
	pFrame->ChangeStatusMessage(const_cast<char*>(mes.c_str()));
}

void CDicClientView::OnSelchangeDiclist() 
{
	CComboBox* diclist = static_cast<CComboBox*>(GetDlgItem(IDC_DICLIST));
	DicInfo* pdi = static_cast<DicInfo*>(diclist->GetItemDataPtr(diclist->GetCurSel()));

	GetDocument()->SetTitle(pdi->FullName.c_str());
}
