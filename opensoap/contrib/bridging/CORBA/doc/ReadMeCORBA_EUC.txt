CORBA ブリッジング

1. OpenSOAP CORBA ブリッジングの作成：

1.1 CORBAのIDLインターフェースを定義します.

1.2 IDLコンパイラであるomniidlでIDLファイルをコンパイルします.
インターフェースに関する.ccファイルと.hhファイルが作成されます.

1.3 CORBAオブジェクトのサーバプログラムを開発します.

1.4 ブリッジングインターフェース(すなわち,OpenSOAPサービス)である
CORBAオブジェクトのクライアントプログラムを開発します.

1.5 ブリッジングインターフェースを利用するOpenSOAPクライアントプログラムを開発します.

2. OpenSOAP CORBAブリッジングのサンプル実装
 opensoap/bridging/CORBA/のCORBAExeCalcとCORBADllCalcを参照して下さい.