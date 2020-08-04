# bcc入門

## サンプルプログラムについて
公式サイトで配布しているサンプルプログラムなどのうち，
本ドキュメントで参照しているものをOriginalSampleディレクトリに
収容している(公式サイトで消されたりすることへの対策)．

また，各機能の説明(各ディレクトリのREADME.md)では，添付してあるソースプログラムは
記載していないため，リンクを別のウィンドウで開いて，並べて読んでいただきたい．


## 本ドキュメントで記載していないもの
以下の機能については，本ドキュメントでは対応していない

### カーネルバージョン
ディストリビューションが対応していない新しい
カーネルを要求するものについては，
カーネルビルドがあまり難しいものはユーザの敷居が高いので
安定系カーネル5.8が出るまで保留する．

#### カーネルバージョン5.7でサポート
- lsm probes

#### カーネルバージョン5.8でサポート(予定)
- BPF_RINGBUF_OUTPUT
- ringbuf_output()
- ringbuf_reserve()
- ringbuf_submit()
- ringbuf_discard()

### 特殊な環境が必要なもの
XDP専用の機能については，動作の検証やサンプルプログラムの開発に
物理NICが複数必要になるため，しばらく保留．
- map.redirect_map()
- BPF_XSKMAP
- BPF_CPUMAP
- BPF_DEVMAP


## もくじ
- <a href="FirstStep">はじめの一歩</a>
- <a href="output">eBPMのVMからユーザ空間のプログラムへのデータ出力</a>
- <a href="kprobe">カーネル内部関数の監視(kprobe/kretprobe)</a>
- <a href="uprobe">ユーザアプリの監視(uprobe/uretprobe)</a>
- <a href="USDT">User Statically-Defined Tracing</a>
- <a href="tracepoint">トレースポイント</a>
- <a href="raw_tracepoint">Rawトレースポイント</a>
- <a href="syscall_tracepoint">システムコールトレースポイント</a>
- <a href="kfunc">kfunc/kretfunc</a>
- <a href="errors">実行時に発生するエラーについて</a>
- <a href="data_access">データへのアクセス</a>
- <a href="debug">デバッグ用機能</a>
- <a href="maps">各種map</a>
- <a href="python">PythonのeBPFライブラリ(bcc Python)の機能</a>
