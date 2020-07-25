# トレースポイント
Linuxカーネルでは，カーネル内のいろいろな関数にtracepointと呼ばれる
eBPFでキャッチ可能な場所「デバッガにおけるブレークポイントのようなもの」が
定義されている．

tracepointをeBPFのVMでキャッチすることで，カーネル内の
情報を捕まえることができる．

## 参考文献
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#3-tracepoints


## 公式リファレンスガイドの内容を解説
公式リファレンスガイドでは，カーネル内で疑似乱数列を生成するurandom (/dev/urandom)の読み取りに
対応するカーネル内の関数「urandom_read()」が実行されたことを検出するeBPFのプログラムを用いて説明している．

<code>urandom_read()</code>のトレースポイントで取得可能なデータは以下の例に示しているファイルの中にかかれている「common_xxx」より下に出力されているデータ
で，urandom_read()の場合は3種類の値(int got_bits, int pool_left, int input_left)である．

```
# cat /sys/kernel/debug/tracing/events/random/urandom_read/format
name: urandom_read
ID: 1181
format:
        field:unsigned short common_type;       offset:0;       size:2; signed:0;
        field:unsigned char common_flags;       offset:2;       size:1; signed:0;
        field:unsigned char common_preempt_count;       offset:3;       size:1; signed:0;
        field:int common_pid;   offset:4;       size:4; signed:1;

        field:int got_bits;     offset:8;       size:4; signed:1;
        field:int pool_left;    offset:12;      size:4; signed:1;
        field:int input_left;   offset:16;      size:4; signed:1;

print fmt: "got_bits %d nonblocking_pool_entropy_left %d input_entropy_left %d", REC->got_bits, REC->pool_left, REC->input_left
#
```

この3種類のデータのうち，「got_bits」を取得するサンプルプログラムを開発グループは以下のURLで公開している．
- https://github.com/iovisor/bcc/blob/master/examples/tracing/urandomread.py


```
#!/usr/bin/python
#
# urandomread  Example of instrumenting a kernel tracepoint.
#              For Linux, uses BCC, BPF. Embedded C.
#
# REQUIRES: Linux 4.7+ (BPF_PROG_TYPE_TRACEPOINT support).
#
# Test by running this, then in another shell, run:
#     dd if=/dev/urandom of=/dev/null bs=1k count=5
#
# Copyright 2016 Netflix, Inc.
# Licensed under the Apache License, Version 2.0 (the "License")

from __future__ import print_function
from bcc import BPF
from bcc.utils import printb

# load BPF program
b = BPF(text="""
TRACEPOINT_PROBE(random, urandom_read) {
    // args is from /sys/kernel/debug/tracing/events/random/urandom_read/format
    bpf_trace_printk("%d\\n", args->got_bits);
    return 0;
}
""")

# header
print("%-18s %-16s %-6s %s" % ("TIME(s)", "COMM", "PID", "GOTBITS"))

# format output
while 1:
    try:
        (task, pid, cpu, flags, ts, msg) = b.trace_fields()
    except ValueError:
        continue
    except KeyboardInterrupt:
        exit()
    printb(b"%-18.9f %-16s %-6d %s" % (ts, task, pid, msg))
```

上のプログラム中の「TRACEPOINT_PROBE(random, urandom_read) {...}」の部分が
「urandom_read()」が実行された場合に呼び出される関数となっている．
なお，引数の「(random, urandom_read)」は，「random」がカテゴリ，「urandom_read」が
実際に監視する対象の関数を表している．
なお，カテゴリや関数名を指定する方法については，後ほど説明する．

「TRACEPOINT_PROBE(random, urandom_read) {...}」の中で，トレースポイントで
取得可能なデータのうち，「got_bits」にアクセスしているが，ソースを見てわかるように，
取得可能なデータはすべて「args」という構造体のメンバとしてアクセスできる．

この構造体の定義は別の方法で参照することができる．

###  tplistコマンドによるによる方法
以下の例のように，tplistコマンドを使うと，読み取ることができるデータの名前と型を読み取ることができる．
```
# tplist -v 'random:urandom_read'
random:urandom_read
    int got_bits;
    int pool_left;
    int input_left;
#
```
### カーネルソースのヘッダファイルから読み取る方法
トレースポイントに関する情報は，「(カーネルソースのトップディレクトリ)/include/trace/events/」の下に
いろいろなファイルに分けて記載されている．「urandom_read」の場合は「random.h」を見るとわかる．
```
# cat random.h
TRACE_EVENT(urandom_read,
        TP_PROTO(int got_bits, int pool_left, int input_left),

        TP_ARGS(got_bits, pool_left, input_left),

        TP_STRUCT__entry(
                __field(          int,  got_bits                )
                __field(          int,  pool_left               )
                __field(          int,  input_left              )
        ),

        TP_fast_assign(
                __entry->got_bits       = got_bits;
                __entry->pool_left      = pool_left;
                __entry->input_left     = input_left;
        ),

        TP_printk("got_bits %d nonblocking_pool_entropy_left %d "
                  "input_entropy_left %d", __entry->got_bits,
                  __entry->pool_left, __entry->input_left)
);

```
上のように，「TP_STRUCT__entry()」の中を見ると「TRACEPOINT_PROBE()関数」の中で
「argのメンバ」としてアクセス可能なデータのリストとそれらの型を読み取ることができる．

## トレースポイントが定義されているカーネル内関数を知る方法
先の例にあるように，トレースポイントを指定するためには，「カテゴリ」と「関数名」の
両方が必要であるが，その一覧は「tplist」コマンドで取得することができる．
以下は，「ubuntu-20.04 LTS(Focal Fossa)」でカーネルバージョンは「5.4.0-28」
で実行した場合の先頭部分だけをコピペしたもの(量が多いので先頭だけ)．

```
# tplist
btrfs:btrfs_transaction_commit
btrfs:btrfs_inode_new
btrfs:btrfs_inode_request
btrfs:btrfs_inode_evict
btrfs:btrfs_get_extent
btrfs:btrfs_handle_em_exist
btrfs:btrfs_get_extent_show_fi_regular
btrfs:btrfs_truncate_show_fi_regular
btrfs:btrfs_get_extent_show_fi_inline
(続く)
```

総数がいくつであるかは，以下の例を見てもらえばわかるが，とても多い．
```
# tplist |wc -l
1513
#
```

トレースポイントのうち，カテゴリ部分がいくつあるかを確認するには，以下のコマンドを利用する．

```
# tplist |sed 's/:/ /g'|awk '$0~/.*/ {print $1};' |sort |uniq |wc -l
94
#
```

また，カテゴリだけの一覧は以下のコマンドで取得可能．

```
# tplist |sed 's/:/ /g'|awk '$0~/.*/ {print $1};' |sort |uniq
alarmtimer
block
bpf_test_run
bridge
btrfs
cgroup
clk
(中略)
wbt
workqueue
writeback
x86_fpu
xdp
xen
xhci-hcd
#
```

## 実際に使ってみる
ここでは，通信性能を測るのに役に立つ「tcp_probe()」のデータをeBPFで読み取るプログラムを
作ってみる．

### 取得可能なデータを知る

#### /sysファイルシステムからの取得
まず，読み取ることができるデータの一覧を「/sys」から読み取ってみる．

```
# cat /sys/kernel/debug/tracing/events/tcp/tcp_probe/format
name: tcp_probe
ID: 1394
format:
        field:unsigned short common_type;       offset:0;       size:2; signed:0;
        field:unsigned char common_flags;       offset:2;       size:1; signed:0;
        field:unsigned char common_preempt_count;       offset:3;       size:1; signed:0;
        field:int common_pid;   offset:4;       size:4; signed:1;

        field:__u8 saddr[sizeof(struct sockaddr_in6)];  offset:8;       size:28;        signed:0;
        field:__u8 daddr[sizeof(struct sockaddr_in6)];  offset:36;      size:28;        signed:0;
        field:__u16 sport;      offset:64;      size:2; signed:0;
        field:__u16 dport;      offset:66;      size:2; signed:0;
        field:__u32 mark;       offset:68;      size:4; signed:0;
        field:__u16 data_len;   offset:72;      size:2; signed:0;
        field:__u32 snd_nxt;    offset:76;      size:4; signed:0;
        field:__u32 snd_una;    offset:80;      size:4; signed:0;
        field:__u32 snd_cwnd;   offset:84;      size:4; signed:0;
        field:__u32 ssthresh;   offset:88;      size:4; signed:0;
        field:__u32 snd_wnd;    offset:92;      size:4; signed:0;
        field:__u32 srtt;       offset:96;      size:4; signed:0;
        field:__u32 rcv_wnd;    offset:100;     size:4; signed:0;
        field:__u64 sock_cookie;        offset:104;     size:8; signed:0;

print fmt: "src=%pISpc dest=%pISpc mark=%#x data_len=%d snd_nxt=%#x snd_una=%#x snd_cwnd=%u ssthresh=%u snd_wnd=%u srtt=%u rcv_wnd=%u sock_cookie=%llx", REC->saddr, REC->daddr, REC->mark, REC->data_len, REC->snd_nxt, REC->snd_una, REC->snd_cwnd, REC->ssthresh, REC->snd_wnd, REC->srtt, REC->rcv_wnd, REC->sock_cookie
#
```


#### tplistで知る

```
# tplist -v 'tcp:tcp_probe'
tcp:tcp_probe
    __u8 saddr[sizeof(struct sockaddr_in6)];
    __u8 daddr[sizeof(struct sockaddr_in6)];
    __u16 sport;
    __u16 dport;
    __u32 mark;
    __u16 data_len;
    __u32 snd_nxt;
    __u32 snd_una;
    __u32 snd_cwnd;
    __u32 ssthresh;
    __u32 snd_wnd;
    __u32 srtt;
    __u32 rcv_wnd;
    __u64 sock_cookie;
#
```

### プログラムの例
基本的な例を本ディレクトリの<a href="tcp_probe_simple">tcp_probe_simple</a>として収納している．

まず，10行目の「<code>#include <net/ipv6.h></code>」なしに，「args」にアクセス(13)行目すると，「<code>struct sockaddr_in6</code>」なんて
知らないとエラーがでる．通常は，「/usr/inclucde/netinet/in.h」に定義があるので，
「#include <inetnet/in.h>」と書くと，『「inetinet/in.h」』などというファイルは無いと
怒られる．仕方がないので，カーネルソースのincludeファイルで一番近いものを選んで「<code>#include <net/ipv6.h></code>」
と書くと，今度は「<code>KBUILD_MODNAME</code>」は未定義だと怒られる．

いろいろぐぐってみると，「とりあえず，なにかの名前をつけろ」と書いてあったので，今回は「監視対象の関数名そのもの」を
モジュール名にしたところ(9行目)，コンパイルが通り，eBPFのプログラムが動きはじめた．

ただし，bccのバージョンを0.15.0まで上げると<code>KBUILD_MODNAME</code>はbccによって
自動で「<code>#define KBUILD_MODNAME "bcc"</code>」という定義がされる仕組みに
かわっており，このディレクトリに収めている全てのサンプルプログラムで以下のような
シンボル2重定義の警告がでる．

```
# ./tcp_probe_simple
/virtual/main.c:2:9: warning: 'KBUILD_MODNAME' macro redefined
      [-Wmacro-redefined]
#define KBUILD_MODNAME "tcp_probe"
        ^
<command line>:6:9: note: previous definition is here
#define KBUILD_MODNAME "bcc"
        ^
^C1 warning generated.
Traceback (most recent call last):
  File "./tcp_probe_simple", line 8, in <module>
    b = BPF(text="""
  File "/usr/lib/python3/dist-packages/bcc/__init__.py", line 353, in __init__
    self.module = lib.bpf_module_create_c_from_string(text,
KeyboardInterrupt

#
```
これは無視しても動くこと，もし，この定義を削除すると各ディストリビューションの
パッケージとして配られている古いbccをインストールしている人が動かなくなるためである．

### アドレス解釈の必要性
アクセスできる情報のうち，送信元アドレスと着信先アドレスの両方とも，型は「<code>struct sockaddr_in6</code>」となっているが，
実際は，「<code>struct sockaddr_in</code>」と「<code>struct sockaddr_in6</code>」の「<code>union</code>」となっているので，実際に格納されている
アドレスの情報が「IPv4/IPv6」のいずれかを判別する方法が必要になる．
```
    __u8 saddr[sizeof(struct sockaddr_in6)];
    __u8 daddr[sizeof(struct sockaddr_in6)];
```

### アドレス情報の判別方法
IPv4の場合は，「<code>sockaddr_in</code>」で，その型定義は以下のようなものとなっている．
```
struct sockaddr_in
  {
    __SOCKADDR_COMMON (sin_);
    in_port_t sin_port;                 /* Port number.  */
    struct in_addr sin_addr;            /* Internet address.  */
    (中略)
  }

typedef uint32_t in_addr_t;
struct in_addr
  {
    in_addr_t s_addr;
  };
```
それに対して，IPv6は次のような定義となっている．
```
struct sockaddr_in6
  {
    __SOCKADDR_COMMON (sin6_);
    in_port_t sin6_port;        /* Transport layer port # */
    uint32_t sin6_flowinfo;     /* IPv6 flow information */
    struct in6_addr sin6_addr;  /* IPv6 address */
    uint32_t sin6_scope_id;     /* IPv6 scope-id */
  };
```

ここで，<code> __SOCKADDR_COMMON (sin6_); </code>の部分は，16bitとなっている．

なお，参考にしたファイルは以下の2つ．
- /usr/include/x86_64-linux-gnu/bits/socket.h
- /usr/include/netinet/in.h

### 実行してみた
上のプログラムを実行してみて，宛先アドレス(daddr[])を先頭からどのような値が含まれているかを確認してみたところ，
以下のようになっていた．

#### IPv4の場合
| データ |   値 |  内容(推定) |
|:--|:--:|:--|
|daddr[0] | 2 | PF_INET(AF_INET) x86_64-linux-gnu/bits/socket.h |
|daddr[1] | 0 | IPPROTO_IP (Dummy protocol for TCP) |
|daddr[2],daddr[3]| (204, 165) |204*256+156=52389番ポート|
|daddr[4]～daddr[7]|| IPv4アドレスの値

#### IPv6の場合
全部書くと量が多いので，先頭のみ表示．
| データ |   値 |  内容(推定) |
|:--|:--:|:--|
|daddr[0]| 10 |  PF_INET6 (AF_INET6) x86_64-linux-gnu/bits/socket.h |
|daddr[1]| 0 | IPPROTO_IP (Dummy protocol for TCP) /usr/include/netinet/in.h|
|daddr[2],daddr[3]| |ポート番号|

#### IPv4とv6の識別方法
IPv4の場合とIPv6の場合を比較して見ると，アドレスを格納している配列の先頭を見て値が「2」か「10」かで判定することができる．この結果をベースにして，後ろの領域に入っているデータの処理を変更する．

## フィルタ付きのサンプルプログラム
上のサンプルプログラム(<a href="tcp_probe_simple">tcp_probe_simple</a>)では，TCPのパラメータが変更される(1回のACKが受信された場合や，パケットロスが発生した場合)たびにイベントが発生し，<code>bpf_trace_printk()</code>の出力がユーザ空間プログラムで処理されるため，大量のイベントが発生することになる．
これを避けるためには，実際に見るべきものに絞り込んで，ユーザ空間に通知を上げるべきである．

本ディレクトリの<a href="tcp_probe_filter">tcp_probe_filter</a>は，<code>TRACEPOINT_PROBE(tcp, tcp_probe)</code>の先頭で，引数の中身を確認して，IPv4のイベントでなければそのまま終了するコードとなっている．


## 通常使われるであろうプログラムのベース
本ディレクトリに格納されている<a href="tcp_probe">tcp_probe</a>では，<code>tcp_probe()</code>で取得可能なパラメータの他，該当イベントが起きる元となったプロセスIDとそのプロセスの名前を取得した上，<code>bpf_trace_printk()</code>ではなく，<code>perf_submit()</code>で通知している．

<code>bpf_trace_printk()</code>を用いない理由は，<code>bpf_trace_printk()</code>の出力をユーザ空間で受けた場合，データを別の目的で計算しようとすると，文字列解析等を行う必要があることが１つ目の理由である．
もう一つの理由は，「はじめの一歩」でも説明したように，いろいろ利用上の制約が厳しいためである．

このプログラム中で，アドレス情報をコピーする際に，
<code>bpf_probe_read()</code>を用いている(46と47行目)が，
配列の中身をコピーするために，for文を用いると，
VMのセキュリティ機構により，危険なメモリへの
アクセスとして，VMがプログラムの実行を強制終了してしまうためである．

## attach_tracepoint()の利用
今までのサンプルプログラムでは，eBPFのVMに与えるCの関数側で監視対象を指定していたが，
Python側から監視場所を指定する方法もある．この例のプログラムが本ディレクトリに
収納している
<a href="tcp_probe_attach_tracepoint">tcp_probe_attach_tracepoint</a>である．

公式リファレンスガイドを見ると，<code>TRACEPOINT_PROBE</code>の代わりに<code>attach_tracepoint()</code>を使う方法は
別のサンプルプログラムで提供(以下のURL)されている．
- https://github.com/iovisor/bcc/blob/master/examples/tracing/urandomread-explicit.py

上のURLのプログラムを下に掲載するが，<code>tracepoint</code>の引数を
先に定義し，それを使ってeBPFのプログラム(関数)を定義した上で，
Python側でロード(<code>attach_tracepoint()</code>)している．

```
#!/usr/bin/python
#
# urandomread-explicit  Example of instrumenting a kernel tracepoint.
#                       For Linux, uses BCC, BPF. Embedded C.
#
# This is an older example of instrumenting a tracepoint, which defines
# the argument struct and makes an explicit call to attach_tracepoint().
# See urandomread for a newer version that uses TRACEPOINT_PROBE().
#
# REQUIRES: Linux 4.7+ (BPF_PROG_TYPE_TRACEPOINT support).
#
# Test by running this, then in another shell, run:
#     dd if=/dev/urandom of=/dev/null bs=1k count=5
#
# Copyright 2016 Netflix, Inc.
# Licensed under the Apache License, Version 2.0 (the "License")

from __future__ import print_function
from bcc import BPF
from bcc.utils import printb

# define BPF program
bpf_text = """
#include <uapi/linux/ptrace.h>
struct urandom_read_args {
    // from /sys/kernel/debug/tracing/events/random/urandom_read/format
    u64 __unused__;
    u32 got_bits;
    u32 pool_left;
    u32 input_left;
};
int printarg(struct urandom_read_args *args) {
    bpf_trace_printk("%d\\n", args->got_bits);
    return 0;
}
"""

# load BPF program
b = BPF(text=bpf_text)
b.attach_tracepoint(tp="random:urandom_read", fn_name="printarg")

# header
print("%-18s %-16s %-6s %s" % ("TIME(s)", "COMM", "PID", "GOTBITS"))

# format output
while 1:
    try:
        (task, pid, cpu, flags, ts, msg) = b.trace_fields()
    except ValueError:
        continue
    except KeyboardInterrupt:
        exit()
    printb(b"%-18.9f %-16s %-6d %s" % (ts, task, pid, msg))
```

これを真似て，<a href="tcp_probe_simple">tcp_probe_simple</a>を<code>attach_tracepoint()</code>を
使うよう書き換えたものが，本ディレクトリに収容している
<a href="tcp_probe_attach_tracepoint">tcp_probe_attach_tracepoint</a>である．

<a href="tcp_probe_simple">tcp_probe_simple</a>と
<a href="tcp_probe_attach_tracepoint">tcp_probe_attach_tracepoint</a>を比べると，
主な違いとして，
引数のデータ型を12行目から29行目で定義していること．ここで注意が必要なのは，<code>u64 __unused__;</code>というメンバを
用意しているところ．
```
// トレースポイントデータ用の型定義
struct tcp_probe_args {
    u64 __unused__;
    u8 saddr[sizeof(struct sockaddr_in6)];
    u8 daddr[sizeof(struct sockaddr_in6)];
    u16 sport;
    u16 dport;
    u32 mark;
    u16 data_len;
    u32 snd_nxt;
    u32 snd_una;
    u32 snd_cwnd;
    u32 ssthresh;
    u32 snd_wnd;
    u32 srtt;
    u32 rcv_wnd;
    u64 sock_cookie;
};
```
tplistコマンドで情報をひろうと，<code>saddr[]</code>の前にはなにもない．
```
# tplist -v 'tcp:tcp_probe'
tcp:tcp_probe
    __u8 saddr[sizeof(struct sockaddr_in6)];
    __u8 daddr[sizeof(struct sockaddr_in6)];
    __u16 sport;
    __u16 dport;
    __u32 mark;
    __u16 data_len;
    __u32 snd_nxt;
    __u32 snd_una;
    __u32 snd_cwnd;
    __u32 ssthresh;
    __u32 snd_wnd;
    __u32 srtt;
    __u32 rcv_wnd;
    __u64 sock_cookie;
#
```
それに対して，
/sysファイルシステムからフォーマットを参照すると，下のように<code>common_xxx</code>というデータが存在しており，
その部分が合計で64bitとなっている．
```
# cat /sys/kernel/debug/tracing/events/tcp/tcp_probe/format
name: tcp_probe
ID: 1394
format:
        field:unsigned short common_type;       offset:0;       size:2; signed:0;
        field:unsigned char common_flags;       offset:2;       size:1; signed:0;
        field:unsigned char common_preempt_count;       offset:3;       size:1; signed:0;
        field:int common_pid;   offset:4;       size:4; signed:1;

        field:__u8 saddr[sizeof(struct sockaddr_in6)];  offset:8;       size:28;        signed:0;
        field:__u8 daddr[sizeof(struct sockaddr_in6)];  offset:36;      size:28;        signed:0;
        field:__u16 sport;      offset:64;      size:2; signed:0;
        field:__u16 dport;      offset:66;      size:2; signed:0;
        field:__u32 mark;       offset:68;      size:4; signed:0;
        field:__u16 data_len;   offset:72;      size:2; signed:0;
        field:__u32 snd_nxt;    offset:76;      size:4; signed:0;
        field:__u32 snd_una;    offset:80;      size:4; signed:0;
        field:__u32 snd_cwnd;   offset:84;      size:4; signed:0;
        field:__u32 ssthresh;   offset:88;      size:4; signed:0;
        field:__u32 snd_wnd;    offset:92;      size:4; signed:0;
        field:__u32 srtt;       offset:96;      size:4; signed:0;
        field:__u32 rcv_wnd;    offset:100;     size:4; signed:0;
        field:__u64 sock_cookie;        offset:104;     size:8; signed:0;

print fmt: "src=%pISpc dest=%pISpc mark=%#x data_len=%d snd_nxt=%#x snd_una=%#x snd_cwnd=%u ssthresh=%u snd_wnd=%u srtt=%u rcv_wnd=%u sock_cookie=%llx", REC->saddr, REC->daddr, REC->mark, REC->data_len, REC->snd_nxt, REC->snd_una, REC->snd_cwnd, REC->ssthresh, REC->snd_wnd, REC->srtt, REC->rcv_wnd, REC->sock_cookie
#
```
この差は<code>TRACEPOINT_PROBE()</code>を使う場合は気にしなくて良いが，
<code>attach_tracepoint()</code>を使う場合は意識する必要がある．



