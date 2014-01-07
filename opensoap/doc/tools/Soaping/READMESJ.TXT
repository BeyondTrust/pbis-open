Soaping サービスツール
======================

このディレクトリには、以下に示す、OpenSOAP用の応答時間を測定する 
Pingコマンド、Soaping のクライアントおよびサービスのソースコード
が含まれています。

* soaping
  Soaping クライアントプログラムです。
  使い方の詳細は -h オプションの出力[*1]を参照してください。
  実行結果は [*2]のようになります。

* SoapingService
  標準入出力を用いた Soaping サービスプログラムです。

* SoapingService.cgi
  CGI 型の Soaping サービスプログラムです。


[*1]
$ soaping -h
Usage: soaping [-h] [-s size[K|M]] [-c count [-i wait]] [-v] [-o] [URL | host]
Options:
  -h         display this information
  -s size    size of string data to send [[K|M]byte] (default: 0)
  -c count   number of times for continuous transmissions (default: 1)
  -i wait    interval time for continuous transmissions [msec] (default: 0)
  -v         verbose mode.  display SOAP message
  -o         via OpenSOAP server (http://host/cgi-bin/soapInterface.cgi)
URL :  Soaping end-point (default: http://localhost/cgi-bin/SoapingService.cgi)
host : end-point addressed by hostname (default: localhot)
       -> http://host/cgi-bin/{SoapingService.cgi,soapInterface.cgi}

[*2]
$ soaping -s 100 -c 5 localhost
SOAPING http://localhost/cgi-bin/SoapingService.cgi : 100 byte string.
soaping-seq=0 time=12.218 msec
soaping-seq=1 time=9.859 msec
soaping-seq=2 time=9.845 msec
soaping-seq=3 time=9.881 msec
soaping-seq=4 time=9.875 msec

--- http://localhost/cgi-bin/SoapingService.cgi soaping statistics ---
5 messages transmitted, 5 received, 0% loss, time 51.678ms
Round-Trip min/avg/max/mdev = 9.845/10.335/12.218/0.948 msec
