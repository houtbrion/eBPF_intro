# インストール
カーネルのアップデートをする場合，以下の手順で
全部アップデートする必要がある．


- 関連ツール(pahole)のバージョンアップ
- カーネルバージョンアップ
- [bcc][bcc]のバージョンアップ
- [bpftrace][bpftrace]のバージョンアップ

もし，カーネルのバージョンアップが必要ない場合は，
bccのインストールまで飛ばして構わない．


## 関連ツール(pahole)のインストール
kfunc/kretfuncでは，paholeコマンド1.6以上が必要であるが，
Ubuntu20.04LTSは1.5系列しか利用できないため，個別に
インストールする必要がある．

paholeコマンドは，dwarvesパッケージに含まれており，
既にインストール済の場合はアンイストールしておく．

インストールしていない場合，ビルドにlibdw-devが
必要になるため，これはインストールしておく．

```
# apt install libdw-dev
```

### ソースをgitからダウンロード．

```
$ git clone https://github.com/acmel/dwarves.git
Cloning into 'dwarves'...
remote: Enumerating objects: 150, done.
remote: Counting objects: 100% (150/150), done.
remote: Compressing objects: 100% (85/85), done.
remote: Total 5601 (delta 73), reused 122 (delta 53), pack-reused 5451
Receiving objects: 100% (5601/5601), 2.39 MiB | 2.47 MiB/s, done.
Resolving deltas: 100% (3601/3601), done.
$ 
```

### インストール
```
$ mkdir dwarves/build
$ cd dwarves/build
$ cmake -D__LIB=lib ..
-- The C compiler identification is GNU 9.3.0
-- Check for working C compiler: /usr/bin/cc
-- Check for working C compiler: /usr/bin/cc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
(中略)
$ sudo make install
```

### ユーザの環境変数設定
rootを含め，必要なユーザの環境変数を定義するため，
以下の定義を.bashrcに追加(bash利用ユーザの場合)．
```
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```
dwarvesは，/usr/local配下にインストールしようとするため，
このような対処が必要．
/usrに直接インストールすると，公式パッケージとかち合うので
/usr/localのままにしておく．

### インストール後の確認
バージョン番号が1.16以降であることを確認
```
$ pahole --version
v1.17
$
```

## カーネルバージョンアップ手順

### カーネルビルド用ツールのインストール
カーネルのビルドに必要なツールを以下のコマンドでインストール．
```
$ sudo apt-get install build-essential libncurses-dev fakeroot dpkg-dev
(中略)
$ sudo apt-get build-dep linux
(以下略)
```

### カーネルソースのダウンロード
```
$ git clone git://git.launchpad.net/~ubuntu-kernel-test/
ubuntu/+source/linux/+git/mainline-crack cod/mainline/v5.6.19
Cloning into 'cod/mainline/v5.6.19'...
remote: Counting objects: 9765697, done.
remote: Compressing objects: 100% (494536/494536), done.
remote: Total 9765697 (delta 1625772), reused 1649954 (delta 1406065)
Receiving objects: 100% (9765697/9765697), 1.93 GiB | 7.99 MiB/s, done.
Resolving deltas: 100% (8277484/8277484), done.
Checking objects: 100% (33554432/33554432), done.
Updating files: 100% (67975/67975), done.
$ cd cod/mainline/v5.6.19/
```

### カーネルビルドに必要な設定

#### 現状のconfigを元に，新しいカーネル用の設定を作成
以下のコマンドを入力して，現状のconfigから新しいカーネルの
configを作成するが，yes/noをたくさん聞かれるが，
とりあえずは，全てリターンキー入力(
デフォルトを選択)しておく．
```
$ make localmodconfig
```

#### 必要な機能をサポートしたconfigファイルの作成
以下のコマンドを使い，カーソルベースとはいえ，
カラーのUIを使って，ビルド用のconfigを作成する．
```
$ make menuconfig
```

この際，以下の項目を<code>y</code>にする(もしくはyであることを確認する)．

- CONFIG_BPF_KPROBE_OVERRIDE
- CONFIG_PERF_EVENTS
- CONFIG_DEBUG_INFO_DWARF4
- CONFIG_DEBUG_INFO_BTF

### カーネルビルド
以下のコマンドでカーネルのパッケージをビルド．
```
make -j$(grep -c processor /proc/cpuinfo) bindeb-pkg
```
ビルド終了後，
必要なパッケージがビルドできているか確認．
```
$ ls ../
linux-5.7.0-rc7+_5.7.0-rc7+-1_amd64.buildinfo
linux-5.7.0-rc7+_5.7.0-rc7+-1_amd64.changes
linux-headers-5.7.0-rc7+_5.7.0-rc7+-1_amd64.deb
linux-image-5.7.0-rc7+_5.7.0-rc7+-1_amd64.deb
linux-image-5.7.0-rc7+-dbg_5.7.0-rc7+-1_amd64.deb
linux-libc-dev_5.7.0-rc7+-1_amd64.deb
v5.6.19
$
```

ソースをダウンロードしたときは，5.6.19を指定していたはずなのに，
ビルドしてみると，5.7.0-rc7だった(謎)．

### カーネルインストール
生成されたdebファイルを全てインストール．
```
$ cd ..
$ sudo dpkg -i *.deb
Selecting previously unselected package linux-headers-5.7.0-rc7+.
(Reading database ... 157718 files and directories curr
```


## bccのインストール
[bcc][bcc]をインストールする場合，利用しているOS用の
パッケージが既に存在しているか否か，パッケージが存在する場合の
インストール方法，ソースをビルドする場合のビルドに必要なツール類の
インストールについては，[インストールマニュアル][bcc-install]を
参照してください．

[インストールマニュアル][bcc-install]で問題があるのは，
ソースからビルドする場合で，必要な手順が[インストールマニュアル][bcc-install]には
欠けているので，そのため以下に手順を記載する．
(現在の最新版はver0.15.0であるので，それを前提に作業を実施．)

```
$ export version=0.15.0
$ export url=https://github.com/iovisor/bcc.git
$ git clone -b "v${version}" --single-branch --depth 1 ${url} bcc-${version}
Cloning into 'dwarves'...
remote: Enumerating objects: 150, done.
remote: Counting objects: 100% (150/150), done.
remote: Compressing objects: 100% (85/85), done.
remote: Total 5601 (delta 73), reused 122 (delta 53), pack-reused 5451
Receiving objects: 100% (5601/5601), 2.39 MiB | 2.47 MiB/s, done.
Resolving deltas: 100% (3601/3601), done.
$ pushd bcc-${version}
$ git submodule update --init
$ popd
$ mkdir bcc/build; cd bcc/build
$ cmake ..
$ make
$ sudo make install
$ cmake -DPYTHON_CMD=python3 ..
$ pushd src/python/
$ make
$ sudo make install
```

上の手順のうち，[インストールマニュアル][bcc-install]で記載が欠けているのは
以下の部分．
```
$ git submodule update --init
```

これがないと，libbpfのバージョンとbccのバージョンが合わないので，
コンパイルエラーになる場合がある．

## bpftraceのインストール
bpftraceを使うためには[bcc][bcc]が必要であるので，
先に[bcc][bcc]をインストールしていただきたい．
また，
bpftraceの[インストールマニュアル][bpftrace-install]には
問題がないので，その手順を実行していただきたい．


<!-- 参考文献リスト -->
[bpftrace]: <https://github.com/iovisor/bpftrace> "bpftrace"
[bpftrace-ref-guide]: <https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md>  "bpftrace公式リファレンスガイド"
[bpftrace-install]: <https://github.com/iovisor/bpftrace/blob/master/INSTALL.md> "bpftraceインストールマニュアル"
[bcc]: https://github.com/iovisor/bcc "bcc"
[bcc-install]: <https://github.com/iovisor/bcc/blob/master/INSTALL.md> "bccインストールマニュアル"
[bcc-ref-guide]: <https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md> "bcc公式リファレンスガイド"
[kernel-version]: <https://github.com/iovisor/bcc/blob/master/docs/kernel-versions.md> "bccの機能と利用可能なカーネルバージョンの対応関係"

