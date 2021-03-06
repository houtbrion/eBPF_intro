# プローブ



## 一定時間間隔でイベントを発生させるプローブ : </code>interval</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
interval:ms:rate
interval:s:rate
interval:us:rate
interval:hz:rate
```
</code>interval</code>は<code>rate</code>間隔でイベントを発生させる．
時間の単位は2番目の<code>ms, s, us, hz</code>で示す．
なお，マルチCPUの環境では注意が必要で，[公式リファレンスの説明][ref-guide-interval]では以下のように記載されている．

```
> This fires on one CPU only, and can be used for generating per-interval output.
```

[公式リファレンス][ref-guide-interval]のサンプルスクリプトは，
0.9.4では動くが，0.10以上のバージョンでは，
バグありなので以下に
両方で動作する例を示す．下のスクリプトは1秒間隔で<code>sys_enter</code>が実行された回数を出力する．
```
# bpftrace  -e 'tracepoint:raw_syscalls:sys_enter { @syscalls = count(); } interval:s:1 { print(@syscalls); zero(@syscalls); }'
Attaching 2 probes...
@syscalls: 37

@syscalls: 67

^C

@syscalls: 41

#
```
[公式リファレンス][ref-guide-interval]のサンプルスクリプトは上の例の<code>zero()</code>の部分が
<code>clear()</code>となっており，これが実行できないというエラーメッセージが出力される．
(以下はCentOSの例)
```
[root@centos Probes]# bpftrace  -e 'tracepoint:raw_syscalls:sys_enter { @syscalls = count(); } interval:s:1 { print(@syscalls); clear(@syscalls); }'
Attaching 2 probes...
@syscalls: 46

Error looking up elem: -1
terminate called after throwing an instance of 'std::runtime_error'
  what():  Could not clear map with ident "@syscalls", err=-1
中止 (コアダンプ)
[root@centos Probes]#
```

## 周期的に性能データを取得するためのイベント用probe : <code>profile</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
profile:hz:rate
profile:s:rate
profile:ms:rate
profile:us:rate
```
</code>profile</code>は</code>interval</code>と同じく<code>rate</code>間隔でイベントを発生させる．
時間の単位は2番目の<code>ms, s, us, hz</code>で示す．
Linuxの<code>perf</code>コマンドと同じく，<code>perf_events</code>を利用して性能データを収集するために用いる．

[公式リファレンス][ref-guide]の例(以下に示す)は，インデック付きのmapを利用して，probeが発火した時点で動作していた
スレッド(実態はプロセス)のtidをキーとして動作の回数を<code>count()</code>で数えている．
なお，mapについては後に述べる．
```
# bpftrace -e 'profile:hz:99 { @[tid] = count(); }'
Attaching 1 probe...
^C

@[32586]: 98
@[0]: 579
#
```

## 組み込みイベント : <code>BEGIN</code>と<code>END</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

今まで何度も出てきているが，<code>BEGIN</code>はスクリプト実行開始時に1度だけ実行され，<code>END</code>は終了時に実行されるprobeである．

## カーネル内部の関数を監視するためのprobe : <code>kprobe</code>, <code>kretprobe</code>
|環境|動作|備考|
|:--|:--|:--|
|Ubuntu公式|▲|offset指定とBTFが動かない|
|CentOS公式|△|BTFが動かない|
|Ubuntu最新|△|BTFが動かない|

文法:
```
kprobe:function_name[+offset]
kretprobe:function_name
```
<code>kprobe</code>は監視対象の関数実行開始直後を捉え，<code>kretprobe</code>は実行終了直前を捉える．
[公式リファレンス][ref-guide]が示している以下の例は，<code>nanosleep()</code>関数をなにかの
プロセスが実行した場合に，それを捉えて<code>tid</code>を出力するものである．
```
# bpftrace -e 'kprobe:do_nanosleep { printf("sleep by %d\n", tid); }'
Attaching 1 probe...
sleep by 1396
sleep by 3669
^C
#
```
ubuntu公式環境はoffset指定ができない．
```
# bpftrace -e 'kprobe:do_sys_open+9 { printf("in here\n"); }'
Attaching 1 probe...
Can't check if kprobe is in proper place (compiled without (k|u)probe offset support): /usr/lib/debug/boot/vmlinux-5.4.0-42-generic:do_sys_open+9
#
```
カーネルは古いが，CentOSでは動作する．
```
[root@centos Probes]# bpftrace -e 'kprobe:do_sys_open+9 { printf("in here\n"); }'
Attaching 1 probe...
in here
in here
in here
in here
in here
in here
in here
in here
in here
in here
in here
^C

[root@centos Probes]#
```

実際に利用するためには，カーネルのデバッグシンボルが必要になるため，それをインストールするが，
まずリポジトリの定義を修正・確認する．
```
# cd /etc/apt/sources.list.d
# cat ddebs.list
deb http://ddebs.ubuntu.com focal main restricted universe multiverse
deb http://ddebs.ubuntu.com focal-updates main restricted universe multiverse
#deb http://ddebs.ubuntu.com $(lsb_release -cs)-proposed main restricted universe multiverse

#
```
次に，キーリングのインストール．
```
apt install ubuntu-dbgsym-keyring
```

最後に，デバッグシンボルのインストール．
```
# apt-get update
(中略)
# apt install linux-image-$(uname -r)-dbgsym
Reading package lists... Done
Building dependency tree
Reading state information... Done
The following additional packages will be installed:
  linux-image-unsigned-5.4.0-42-generic-dbgsym
The following NEW packages will be installed:
  linux-image-5.4.0-42-generic-dbgsym
  linux-image-unsigned-5.4.0-42-generic-dbgsym
0 upgraded, 2 newly installed, 0 to remove and 0 not upgraded.
Need to get 963 MB of archives.
After this operation, 6,880 MB of additional disk space will be used.
Do you want to continue? [Y/n]y
Get:1 http://ddebs.ubuntu.com focal-updates/main amd64 linux-image-unsigned-5.4.0-42-generic-dbgsym amd64 5.4.0-42.46 [963 MB]
Get:2 http://ddebs.ubuntu.com focal-updates/main amd64 linux-image-5.4.0-42-generic-dbgsym amd64 5.4.0-42.46 [15.0 kB]
Fetched 963 MB in 1h 23min 20s (193 kB/s)
Selecting previously unselected package linux-image-unsigned-5.4.0-42-generic-dbgsym.
(Reading database ... 151517 files and directories currently installed.)
Preparing to unpack .../linux-image-unsigned-5.4.0-42-generic-dbgsym_5.4.0-42.46_amd64.ddeb ...
Unpacking linux-image-unsigned-5.4.0-42-generic-dbgsym (5.4.0-42.46) ...
Selecting previously unselected package linux-image-5.4.0-42-generic-dbgsym.
Preparing to unpack .../linux-image-5.4.0-42-generic-dbgsym_5.4.0-42.46_amd64.ddeb ...
Unpacking linux-image-5.4.0-42-generic-dbgsym (5.4.0-42.46) ...
Setting up linux-image-unsigned-5.4.0-42-generic-dbgsym (5.4.0-42.46) ...
Setting up linux-image-5.4.0-42-generic-dbgsym (5.4.0-42.46) ...
#
```

文法のところで示されているように，監視対象を関数だけでなく，関数の入り口からのオフセットで
指定することができる．[公式リファレンス][ref-guide]の例を手元の環境で実行した例を以下に示す．

```
# gdb -q /usr/lib/debug/boot/vmlinux-`uname -r` --ex 'disassemble do_sys_open'
Reading symbols from /usr/lib/debug/boot/vmlinux-5.4.0-42-generic...
Dump of assembler code for function do_sys_open:
   0xffffffff812d9740 <+0>:     callq  0xffffffff81c01950 <__fentry__>
   0xffffffff812d9745 <+5>:     push   %rbp
   0xffffffff812d9746 <+6>:     mov    %rsp,%rbp
   0xffffffff812d9749 <+9>:     push   %r15
   0xffffffff812d974b <+11>:    push   %r14
   0xffffffff812d974d <+13>:    push   %r13
   0xffffffff812d974f <+15>:    mov    %edx,%r13d
   0xffffffff812d9752 <+18>:    push   %r12
   0xffffffff812d9754 <+20>:    mov    %edi,%r12d
   0xffffffff812d9757 <+23>:    mov    %rsi,%rdi
   0xffffffff812d975a <+26>:    push   %rbx
   0xffffffff812d975b <+27>:    mov    %ecx,%ebx
   0xffffffff812d975d <+29>:    sub    $0x38,%rsp
   0xffffffff812d9761 <+33>:    mov    %gs:0x28,%rax
   0xffffffff812d976a <+42>:    mov    %rax,-0x30(%rbp)
   0xffffffff812d976e <+46>:    xor    %eax,%eax
   0xffffffff812d9770 <+48>:    mov    %edx,%eax
   0xffffffff812d9772 <+50>:    and    $0x3,%eax
   0xffffffff812d9775 <+53>:    movzbl -0x7dc91c25(%rax),%edx
   0xffffffff812d977c <+60>:    mov    %ecx,%eax
   0xffffffff812d977e <+62>:    mov    $0x0,%ecx
--Type <RET> for more, q to quit, c to continue without paging--q
Quit
(gdb) quit
#
```

[公式リファレンス][ref-guide]でも紹介されているが，offセットとして指定できる値は上の出力結果の一部(以下に引用)を
参照することでわかる．下に引用した出力結果のうち，<code><+n></code>の<code>n</code>の値である．
```
   0xffffffff812e9d40 <+0>:     callq  0xffffffff81c01940 <__fentry__>
   0xffffffff812e9d45 <+5>:     push   %rbp
   0xffffffff812e9d46 <+6>:     mov    %rsp,%rbp
   0xffffffff812e9d49 <+9>:     sub    $0x20,%rsp
   0xffffffff812e9d4d <+13>:    mov    %gs:0x28,%rax
   0xffffffff812e9d56 <+22>:    mov    %rax,-0x8(%rbp)
```
上の結果から，指定可能な値は<code>0, 5, 6, 9, 13, 22</code>であるので，<code>1</code>
を指定すると以下のような結果(エラー)となる．
```
# bpftrace -e 'kprobe:do_sys_open+1 { printf("in here\n"); }'
Attaching 1 probe...
Could not add kprobe into middle of instruction: /usr/lib/debug/boot/vmlinux-5.7.0-rc7+:do_sys_open+1
#
```

利用上の注意:
- bpftraceを<code>ALLOW_UNSAFE_PROBE</code>付きでコンパイルした場合，bpftrace自身の上のチェックをパイパスする引数<code>--unsafe</code>
を利用することができるが，カーネルはチェックを行っている．
- bpftraceが参照するvmlinuxのパスは環境変数<code>BPFTRACE_VMLINUX</code>で上書きすることができる．

### カーネル内関数の引数/返り値を参照する方法
カーネル内関数呼び出し時の引数を参照する場合，アクション部分で各引数を以下のように，<code>arg<n></code>で指定する．
```
arg0, arg1, ..., argN
```
また，監視対象関数の返り値を参照する場合はkretprobeを用い，アクションの内部で<code>retval</code>というキーワードで返り値を参照することができる．

[公式リファレンスガイド][ref-guide]のサンプルでは，<code>do_sys_open()</code>を
利用して，その第2引数と第3引数を参照しているが，手元の環境(カーネルバージョン5.6.18)では以下のインクルードファイルに
定義が存在する．
```
# ls /usr/src/linux-headers-5.4.0-42-generic/include/linux/syscalls.h
/usr/src/linux-headers-5.4.0-42-generic/include/linux/syscalls.h
#
```
このインクルードファイルの中で<code>do_sys_open()</code>は以下のように宣言されており，
第2引数がオープンするファイル名(型は文字列)，第3引数がフラグ(型はint)となっている．
```
extern long do_sys_open(int dfd, const char __user *filename, int flags,
                        umode_t mode);

extern long do_sys_open(int dfd, const char __user *filename, int flags,
                        umode_t mode);
```

[公式リファレンスガイド][ref-guide]のサンプルを手元の環境で実行した例を以下に示すが，<code>do_sys_open()</code>の第2引数のファイル名を文字列として出力するサンプルである．
```
# bpftrace -e 'kprobe:do_sys_open { printf("opening: %s\n", str(arg1)); }'
Attaching 1 probe...
opening: /sys/fs/cgroup/unified/system.slice/systemd-udevd.service/cgrou
opening: /sys/fs/cgroup/unified/system.slice/systemd-udevd.service/cgrou
opening: /proc/interrupts
opening: /proc/stat
opening: /proc/irq/11/smp_affinity
opening: /proc/irq/0/smp_affinity
opening: /proc/irq/1/smp_affinity
opening: /proc/irq/8/smp_affinity
opening: /proc/irq/12/smp_affinity
(中略)
opening: /var/run/postgresql/12-main.pg_stat_tmp/global.stat
opening: /var/run/postgresql/12-main.pg_stat_tmp/db_16385.stat
opening: /var/run/postgresql/12-main.pg_stat_tmp/db_0.stat
opening: global/1262
opening: base/16385/1259
^C

#
```

また第3引数を参照するサンプルは以下のような動作となる．
```
# bpftrace -e 'kprobe:do_sys_open { printf("open flags: %d\n", arg2); }'
Attaching 1 probe...
open flags: 32768
open flags: 32768
open flags: 32768
open flags: 32768
open flags: 32768
open flags: 32768
open flags: 32768
open flags: 32768
open flags: 32768
^C

#
```
<code>do_sys_open()</code>の返り値を参照する[公式リファレンスガイド][ref-guide]の
サンプルも以下のような動作をする．
```
# bpftrace -e 'kretprobe:do_sys_open { printf("returned: %ld\n", retval); }'
Attaching 1 probe...
returned: 6
returned: 6
returned: 6
returned: 6
returned: 6
returned: 6
returned: 6
returned: 6
returned: 6
^C

#
```

引数が構造体の場合は，その型を定義しているインクルードファイルを以下の例のように，
読み込む必要がある．
```
#include <linux/path.h>
#include <linux/dcache.h>

kprobe:vfs_open
{
	printf("open path: %s\n", str(((struct path *)arg0)->dentry->d_name.name));
}
```

上のサンプルをファイル([path.bt][path.bt])に保存し，実行すると以下のような
動作をする．
```
# bpftrace path.bt
Attaching 1 probe...
open path: interrupts
open path: stat
open path: smp_affinity
open path: smp_affinity
open path: smp_affinity
open path: smp_affinity
open path: smp_affinity
open path: smp_affinity
open path: smp_affinity
^C

#
```

上のサンプル([path.bt][path.bt])では型定義のインクルードファイルを読み込んでいたが，
利用しているOS(カーネル)とbpftraceがBTF (BPF Type Format)を
サポートしている場合，インクルードしなくても動作する．
```
# bpftrace -e 'kprobe:vfs_open { printf("open path: %s\n", 
                                 str(((struct path *)arg0)->dentry->d_name.name)); }'
```

ただし，コマンドラインのところでも述べたが，現状，bpftraceのビルドの仕組みのbugのため，
BTFが動作しなくなっている．

## ユーザアプリ内部を監視するためのprobe : <code>uprobe</code>, <code>uretprobe</code>
|環境|動作|備考|
|:--|:--|:--|
|Ubuntu公式|▲|offset指定とBTFが動かない|
|CentOS公式|△|BTFが動かない|
|Ubuntu最新|△|BTFが動かない|

文法:
```
uprobe:library_name:function_name[+offset]
uprobe:library_name:address
uretprobe:library_name:function_name
```

uprobe/uretprobeはユーザアプリの監視を実現することができるが，[公式リファレンスガイド][ref-guide]は共有ライブラリの監視しか
例示していないため，非常にシンプルなユーザが作成した[アプリ][target-sample.c](以下に引用)を監視する場合で説明する．
このアプリは<code>main()</code>から非負整数の引数を与えて，<code>func()</code>を呼び出すと，<code>func()</code>は
引数をインクリメントして返す．
```
bash$ cat target-sample.c
#include <stdio.h>
#include <unistd.h>

unsigned int func(unsigned int counter) {
        counter++;
        return counter;
}

void main(){
        unsigned int counter=0;
        pid_t pid=getpid();
        printf("pid = %d\n",pid);
        while(1) {
                unsigned int ret=func(counter);
                printf("func=%d\n",ret);
                counter++;
                sleep(1);
        }
}
bash$
```
[上のサンプルプログラム][target-sample.c]をuprobe/uretprobeを使って監視するbpftrace用[スクリプト][uprobe-app.bt]を以下に示す．なお，バイナリファイルの存在するディレクトリのパス名は適宜書き換える必要がある．まず，「<code>uprobe</code>と
<code>uretprobe</code>」からの
行で，監視対象アプリのバイナリファイル名と監視対象の関数<code>func</code>を指定している．
この指定でを行うことで，指定したアプリ(target-sample)内で<code>func()</code>が呼び出された場合
(<code>uprobe</code>の部分)と<code>func()</code>の実行が終わった場合(<code>uretprobe</code>の部分)
にそれぞれのアクションが実行される．
```
bash$ cat uprobe-app.bt
uprobe:バイナリの存在するディレクトリ/target-sample:func
{
        printf("%lu, func, start, pid = %d, tid = %d, arg    = %d \n",nsecs, pid, tid, arg0);
}
uretprobe:バイナリの存在するディレクトリ/target-sample:func
{
        printf("%lu, func, end,   pid = %d, tid = %d, retval = %d\n",nsecs, pid, tid, retval);
}
bash$
```

### 引数の参照
引数の参照はkprobeの場合と基本的には同じであるが，上の[スクリプト][uprobe-app.bt]の
場合で説明する．以下は上の[スクリプト][uprobe-app.bt]の<code>uprobe</code>部分である．
```
uprobe:バイナリの存在するディレクトリ/target-sample:func
{
        printf("%lu, func, start, pid = %d, tid = %d, arg    = %d \n",nsecs, pid, tid, arg0);
}
```
この場合，<code>target-sample</code>が関数<code>func()</code>の実行を開始すると，
一時的にアプリの実行が止まり，eBPFのVMが呼び出されて，関数実行開始時のデータがユーザ空間に
通知され，アプリの実行が再開される．bpftraceはeBPFのVMから通知されたデータ(上の例では，<code>nsecs, pid, tid, arg0</code>)を
<code>printf()</code>で指定されたフォーマットで整形して出力する．

なお，<code>nsecs, pid, tid, arg0</code>の意味を以下の表に示す．
|変数名|意味|
|:--|:--|
|nsecs|カーネル内の時刻(uptime相当)|
|pid|プロセスID|
|tid|スレッドID|
|arg0|第1引数|

<code>arg<n></code>はkprobeの場合と同じで<code>n</code>を変更することで，参照する
引数を変更することができる．<code>nsecs</code>や<code>pid</code>, <code>tid</code>は
組み込み変数のところで説明するが，<code>nsecs</code>で取得可能な値は，eBPF開発環境[bcc][bcc]の
[bpf_ktime_get_ns()][bpf_ktime_get_ns]出力となる．

### 返り値の参照
返り値の参照方法もkretprobeの場合と同じ．
上の[スクリプト][uprobe-app.bt]のuretprobe部分を下に引用する．
```
uretprobe:バイナリの存在するディレクトリ/target-sample:func
{
        printf("%lu, func, end,   pid = %d, tid = %d, retval = %d\n",nsecs, pid, tid, retval);
}
```
引数の参照の場合の<code>arg0</code>の代わりに，監視対象の関数の返り値を示す組み込み変数<code>retval</code>を
用いている．

### 実行例
[上のサンプルプログラム][target-sample.c]をコンパイルして，実行すると以下のような出力となる．
```
bash$ ./target-sample
pid = 7798
func=1
func=2
func=3
func=4
(以下略)
```
アプリが動作している状態で，bpftraceに[uprobe-app.bt][uprobe-app.bt]を与えて動作させると以下のような出力となる．
```
# bpftrace uprobe-app-arg.bt
Attaching 2 probes...
77986894662818, func, start, pid = 7798, tid = 7798, arg    = 0
77986894717242, func, end,   pid = 7798, tid = 7798, retval = 1
77987895094687, func, start, pid = 7798, tid = 7798, arg    = 1
77987895126300, func, end,   pid = 7798, tid = 7798, retval = 2
77988895822136, func, start, pid = 7798, tid = 7798, arg    = 2
77988895854929, func, end,   pid = 7798, tid = 7798, retval = 3
77989914189393, func, start, pid = 7798, tid = 7798, arg    = 3
77989914250079, func, end,   pid = 7798, tid = 7798, retval = 4
77990914895468, func, start, pid = 7798, tid = 7798, arg    = 4
77990914964849, func, end,   pid = 7798, tid = 7798, retval = 5
77991933254048, func, start, pid = 7798, tid = 7798, arg    = 5
77991933314290, func, end,   pid = 7798, tid = 7798, retval = 6
77992933970883, func, start, pid = 7798, tid = 7798, arg    = 6
77992934003075, func, end,   pid = 7798, tid = 7798, retval = 7
77993934502338, func, start, pid = 7798, tid = 7798, arg    = 7
77993934577558, func, end,   pid = 7798, tid = 7798, retval = 8
^C

#
```

### offsetの指定


kprobeの場合と同じく，uprobeでも実行開始から実行が少し進んだ場所を監視することができる．

まず，監視対象のアプリをディスアセンブルして，その出力から監視対象の関数を探す．
```
bash$ objdump -d ./target-sample

./target-sample:     file format elf64-x86-64


Disassembly of section .init:

(中略)
0000000000001189 <func>:
    1189:       f3 0f 1e fa             endbr64
    118d:       55                      push   %rbp
    118e:       48 89 e5                mov    %rsp,%rbp
    1191:       89 7d fc                mov    %edi,-0x4(%rbp)
    1194:       83 45 fc 01             addl   $0x1,-0x4(%rbp)
    1198:       8b 45 fc                mov    -0x4(%rbp),%eax
    119b:       5d                      pop    %rbp
    119c:       c3                      retq
(以下略)

```
この出力結果から，最初の<code>push</code>の部分のインデックス値は<code>0x118d-0x1189</code>から
4であることがわかる．この値を用いてインデックスを指定すると以下のような出力となる．
```
# bpftrace -e 'uprobe:./target-sample:func+4
{
        printf("%lu, func, start, pid = %d, tid = %d, arg    = %d \n",nsecs, pid
, tid, arg0);
}'
Attaching 1 probe...
100460145339169, func, start, pid = 9990, tid = 9990, arg    = 0
100461146471469, func, start, pid = 9990, tid = 9990, arg    = 1
100462160313365, func, start, pid = 9990, tid = 9990, arg    = 2
^C

#
```
上の出力は基本的には元のサンプルプログラムと同じ．この機能が有用なのは，if文等で分岐しているところを監視するような
場合である．

次に，インデックスに命令のアラインメントを無視した値を指定した場合は，kprobeと同じく
アラインメントのエラーが出力される．
```
# bpftrace -e 'uprobe:./target-sample:func+1
{
        printf("%lu, func, start, pid = %d, tid = %d, arg    = %d \n",nsecs, pid, tid, arg0);
}'
Attaching 1 probe...
Could not add uprobe into middle of instruction: ./target-sample:func+1
```
uprobeの場合も，bpftraceが<code>ALLOW_UNSAFE_PROBE</code>を有効にしてコンパイルした場合は，
アラインメントのチェックを無効にするオプション<code>--unsafe</code>が利用できる．

Ubuntu公式版は動作しない．
```
# bpftrace -e 'uprobe:./target-sample:func+4
> {
>         printf("%lu, func, start, pid = %d, tid = %d, arg    = %d \n",nsecs, pid
> , tid, arg0);
> }'
Attaching 1 probe...
Can't check if uprobe is in proper place (compiled without (k|u)probe offset support): ./target-sample:func+4
#
```




### 監視対象の関数名の代わりに，アドレスを用いる．
インデックス指定似た機能で，監視対象を関数名の代わりにアドレスだけで指定する方法もある．
先程までと同じアプリでは，アドレス<code>0x1189</code>から関数<code>func()</code>が
始まっている．これを利用して<code>0x1189</code>をアドレスとして指定して，
<code>func()</code>を監視するアクションを実行することもできる．
```
# bpftrace -e 'uprobe:./target-sample:0x1189
> {
>         printf("%lu, func, start, pid = %d, tid = %d, arg    = %d \n",nsecs, pid, tid, arg0);
> }'
Attaching 1 probe...
101233346227784, func, start, pid = 10051, tid = 10051, arg    = 0
101234353278141, func, start, pid = 10051, tid = 10051, arg    = 1
101235358756380, func, start, pid = 10051, tid = 10051, arg    = 2
101236359159684, func, start, pid = 10051, tid = 10051, arg    = 3
^C

#
```

### 共有ライブリの監視
上の仕組みを使って，共有ライブリの監視する方法については，[公式リファレンスガイド][ref-guide]に
記載されているので，そちらを参照のこと．基本的な方法は上のアプリを監視する場合と同じなので，
読んで理解するのに困難はないはずである．

## <code>tracepoint</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

Ubuntu公式の環境では，約1600個のトレースポイントが存在する．
```
# uname -r
5.4.0-42-generic
# /usr/sbin/tplist-bpfcc |wc -l
1601
#
```
上記の例で利用したコマンド「<code>tplist-bpfcc</code>」はbccをそのままインストールすると，
「<code>tplist</code>」という名前でインストールされる．そのため，CentOS公式版や
Ubuntu最新版は，「<code>tplist</code>」利用する必要がある．
このコマンドは，bccが提供する各種ツール類の1つで，Ubuntu20.04のパッケージでは，
わざわざ各種ツールの名前の末尾に「<code>-bpfcc</code>」を付けているが，このドキュメントでは，「<code>-bpfcc</code>」無しで記載する．

[公式リファレンスガイド][ref-guide]のサンプルでは，<code>block_rq_insert</code>を紹介している(下に引用)．
```
# bpftrace -e 'tracepoint:block:block_rq_insert { printf("block I/O created by %d\n", tid); }'
Attaching 1 probe...
block I/O created by 28922
block I/O created by 3949
block I/O created by 883
block I/O created by 28941
block I/O created by 28941
block I/O created by 28941
[...]
```
上の例のように，<code>tracepoint</code>を使うためには，<code>tracepoint</code>のタイプ(上の例では<code>block</code>)を
指定する必要があるため，以下のように<code>tplist</code>コマンドで取得する．

```
# tplist |grep block_rq_insert
block:block_rq_insert
#
```
同じく，<code>tplist</code>の<code>-v</code>オプションで，参照することができる引数を知ることができる．
```
# tplist -v syscalls:sys_enter_openat
syscalls:sys_enter_openat
    int __syscall_nr;
    int dfd;
    const char * filename;
    int flags;
    umode_t mode;
#
```
上の情報から，参照可能な引数のうち<code>filename</code>を出力する例が[リファレンスガイド][ref-guide]で紹介されている．

```
# bpftrace -e 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\n", comm, str(args->filename)); }'
Attaching 1 probe...
systemd /proc/615/cgroup
systemd-timesyn /var/lib/systemd/timesync/clock
systemd-timesyn /run/systemd/timesync/synchronized
irqbalance /proc/interrupts
irqbalance /proc/stat
irqbalance /proc/irq/11/smp_affinity
irqbalance /proc/irq/0/smp_affinity
^C

#
```

[リファレンスガイド][ref-guide]と異なり，<code>tplist</code>コマンドを活用する例を説明したが，
[リファレンスガイド][ref-guide]と同じく，<code>/sys</code>ファイルシステムを利用する例も
紹介しておく．

[リファレンスガイド][ref-guide]はなんの説明もなく，いきなり<code>/sys</code>ファイルシステムを
参照しているが，<code>/sys/kernel/debug/tracing/events</code>に<code>tracepoint</code>の
タイプがディレクトリになっている．

```
# cd /sys/kernel/debug/tracing/events
# ls
alarmtimer    gpio          msr             signal
block         header_event  napi            skb
bpf_test_run  header_page   neigh           smbus
bridge        huge_memory   net             sock
btrfs         hwmon         nmi             spi
cgroup        hyperv        oom             swiotlb
clk           i2c           page_isolation  sync_trace
cma           initcall      pagemap         syscalls
compaction    intel_iommu   page_pool       task
cpuhp         iocost        percpu          tcp
cros_ec       iommu         power           thermal
devfreq       irq           printk          thermal_power_allocator
devlink       irq_matrix    qdisc           timer
dma_fence     irq_vectors   random          tlb
drm           jbd2          ras             udp
enable        kmem          raw_syscalls    vmscan
exceptions    kvm           rcu             vsyscall
ext4          kvmmmu        regmap          wbt
fib           libata        regulator       workqueue
fib6          mce           resctrl         writeback
filelock      mdio          rpm             x86_fpu
filemap       migrate       rseq            xdp
fs            mmc           rtc             xen
fs_dax        module        sched           xhci-hcd
ftrace        mpx           scsi
#
```

最初の例では，<code>block</code>ディレクトリを<code>ls</code>すると，
トレースポイントの一覧を得ることができる．
```
# ls block
block_bio_backmerge   block_bio_remap     block_rq_insert   block_split
block_bio_bounce      block_dirty_buffer  block_rq_issue    block_touch_buffer
block_bio_complete    block_getrq         block_rq_remap    block_unplug
block_bio_frontmerge  block_plug          block_rq_requeue  enable
block_bio_queue       block_rq_complete   block_sleeprq     filter
#
```

最後に，利用するトレースポイントで参照できる引数の一覧は，監視対象の
トレースポイントディレクトリ内の<code>format</code>ファイルを見ると
引数が読み取れる．

```
# cd block/block_rq_insert
# cat format
name: block_rq_insert
ID: 1133
format:
        field:unsigned short common_type;       offset:0;       size:2; signed:0;
        field:unsigned char common_flags;       offset:2;       size:1; signed:0;
        field:unsigned char common_preempt_count;       offset:3;       size:1; signed:0;
        field:int common_pid;   offset:4;       size:4; signed:1;

        field:dev_t dev;        offset:8;       size:4; signed:0;
        field:sector_t sector;  offset:16;      size:8; signed:0;
        field:unsigned int nr_sector;   offset:24;      size:4; signed:0;
        field:unsigned int bytes;       offset:28;      size:4; signed:0;
        field:char rwbs[8];     offset:32;      size:8; signed:1;
        field:char comm[16];    offset:40;      size:16;        signed:1;
        field:__data_loc char[] cmd;    offset:56;      size:4; signed:1;

print fmt: "%d,%d %s %u (%s) %llu + %u [%s]", ((unsigned int) ((REC->dev) >> 20)), ((unsigned int) ((REC->dev) & ((1U << 20) - 1))), REC->rwbs, REC->bytes, __get_str(cmd), (unsigned long long)REC->sector, REC->nr_sector, REC->comm
#
```

## USDT : User Statically-Defined Tracing
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

この機能は元々DTraceのものであるため，そのためのパッケージをインストールする．
```
# apt install systemtap-sdt-dev
Reading package lists... Done
Building dependency tree
Reading state information... Done
The following NEW packages will be installed:
  systemtap-sdt-dev
0 upgraded, 1 newly installed, 0 to remove and 0 not upgraded.
Need to get 16.3 kB of archives.
After this operation, 75.8 kB of additional disk space will be used.
Get:1 http://archive.ubuntu.com/ubuntu focal/universe amd64 systemtap-sdt-dev amd64 4.2-3 [16.3 kB]
Fetched 16.3 kB in 1s (20.9 kB/s)
Selecting previously unselected package systemtap-sdt-dev.
(Reading database ... 158121 files and directories currently installed.)
Preparing to unpack .../systemtap-sdt-dev_4.2-3_amd64.deb ...
Unpacking systemtap-sdt-dev (4.2-3) ...
Setting up systemtap-sdt-dev (4.2-3) ...
Processing triggers for man-db (2.9.1-1) ...
#
```

USDTは，ユーザ空間で動作するアプリの中(ソース)にDTrace由来のトレース用のコードを埋め込み，それを
eBPFで監視するための物であるにもかかわらず，
[公式リファレンスガイド][ref-guide]は監視される側のコードは一切示さずに，bpftrace側の例だけを
示している．これでは，DTraceやbccのUSDTに詳しくない人は全く理解が困難であるため，
監視される側のアプリの
[ソースコード][usdt_sample.c]を添付し，それに基づいて説明する．

該当の[ソースコード][usdt_sample.c]には以下のような行が存在する．
```
// probeの引数として2つあげるので，DTRACE_PROBE2を使う
DTRACE_PROBE2(foo, bar, counter, "hello");
```
上のコードは2つ情報(引数)が存在するプローブであるため，「<code>DTRACE_PROBE2()</code>」を使う．
<code>DTRACE_PROBE2()</code>の引数のうち，最初の<code>foo</code>がプローブのプロバイダ名で，
<code>bar</code>がプローブ名である．残りの2つは，引数として取り扱うための変数(もしくは定数)である．

この[ソースコード(usdt_sample.c)][usdt_sample.c]をコンパイルして実行すると，<code>DTRACE_PROBE2()</code>
で与えた<code>counter</code>の値が出力される．
```
bash$ gcc -o usdt_sample usdt_sample.c
bash$ ./usdt_sample
pid = 1826
counter=1
counter=2
^C
bash$
```

これを監視するためのbpftraceは以下のようなコマンドとなる．
```
# bpftrace -e 'usdt:./usdt_sample:foo:bar { printf("counter=%d, string=%s\n", arg0, str(arg1)); }'
Attaching 1 probe...
counter=1, string=hello
counter=2, string=hello
^C

#
```
最初の<code>usdt</code>は「probe」が<code>usdt</code>であることを表し，その次がバイナリファイルのパス名，
3つめがUSDTのプローブで指定したプロバイダ名で，最後がプローブ名となる．

<code>DTRACE_PROBE2()</code>で指定した参照可能な引数は<code>foo</code>と<code>"hello"</code>であるが，
<code>foo</code>が第一引数かつ整数で，<code>"hello"</code>が第二引数の文字列となるため，
bpftraceのスクリプトでは，それぞれを<code>arg0</code>と<code>str(arg1)</code>として
取り扱っている．

なお，[公式リファレンスガイド][ref-guide]の例にもあるように，「<code>arg0</code>や<code>arg1</code>は
bpftraceのフィルタにも用いることができ，引数の値が特定の場合だけアクションが実行されるような仕組みも
実現できる．

先程のアプリの場合，<code>counter</code>の値が5より大きい場合だけ，アクションが実行される
ような仕掛けは，以下のアプリとbpftraceの実行例を参照してもらえれば，すぐにわかる．

#### アプリの実行例
```
bash$ ./usdt_sample
pid = 1874
counter=1
counter=2
counter=3
counter=4
counter=5
counter=6
counter=7
^C
bash$
```

#### bpftraceの実行例

```
# bpftrace -e 'usdt:./usdt_sample:foo:bar /arg0 > 5 / { printf("counter=%d, string=%s\n", arg0, str(arg1)); }'
Attaching 1 probe...
counter=6, string=hello
counter=7, string=hello
^C

#
```


### USDTセマフォ
これについては，[公式リファレンスガイド][ref-guide]を参照していただきたい．



## Pre-defined (Software|Hardware) イベント

[公式リファレンスガイド][ref-guide]にもあるように，<code>perf</code>で使われてきた
パフォーマンスデータ取得用のイベントで，<code>man perf_event_open</code>に
各イベントの説明が記載されている．


### ソフトウェアイベント
|環境|動作|備考|
|:--|:--|:--|
|Ubuntu公式|○||
|CentOS公式|△|countの省略が動かない|
|Ubuntu最新|△|countの省略が動かない|

文法:
```
software:event_name:count
software:event_name:
```

|bpftraceのキーワード|perf_event_open(2)でtype=PERF_TYPE_SOFTWAREの場合のconfig値|
|:---|:---|
|<code>cpu-clock</code> or <code>cpu</code>|PERF_COUNT_SW_CPU_CLOCK|
|<code>task-clock</code>|PERF_COUNT_SW_TASK_CLOCK|
|<code>page-faults</code> or <code>faults</code>|PERF_COUNT_SW_PAGE_FAULTS|
|<code>context-switches</code> or <code>cs</code>|PERF_COUNT_SW_CONTEXT_SWITCHES|
|<code>cpu-migrations</code>|PERF_COUNT_SW_CPU_MIGRATIONS|
|<code>minor-faults</code>|PERF_COUNT_SW_PAGE_FAULTS_MIN|
|<code>major-faults</code>|PERF_COUNT_SW_PAGE_FAULTS_MAJ|
|<code>alignment-faults</code>|PERF_COUNT_SW_ALIGNMENT_FAULTS|
|<code>emulation-faults</code>|PERF_COUNT_SW_EMULATION_FAULTS|
|<code>dummy</code>|PERF_COUNT_SW_DUMMY|
|<code>bpf-output</code>||

下の例は，ページフォルトを数えて100回目の時に，アクションが実行されるため，
どのプログラムが100回目のページフォルトの時に動いていたかが，カウントされる．
手元の環境では，それほど多くのプロセスが動作していないため，postgresしか
対応しなかった．
```
# bpftrace -e 'software:page-faults:100 { @[comm] = count(); }'
Attaching 1 probe...
^C

@[postgres]: 33

#
```

#### countの省略
[公式リファレンスガイド][ref-guide]では，<code>count</code>を省略すると，デフォルト値が
利用されるとある．
0.9.4の環境では動くが，新しい
環境では，<code>count</code>を省略する記法はエラーとなる．
```
# bpftrace -e 'software:faults: { @[comm] = count(); }'
stdin:1:1-17: ERROR: Failed to parse '': stoi
software:faults: { @[comm] = count(); }
~~~~~~~~~~~~~~~~
# bpftrace -e 'software:faults:1 { @[comm] = count(); }'
Attaching 1 probe...
^C

#
```

### ハードウェアイベント
|環境|動作|
|:--|:--|
|Ubuntu公式|×|
|CentOS公式|×|
|Ubuntu最新|×|



[公式リファレンスガイド][ref-guide]とperf_event_open(2)のmanを見比べると，以下の
項目が利用可能であることがわかる．
|bpftraceのキーワード|perf_event_open(2)でtype=PERF_TYPE_HARDWAREの場合のconfig値|
|:---|:---|
|<code>cpu-cycles</code> or <code>cycles</code>|PERF_COUNT_HW_CPU_CYCLES|
|<code>instructions</code>|PERF_COUNT_HW_INSTRUCTIONS|
|<code>cache-references</code>|PERF_COUNT_HW_CACHE_REFERENCES|
|<code>cache-misses</code>|PERF_COUNT_HW_CACHE_MISSES|
|<code>branch-instructions</code> or <code>branches</code>|PERF_COUNT_HW_BRANCH_INSTRUCTIONS|
|<code>branch-misses</code>|PERF_COUNT_HW_BRANCH_MISSES|
|<code>bus-cycles</code>|PERF_COUNT_HW_BUS_CYCLES|
|<code>frontend-stalls</code>|PERF_COUNT_HW_STALLED_CYCLES_FRONTEND|
|<code>backend-stalls</code>|PERF_COUNT_HW_STALLED_CYCLES_BACKEND|
|<code>ref-cycles</code>|PERF_COUNT_HW_REF_CPU_CYCLES|

ただし，手元の環境virtual box + x64 ubuntuの環境では，HWイベントが
そもそも有効でないため(コンパイルしようにも，menuconfigに現れない)，
試すことができない．Ubuntu公式版を物理マシンで動作させた際には，
bcc側のハードウェアイベントは取り扱うことができたが，
bpftraceでは動作しない．

以下は手元の環境での実行結果．
```
# bpftrace -e 'hardware:bus-cycles:100 { @[pid] = count(); }'
Attaching 1 probe...
perf_event_open failed: No such file or directory
Error attaching probe: hardware:bus-cycles:100
#
```

## 見送り
「watchpoint」と「kfunc/kretfunc」は今の所experimentalだと[公式リファレンスガイド][ref-guide]に
書いてあるので，ここでは説明しない．



<!-- 参考文献リスト -->
[ref-guide]: <https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md>  "公式リファレンスガイド"
[ref-guide-interval]: <https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md#10-interval-timed-output>  "公式リファレンスガイドのintervalの説明"
[path.bt]: <path.bt> "path.bt"
[uprobe-app.bt]: <uprobe-app.bt> "uprobe-app.bt"
[target-sample.c]: <target-sample.c> "target-sample.c"
[usdt_sample.c]: <usdt_sample.c> "usdt_sample.c"
[bcc]: <https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md> "eBPF開発環境bcc"


[bpf_ktime_get_ns]: <https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#3-bpf_ktime_get_ns> "bpf_ktime_get_ns"

