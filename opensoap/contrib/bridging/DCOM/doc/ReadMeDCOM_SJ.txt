OpenSOAP DCOM ブリッジングサービス使用説明


1．DCOM コンポーネントの登録
   DCOM コンポーネントの登録はクライアントマシンとサーバーマシン両方に必要です．

1.1 必要なファイル
1.1.1 DLLサーバーの場合：
      (1)***.dll：
		  	クラス情報を提供するファイル
      (2)***ps.dll：
			プロキシースタブインターフェース情報を提供するファイル
1.1.2 EXEサーバーの場合：
      (1)***.exe：
			クラス情報を提供するファイル
      (2)***ps.dll：
			プロキシースタブインターフェース情報を提供するファイル

1.2 クラスとプロキシースタブインターフェースの登録
1.2.1 DLLサーバーの場合
      (1) クラスの登録：
          	regsvr32 ***.dll
      (2) プロキシースタブインターフェースの登録：
          	regsvr32 ***ps.dll
1.2.2 EXEサーバーの場合：
      (1) クラスの登録：
			***.exe /regserver
      (2) プロキシースタブインターフェースの登録：
			regsvr32 ***ps.dll

1.3	登録の確認
    OLE/COM Object Viewer(OleView.exe)を起動し，[View]メニューで[Expert Mode]が選択されていることを確認ください．次に，[All Objects]のツリーをオープンし，[*** Class]が登録されたことが確認できます．

2．OpenSOAP DCOM ブリッジングサービスの作成
2.1 必要なファイル
	(1) ***.tlb
		DCOMコンポーネントオブジェクトのタイプライブラリファイル

2.2 サービスの作成
	OpenSOAP DCOMブリッジングサービスはOpenSOAPサービスの一種類であり，DCOMのコンポーネントを利用する側であるため，OpenSOAP DCOMブリッジングサービスはDCOMのクライアント側になります．
	作成するとき，"***.tlb"をimportし，以下の関数を使用してリモートマシン上のDCOMオブジェクトを呼び出し，利用します．
    CoCreateInstanceEx()

2.3 サンプル(C言語): 
	詳細はDCOMブリッジングサンプルを参考に．

	例えば，簡易演算の引き算は以下のようになります．

#define _WIN32_DCOM

#import "SubExe.tlb"

#include <iostream>

int
Subtract(double a,
         double b,
         double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

	HRESULT hr = CoInitialize(NULL);

	if (FAILED(hr)) {
		std::cerr << "COM Initialize failed" << std::endl;
		ret = OPENSOAP_MEM_ERROR;
		return ret;
	}

	try{
		SUBEXELib::ISubtractExePtr subService;
		COSERVERINFO serverInfo;
		MULTI_QI multiQi;

		serverInfo.dwReserved1 = NULL;
		serverInfo.pwszName = L"MIHARU";/* name of remote DCOM server machine */
		serverInfo.pAuthInfo = NULL;
		serverInfo.dwReserved2 = NULL;

		multiQi.pIID = &__uuidof(SUBEXELib::ISubtractExe);
		multiQi.pItf = NULL;
		multiQi.hr = S_OK;

		hr = CoCreateInstanceEx(__uuidof(SUBEXELib::SubtractExe),
								NULL,
								CLSCTX_REMOTE_SERVER,
								&serverInfo,
								1,
								&multiQi);
		if(FAILED(hr)) {
			_com_issue_error(hr);
		}

		SUBEXELib::ISubtractExe* pInt =
			 static_cast<SUBEXELib::ISubtractExe*>(multiQi.pItf);
		subService.Attach(pInt);

		*r = subService->Subtract(a, b);
	}

	catch (const _com_error & err) {
		hr = err.Error();
		cerr
			<< err.ErrorMessage()
			<< "0x"
			<<	hex
			<<	hr
			<<	endl;
	}

	CoUninitialize();

	ret = OPENSOAP_NO_ERROR;
    return ret;
}

3. DLLサーバーを実行させるための設定
3.1 代理プロセス
	システムでサポートされている代理プロセスDllHost.exeを使い，DLLサーバーにリモートアクセスできるようにします．
3.2 代理プロセスの設定
    OLE/COM Object Viewer(OleView.exe)を起動し，[View]メニューで[Expert Mode]が選択されていることを確認ください．次に，[All Objects]のツリーをオープンし，相当する[*** Class]を探します．
	ツリービューの中でクラスを選択し，右側のウィンドウ枠で[Implementation]および[Improc Server]を選択します．
	[Use Surrogate Process]ボックスをクリックします．しかし，[Path to Custom Surrogate]ボックスの中が変更されているという旨が検出されないと，Object ViewerはDllSurrogate値を追加しません．そのため，[Path to Custom Surrogate]ボックスの中にスペースを一つ入力した後，それを削除します．そうすれば，DllHost.exe代理プロセスに，サーバーが関連つけられます．別のオブジェクトのエントリをクリックすれば，レジストリが更新されます．

4．EXEサーバーを実行させるための設定
	特別な設定は要りません．

5．DCOMの動作確認
5.1 DLLサーバーの場合：
	DCOMのDLLサーバーに相当するマシンのタスクマネージャを開きます．OpenSOAP DCOMブリッジングサービスからDCOMサーバー内のDCOMオブジェクトを呼び出す時，タスクマネージャでDllHost.exeのプロセスが起動されたことを確認できます．

5.2 EXEサーバーの場合：
	DCOMのEXEサーバーに相当するマシンのタスクマネージャを開きます．OpenSOAP DCOMブリッジングサービスからDCOMサーバー内のDCOMオブジェクトを呼び出す時，タスクマネージャでEXEサーバーに相当する***.exeのプロセスが起動されたことを確認できます．
