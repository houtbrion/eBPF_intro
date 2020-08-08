# Linuxディストリビューション公式パッケージのインストール

## Ubuntu公式環境のセットアップ
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

## CentOSのセットアップ
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
