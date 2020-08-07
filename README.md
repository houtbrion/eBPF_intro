# eBPFを使ったプログラム開発入門

eBPFを使うためには，eBPFの開発環境にはメジャーなものとして以下の2つがある．
- [bcc][bcc]
- [bpftrace][bpftrace]

[bcc][bcc]はeBPFのVM内にロードさせるプログラムはCで記述し，ユーザ空間でVMから出力されるデータやメッセージを
受信するプログラムはCもしくはPythonで記述する(他にもgo等での環境も別途開発されている)．ただし，使いやすさの
面から，ユーザ空間のプログラムはpythonで記述するのが主流である．

もう一つの[bfptrace][bpftrace]はawk風にeBPFのプログラムを記述でき，[bcc][bcc]に比べて
圧倒的に簡単に使える．もちろん，パケットを取り扱うXDPには非対応など実現できる機能には制約があるものの，
必要なプログラムの機能が特に高度なものでない場合は，[bpftrace][bpftrace]で十分である．

これらを使うためには，それぞれのソース([bcc][bcc]と[bpftrace][bpftrace])をダウンロードし，
インストールマニュアル([bcc用][bcc-install]と[bpftrace用][bpftrace-install])を参照してインストールした後に，
[bcc公式リファレンスガイド][bcc-ref-guide]と[bpftrace公式リファレンスガイド][bpftrace-ref-guide]を
使って学習するという手順になる．

ただし，公式インストールマニュアルやリファレンスガイドに以下のような問題がある．
- [bccのインストールマニュアル][bcc-install]にはインストールの手順に必要なものが抜けており，バージョンによってはコンパイルエラーとなる．
- 各機能がカーネルやそれぞれの開発環境のバージョンいくつに依存しているか，明示していないものが散見される．
- サンプルプログラムの中に複雑すぎて初心者向けではないものが存在する．
- サンプルプログラムやリファレンスガイドの記述にバグがあるものが含まれている．

これらを初心者が全てクリアするには，時間と忍耐，LinuxカーネルやPython,Cの知識が必要となるため，
eBPFを使ったプログラミングをこれから始める人，始めてはみたものの「良くわからん」とお怒りの人向けに
リファレンスガイドを補う(もしくは置き換える)ためのドキュメントを作成した．

基本はリファレンスガイドと同じ内容であるが，内容を補足したり，バグを除去したり，サンプルプログラムで
適切でないものが示されている場合は，別のサンプルプログラムを提供したりしている．
[bcc][bcc]や[bpftrace][bpftrace]は頻繁にバージョンアップしているが，本ドキュメントは
追従していないので，本ドキュメントである程度経験を積んだ後は公式リファレンスガイドを参照していただきたい．

## 想定している環境
本ドキュメントで利用した環境は以下の3種類．
|種類|OS|カーネル|bcc|bpftrace|備考|
|--:|:--|:--|:--|:--|:--|
|Ubuntu公式|Ubuntu20.04|5.4.0-42-generic|0.12.0|0.9.4|すべて公式リポジトリのもの|
|CentOS公式|CentOS8.2|4.18.0|0.15.0|0.11.0|すべて公式リポジトリのもの|
|Ubuntu最新|Ubuntu20.04|5.8.0|0.15.0|0.11.0|カーネルbcc,bpftrace全てソースからインストール|

この3つの環境のうちメインはUbuntu公式である．bpftraceの新しいバージョンは
機能が多い代わりに，バグが多いため初心者にはおすすめできない．残りの2つは
カーネルの違いによる差や新機能の入門のために用意している．

### Ubuntu公式環境のセットアップ
Ubuntu20.4とパッケージ版の組み合わせを作る場合は，公式の
[bccのインストールマニュアル][bcc-install]と
[bpftraceのインストールマニュアル][bpftrace-install]を見て
インストールしていただきたい．

あと，Pythonは基本v3を利用しているが，
本ドキュメントで引用(コピーを収録)している公式リポジトリのサンプルプログラムが，
Python2系を要求するものがあるため，pythonとpipは両方のバージョンを
インストールしてください．

本ドキュメントでも取り扱っているUSDTを使うため，DTraceの環境を入れる必要がある．
```
# apt install systemtap-sdt-dev
```

### CentOSのセットアップ
CentOSで環境を構築する場合，基本は[bccのインストールマニュアル][bcc-install]のRedHatの部分
を見て
インストールしていただきたい．

ただし，CentOSも8.2の環境だと特別な手順が必要である．
まず，
bccが使っているPythonは今でも2のものが残っており，Python2とpipを入れる必要がある．
ただし，bccのパッケージが依存しているものはPython2の他に，Python2のnetaddrがあるが，CentOS8.2にPython2用netaddrは存在しないため，
bccのパッケージ(bcc-tools)は強制インストールする必要がある．
```
# dnf install bcc-tools --nobest
```

その後，必要に応じてpipで
netaddrをインストールする．

bpftraceは，CentOS8.2用のパッケージが存在するため，それをインストールしていただきたい．
```
dnf install bpftrace
```

最後に，bccに付属しているツールはPATHに入らないため，自分でパスに追加(.bashrcなど)してください．

### カーネルやbcc, bpftraceのアップデートの必要性
bccのリポジトリに機能とカーネルバージョンの関係を記述した[文書][kernel-version]が
存在するが，本ドキュメントの想定環境であるUbuntu20.04が標準で搭載している
カーネルバージョン5.4では対応していないものが多数ある．

本書の範囲の機能で，特にユーザの興味を引きそうな機能のうち，
想定環境と商用環境で良く使われるRedHat8(もしくはCentOS8)系列の
OS(カーネルバージョンが4.18)の対応状況は以下のようになっている．

- 対応状況

|分類 |機能 | CenOS8.2 | Ubuntu20.04 |備考|
|:--|:--|:--:|:--:|:--|
|debug | bpf_override_return() | △ | ○ ||
|kfunc/kretfunc| |×|×|関連ツールのビルドとインストールも必要|

- 凡例

|記号| 内容|
|:--:|:--|
|○| 動作可能 |
|△| カーネル設定変更と再ビルドで動作|
|×| 新しいバージョンのカーネルインストールが必要|

しかし，bccやbpftraceの便利な機能がカーネルバージョン5.5移行や5.7以降というものが
いくつかあり，最近は，5.9以降というものも追加されている．
他の機能についても，確認が取れているものについてはドキュメント中に
OSやカーネルのバージョンについて記載しているので，必要と感じたら
カーネルのアップデートにトライしていただきたい．

もし，カーネルのバージョンアップが必要ない場合は，
bccのインストールまで飛ばして構わない．

### カーネルのアップデートやbcc, bpftrace最新版のインストール方法
新しい機能を利用したい場合は[インストール][install]を見て，
カーネルやbcc, bpftraceをソースからインストールして
いただきたい．

## 本ドキュメントのライセンスについて
最初に述べたように，本ドキュメントは[bcc][bcc]と[bpftrace][bpftrace]の
公式リファレンスガイドをベースにしているため，ライセンスは
それぞれのリポジトリのライセンスであるApache2.0を踏襲している．

具体的なライセンス文は[LICESE][license]を参照していただきたい．



## 目次
- [インストール][install]
- [bpftrace入門][bpftrace-intro]
- [bcc入門][bcc-intro]



<!-- 参考文献リスト -->
[bpftrace]: <https://github.com/iovisor/bpftrace> "bpftrace"
[bpftrace-ref-guide]: <https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md>  "bpftrace公式リファレンスガイド"
[bpftrace-install]: <https://github.com/iovisor/bpftrace/blob/master/INSTALL.md> "bpftraceインストールマニュアル"
[bcc]: https://github.com/iovisor/bcc "bcc"
[bcc-install]: <https://github.com/iovisor/bcc/blob/master/INSTALL.md> "bccインストールマニュアル"
[bcc-ref-guide]: <https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md> "bcc公式リファレンスガイド"
[kernel-version]: <https://github.com/iovisor/bcc/blob/master/docs/kernel-versions.md> "bccの機能と利用可能なカーネルバージョンの対応関係"
[install]: <INSTALL.md> "インストールドキュメント"
[license]: <LICENSE> "ライセンスファイル"
[bpftrace-intro]: <bpftrace/README.md> "bpftrace入門"
[bcc-intro]: <bcc/README.md> "bcc入門"



