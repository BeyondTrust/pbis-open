OpenSOAP c++ library

====================
OpenSOAPのAPIの一部をc++のクラスとしてラップしたものです．
１．モジュールの生成，破棄などが自動化されます．
２．SOAPメッセージの生成，解析が少し楽になります．

ただし，すべてのAPIをサポートしているわけではありません．

ファイル構成

OpenSOAPBlock.h		
OpenSOAPByteArray.h	
OpenSOAPEnvelope.h	
OpenSOAPService.h	
OpenSOAPString.h	
OpenSOAPTransport.h	
OpenSOAPXMLElm.h	
同名のOpenSOAP APIのラッパークラス
	
OpenSOAPInitializer.h	OpenSOAPの初期化，終了
OpenSOAPMethod.h	SOAPメッセージの作成，解析
OpenSOAPObject.h	すべてのクラスの基底クラス
OpenSOAPStructure.h	構造体の基底クラス

samples/
	Client.cpp	複雑なクライアント例
	Helloc.cpp	Helloサービスクライアント
	Hellos.cpp	Helloサービス
	Hello.hpp	サービス、クライアント，共通実装部分

クラス構成：
using namespace COpenSOAP;

Object <- Block
       <- ByteArray
       <- Envelope
       <- Service
       <- String
       <- Transport
       <- XMLElm
       <- Structure(V) <- InStructure(V)
                       <- OutStructure(V)
                       <- EmptyStructure
       <- Method(V)    <- ClientMethod(T)
                       <- ServiceMethod(T)

(V)は抽象基本クラス，(T)はクラステンプレート

簡単な使い方：
SOAP呼び出しを行うには，
まず，入出力構造体(Structureから派生したもの）を実装します
例：
#include "OpenSOAPMethod.h"

using namespace COpenSOAP;
using namespace std;

struct Hello_in : public Structure // サーバに渡す値を定義
{
	void CreateMessage(XMLElm& elm) const {　
		elm.SetChildValue("name", name);
	}
	void ParseMessage(const XMLElm& elm) {   
		elm.GetChildValue("name", name);
	}

	string name; //std::stringも直接SetChildValueに渡せる
};

struct Hello_out : public Structure //　サーバから受け取る値を定義
{
	void CreateMessage(XMLElm& elm) const {
		elm.SetChildValue("reply", reply);
	}
	void ParseMessage(const XMLElm& elm) {
		elm.GetChildValue("reply", reply);
	}

	string reply;
};

Methodの派生クラスを実装して
サービス名（ボディーブロック名），ソープアクション，ネームスペースなど
を定義します
例：

class Hello_method : public Method //必要なメソッドをオーバーライド
{
public:
	std::string GetSOAPAction()
	{ return ""; }
	std::string GetMethodName()
	{ return "Hello"; }

	std::string GetNamespaceURI()
	{ return "http://opensoap.jp/samples/hello"; }
};

ClientMethodテンプレートでクライアントクラスを生成します
例：
typedef ClientMethod <Hello_in, Hello_out, Hello_method> Hello;

エンドポイントを設定して，Invokeメソッドを呼び出します
例：
	Hello he;
	he.SetEndpoint("http://localhost/cgi-bin/hellos.cgi");
	try{
		he.PrintEnvelopeTo(&cout);//エンベロープの内容を出力
		he.Invoke();
	}catch(opensoap_failed e){
		cout << "error " << hex << e.GetErrorCode() 
			<< "@"<< e.GetObjectName() << endl;
	}

エラーメッセージはopensoap_failed例外を通して受け取ります．

サービス側では，
ServiceMethodテンプレートでサービスクラスを生成して，
void ServiceMethod::ExecuteService()を実装します．
例：
typedef ServiceMethod<Hello_in, Hello_out, Hello_method> Hello;

void Hello::ExecuteService()
{
	out.reply = "Hello! " + in.name;
}

Runメソッドを呼び出します．
例
	try
	{
		Hello he;
		he.Run();
	}
	catch(opensoap_failed e)
	{
		cerr << "errorcode:" << e.GetErrorCode() 
		     <<"@"<< e.GetObjectName() << endl;
	}

○Structure,XMLElmについて
 Structureから派生した構造体は，XMLElm::SetChildValueに
直接渡すことができます．生成されるメッセージは
派生したStructureクラスの実装に依存します．
さらに，SetChildValueにはSTLのvector<T>を渡すことができます．
その際のメッセージは，
		typename std::vector<T>::iterator it;
		for( it=ar.begin(); it<ar.end(); it++) {
			SetChildValue(name, *it);
		}
によって生成されます．

派生したクラスは，
CreateMessage(),ParseMessage()
を実装する必要があります．
クライアントのみを作成する場合で，入力のみを
行いたい場合などは，必要に応じて
InStructure(createのみ必要で,parseする必要がない構造体)
OutStructure(parseのみ必要で，createする必要がない構造体)
から派生してください．

値が必要ない場合（bodyブロックに子要素が必要ない場合）は
ClientMethodの引数にEmptyStructureを指定してください．
例：

struct GetDicList_out : public OutStructure
{
	void ParseMessage(const XMLElm& elm) {
		elm.GetChild("DicInfoList")
			.GetChildValue("DicInfo", DicInfoList);
	}
	vector<DicInfo> DicInfoList;
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

typedef ClientMethod<EmptyStructure, GetDicList_out, 
			GetDicList_method> GetDicList;
