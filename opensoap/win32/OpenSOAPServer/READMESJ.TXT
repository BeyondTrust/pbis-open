OpenSOAP Server for Windows2000　インストール方法

-------
前提：
-------
（１）libxml2がインストールされていること。
　　インストール先として、C:\libxml2　を前提としている。
　　それ以外にインストールした場合は、以下の OpenSOAPServer.dswに登録されている
　　各プロジェクトの設定中のC:\libxml2\のパスを変更する必要がある。
　　※ また、C:\libxml2\bin をシステムのPATHに追加すること。

（２）opensslがインストールされていること。
　　インストール先として、C:\openssl　を前提としている。
　　それ以外にインストールした場合は、以下の OpenSOAPServer.dswに登録されている
　　各プロジェクトの設定中のC:\openssl\のパスを変更する必要がある。
    ※ また、C:\openssl\bin をシステムのPATHに追加すること。
（３）Apacheがインストールされていること。
    デフォルトのインストール先を変更した場合は、installOpenSOAPServer.bat　中の
　　APACHE_CGI_DIRの記述を変更すること。
（４）パッケージの win32\READMESJ.TXT に従い、OpenSOAP.dsw がコンパイルされ、
    インストールされていること。

-------
インストール手順：
-------

（１）OpenSOAPServer.dswを開き、「ビルド」メニューから
    「バッチビルド」を選択し「ビルド」する。
    ※動作確認は Release モードにて行っているため、Debug モードのビルドを
　　　行わないよう各プロジェクトの Debug のチェックは外し、Release のみ
　　　ビルドしてください。

（２）etc\installOpenSOAPServer.bat を実行する。
    - これにより、%SystemDrive%\OpenSOAPディレクトリが作成され、必要なDLLや
    　exeファイル等がコピーされる。
      また、%SystemDrive%\usr\local\opensoap\ 以下に実行環境が作成される。
      この実行環境以下に、ログファイルや一時メッセージなどが出力される。

   ※ この実行環境の \usr\local\opensoap\ は、カスタマイズが可能です。
　　　その場合は、上記installOpenSOAPServer.batの実行前に、同ファイル中の
　　　OPENSOAP_ROOTの設定を修正してください。
　　　その際に注意が必要なのは、インストール後に、\usr\local\opensoap\にあたる
　　　ディレクトリを参照している以下のファイルの内容もカスタマイズに合わせて
　　　適切なディレクトリを指すように修正した後、OpenSOAP Serverプロセスを起動
　　　してください。

    修正が必要なファイル：
　　　　　A) \OpenSOAP\etc\StartServerWinService.bat
　　　　　　- OPENSOAP_ROOTの設定をinstallOpenSOAPServer.bat中の変更内容に
　　　　　　　合わせて修正する。
          B) \OpenSOAP\etc\server.conf
            - サーバプロセスの実行環境に関するpathにあたる部分をすべて修正する。
　　　　　　　例えば、\usr\local\opensoap\ を \opensoap\ にカスタマイズした
　　　　　　　場合は、
               <!-- log -->
                 <log>
                   <path>/usr/local/opensoap/var/log/</path>
                 </log>

　　　　　　　　を、以下のように修正する。

               <!-- log -->
                 <log>
                   <path>/opensoap/var/log/</path>
                 </log>

　　　　　　　　同様に/usr/local/opensoap/を指すpathをすべて修正する。

 ※以降の C: は実際は %SystemDrive% を指すため、システムインストール環境に
   よる。

（３）システムのPATHに以下を追加し、システムを再起動する。
    追加パス：
     C:\OpenSOAP\bin;C:\OpenSOAP\user\bin

（４）C:\OpenSOAP\etc\SetUpServerWinService.bat を実行する。
     - これにより、OpenSOAPServer各プロセスがWindowsサービスとして登録される。
     ※登録したサービスを削除する場合は、
       C:\OpenSOAP\etc\DeleteServerWinService.bat を実行。

-------
実行環境：
-------
　インストール作業によって、C:\usr\local\opensoap ディレクトリが
　作成され、これ以下に以下のディレクトリが作成される。
  ※以下の（１）以外は、インストール手順の（２）でカスタマイズした内容に
　　よって変わる。

（１）C:\OpenSOAP\etc\server.conf
    - OpenSOAP Server の各種設定情報ファイル。
　　　※ パッケージ中の doc\server\READMESJ.txt の server.conf に関する記述に
　　　　従い、pathの設定や、転送サーバ情報などを適切に設定すること。

（２）etc\ssml
    - サービスを定義するSSMLファイルを置くディレクトリ。
　　　※サーバプロセスが起動時にここを参照し、サービスを管理します。
　　　　(サービス名).ssmlファイルが正しく記述され、置かれている必要があります。

（３）var\spool
    - 内部処理用一時メッセージおよび一時テーブルが作成されるディレクトリ。

（４）var\log
    - OpenSOAP Server の各プロセスのログファイルが作成される。

-------
OpenSOAP Serverプロセス起動：
-------
（１）C:\OpenSOAP\etc\StartServerWinService.bat を実行。

-------
OpenSOAP Serverプロセス停止：
-------
（１）C:\OpenSOAP\etc\StopServerWinService.bat を実行。

-------
OpenSOAP Server設定情報再読み込み：
-------
 サービス用SSMLファイルを更新した場合は以下により、
 情報を更新することが出来る。
（１）C:\OpenSOAP\etc\ReloadServerWinService.bat を実行。
 
 ※修正の反映は、上記方法のほか、OpenSOAP Serverプロセスの再起動（停止＋起動）
　を行ってはじめて行われることに注意する。

------
テストサンプル実行例：
------
　以下に、参考として、Helloサンプルを実行する手順を示します。

（１）サンプルのクライアントおよびサービスを作成する。

　　パッケージの win32\OpenSOAPSamples\ 以下の HelloClient_DLL.dsp および
　　HelloService_DLL.dsp によりビルドを行い、作成された Debug_DLL\以下の
　　HelloClient.exe および HelloService.exe を C:\opensoap\user\bin 以下に
　　コピーする。

    ※HelloClientでは、サービスをCGI経由で呼び出すモードと、OpenSOAPサーバー
　　　管理で呼び出すモードをOpenSOAPTransportSetURL()の二つめの引数で区別して
　　　いるため、OpenSOAPサーバ管理のサービス呼び出しとする必要があります。
　　　デフォルトではコンパイルスイッチ(#if 1)によりCGIモードになっているので、
　　　OpenSOAPサーバ呼び出しに修正してから(#if 0に修正)ビルドしてください。
　　　修正対象ソースはパッケージの samples\Hello\HelloClient.c です。

　　※サービスのモジュールに関して、win32\OpenSOAPSamples\HelloService_cgi_DLL.dsp 等、
　　　*_cgi_DLL.dsp は、OpenSOAPサーバ経由でのサービス呼び出し用ではなく、
　　　CGI経由で直接サービスを呼び出すためのものなので、注意が必要です。
　　　win32\OpenSOAPSampes\OpenSOAPSamples.dswを使って一括ビルドなどを行った
　　　場合は、CGI用サービスではなく、サーバー用サービスをリビルドし作成し直して
　　　ください。

（２）SSMLファイルによりサービスを登録する。

　　パッケージの samples\Hello\Hello.ssml ファイルを所定のディレクトリ
　　(server.confにて設定)にコピーし、サービスのインストールパスを修正後、
　　OpenSOAPサーバプロセスを再起動する。

　　修正前：
    <StdIO>
        <exec prog='/usr/local/opensoap/services/Hello/HelloService'/>
    </StdIO>

　　修正後：
    <StdIO>
        <exec prog='/opensoap/user/bin/HelloService'/>
    </StdIO>

（３）サンプルの実行

　　cmd.exeを
　　cmd.exeを起動し、コマンドプロンプトから HelloClient を起動する。

　　例）
     > HelloClient OpenSOAP


------
トラブルシューティング：
------
　動作が期待通りに動かない場合は以下のポイントを確認してください。

（１）システムの PATH が正しく設定されているか。
　　デフォルトでは、以下のパスが追加されている必要がある。
    C:\libxml2\bin;C:\openssl\bin;C:\opensoap\bin;C:\opensoap\user\bin

（２）server.conf が正しい場所に置かれているか、またその内容に間違いがないか。
　　デフォルトでは、C:\opensoap\etc 以下に置かれる必要があります。
　　ログの出力先や、スプールディレクトリ、SSMLファイルの参照先などが、初期状態
　　では、/usr/local/opensoap/...になっているため、/opensoap/...等に環境を変更
　　している場合は、それらに合わせて修正する必要がある。
　　また、指定されているディレクトリが存在している必要がある。

（３）SSMLファイルは参照先に置かれているか、また内容は正しいか。
　　SSMLファイルは、C:\opensoap\etc\server.confで指定されているディレクトリに
　　置く必要があります。
　　また、SSMLファイル中のサービスの実行モジュールのパスが正しく指定されている
　　必要があります。
　　　例）
    <StdIO>
        <exec prog='/opensoap/user/bin/HelloService'/>
    </StdIO>

（４）サービスの実行モジュールは正しく作成されているか。
　　CGI呼び出し用のサービスが使われている場合は、サーバ経由では正しく動作
　　しません。

（５）Apache HTTPサーバが起動しているか。
　　起動していない場合は、Apache HTTPサーバを起動してください。

