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
本ドキュメントである程度経験を積んだ後は，公式リファレンスガイドを利用してもらいたい．
[bcc][bcc]や[bpftrace][bpftrace]は頻繁にバージョンアップしているが，本ドキュメントは
追従していないので，公式版を参照していただきたい．

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
[bpftrace-intro]: <bpftrace/README.md> "bpftrace入門"
[bcc-intro]: <bcc/README.md> "bcc入門"

