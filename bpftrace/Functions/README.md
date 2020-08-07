# 関数

|変数名|内容|
|:---|:---|
|<code>printf(char *fmt, ...)</code>| フォーマット出力 |
|<code>time(char *fmt)</code>| 整形した時刻情報 |
|<code>join(char *arr[] [, char *delim])</code>| Print the array |
|<code>str(char *s [, int length])</code>| Returns the string pointed to by s |
|<code>buf(void *d [, int length])</code>| Returns a hex-formatted string of the data pointed to by d |
|<code>ksym(void *p)</code>| Resolve kernel address |
|<code>usym(void *p)</code>| Resolve user space address |
|<code>kaddr(char *name)</code>| Resolve kernel symbol name |
|<code>uaddr(char *name)</code>| Resolve user-level symbol name |
|<code>reg(char *name)</code>| Returns the value stored in the named register |
|<code>system(char *fmt)</code>| Execute shell command |
|<code>exit()</code>| Quit bpftrace |
|<code>cgroupid(char *path)</code>| Resolve cgroup ID |
|<code>kstack([StackMode mode, ][int level])</code>| Kernel stack trace |
|<code>ustack([StackMode mode, ][int level])</code>| User stack trace |
|<code>ntop([int af, ] (int\|char) [4\|16] addr)</code>| Convert IP address data to text |
|<code>cat(char *filename)</code>| Print file content |
|<code>signal(char[] signal \| u32 signal)</code>| Send a signal to the current task |
|<code>strncmp(char *s1, char *s2, int length)</code>| Compare first n characters of two strings |
|<code>override(u64 rc)</code>| Override return value |

## <code>printf()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|


C言語等の<code>printf()</code>と基本的には同じ動きをするが，使えるフォーマットに制約がある．
詳細はどこにも記載がないので不明．

## <code>time()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

<code>strftime(3)</code>と同じで，現在時刻をフォーマットして出力する．

## <code>join()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|


文法:
```
join(char *arr[] [, char *delim])
```

引数に文字列の配列を与えると，<code>delim</code>で指定した文字列をデリミタとして使い，配列内の文字列を繋げて返す関数．
もし，<code>delim</code>を省略すると，空白がデリミタとして利用される．

下の実行例は，[公式リファレンスガイド][ref-guide]のデリミタの指定なしの例であるが，別のウィンドウで<code>ls</code>と<code>man man</code>
を実行した場合の出力である．
```
# bpftrace -e 'tracepoint:syscalls:sys_enter_execve { join(args->argv); }'
Attaching 1 probe...
ls --color=auto
man man
pager
pager
pager
pager
pager
preconv -e UTF-8
nroff -mandoc -Tutf8
preconv -e UTF-8
preconv -e UTF-8
preconv -e UTF-8
preconv -e UTF-8
tbl
nroff -mandoc -Tutf8
nroff -mandoc -Tutf8
nroff -mandoc -Tutf8
nroff -mandoc -Tutf8
tbl
tbl
tbl
tbl
locale charmap
groff -mtty-char -Tutf8 -mandoc
grotty
troff -mtty-char -mandoc -Tutf8
^C

#
```
次に，デリミタとしてコンマ<code>,</code>を指定した場合は以下のような出力となる．
```
# bpftrace -e 'tracepoint:syscalls:sys_enter_execve { join(args->argv, ","); }'
Attaching 1 probe...
ls,--color=auto
man,man
pager
tbl
pager
preconv,-e,UTF-8
pager
pager
pager
preconv,-e,UTF-8
preconv,-e,UTF-8
preconv,-e,UTF-8
preconv,-e,UTF-8
tbl
tbl
tbl
tbl
nroff,-mandoc,-Tutf8
nroff,-mandoc,-Tutf8
nroff,-mandoc,-Tutf8
nroff,-mandoc,-Tutf8
nroff,-mandoc,-Tutf8
locale,charmap
groff,-mtty-char,-Tutf8,-mandoc
grotty
troff,-mtty-char,-mandoc,-Tutf8
^C

#
```

## <code>str()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|


文法:
```
str(char *s [, int length])
```

Cのキャラクタの配列(実質上の文字列)を実際の文字列に変換する関数で，長さは省略可能．
長さを省略した場合は，環境変数<code>BPFTRACE_STRLEN</code>で指定されている値が
用いられる．もし，環境変数の指定がない場合は64となる．

下の実行例は，[公式リファレンスガイド][ref-guide]の例を手元の環境で実行したもので，別のウィンドウで「<code>ls</code>」を
実行した場合の出力である．
```
# bpftrace -e 'tracepoint:syscalls:sys_enter_execve { printf("%s called %s\n", comm, str(args->filename)); }'
Attaching 1 probe...
bash called /usr/bin/ls
^C

#
```

次の例も[公式リファレンスガイド][ref-guide]のものであるが，別ウィンドウのbashで
linuxカーネルソースの奥深くのディレクトリで
lsを実行した場合の出力となっている．
```
# BPFTRACE_STRLEN=200 bpftrace -e 'tracepoint:syscalls:sys_enter_write /pid == 3142/
>    { printf("<%s>\n", str(args->buf, args->count)); }'
Attaching 1 probe...
<l>
<s>
<
>
<anon@ebpf:/usr/src/linux-source-5.4.0/linux-source-5.4.0/include/linux/platform_data/x86$ >
^C

#
```

```
bash$ ls
apple.h  asus-wmi.h  clk-lpss.h  clk-pmc-atom.h  mlxcpld.h  pmc_atom.h
bash$ 
```

## <code>ksym()</code>と<code>reg()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|


文法:
```
ksym(addr)
reg(char *name)
```
<code>ksym()</code>はカーネルレベルのシンボル解決を実現する関数，<code>reg()</code>はレジスタ名を
与えると，レジスタの値を返す関数である．<code>reg()</code>で使えるレジスタ名はbpftraceのソース(src/arch/x86_64.cpp)
に定義されており，手元のバージョンでは以下のようなものが指定されている．

```
static std::array<std::string, 27> registers = {
  "r15",
  "r14",
  "r13",
  "r12",
  "bp",
  "bx",
  "r11",
  "r10",
  "r9",
  "r8",
  "ax",
  "cx",
  "dx",
  "si",
  "di",
  "orig_ax",
  "ip",
  "cs",
  "flags",
  "sp",
  "ss",
};
```

bpftraceには，probeで指定した関数名を表す組み込み変数<code>func</code>が存在するが，
それを用いずに，インストラクションポインタをレジスタから読み出して，それに該当関数からはみ出さない範囲の
値を足しても同じ結果を得ることができる．ただし，それより大きい値を指定すると隣や隣の隣の関数名が拾える．
```
# bpftrace -e 'kprobe:do_nanosleep { printf("%s\n", func); }'
Attaching 1 probe...
do_nanosleep
do_nanosleep
^C

# bpftrace -e 'kprobe:do_nanosleep { printf("%s\n", ksym(reg("ip")+4)); }'
Attaching 1 probe...
do_nanosleep
do_nanosleep
^C

# bpftrace -e 'kprobe:do_nanosleep { printf("%s\n", ksym(reg("ip")+1024)); }'
Attaching 1 probe...
ldsem_down_read
ldsem_down_read
^C

#
```

## <code>kaddr()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|


文法:
```
kaddr(char *name)
```
カーネル内の変数の名前を引数として与えると，そのアドレスを返す関数．
下の例は，カーネルソースの「drivers/usb/core/usb.c」に含まれる変数
<code>usbcore_name</code>を使っている．
なお，ソース内で<code>usbcore_name</code>は次のように定義されている．
```
const char *usbcore_name = "usbcore";
```

この例は，変数<code>usbcore_name</code>のアドレスを<code>kaddr()</code>で
取得し，その内容を<code>str()</code>で文字列に変換している．
```
# bpftrace -e 'BEGIN { printf("%s\n", str(*kaddr("usbcore_name"))); }'
Attaching 1 probe...
usbcore
^C

#
```

## <code>usym()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
usym(addr)
```

<code>usym()</code>は上の<code>ksym()</code>と類似の関数で，
ユーザアプリのシンボル名を解決するものである．<code>ksym()</code>と同じく
アドレス指定で，他の関数名も取得することができる．

```
# bpftrace -e 'uprobe:/bin/bash:readline { printf("%s\n", usym(reg("ip"))); }'
Attaching 1 probe...
readline
readline
readline
readline
^C

# bpftrace -e 'uprobe:/bin/bash:readline { printf("%s\n", usym(reg("ip")+512)); }'
Attaching 1 probe...
rl_restore_state
^C

#
```

## <code>uaddr()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|×|
|CentOS公式|×|
|Ubuntu最新|×|


文法:
```
u64 *uaddr(symbol) (default)
u32 *uaddr(symbol)
u16 *uaddr(symbol)
u8 *uaddr(symbol)
```

<code>uaddr()</code>は<code>kaddr()</code>と同じくシンボルのアドレスを返す関数であるが，
<code>kaddr()</code>と異なり，ユーザ空間のプログラムを対象としているため，以下の
2種類のprobeを使う場合に限られる．
- uprobe/uretprobe
- USDT

なお，注意事項としてアドレス空間のランダム化(ASLR:Address Space Layout Randomization)
が有効な場合は動かない．Ubuntu20.04でsystemctlで一時的にASLRを無効化しても動かなかった．


## <code>system()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
system(command)
```
この関数は，<code>command</code>で指定したコマンドを実行するためのものである．
下の例からもわかるように，printfのように文字列を展開することが可能である．

```
# bpftrace --unsafe -e 'kprobe:do_nanosleep { system("ps -p %d\n", pid); }'
Attaching 1 probe...
    PID TTY          TIME CMD
    567 ?        00:00:02 multipathd
    PID TTY          TIME CMD
    665 ?        00:00:00 cron
    PID TTY          TIME CMD
    567 ?        00:00:02 multipathd
    PID TTY          TIME CMD
    567 ?        00:00:02 multipathd
    PID TTY          TIME CMD
    567 ?        00:00:02 multipathd
^C

#
```

ただし，この関数を利用するためには，上のように<code>--unsafe</code>が必要である．
もし，<code>--unsafe</code>オプション無しに実行すると次のようなエラーとなる．

```
# bpftrace -e 'kprobe:do_nanosleep { system("ps -p %d\n", pid); }'
stdin:1:23-48: ERROR: system() is an unsafe function being used in safe mode
kprobe:do_nanosleep { system("ps -p %d\n", pid); }
                      ~~~~~~~~~~~~~~~~~~~~~~~~~
#
```

## <code>exit()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

<code>exit()</code>はbpftraceの実行を終了する(コンソールでCtrl-Cを入力したのと同じ)コマンドである．
そのため，<code>exit()</code>が実行されるとEND節の実行等が行われる．

以下の例は，bpftrace実行開始後1秒経過で自動的に終了する．
```
# bpftrace -e 'interval:s:1 {printf("1sec passed\n"); exit(); } END{printf("END\n");}'
Attaching 2 probes...
1sec passed
END


#
```

## <code>cgroupid()</code>
|環境|動作|備考|
|:--|:--|:--|
|Ubuntu公式|○||
|CentOS公式|×|デフォルトでは例のsliceが存在しないので，cgroupを作るところから始めないといけない|
|Ubuntu最新|○||

文法:
```
cgroupid(char *path)
```
引数にcgourpを表す/sysファイルシステムのディレクトリを渡すと，そのIDを返す関数である．
[公式リファレンスガイド][ref-guide]では，特定のcgruopで起こったことを監視するために，
この関数をフィルタ内部で使っている．[公式リファレンスガイド][ref-guide]の例を
少し丁寧に以下に説明する．

### 準備
bpftraceを動作させるウィンドウとは別のウィンドウでcgroupを有効にする操作を行う．
```
# cd /sys/fs/cgroup/unified/user.slice/
# ls
cgroup.controllers      cgroup.stat             init.scope       user.slice
cgroup.max.depth        cgroup.subtree_control  io.pressure
cgroup.max.descendants  cgroup.threads          memory.pressure
cgroup.procs            cpu.pressure            system.slice
# echo $$ > cgroup.procs
```

### bpfrraceの実行
先程の準備をしたウィンドウとは別のウィンドウで，以下のようにbpftraceを実行する．
すると，対象cgroupでopenatが実行されるまでなにも出力されない．
```
# bpftrace -e 'tracepoint:syscalls:sys_enter_openat /cgroup == cgroupid("/sys/fs/cgroup/unified/user.slice")/
>    { printf("%s\n", str(args->filename)); }'
Attaching 1 probe...
```

### 監視される側の実行とbpftraceの出力
元々準備の操作をしたウィンドウで，以下のコマンドを実行．
```
# cat /etc/shadow
```

これにより，bpftrace側に<code>openat</code>の引数のうち，ファイル名の部分が文字列に変換されて，下のような出力となる．
見るとわかるように，コマンドの実行に必要な共有ライブラリが開かれて，最後に，引数として指定した<code>/etc/shadow</code>
が開かれている．
```
# bpftrace -e 'tracepoint:syscalls:sys_enter_openat /cgroup == cgroupid("/sys/fs/cgroup/unified/user.slice")/
    { printf("%s\n", str(args->filename)); }'
Attaching 1 probe...
tls/x86_64/x86_64/libc.so.6
tls/x86_64/libc.so.6
tls/x86_64/libc.so.6
tls/libc.so.6
x86_64/x86_64/libc.so.6
x86_64/libc.so.6
x86_64/libc.so.6
libc.so.6
/usr/local/lib/tls/x86_64/x86_64/libc.so.6
/usr/local/lib/tls/x86_64/libc.so.6
/usr/local/lib/tls/x86_64/libc.so.6
/usr/local/lib/tls/libc.so.6
/usr/local/lib/x86_64/x86_64/libc.so.6
/usr/local/lib/x86_64/libc.so.6
/usr/local/lib/x86_64/libc.so.6
/usr/local/lib/libc.so.6
/etc/ld.so.cache
/lib/x86_64-linux-gnu/libc.so.6

/etc/shadow
^C

#
```

## <code>ntop()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
ntop([int af, ]int|char[4|16] addr)
```
IPv4/v6のアドレスの数値(パケットに使う形式)をホストの形式に変更する関数．
```
# bpftrace -e 'BEGIN { printf("%s\n", ntop(0x0100007f));}'
Attaching 1 probe...
127.0.0.1
^C

#
```
[公式リファレンスガイド][ref-guide]にあるように，アドレス形式を明示することも可能．
```
# bpftrace -e '#include <linux/socket.h>
> BEGIN { printf("%s\n", ntop(AF_INET, 0x0100007f));}'
Attaching 1 probe...
127.0.0.1
^C

#
```
[公式リファレンスガイド][ref-guide]の最後の例は手元の環境では
<code>tcp_set_state()</code>にトレースポイントが存在しないため，
動かないことから，別の関数で試してみた．

まず，類似の引数を使い，トレースポイントがある関数に以下のものがある．
<code>tplist</code>コマンドで，引数を確認すると以下の情報が得られる．
```
# tplist -v tcp:tcp_destroy_sock
tcp:tcp_destroy_sock
    const void * skaddr;
    __u16 sport;
    __u16 dport;
    __u8 saddr[4];
    __u8 daddr[4];
    __u8 saddr_v6[16];
    __u8 daddr_v6[16];
    __u64 sock_cookie;
#
```
この情報を[公式リファレンスガイド][ref-guide]の例に当てはめて実行したものが以下の実行例である．
```
# bpftrace -e '#include <net/ipv6.h>
> tracepoint:tcp:tcp_destroy_sock { printf("%s\n", ntop(args->daddr_v6));}'
Attaching 1 probe...
2404:6800:4004:813::2003
^C

# bpftrace -e '#include <net/ipv6.h>
> tracepoint:tcp:tcp_destroy_sock { printf("%s\n", ntop(AF_INET6, args->daddr_v6))
> ;}'
Attaching 1 probe...
2404:6800:4004:813::2003
^C

#
```
2つの実行例のうち，最初のものはIPv6の指定をせずに実行したもので，2つめはIPv6を指定したもの．
この出力を得るために，別のウィンドウで<code>wget www.google.co.jp</code>を実行している．

IPv4しかサポートしていないサーバ(yahoo)に対するアクセスを同じbpftraceスクリプトで実行すると
以下のような結果(IPv4互換アドレスの表示)となる．
```
# bpftrace -e '#include <net/ipv6.h>
tracepoint:tcp:tcp_destroy_sock { printf("%s\n", ntop(args->daddr_v6));}'
Attaching 1 probe...
::ffff:183.79.250.123
::ffff:183.79.250.123
^C

# bpftrace -e '#include <net/ipv6.h>
tracepoint:tcp:tcp_destroy_sock { printf("%s\n", ntop(AF_INET6, args->daddr_v6));}'
Attaching 1 probe...
::ffff:183.79.250.123
::ffff:183.79.250.123
^C

#
```


## <code>kstack()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|


文法:
```
kstack([StackMode mode, ][int limit])
```
スタックの<code>mode</code>を指定せずに実行する例を以下に示す．
```
# bpftrace -e 'kprobe:ip_output { @[kstack()] = count(); }'
Attaching 1 probe...
^C

@[
    ip_output+1
    __ip_queue_xmit+375
    ip_queue_xmit+16
    __tcp_transmit_skb+1351
    tcp_write_xmit+928
    __tcp_push_pending_frames+55
    tcp_push+253
    tcp_sendmsg_locked+3184
    tcp_sendmsg+45
    inet_sendmsg+67
    sock_sendmsg+94
    sock_write_iter+147
    new_sync_write+428
    __vfs_write+41
    vfs_write+185
    ksys_write+177
    __x64_sys_write+26
    do_syscall_64+87
    entry_SYSCALL_64_after_hwframe+68
]: 4

# bpftrace -e 'kprobe:ip_output { @[kstack(3)] = count(); }'
Attaching 1 probe...
^C

@[
    ip_output+1
    __ip_queue_xmit+375
    ip_queue_xmit+16
]: 1

#
```

[公式リファレンスガイド][ref-guide]は引数の<code>mode</code>の部分を
「format」と記載しているが，文法と矛盾するため「mode」と書くのが
正しいだろう．利用可能なモードは「<code>perf</code>」と「<code>bpftrace</code>」
が利用可能で，表示形式が変更される．以下はその実行例である．
```
# bpftrace -e 'kprobe:ip_output { @[kstack(perf,3)] = count(); }'
Attaching 1 probe...
^C

@[
        ffffffffbc3d4141 ip_output+1
        ffffffffbc3d3bf7 __ip_queue_xmit+375
        ffffffffbc3f7680 ip_queue_xmit+16
]: 1

# bpftrace -e 'kprobe:ip_output { @[kstack(bpftrace,3)] = count(); }'
Attaching 1 probe...
^C

@[
    ip_output+1
    __ip_queue_xmit+375
    ip_queue_xmit+16
]: 1

#
```

## <code>ustack()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
ustack([StackMode mode, ][int limit])
```
<code>ustack()</code>も対象がユーザ空間のプログラムであること以外は<code>kstack()</code>と同じ．
以下に，リミットの値を変更したり，modeを変更して実行した事例を示す．

```
# bpftrace -e 'kprobe:do_sys_open /comm == "bash"/ { @[ustack(1)] = count(); }'
Attaching 1 probe...
^C

@[
    0x7fbf0927cd1b
]: 1
@[
    0x7fbf092824cc
]: 2
@[
    0x7fbf093bfec8
]: 38

# bpftrace -e 'kprobe:do_sys_open /comm == "bash"/ { @[ustack(2)] = count(); }'
Attaching 1 probe...
^C

@[
    0x7f58d8d174cc
    0x7f58d8c2b4f1
]: 1
@[
    0x7f58d8d11d1b
]: 1
@[
    0x7f58d8d174cc
    0x7f58d8c352bf
]: 1
@[
    0x7f58d8e54ec8
    0x7f58d8e4098a
]: 1
@[
    0x7f58d8e54ec8
    0x7f58d8e409cd
]: 3
@[
    0x7f58d8e54ec8
    0x7f58d8e3ec16
]: 34

# bpftrace -e 'kprobe:do_sys_open /comm == "bash"/ { @[ustack(perf,2)] = count(); }'
Attaching 1 probe...
^C

@[
        7f227070d4cc 0x7f227070d4cc ([unknown])
        7f227062b2bf 0x7f227062b2bf ([unknown])
]: 1
@[
        7f2270707d1b 0x7f2270707d1b ([unknown])
]: 1
@[
        7f227070d4cc 0x7f227070d4cc ([unknown])
        7f22706214f1 0x7f22706214f1 ([unknown])
]: 1
@[
        7f227084aec8 0x7f227084aec8 ([unknown])
        7f227083698a 0x7f227083698a ([unknown])
]: 1
@[
        7f227084aec8 0x7f227084aec8 ([unknown])
        7f22708369cd 0x7f22708369cd ([unknown])
]: 3
@[
        7f227084aec8 0x7f227084aec8 ([unknown])
        7f2270834c16 0x7f2270834c16 ([unknown])
]: 34

# bpftrace -e 'kprobe:do_sys_open /comm == "bash"/ { @[ustack(bpftrace,2)] = count(); }'
Attaching 1 probe...
^C

@[
    0x7fd18c1534cc
    0x7fd18c0712bf
]: 1
@[
    0x7fd18c290ec8
    0x7fd18c27c98a
]: 1
@[
    0x7fd18c14dd1b
]: 1
@[
    0x7fd18c1534cc
    0x7fd18c0674f1
]: 1
@[
    0x7fd18c290ec8
    0x7fd18c27c9cd
]: 3
@[
    0x7fd18c290ec8
    0x7fd18c27ac16
]: 34

#
```

## <code>cat()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
cat(filename)
```

<code>cat()</code>はファイルの出力を行う関数で，<code>printf()</code>風のフォーマットも利用可能．
```
# bpftrace -e 't:syscalls:sys_enter_execve { printf("%s ", str(args->filename)); cat("/proc/loadavg"); }'
Attaching 1 probe...
/usr/bin/ps 0.00 0.00 0.00 3/123 1380
/usr/bin/ls 0.00 0.00 0.00 2/123 1381
^C

# bpftrace -e 'tracepoint:syscalls:sys_enter_sendmsg { printf("%s => ", comm);
>     cat("/proc/%d/cmdline", pid); printf("\n") }'
Attaching 1 probe...
systemd-logind => /lib/systemd/systemd-logind
systemd-udevd => /lib/systemd/systemd-udevd
systemd-resolve => /lib/systemd/systemd-resolved
^C

#
```

## <code>signal()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
signal(u32 signal)
signal("SIG")
```

利用可能なプローブは以下の通り．[公式リファレンスガイド][ref-guide]では，カーネルバージョン5.3以降とある．
(未確認)
- kprobes/kretprobes
- uprobes/uretprobes
- USDT
- profile

[公式リファレンスガイド][ref-guide]の例は，「bash」が他のプログラムを実行しようとすると，
シグナルを送る．
```
# bpftrace  -e 'kprobe:__x64_sys_execve /comm == "bash"/ { signal(5); }' --unsafe
Attaching 1 probe...
^C

#
```
別のウィンドウ(bash)でlsするとbashにシグナルが送られる．

```
# ls
Trace/breakpoint trap (core dumped)
#
```

シグナルの指定は，番号でなく名前でもOK．
```
# bpftrace  -e 'kprobe:__x64_sys_execve /comm == "bash"/ { signal("KILL"); }' --unsafe
Attaching 1 probe...
^C

# bpftrace  -e 'kprobe:__x64_sys_execve /comm == "bash"/ { signal("SIGINT"); }' --unsafe
Attaching 1 probe...
^C

#
```

## <code>strncmp()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
strncmp(char *s1, char *s2, int length)
```


[公式リファレンスガイド][ref-guide]の例はそのままでは何も出力されないので，
bashを監視するように書き換えて実行してみた．当然ではあるが，
別窓のbashでコマンドを実行した場合の出力となっている．
```
# bpftrace -e 't:syscalls:sys_enter_* /strncmp("bas", comm, 3) == 0/ { @[comm, probe] = count() }'
Attaching 334 probes...
^C

@[bash, tracepoint:syscalls:sys_enter_rt_sigreturn]: 1
@[bash, tracepoint:syscalls:sys_enter_pipe]: 1
@[bash, tracepoint:syscalls:sys_enter_getpid]: 1
@[bash, tracepoint:syscalls:sys_enter_fcntl]: 1
@[bash, tracepoint:syscalls:sys_enter_execve]: 1
@[bash, tracepoint:syscalls:sys_enter_clone]: 1
@[bash, tracepoint:syscalls:sys_enter_wait4]: 2
@[bash, tracepoint:syscalls:sys_enter_select]: 2
@[bash, tracepoint:syscalls:sys_enter_setpgid]: 2
@[bash, tracepoint:syscalls:sys_enter_pselect6]: 3
@[bash, tracepoint:syscalls:sys_enter_close]: 4
@[bash, tracepoint:syscalls:sys_enter_read]: 4
@[bash, tracepoint:syscalls:sys_enter_write]: 4
@[bash, tracepoint:syscalls:sys_enter_ioctl]: 16
@[bash, tracepoint:syscalls:sys_enter_rt_sigprocmask]: 17
@[bash, tracepoint:syscalls:sys_enter_rt_sigaction]: 43

#
```

ちなみに，<code>strncmp()</code>の中を<code>comm</code>の代わりに
<code>probe</code>を使うと，そのままでも，<code>str()</code>を使っても，
エラーとなる．


## <code>override()</code>
|環境|動作|備考|
|:--|:--|:--|
|Ubuntu公式|○||
|CentOS公式|×|カーネルコンフィグの問題|
|Ubuntu最新|○||

文法:
```
override(u64 rc)
```
[公式リファレンスガイド][ref-guide]では，Kernel4.16以降とあるが未確認．

この機能は，カーネルがCONFIG_BPF_KPROBE_OVERRIDEを有効にしてコンパイルしてあること，
カーネル内の対象の関数がALLOW_ERROR_INJECTIONとタグ付けされていることが
条件．


[公式リファレンスガイド][ref-guide]の例はわかりずらいので，bashで
コマンドを実行すると拒否する例を以下に示す．

```
# bpftrace  -e '#include <linux/sched.h>
> kprobe:__x64_sys_execve /comm == "bash"/ { override(-EACCES); }' --unsafe
Attaching 1 probe...
^C

#
```
上のコマンドを実行している状態で，別窓のbashでなにかのコマンドを実行しようとすると，
以下のように，拒否される．
```
# ls
bash: /usr/bin/ls: Permission denied
#
```

```
[root@centos Variables]#  bpftrace  -e '#include <linux/sched.h>
> kprobe:__x64_sys_execve /comm == "bash"/ { override(-EACCES); }' --unsafe
stdin:2:44-63: ERROR: BPF_FUNC_override_return not available for your kernel version
kprobe:__x64_sys_execve /comm == "bash"/ { override(-EACCES); }
                                           ~~~~~~~~~~~~~~~~~~~
[root@centos Variables]#
```

## <code>buf()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|×|
|CentOS公式|○|
|Ubuntu最新|○|



文法:
```
buf(void *d [, int length])
```
[公式リファレンスガイド][ref-guide]の例は，pingの出力とbpftraceの出力が混じって読みづらいので，
bpftraceを実行し，別窓でpingを実行した例を示す．
```
# bpftrace -e 'tracepoint:syscalls:sys_enter_sendto { printf("Datagram bytes: %r\n", buf(args->buff, args->len)); }'
Attaching 1 probe...
Datagram bytes: \x08\x00\x91\xb4\x00\x00\x00\x01\xfe\x84\x1a_\x00\x00\x00\x00\x8c\x93\x02\x00\x00\x00\x00\x00\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f !"#$%&'()*+,-./01234567
^C

#
```

[公式リファレンスガイド][ref-guide]の説明にもあるように，バッファのサイズは環境変数「BPFTRACE_STRLEN」で変更可能(デフォルトは64バイト)．


## <code>sizeof()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|×|
|CentOS公式|○|
|Ubuntu最新|○|


文法:
```
sizeof(TYPE)
sizeof(EXPRESSION)
```

構造体のサイズを計る例([公式リファレンスガイド][ref-guide]の一番基本的な例)は以下のとおり．
```
# bpftrace -e 'struct Foo { int x; char c; } BEGIN { printf("%d\n", sizeof(struct Foo)); }'
Attaching 1 probe...
8
^C

#
```

構造体の特定のメンバの値を計る例は以下のとおり．
```
# bpftrace -e 'struct Foo { int x; char c; } BEGIN { printf("%d\n", sizeof(((struct Foo)0).c)); }'
Attaching 1 probe...
1
^C

# bpftrace -e 'struct Foo { int x; char c; } BEGIN { printf("%d\n", sizeof(((struct Foo)0).x)); }'
Attaching 1 probe...
4
^C

#
```

もちろん，上の値を合計しても，構造体全体のサイズより小さいのは，ワード境界の問題がその理由である．

次に，構造体メンバを参照するために通常は以下のようにヘッダのインクルードが
必要となる．
```
# bpftrace -e '#include <linux/sched.h>
> BEGIN { printf("%d\n", sizeof(struct task_struct)); }'
Attaching 1 probe...
9216
^C

#
```

[公式リファレンスガイド][ref-guide]にある下の例は，システムがbtfに対応していれば，
インクルード無しに実行可能なはずであるが，手元の環境ではbtfサポートの
構築に成功していないので動かない(エラーとなる)．
```
# bpftrace --btf -e 'BEGIN { printf("%d\n", sizeof(struct task_struct)); }'
Attaching 1 probe...
13120

#
```
```
# bpftrace --btf -e 'BEGIN { printf("%d\n", sizeof(struct task_struct)); }'
stdin:1:24-50: ERROR: Unknown identifier: 'struct task_struct'
BEGIN { printf("%d\n", sizeof(struct task_struct)); }
                       ~~~~~~~~~~~~~~~~~~~~~~~~~~
#
```

あと，通常のCの構造体以外の利用例は以下の通り．
```
# bpftrace -e 'BEGIN { printf("%d\n", sizeof(1 == 1)); }'
Attaching 1 probe...
8
^C

# bpftrace -e 'BEGIN { $x = 3; printf("%d\n", sizeof($x)); }'
Attaching 1 probe...
8
^C

# bpftrace -e 'BEGIN { printf("%d\n", sizeof("hello")); }'
Attaching 1 probe...
64
^C

#
```

## <code>print()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|×|
|CentOS公式|○|
|Ubuntu最新|○|


以下の例は，ノーマルな変数(mapではない)に対する例であるため，
Ubuntu公式の環境(bpftraceが0.9.4)では動かない． map変数のみに対応している．
map変数については，mapの章を参照していただきたい．

文法:
```
print(value)
```
<code>print()</code>はmap以外のいろいろなデータを出力可能な関数である．
以下は，[公式リファレンスガイド][ref-guide]の例にいくつか追加したもの．



```
# bpftrace -e 'BEGIN { $t = (1, "string"); print(123); print($t); print(sizeof($t)); print(comm); print(probe); }'
Attaching 1 probe...
123
(1, string)
72
bpftrace
BEGIN
^C

#
```

あと，mapのプリントと<code>print()</code>で変数をプリントアウトする場合の注意事項
(カーネル内での処理の方法やタイミングの話)は[公式リファレンスガイド][ref-guide]に
記載があるので，参照していただきたい．


## <code>strftime()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|×|
|CentOS公式|×|
|Ubuntu最新|○|

文法:
```
strftime(const char *format, int nsecs)
```

これは，時刻情報をCの<code>strftime(3)</code>でフォーマットして出力する関数．
[公式リファレンスガイド][ref-guide]には，注意事項としてフォーマットはeBPFのVMの中ではなく，
ユーザ空間で実行されるということが記載されている．

また，この機能が利用可能なカーネルやbpftraceのバージョンの記載が[公式リファレンスガイド][ref-guide]には
なく，手元の環境のうち，CentOSとUbuntuの公式版では動かない．
```
# bpftrace -e 'i:s:1 { printf("%s\n", strftime("%H:%M:%S", nsecs)); }'
stdin:1:24-32: ERROR: Unknown function: strftime
i:s:1 { printf("%s\n", strftime("%H:%M:%S", nsecs)); }
                       ~~~~~~~~
#
```

Ubuntu最新環境では動く．
```
# bpftrace -e 'i:s:1 { printf("%s\n", strftime("%H:%M:%S", nsecs)); }'
Attaching 1 probe...
02:00:11
02:00:12
^C

#
```


<!-- 参考文献リスト -->
[ref-guide]: <https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md>  "公式リファレンスガイド"



