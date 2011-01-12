/*-----------------------------------------------------------------------------
 * $RCSfile: Client.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <iostream>
#include "OpenSOAPInitializer.h"
#include "OpenSOAPMethod.h"
#include "OpenSOAPStructure.h"
using namespace std;
using namespace COpenSOAP;

#define LOOKUP 1
#if !LOOKUP
//structure of dictionaly infomation

struct  DicInfo : public OutStructure
{
	void ParseMessage(const XMLElm& elm) {
		elm.GetChildValue("DicID", DicID);
		elm.GetChildValue("FullName", FullName);
		elm.GetChildValue("ShortName", ShortName);
		elm.GetChildValue("Publisher", Publisher);
		elm.GetChildValue("Abbrev", Abbrev);
		elm.GetChildValue("LogoURL", LogoURL);
		elm.GetChildValue("StartItemID", StartItemID);
		elm.GetChildValue("DefSearchOptionIndex", DefSearchOptionIndex);
	}

    string DicID;          //unique ID
    string FullName;         //official name
    string ShortName;        //short name
    string Publisher;        //publisher
    string Abbrev;           //internal identifier（less than 8 charactors）
    string LogoURL;          //URL of picture for banner
    string StartItemID;      //item ID for initial display
	//         SearchOption[] SearchOptionList; //array of recommended search option
	long   DefSearchOptionIndex; //default search option

	void Print(ostream& os)
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

struct GetDicList_out : public OutStructure
{
	void ParseMessage(const XMLElm& elm) {
		elm.GetChild("DicInfoList").GetChildValue("DicInfo", DicInfoList);

//		elm.GetChildValue("ErrorMessage", ErrorMessage);
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

typedef ClientMethod<EmptyStructure, GetDicList_out, GetDicList_method> GetDicList;

#else

class AWSInfo : public OutStructure { 
public:
	void ParseMessage(const XMLElm& elm) {
		elm.GetChildValue("id", id);
		elm.GetChildValue("word", word);
		elm.GetChildValue("author", author);
	}

	long id;  
	string word;//wise saw 
	string author;
};

class lookup_in : public InStructure {
public:
	void CreateMessage(XMLElm& elm) const {
		elm.EnableAddTypeName();
		elm.SetChildValue("arg0", key);
	}

	string key;
};

class lookup_out : public OutStructure {
public:
	void ParseMessage(const XMLElm &elm) {
		elm.GetFirstChild().GetChildValue("item", item);
	}

	vector<AWSInfo> item;
};

class lookup_method : public Method
{
public:
	std::string GetMethodName()
	{ return "lookup"; }

	std::string GetNamespaceURI()
	{ return "urn:awisesaying-service"; }

	std::string GetNamespacePrefix()
	{ return ""; }

	void PrepareRequestEnvelope(Envelope &req)
	{
		DefineXMLSchema(req);
	}
};

typedef ClientMethod<lookup_in, lookup_out, lookup_method> lookup;
#endif

int 
main(int argc, char* argv[])
{
	if(argc != 2) {
		cout << "Searching word required!" << endl;
		return 1;
	}
		
	COpenSOAP::Initializer soap_initializer;
#if LOOKUP
	lookup lo;
	lo.SetEndpoint("http://nile.esm.co.jp:8080/soap/servlet/rpcrouter");
	lo.in.key = argv[1];

	try{
		lo.PrintEnvelopeTo(&cout);
		lo.Invoke();
	}catch(opensoap_failed e){
		cout << "error " << hex << e.GetErrorCode() 
			<< "@"<< e.GetObjectName() << endl;
	}
	vector<AWSInfo>::iterator i;
	for(i=lo.out.item.begin(); i<lo.out.item.end(); i++)
		cout << "result: " << i->word << endl;

#else
	GetDicList ge;
	ge.SetEndpoint("http://btonic.est.co.jp/NetDic/NetDicv06.asmx");

	cout << "request GetDicList" << endl;
	try{
		ge.PrintEnvelopeTo(&cout);
		ge.Invoke();
	}catch(opensoap_failed e){
		cout << "error " << hex << e.GetErrorCode() 
			<< "@"<< e.GetObjectName() << endl;
	}

	cout << ge.out.ErrorMessage;
	vector<DicInfo>::iterator i;
	for(i=ge.out.DicInfoList.begin(); i<ge.out.DicInfoList.end(); i++) {
		i->Print(cout);
		cout << endl;
	}
#endif

	cin.get();

	return 0;
}
