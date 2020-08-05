# カーネル内部関数の監視(kprobe / kretprobe)
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新||

今までのサンプルプログラムでは，詳細については説明していなかったが，カーネル内の関数の実行を監視するための
機能がkprobeとkretprobeである．
kprobeはある関数の実行開始時をキャッチするための機能で，kretprobeは実行終了時にeBPFのプログラムが
実行される．

## 基本的な使い方
参考URL:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#1-kprobes

オリジナルのプログラム:
- https://github.com/iovisor/bcc/blob/master/examples/tracing/tcpv4connect.py

本ディレクトリの「<a href="tcpv4connect_simple">tcpv4connect_simple</a>」は
上記オリジナルのプログラムから「kprobe/kretprobe」の使い方の基本に不要な部分を取り除いたものになっている．

eBPFの環境では，VMにロードするプログラムをカーネルのどこに結びつけるかを指定する方法は以下の2種類がある．
- Cのソース内に記載
- 外のpythonで記載

<a href="tcpv4connect_simple">tcpv4connect_simple</a>の8行目から28行目で定義しているeBPF用プログラムの
ソース(bpf_text)見てわかるように「kprobe/kretprobe」を使うためのヘルパー関数(kprobe__関数名/kretprobe__関数名)を
VMにロードするプログラム(関数)の名前とすることで，目的の関数を監視する「kprobe/kretprobe」とCのコードを結びつけている(具体的には12行目と19行目)．

kprobeで引数を読み出す場合は少し説明が必要になるので，この例では削除しているが，「kretprobe」内でこの関数終了時の
返り値を「<code>PT_REGS_RC(ctx)</code>」で取得(22行目)し，「<code>bpf_trace_printk()</code>」でユーザ空間で動作しているpythonプログラムに
通知している(25行目)．

このプログラムを実行し，別のshellウィンドウでwgetでyahooのトップページにアクセスすると
以下のような出力が得られる．
```
$ sudo ./tcpv4connect_simple
PID    COMM         OUTPUT
1469   wget         start of tcp4connect
1469   wget         return value of tcp4connect 0
1469   wget         start of tcp4connect
1469   wget         return value of tcp4connect 0
^C$
```
この出力からわかるのは，yahooのトップページにアクセスすると，別のURLがyahooから返却され，その
URLにデータを取りに行っている(再度TCPのコネクションを確立している)ことがわかる．


## tcp_v4_connect()の引数確認
カーネルソースのインクルードファイルをgrepすると，対象関数の定義が参照できる．今回のサンプルでは「<code>tcp_v4_connect()</code>」を
取り扱うので，「<code>tcp_v4_connect</code>」でインクルードファイルをgrepすると，以下の出力が得られる．
```
root@venus:~# grep tcp_v4_connect /usr/src/linux-headers-5.4.0-42-generic/include/net/*
grep: /usr/src/linux-headers-5.4.0-42-generic/include/net/9p: Is a directory
grep: /usr/src/linux-headers-5.4.0-42-generic/include/net/bluetooth: Is a directory
grep: /usr/src/linux-headers-5.4.0-42-generic/include/net/caif: Is a directory
grep: /usr/src/linux-headers-5.4.0-42-generic/include/net/iucv: Is a directory
grep: /usr/src/linux-headers-5.4.0-42-generic/include/net/netfilter: Is a directory
grep: /usr/src/linux-headers-5.4.0-42-generic/include/net/netns: Is a directory
grep: /usr/src/linux-headers-5.4.0-42-generic/include/net/nfc: Is a directory
grep: /usr/src/linux-headers-5.4.0-42-generic/include/net/phonet: Is a directory
grep: /usr/src/linux-headers-5.4.0-42-generic/include/net/sctp: Is a directory
grep: /usr/src/linux-headers-5.4.0-42-generic/include/net/tc_act: Is a directory
/usr/src/linux-headers-5.4.0-42-generic/include/net/tcp.h:int tcp_v4_connect(struct sock *sk, struct sockaddr *uaddr, int addr_len);
root@venus:~#
```
```
/usr/src/linux-headers-5.4.0-29-generic/include/net/tcp.h:int tcp_v4_connect(struct sock *sk, struct sockaddr *uaddr, int addr_len);
```
第一引数がソケットの構造体，第二引数がアドレス構造体，第三引数がアドレス構造体の大きさとなっている．

## kprobe()における引数へのアクセス
- 参考URL: https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#1-kprobes

本ディレクトリに格納している<a href="tcpv4connect_args">tcpv4connect_args</a>の20行目では，
すべての引数を「<code>kretprobe__tcp_v4_connect()</code>」に与えているが，上記参考URLを見ると
わかるように，なからずしも全ての引数を
与える必要はない．第一引数のみを参照したい場合は，20行目の定義の部分を以下のように
変えれば良い．
```
int kprobe__tcp_v4_connect(struct pt_regs *ctx, struct sock *sk)
```
ただし，第2引数や第3引数のみを参照したい場合，その引数だけを「<code>kretprobe__tcp_v4_connect()</code>」の
引数として与えるのではうまく動かなくて，アクセスする必要がある引数より前すべてを「<code>kretprobe__tcp_v4_connect()</code>」の
引数として定義しておく必要がある．

もし，以下のような定義をするとプログラム自体は動作するが，「<code>addr_len</code>」として読み出されたる値は，
元々の第1引数「<code>struct sock *sk</code>」のポインタのアドレスが整数として読み出される．
```
int kprobe__tcp_v4_connect(struct pt_regs *ctx, int addr_len)
```

