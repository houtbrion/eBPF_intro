# PythonのeBPFライブラリ(bcc Python)の機能

参考文献
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#bcc-python


今までの説明で使われていない(もしくは説明が少ない)PythonのeBPF機能について
ここでは説明する．

## Debug Output機能
参考文献
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#debug-output

<code>bpf_trace_printk()</code>については，「はじめの一歩」から沢山のサンプルプログラムで
利用と紹介しているが，それらのサンプルプログラムでは<code>trace_fields()</code>
を利用している．それに加えて，Python側で<code>bpf_trace_printk()</code>を
受信する方法は<code>trace_print()</code>が存在する．

本ディレクトリには，<code>bpf_trace_printk()</code>の出力を受信する部分以外は
まったく同じ2つのサンプルプログラム
<a href="trace_print">trace_print</a>
(<code>trace_print()</code>を
使うもの)と
<a href="printk_sample">printk_sample</a>
(<code>trace_fields()</code>を使うもの)を
収納している．なお，<a href="printk_sample">printk_sample</a>は「はじめの一歩」のサンプルプログラムと全く
同じものである．

2つのプログラムの違いで<code>trace_print()</code>と
<code>trace_fields()</code>の違いを説明する．

### trace_fields()
「はじめの一歩」でも説明したように，<code>bpf_trace_printk()</code>の出力を
ユーザ空間のpythonプログラムで<code>trace_fields()</code>を用いて
受信する場合，<a href="printk_sample">printk_sample</a>の25行目以降(以下に引用)にあるように，
<code>trace_fields()</code>の出力は6個組であり，6個組の末尾(<code>msg</code>)が
<code>bpf_trace_printk()</code>の出力となる．

```
while 1:
        # Read messages from kernel pipe
        try:
            # bpf_trace_printk()の出力を受信．
            (task, pid, cpu, flags, ts, msg) = b.trace_fields()
        except ValueError:
            # Ignore messages from other tracers
            continue
        except KeyboardInterrupt:
            # キーボードインタラプトの時に無限ループから抜ける
            exit()
        # フォーマットを合わせて，受信した内容を出力
        printb(b"%-6d %-12.12s %s" % (pid, task, msg))
```
この
<a href="printk_sample">サンプルプログラム</a>
では，6個組出力のうち，<code>pid</code>,<code>task</code>,<code>msg</code>の
3つを出力しており，以下のような出力が得られる．

```
# ./printk_sample
PID    COMM         OUTPUT
3239   <...>        start of execve time = 18194589254782 , pid = 3239, comm = bash
^C#
```
つまり，<code>trace_fields()</code>の6個組出力の2番目(<code>task</code>)は，本来プロセス名が入るはずの部分である．

### trace_print()
<code>trace_fields()</code>に対して，
<code>
trace_print()
</code>を使う場合は，サンプルプログラム
<a href="trace_print">trace_print</a>
の25行目以降(以下に引用)のようになる．

```
while 1:
        # Read messages from kernel pipe
        try:
            # bpf_trace_printk()の出力を受信．
            b.trace_print()
        except ValueError:
            # Ignore messages from other tracers
            continue
        except KeyboardInterrupt:
            # キーボードインタラプトの時に無限ループから抜ける
            exit()
```

両者の違いは，<code>trace_print()</code>はeBPFのVMからの<code>bpf_trace_printk()</code>の内容を
そのままstdoutに出力するものであり，<code>
trace_fields()
</code>は，
<code>
bpf_trace_printk()
</code>の内容を受信した際に得られる情報を6個組のデータとして受け取るものとなっている．

なお，<code>trace_print()</code>のサンプルプログラム(<a href="trace_print">trace_print</a>)を実行すると
次のような出力が得られる．

```
# ./trace_print
b'            bash-3237  [000] .... 18174.550841: 0: start of execve time = 18174509036604 , pid = 3237, comm = bash'
^C#
```

<a href="printk_sample">printk_sample</a>
の出力を下に引用する．
```
# ./printk_sample
PID    COMM         OUTPUT
3239   <...>        start of execve time = 18194589254782 , pid = 3239, comm = bash
^C#
```
両者を比べると，"start of"から末尾までが<code>bpf_trace_printk()</code>のoutputであることから，
<code>trace_print()</code>のデフォルト出力は，pidや時刻，プロセス名などを
全て連結したものが出力されることがわかる．

ただし，<code>trace_print()</code>のフォーマットは調整可能であり，
公式リファレンスガイドのサンプルプログラム
<a href="../OriginalSample/trace_fields.py">trace_fields.py</a>
(以下のURL)の
20行目以降(下に引用)を見ると
必要な出力を絞り込んでいることから，フォーマットにこだわらず単に出力するだけなら，<code>trace_print()</code>でも
十分であるが，6個組に分割して別の処理を行う場合は<code>trace_fields()</code>が適していることがわかる．

- https://github.com/iovisor/bcc/blob/master/examples/tracing/trace_fields.py

```
print("PID MESSAGE")
try:
    b.trace_print(fmt="{1} {5}")
except KeyboardInterrupt:
    exit()
```


## Map対応機能

### items()
参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#3-items

これは，<code>BPF_HASH
</code>マップのキーをまとめて取得するための機能である．
公式のサンプルプログラムは以下の2つのプログラム(C部分とPython部分)に分かれている．
- https://github.com/iovisor/bcc/blob/master/examples/tracing/task_switch.c
- https://github.com/iovisor/bcc/blob/master/examples/tracing/task_switch.py

これでは読み取りづらいので，2つのプログラムをマージし，コメントを追加したものを本ディレクトリの
<a href="task_switch">task_switch</a>
として
収めている．

<a href="task_switch">task_switch</a>
の39行目では，eBPFの関数(<code>count_sched()</code>)をカーネル内の
<code>finish_task_switch()</code>に割り当てている．
```
b.attach_kprobe(event="finish_task_switch", fn_name="count_sched")
```
eBPFのC部分の12から17行目で，pidの組をキーとする<code>HASH</code>マップを定義している．
```
struct key_t {
    u32 prev_pid; // 直前に実行されていたプロセスのpid
    u32 curr_pid; // 現在実行されているプロセスのpid
};
BPF_HASH(stats, struct key_t, u64, 1024);
```

22行目から31行目までで，直前まで実行されていたプロセスのpidと実行が再開された
プロセスのpidの組み合わせをキーとして，<code>HASH</code>マップのエントリを取得して，
値をインクリメントしている．
```
key.curr_pid = bpf_get_current_pid_tgid(); // 現在のプロセスのpidを取得
key.prev_pid = prev->pid;                  // 直前のプロセスのpidを保存
// 表のエントリを取得(存在しなければ，0で作成)．
val = stats.lookup_or_try_init(&key, &zero);
// 表のエントリを更新する．
// map.increment(key)を使わずに実施
if (val) {
    (*val)++;
}
```

これの意味するところは，発生したコンテキストスイッチのうち，ある2つのプロセスの組み合わせ(順番も含む)が同じだった
ものを数えていることになる．

python側のプログラムで，無理やりコンテキストスイッチを起こすために，41,42行目でスリープと
ウェイクアップを100回繰り返す．
```
# 10msのスリープを100回実行 (タスクスイッチをたくさん発生させる)
for i in range(0, 100): sleep(0.01)
```
最後に，ここでの主題である<code>items()</code>を用いて，<code>HASH</code>マップの
キーをまとめて取得し，各キーの表の値を表示する(44行目以降)．
```
# BPF_HASHの表であるstatsのkeyをkに，値をvに割り当てて繰り返し実行．
for k, v in b["stats"].items():
    print("task_switch[%5d->%5d]=%u" % (k.prev_pid, k.curr_pid, v.value))
```
ここで，気をつけないといけないのは，<code>items()</code>で取り出したものはpythonの
オブジェクトになっており，そのまま値として
取り扱えないため，<code>オブジェクト.value</code>として値を取り出す必要がある．

上のサンプルプログラムを動作させた時の出力は以下のようなものとなっており，
swapperからpid3397への切り替えが86回，逆の切り替えが100回で最も多くなっている
(多分pid3397は，このサンプルプログラム)．
```
# ./task_switch
task_switch[    0-> 3397]=86
task_switch[   25-> 2518]=1
task_switch[   10->    0]=1
task_switch[    0->  593]=3
task_switch[ 3352-> 2518]=4
task_switch[    0-> 2979]=2
task_switch[  675->   11]=1
task_switch[ 2959->    0]=4
task_switch[    0->  708]=1
task_switch[    0-> 2518]=20
task_switch[    0->   12]=1
task_switch[  343->    0]=2
task_switch[  342->    0]=1
task_switch[    0-> 3352]=21
task_switch[ 2959->   18]=2
task_switch[  589->    0]=1
task_switch[   12->   11]=1
task_switch[ 3352-> 2959]=3
task_switch[  593-> 3148]=1
task_switch[ 3148->   11]=3
task_switch[  342->   10]=1
task_switch[ 3352->    0]=17
task_switch[   11->    0]=36
task_switch[   11-> 3397]=7
task_switch[    0->  257]=13
task_switch[  708->    0]=1
task_switch[ 3148-> 3397]=6
task_switch[  589->   18]=1
task_switch[  589->  708]=1
task_switch[    0->   17]=1
task_switch[ 3397-> 2979]=1
task_switch[  701-> 2518]=1
task_switch[ 2979-> 3397]=1
task_switch[ 3297-> 3148]=1
task_switch[  257-> 3148]=13
task_switch[ 3397->    0]=100
task_switch[ 2959-> 3352]=1
task_switch[  343->  342]=2
task_switch[ 2979->    0]=2
task_switch[  708-> 2959]=1
task_switch[  593->   11]=1
task_switch[    0->  675]=1
task_switch[    0-> 3398]=1
task_switch[    0->   25]=1
task_switch[  590->    0]=1
task_switch[    0->  590]=1
task_switch[    0-> 2959]=3
task_switch[  708-> 2518]=1
task_switch[    0->  342]=2
task_switch[ 2518->    0]=27
task_switch[   17->    0]=1
task_switch[    0-> 3148]=7
task_switch[ 3148->    0]=14
task_switch[    0->   11]=37
task_switch[    0->  701]=1
task_switch[  593-> 3397]=1
task_switch[  342->  343]=1
task_switch[  342-> 3148]=1
task_switch[    0->  343]=3
task_switch[   18-> 3352]=2
task_switch[   18->  708]=1
task_switch[    0->  589]=3
task_switch[    0-> 3297]=1
#
```


### values()
参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#4-values

<code>values()</code>は，マップの値を全て取り出したArrayのデータを作る機能で，
<code>values()</code>を使ったサンプルプログラムが本ディレクトリの
<a href="map_vlues_sample">map_vlues_sample</a>
となっている，

このプログラムのeBPFの関数(C部分の8から30行目)では，<code>execve</code>を実行したプロセスのuidを取得し，
rootか否かでカウントしている．55行目から58行目までで表から値だけを抜き出したArrayを作っているが，
元々の表がTableなので，<code>values()</code>で取り出したArrayも同じ構造となる．
ここで，気をつけないといけないのは先程と同じく，<code>values()</code>で取り出したものに
対して，<code>オブジェクト.value</code>として値を取り出すこと．
```
print("【map.values()を使った場合の出力】")
print("rootユーザ ,  一般ユーザ ")
for v in uidcnt.values():  # 表の値を全部一気に取得してループ
    print(v.value, '(回)           ', end=" ")
```

実際に
<a href="map_vlues_sample">サンプルプログラム</a>
を実行すると以下のような結果となる．
なお，この時，別の2つのウィンドウ(root権限のものと一般ユーザ権限のshellが動作している)で
ls等を繰り返し実行している．
```
# ./map_vlues_sample
=================測定結果===================
【map.keys()を使った場合の出力】
rootユーザ: 0 回execve呼び出し
一般ユーザ: 0 回execve呼び出し

【map.values()を使った場合の出力】
rootユーザ ,  一般ユーザ
0 (回)            0 (回)

=================測定結果===================
【map.keys()を使った場合の出力】
rootユーザ: 6 回execve呼び出し
一般ユーザ: 4 回execve呼び出し

【map.values()を使った場合の出力】
rootユーザ ,  一般ユーザ
6 (回)            4 (回)

=================測定結果===================
【map.keys()を使った場合の出力】
rootユーザ: 0 回execve呼び出し
一般ユーザ: 4 回execve呼び出し

【map.values()を使った場合の出力】
rootユーザ ,  一般ユーザ
0 (回)            4 (回)

=================測定結果===================
【map.keys()を使った場合の出力】
rootユーザ: 9 回execve呼び出し
一般ユーザ: 0 回execve呼び出し

【map.values()を使った場合の出力】
rootユーザ ,  一般ユーザ
9 (回)            0 (回)

^C#
```

この結果に見えるように，1回のラウンドの前半では，<code>keys()</code>を
使って，表のキーから値を取得しており，後半は<code>values()</code>で
値の一覧表を作っているので，比較してみると良い．

### ksym() とnum_open_kprobes()
参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#1-ksym

<code>ksym()</code>は上記参考文献(リファレンスガイドの<code>ksym()</code>のページ)にも
あるように，「カーネルメモリ中のアドレスからカーネル内の関数名に変換する機能」である．
公式リファレンスガイドの該当ページのコードの断片の例にもあるように，アドレスを引数に与えると関数名の文字列が返る．
```
print("kernel function: " + b.ksym(addr))
```

また，<code>num_open_kprobes()</code>は解説がいらない気もするが，名前の通り「kprobeもしくはkretprobe」を
何箇所オープンしているかを把握するためのもので，公式リファレンスガイドには「Returns the number of open k[ret]probes. Can be useful for scenarios where event_re is used while attaching and detaching probes. Excludes perf_events readers.」とあり，
掲載しているコード断片(以下に転載)を見ると，あるkprobeの監視ポイントにすでにeBPFのプログラムが張り付いているか否かを
判定するために用いている．

```
b.attach_kprobe(event_re=pattern, fn_name="trace_count")
matched = b.num_open_kprobes()
if matched == 0:
    print("0 functions matched by \"%s\". Exiting." % args.pattern)
    exit()
```

実際の動作を確認するために用いたプログラムは以下の
<a href="../OriginalSample/stacksnoop.py">公式サンプルプログラム</a>
である．
- https://github.com/iovisor/bcc/blob/master/examples/tracing/stacksnoop.py

この
<a href="../OriginalSample/stacksnoop.py">サンプルプログラム</a>
は，krobeの監視対象を指定すると，89行目から92行目で<code>num_open_kprobes()</code>
を用いて，eBPFの監視プログラムを該当の関数に<code>attach_kprobe()</code>が成功したか否かを
判定するためには用いている．
```
matched = b.num_open_kprobes()
if matched == 0:
    print("Function \"%s\" not found. Exiting." % function)
    exit()
```

eBPFのVM内で動作するCのプログラムを見ると，55行目,62行目,65行目でスタックのIDを構造体データ<code>data</code>の
メンバに代入して，<code>perf_submit()</code>でユーザ空間に通知している．
```
BPF_STACK_TRACE(stack_traces, 128);
(中略)
data.stack_id = stack_traces.get_stackid(ctx, 0);
(中略)
events.perf_submit(ctx, &data, sizeof(data));
```

それをうけたユーザ空間のpythonプログラムでは，
115行目から117行目(以下に引用)でスタックを
イベントで通知されたIDを使って検索して，そこから下に積まれている
スタックのアドレスを取得して，<code>ksym()</code>を使って，シンボルを取り出し印刷している．
```
for addr in stack_traces.walk(event.stack_id):
    sym = b.ksym(addr, show_offset=offset)
    print("\t%s" % sym)
```

実際に動作させて，wgetで外部のwebサーバにアクセスすると以下のような出力が得られる．
```
# ./stacksnoop.py tcp_v4_connect
TIME(s)            FUNCTION
13.598776817       tcp_v4_connect
        tcp_v4_connect
        inet_stream_connect
        __sys_connect
        __x64_sys_connect
        do_syscall_64
        entry_SYSCALL_64_after_hwframe

^C#
```


### ksymname()
参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#2-ksymname

<code>ksym()</code>はアドレスからカーネル内の関数の名前を取り出す機能であったが，
<code>ksymname()</code>はその逆変換を行う機能である．

以下の
<a href="../OriginalSample/statsnoop.py">公式サンプルプログラム</a>
がわかりやすい使い方をしている．
サンプルプログラム:
- https://github.com/iovisor/bcc/blob/master/tools/statsnoop.py

以下に116行目から119行目を引用する．
```
syscall_fnname = b.get_syscall_fnname("stat")
if BPF.ksymname(syscall_fnname) != -1:
    b.attach_kprobe(event=syscall_fnname, fn_name="syscall__entry")
    b.attach_kretprobe(event=syscall_fnname, fn_name="trace_return")
```
このコードを見るとわかるように，<code>stat</code>関数のkrobe用の
参照点を116行目で取り出し，それが今動いているカーネルに実在するか否かを
<code>ksymname()</code>で確認している．

この
<a href="../OriginalSample/statsnoop.py">サンプルプログラム</a>は，引数なしで動作させると，プログラムの21行目にあるように，
3種類のシステムコール(<code>stat</code>, <code>statfs</code>, <code>newstat</code>)を監視する．
```
./statsnoop           # trace all stat() syscalls
```
実際に動作させると，以下のような出力が得られる．
```
# ./statsnoop.py
PID    COMM               FD ERR PATH
2995   ls                 -1   2 /sys/fs/selinux
2995   ls                 -1   2 /selinux
2992   statsnoop.py       -1   2 /usr/lib/python2.7/encodings/ascii
652    sd-resolve          0   0 /etc/resolv.conf
659    systemd-resolve     0   0 /etc/hosts
659    systemd-resolve     0   0 /etc/resolv.conf
659    systemd-resolve     0   0 /run/systemd/resolve/resolv.conf
659    systemd-resolve     0   0 /run/systemd/resolve/stub-resolv.conf
659    systemd-resolve     0   0 /etc/resolv.conf
^C#
```

### sym()
参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#3-sym

<code>ksym()</code>がカーネル内部関数用であったのに対して，<code>sym()</code>は
ユーザアプリの監視(uprobe)用の機能である．

下の
<a href="../OriginalSample/mallocstacks.py">公式サンプルプログラム</a>
で使い方を説明する．
- https://github.com/iovisor/bcc/blob/master/examples/tracing/mallocstacks.py

この
<a href="../OriginalSample/mallocstacks.py">サンプルプログラム</a>
は，56行目を見てわかるように，uprobeを使ってlibcの<code>malloc()</code>関数を
監視している．
```
b.attach_uprobe(name="c", sym="malloc", fn_name="alloc_enter", pid=pid)
```

対応するCのプログラム本体は41行目で定義されており，</code>malloc()</code>の
第一引数を参照している．
```
int alloc_enter(struct pt_regs *ctx, size_t size) {
```
この監視プログラムの中身を見ると，
<code>calls</code>というハッシュ表(38行目)と<code>stack_traces</code>という
名前でスタックトレースの格納領域を用意している．
```
BPF_HASH(calls, int);
BPF_STACK_TRACE(stack_traces, """ + stacks + """);
```
42行目で<code>malloc()</code>を利用したプロセスのスタックのIDを取得し，
そのIDをハッシュ表のキーととして，<code>malloc()</code>の引数となった
メモリ確保のサイズを表のエントリに加算している．
```
int key = stack_traces.get_stackid(ctx, BPF_F_USER_STACK);
(中略)
val = calls.lookup_or_try_init(&key, &zero);
if (val) {
    (*val) += size;
}
```

これに対して，Python側のプログラムでは，68行目でハッシュ表<code>calls</code>の中身
(スタックIDとmallocの引数の値)を取り出し，<code>malloc()</code>が呼び出された時の
スタックの中身をプリントアウトする仕組みになっている．
```
for k, v in reversed(sorted(calls.items(), key=lambda c: c[1].value)):
    print("%d bytes allocated at:" % v.value)
    if k.value > 0 :
        for addr in stack_traces.walk(k.value):
            printb(b"\t%s" % b.sym(addr, pid, show_offset=True))
```

この
<a href="../OriginalSample/mallocstacks.py">サンプルプログラム</a>
を実行した例が以下のものである．
```
# ./mallocstacks.py  886
Attaching to malloc in pid 886, Ctrl+C to quit.
^C2904 bytes allocated at:
        __libc_malloc+0x0
        [unknown]
624 bytes allocated at:
        __libc_malloc+0x0
64 bytes allocated at:
        __libc_malloc+0x0
        [unknown]
#
```




