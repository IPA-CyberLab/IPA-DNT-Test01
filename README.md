﻿# IPA-DN-Ultra ライブラリ リリースノート (更新履歴)
このライブラリは未完成です。まだ使用することはできません。2021/1/20 登

## [Current Version] beta7preview9

## シン・テレワークシステム beta7preview9 の新機能
1. シン・テレワークシステム サーバーの自動インストーラ (無人インストーラ) に対応した。大量のコンピュータへのサーバーのインストールが快適になる。インストーラの EXE ファイルを実行する際に「/auto:1」というコマンドラインオプションを指定することにより、インストーラはデフォルトのオプションのまま、無人で最後まで進み、サーバー設定ツールが自動的に起動するところまで進むようになる。なお、このコマンドラインオプションを指定して実行する際には、Administrators 権限が必要である。権限がない場合は、UAC ポップアップが表示される。また、インストール中にデフォルトで次に進むことができないようなエラーが発生した場合は、当該エラーの表示部分で停止するので、それ以降は手動でインストールをする必要がある。
1. プライベート版で完全閉域化ファイアウォール機能に対応した。アプリケーションビルド時において、ソースコード中の「src/Vars/VarsActivePatch.h」の「ThinFwMode」項目を「true」に変更することにより、「完全閉域化ファイアウォール」機能がクライアント接続時に呼び出されるようになる。「完全閉域化ファイアウォール」において通信を例外的に許可する通信先 IP アドレス (IP サブネット) のリストは、インストーラのビルド時に予め「src/bin/hamcore/WhiteListRules.txt」に列挙しておく必要がある。クライアント側も新しいバージョンが必要である。
1. アプリのインストーラビルド時のソースコード中の「src/Vars/VarsActivePatch.h」の「ThinFwMode」項目が「false」の場合であっても、ポリシー規制サーバーで「ENFORCE_LIMITED_FIREWALL」項目を「1」に設定することにより、「完全閉域化ファイアウォール」機能を強制的に有効にすることができるようにした。クライアント側も新しいバージョンが必要である。プライベート版と LGWAN 版では利用できるが、パブリック版では利用できない。
1. サーバー側で「tunnel_log」ディレクトリにサーバーと中継システムとの間の通信の詳細なログを出力するようにした。サーバーと中継システムとの間が頻繁に切れるような場合は、このログを確認することにより、原因を特定することが容易となる。
1. インストーラのビルドにおいて、サーバーアプリのみをビルドすることができるようになった。ソースコード中の「src/Vars/VarsActivePatch.h」の「ThinSetupServerOnly」項目を「true」に変更することにより、サーバーアプリのみを含むインストーラが作成される。クライアントアプリも含むインストーラと、サーバーアプリのみを含むインストーラの 2 種類をビルドしたい場合は、同ヘッダファイルを書換えて、2 回ビルドすること。
1. 「MAC アドレス認証における MAC アドレスのリストを、ポリシー規制サーバー側で一元管理し、ユーザーに自由に管理させたくない」という要望に対応するため、ポリシー規制サーバーの設定ファイルに「NO_LOCAL_MAC_ADDRESS_LIST」を追加した。これを「1」に設定することにより、ユーザーは MAC アドレス認証における MAC アドレスのリストを手動で設定することができなくなる。なお、「NO_LOCAL_MAC_ADDRESS_LIST」が有効となるためには、ポリシー設定ファイルの「CLIENT_ALLOWED_MAC_LIST_URL」および「ENFORCE_MACCHECK」が設定されている必要がある。
1. LGWAN 版において、クライアントが Administrators または SYSTEM 権限で動作している場合は、ユーザーが指定した mstsc.exe ファイルを実行することができないようにした。
1. OTP における SMS 認証に対応した。(ハイパースケール版のみ。) 詳細は、ハイパースケール版のドキュメントを参照すること。
1. インストーラの EXE ファイルと同じディレクトリに EntryPoint.dat ファイル (テキストファイル) が設置されている場合は、このファイルが、インストーラビルド時に埋め込まれる EntryPoint.dat ファイルに優先して、サーバーと共にインストールされるようにした。これは、たとえばシン・テレワークシステム中継システムを組み込んだアプライアンスを実装したとき、HTML 管理画面等から、その中継システムに接続できるインストーラの ZIP ファイルをダウンロードできるような機能を実装する際に、大変便利である。EXE ファイルそのものはすべてのシステムで共通にしておき、EntryPoint.dat ファイルのみ、システム毎に異なるファイルを自動生成して ZIP でダウンロード可能とするシステムを、容易に構築できるようになった。このことにより、ユーザーはインストーラを独自にビルドする必要がなく、当該アプライアンスの製造元が 1 回のみビルドすればよい。そして、このことは Microsoft Authenticode 署名をインストーラにアプライアンス出荷元があらかじめ付与することができることも意味するのである。
1. クライアント証明書認証における OCSP (Online Certificate Status Protocol) 検証の実装。ポリシー規制サーバーの「ENABLE_OCSP」項目を「1」に設定することにより、サーバーは、クライアント証明書認証要求があった場合で、かつ認証がサーバーにあらかじめ登録されている信頼された証明書 (CA 等) による署名の検証によって実施される場合に、当該クライアント証明書の拡張フィールドに OCSP サーバーの URL が記載されている場合は、その OCSP サーバーの URL 宛に OCSP プロトコルにより証明書が有効かどうかの検証を試みます。無効であると回答された場合は、ログファイルにその旨を記載し、証明書認証は失敗します。OCSP サーバーとの通信に失敗した場合は、検証は成功したものとみなされます。
1. アカウントロックアウト機能の実装。ポリシー規制サーバーの「AUTH_LOCKOUT_COUNT」および「AUTH_LOCKOUT_TIMEOUT」項目を 1 以上の整数に設定することにより、ユーザー認証 (パスワード認証) においてパスワードを誤った場合のアカウントロックアウトが可能となりました。AUTH_LOCKOUT_COUNT には、ロックアウトが発生するまでの認証失敗回数を指定します。AUTH_LOCKOUT_TIMEOUT には、ロックアウトが自動解除されるまでのタイムアウト値を秒単位で指定します。
1. 無操作時のタイムアウト実装。ポリシー規制サーバーの「IDLE_TIMEOUT」項目を 1 以上の整数に設定することにより、ユーザーがクライアント側でマウスを「IDLE_TIMEOUT」で指定された秒数以上無操作であった場合は、クライアント側の接続が切断され、無操作タイムアウトが発生した旨のメッセージボックスがクライアント側の画面に表示されるようになります。この機能が有効となるには、クライアント側のバージョンも beta7preview9 以降である必要があります。それ以前のクライアントの場合は、無視されます。
1. ポリシー規制サーバーの「SERVER_ALLOWED_MAC_LIST_URL」による MAC アドレス一覧テキストファイルの指定において、MAC アドレス一覧テキストファイルの先頭行に UTF-8 の BOM 文字が入っていた場合、その BOM 文字を除外して処理を行なうように改良しました。



