/*-----------------------------------------------------------------------------
 * $RCSfile: dicMethod.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include "OpenSOAPMethod.h"
#include <vector>

using namespace COpenSOAP;
using namespace std;

class SearchOption : public Structure {
public:
	void CreateMessage(XMLElm& elm) const 
	{
		elm.SetChildValue("Name", Name);
		elm.SetChildValue("ScopeOption", ScopeOption);
		elm.SetChildValue("MatchOption", MatchOption);
	}
	void ParseMessage(const XMLElm& elm) 
	{
		elm.GetChildValue("Name", Name);
		elm.GetChildValue("ScopeOption", ScopeOption);
		elm.GetChildValue("MatchOption", MatchOption);
	}

	string Name;
	string ScopeOption;
	string MatchOption;
} ;

struct  DicInfo : public Structure
{
	void CreateMessage(XMLElm& elm) const {
	}
	void ParseMessage(const XMLElm& elm) {
		elm.GetChildValue("DicID", DicID);
		elm.GetChildValue("FullName", FullName);
		elm.GetChildValue("ShortName", ShortName);
		elm.GetChildValue("Publisher", Publisher);
		elm.GetChildValue("Abbrev", Abbrev);
		elm.GetChildValue("LogoURL", LogoURL);
		elm.GetChildValue("StartItemID", StartItemID);
		elm.GetChild("SearchOptionList").GetChildValue("SearchOption", SearchOptionList);
		elm.GetChildValue("DefSearchOptionIndex", DefSearchOptionIndex);
	}

    string DicID;          //unique ID
    string FullName;         //official name
    string ShortName;        //short name
    string Publisher;        //publisher
    string Abbrev;           //internal identifierÅiless than 8 charactorsÅj
    string LogoURL;          //URL of picture for banner
    string StartItemID;      //item ID for initial display
	vector<SearchOption> SearchOptionList; //array of recommended search option
	long   DefSearchOptionIndex; //default search option

	void Print(ostream os)
	{
		os << DicID << endl;
		os << FullName << endl;
		os << ShortName << endl;
		os << Publisher << endl;
		os << Abbrev << endl;
		os << LogoURL << endl;
		os << StartItemID << endl;
		os << DefSearchOptionIndex << endl;
	}
};

class GetDicList_out : public Structure
{
public:
	void CreateMessage(XMLElm& elm) const {
	}
	void ParseMessage(const XMLElm& elm) {
		elm.GetChild("DicInfoList").GetChildValue("DicInfo", DicInfoList);
	}

	vector<DicInfo> DicInfoList;
	string ErrorMessage;
};

class GetDicList_method : public Method
{
public:
	std::string GetSOAPAction()
	{ return "http://btonic.est.co.jp/NetDic/NetDicV06/GetDicList"; }
	std::string GetMethodName()
	{ return "GetDicList"; }

	std::string GetNamespaceURI()
	{ return "http://btonic.est.co.jp/NetDic/NetDicV06"; }
};

typedef ClientMethod <EmptyStructure, GetDicList_out, GetDicList_method> GetDicList;

//////////////////////////////////
class DicItem : public Structure
{
public:
	void CreateMessage(XMLElm& elm) const {}
	void ParseMessage(const XMLElm& elm)
	{
		elm.GetChildValue("DicID", DicID);
		elm.GetChildValue("ItemID", ItemID);
		elm.GetChildValue("Title", Title);
		Head = elm.GetChild("Head").GetCharEncodingString("EUC-JP");
		Body = elm.GetChild("Body").GetCharEncodingString("EUC-JP");
	}

	string DicID;
	string ItemID;
	string Title;
	string Head;
	string Body;
};

class SearchDicItem_in  : public Structure
{
public:
	void CreateMessage(XMLElm &elm) const {
		elm.SetChildValue("DicID", DicID);
		elm.SetChildValue("QueryString", QueryString);
		elm.SetChildValue("ScopeOption", ScopeOption);
		elm.SetChildValue("MatchOption", MatchOption);
		elm.SetChildValue("FormatOption", FormatOption);
		elm.SetChildValue("ResourceOption", ResourceOption);
		elm.SetChildValue("CharsetOption", CharsetOption);
		elm.SetChildValue("ReqItemIndex", ReqItemIndex);
		elm.SetChildValue("ReqItemTitleCount", ReqItemTitleCount);
		elm.SetChildValue("ReqItemContentCount", ReqItemContentCount);
	}
	void ParseMessage(const XMLElm &elm) {}

	string DicID;
	string QueryString;
	string ScopeOption;
	string MatchOption;

	string FormatOption;
	string ResourceOption;
	string CharsetOption;

	long ReqItemIndex;
	long ReqItemTitleCount;
	long ReqItemContentCount;
};

class SearchDicItem_out : public Structure
{
public:
	void CreateMessage(XMLElm &elm) const {}
	void ParseMessage(const XMLElm &elm) 
	{
		elm.GetChildValue("ItemCountTotal",ItemCountTotal);
		elm.GetChildValue("ItemCountInList",ItemCountInList);

		elm.GetChild("ItemList").GetChildValue("DicItem",DicItemList);
	}

	long ItemCountTotal;
	long ItemCountInList;

	vector<DicItem> DicItemList;
	string ErrorMessage;
};

class SearchDicItem_method : public Method
{
public:
	std::string GetSOAPAction()
	{ return "http://btonic.est.co.jp/NetDic/NetDicV06/SearchDicItem"; }
	std::string GetMethodName()
	{ return "SearchDicItem"; }

	std::string GetNamespaceURI()
	{ return "http://btonic.est.co.jp/NetDic/NetDicV06"; }

};

typedef ClientMethod<SearchDicItem_in, SearchDicItem_out, SearchDicItem_method> SearchDicItem;