# kfunc

|環境|動作|
|:--|:--|
|Ubuntu公式|×|
|CentOS公式|×|
|Ubuntu最新|○|
Ubuntu公式，CentOS公式共にサポートしているか否かのチェックコードも動かない．

参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#9-kfuncs

kprobe/kretprobeは監視対象の関数の実行開始直後と終了直前に実行されるのに対し，
kfunc/kretfuncは監視対象関数の実行直前と終了直後に実行されるもの．

## kfunc利用可能性の確認
kfuncはカーネルのビルドオプションで<code>CONFIG_DEBUG_INFO_BTF</code>が選択(y)になっている
ことが条件である．ただし，
ネットでの投稿を見ると「普通のディストリビューションではこの項目はnです．」ということなので，
自分のUbunu20.04の/boot/config-XXXを見ると，やはり「n」になっていた．
20.04で開発者向けに提供されているカーネルバージョン5.6も同じく，
<code>CONFIG_DEBUG_INFO_BTF</code>は無効になっていた．

さらに，<code>CONFIG_DEBUG_INFO_BTF</code>をONにしたカーネルを
インストールしても，kernel用のデバッグ情報のファイルが存在する
必要がある．このファイルを利用可能にするためには，
関連ツールを最新バージョンに上げた後で，カーネルの再コンパイルを
行う必要がある．
詳細は，[本ドキュメントのインストールの説明][install]を参照していただきたい．

実際にデバッグ情報が存在するか否かを確認するのは以下の手段で可能．
```
# cd /sys/kernel/btf
# strings vmlinux |grep bpf_prog_put
#
```
上の実行結果は，kfuncがサポートされていない環境で実行した場合で，
以下がkfunc利用可能な環境の実行結果．
```
# cd /sys/kernel/btf
# strings vmlinux |grep bpf_prog_put
bpf_prog_put
#
```

カーネルのconfigや/sysを調査することなく判定する方法があり，
Rawトレースポイントと同じく，カーネルがサポートしているか否かを判別するヘルパー関数(<code>BPF.support_kfunc()</code>)が提供されている．これを用いたkfuncサポート判別用のプログラムを本ディレクトリ(
<a href="check_kfunc">check_kfunc</a>)に収納している．

これをkfuncをサポートしていないカーネルの環境で実行した結果か以下のもの．
```
# ./check_kfunc
libbpf: failed to find valid kernel BTF
libbpf: vmlinux BTF is not found
kfunc is not supported
#
```
この
<a href="check_kfunc">プログラム</a>の5行目でカーネルがサポートしているか否かを取得して，その直後のif文で
未サポートであることを出力している．
```
is_support_kfunc = BPF.support_kfunc()
if not is_support_kfunc:
    print("kfunc is not supported")
```

ただし，Ubuntu, CentOS共に公式リポジトリのbcc(というよりpython側のライブラリ)は，
kfuncのサポートがないため，チェックコードも動作しない．
```
root@venus:/home/noro/devel/eBPF_intro/bcc/kfunc# ./check_kfunc
Traceback (most recent call last):
  File "./check_kfunc", line 5, in <module>
    is_support_kfunc = BPF.support_kfunc()
AttributeError: type object 'BPF' has no attribute 'support_kfunc'
root@venus:/home/noro/devel/eBPF_intro/bcc/kfunc#
```

## 基本的な利用方法
公式プログラム:
- https://github.com/iovisor/bcc/blob/master/tools/opensnoop.py

公式サイトでは，サンプルプログラムのようなものは提供されておらず，
kfuncを利用しているのは実用的なツールのみとなっている．
また，配布されているツールでkfuncを使うものは2つ配布されているが，
手元の環境で動いたのは上のURLのもののみ．
上のURLのツールは多機能でソースの読解に時間がかかるため，
機能を絞り，最小限のソースとなるよう修正したものを
本ディレクトリの<a href="opensnoop_simple">opensnoop_simple</a>として収容している．
ただし，kfuncを使うのとほぼ同じ機能はkprobeでも可能であり，
公式サイトのツールはOSがkfuncのサポート状況で中の処理を
切り替える仕組みになっているため，
<a href="../OriginalSample/opensnoop.py">そちら</a>を参照していただきたい．

### kfuncの宣言
ソースの41行目を見ると，eBPFのC部分でプログラム自体がkfunc(この場合はkretfunc)のものであることを
<code>KRETFUNC_PROBE</code>で
宣言しており，引数部分で
監視対象の関数が<code>do_sys_open</code>であることと，引数と返り値を宣言している．
python側でロードしたプログラムを特定の監視点に割り付けるようなことはしていない．
```
KRETFUNC_PROBE(do_sys_open, int dfd, const char __user *filename, int flags, int mode, int ret)
```
Cプログラム内で取得した情報は<code>BPF_PERF_OUTPUT</code>(39行目)と<code>perf_submit()</code>(55行目)を使い，Python側に送信している．Python側では99行目の以下のコードで通知が受信できたら，特定の関数を呼び出すことを
宣言している．
```
b["events"].open_perf_buffer(print_event, page_cnt=64)
```
最後に，104行目の<code>perf_buffer_poll()</code>でeBPF VMからの<code>perf_submit()</code>の
データ受信を試みている．

## 実行結果
上記の<a href="opensnoop_simple">サンプルプログラム</a>を実行すると以下のように，
プロセスがファイルオープンを試みた履歴が取得できる．
```
# ./opensnoop_simple
PID    COMM               FD ERR PATH
1      systemd            12   0 /proc/724/cgroup
2116   opensnoop_simpl    -1   2 /usr/lib/python2.7/encodings/ascii.x86_64-linux-gnu.so
2116   opensnoop_simpl    -1   2 /usr/lib/python2.7/encodings/ascii.so
2116   opensnoop_simpl    -1   2 /usr/lib/python2.7/encodings/asciimodule.so

(中略)

704    irqbalance          6   0 /proc/interrupts
704    irqbalance          6   0 /proc/stat
704    irqbalance          6   0 /proc/irq/11/smp_affinity
704    irqbalance          6   0 /proc/irq/0/smp_affinity
704    irqbalance          6   0 /proc/irq/1/smp_affinity
704    irqbalance          6   0 /proc/irq/8/smp_affinity
704    irqbalance          6   0 /proc/irq/12/smp_affinity
704    irqbalance          6   0 /proc/irq/14/smp_affinity
704    irqbalance          6   0 /proc/irq/15/smp_affinity
#
```

<!-- 参考文献リスト -->
[install]: <../../INSTALL.md> "インストールドキュメント"
