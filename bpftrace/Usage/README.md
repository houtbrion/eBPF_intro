# コマンドラインオプション


## バージョン表示 : <code>-V</code>もしくは<code>--version</code>
```
bash$ bpftrace --version
bpftrace v0.10.0-184-g71754
bash$
```

## ヘルプ : 無引数, <code>-h</code>もしくは<code>--help</code>
以下のヘルプはversion0.10.0-184-g71754のもの．
```
bash$ bpftrace --version
bpftrace v0.10.0-184-g71754
bash$ bpftrace
USAGE:
    bpftrace [options] filename
    bpftrace [options] - <stdin input>
    bpftrace [options] -e 'program'

OPTIONS:
    -B MODE        output buffering mode ('full', 'none')
    -f FORMAT      output format ('text', 'json')
    -o file        redirect bpftrace output to file
    -d             debug info dry run
    -dd            verbose debug info dry run
    -b             force BTF (BPF type format) processing
    -e 'program'   execute this program
    -h, --help     show this help message
    -I DIR         add the directory to the include search path
    --include FILE add an #include file before preprocessing
    -l [search]    list probes
    -p PID         enable USDT probes on PID
    -c 'CMD'       run CMD and enable USDT probes on resulting process
    --usdt-file-activation
                   activate usdt semaphores based on file path
    --unsafe       allow unsafe builtin functions
    -v             verbose messages
    --info         Print information about kernel BPF support
    -k             emit a warning when a bpf helper returns an error (except read functions)
    -kk            check all bpf helper functions
    -V, --version  bpftrace version

ENVIRONMENT:
    BPFTRACE_STRLEN             [default: 64] bytes on BPF stack per str()
    BPFTRACE_NO_CPP_DEMANGLE    [default: 0] disable C++ symbol demangling
    BPFTRACE_MAP_KEYS_MAX       [default: 4096] max keys in a map
    BPFTRACE_CAT_BYTES_MAX      [default: 10k] maximum bytes read by cat builtin
    BPFTRACE_MAX_PROBES         [default: 512] max number of probes
    BPFTRACE_LOG_SIZE           [default: 1000000] log size in bytes
    BPFTRACE_PERF_RB_PAGES      [default: 64] pages per CPU to allocate for ring buffer
    BPFTRACE_NO_USER_SYMBOLS    [default: 0] disable user symbol resolution
    BPFTRACE_CACHE_USER_SYMBOLS [default: auto] enable user symbol cache
    BPFTRACE_VMLINUX            [default: none] vmlinux path used for kernel symbol resolution
    BPFTRACE_BTF                [default: none] BTF file

EXAMPLES:
bpftrace -l '*sleep*'
    list probes containing "sleep"
bpftrace -e 'kprobe:do_nanosleep { printf("PID %d sleeping...\n", pid); }'
    trace processes calling sleep
bpftrace -e 'tracepoint:raw_syscalls:sys_enter { @[comm] = count(); }'
    count syscalls by process name
bash$
```

## ワンライナー : <code>-e 'program' </code>
<code>-e</code>の後ろに指定するクオートされた文字列をbpftraceのスクリプトとして実行する．
```
# bpftrace -e 'BEGIN { printf("Hello, World!\n"); }'
Attaching 1 probe...
Hello, World!
^C#
```

## bpftrace用スクリプトのファイル名 : filename
ファイルに保存したbpftrace用のスクリプトを実行する場合，コマンドラインの末尾にスクリプトのファイル名を指定．
```
$ echo 'BEGIN { printf("Hello, World!\n"); }' >foo
$ sudo bpftrace foo
Attaching 1 probe...
Hello, World!
^C

$
```

## stdinからのbpftraceスクリプト入力 : <code>-</code>
コマンドラインの末尾が<code>-</code>で終わる場合，bpftraceのスクリプトをstdinから読み込む．
```
$ echo 'BEGIN { printf("Hello, World!\n"); }' |sudo bpftrace -
Attaching 1 probe...
Hello, World!
^C

$
```

## bpftraceを使ったコマンドの作成
以下の例はbpftraceを用いて"Hello World!"を実現する[プログラム][HelloWorld]を作成する例である．
shellスクリプトと同じ仕組みで実現可能．
```
bash$ cat HelloWorld
#!/usr/bin/env bpftrace

BEGIN { printf("Hello, World!\n"); }

bash$ sudo ./HelloWorld
Attaching 1 probe...
Hello, World!
^C

bash$
```

コマンドラインオプションをデフォルトで与えるやり方もshellスクリプトと同じ．
以下のプログラムは[printhelp][printhelp]としてこのディレクトリに同封．
```
bash$ cat printhelp
#!/usr/bin/env -S bpftrace --help

bash$ sudo ./printhelp
USAGE:
    bpftrace [options] filename
    bpftrace [options] - <stdin input>
    bpftrace [options] -e 'program'

OPTIONS:
    -B MODE        output buffering mode ('full', 'none')
    -f FORMAT      output format ('text', 'json')
    -o file        redirect bpftrace output to file
(中略)
bash$
```

## bpftraceで利用可能なプローブの取得 : <code>-l [search]</code>
カーネルを5.6に上げた環境でbpftraceで利用可能なプローブの総数は以下のように4万9千個近くとなる．
```
bash$ uname -a
Linux ebpf 5.6.18 #1 SMP Wed Jul 1 00:05:32 UTC 2020 x86_64 x86_64 x86_64 GNU/Linux
bash$ sudo bpftrace -l | wc -l
48840
bash$
```

### 全プローブのリスト取得

<code>-l</code>を無引数で起動すると全プローブが出力される．
```
bash$ sudo bpftrace -l
software:alignment-faults:
software:bpf-output:
software:context-switches:
software:cpu-clock:
software:cpu-migrations:
software:dummy:
software:emulation-faults:
software:major-faults:
software:minor-faults:
software:page-faults:
software:task-clock:
hardware:backend-stalls:
hardware:branch-instructions:
hardware:branch-misses:
hardware:bus-cycles:
hardware:cache-misses:
hardware:cache-references:
hardware:cpu-cycles:
hardware:frontend-stalls:
hardware:instructions:
hardware:ref-cycles:
tracepoint:btrfs:btrfs_transaction_commit
tracepoint:btrfs:btrfs_inode_new
(以下略)
```

### パターンマッチによるプローブの検索

特定の名前のプローブの正式名を調べる場合，以下のようにパターンを指定すれば良い．
```
bash$ sudo bpftrace -l '*tcp_probe*'
tracepoint:tcp:tcp_probe
bash$ sudo bpftrace -l '*tcp_pro*'
tracepoint:tcp:tcp_probe
kprobe:tcp_process_tlp_ack
bash$
```

### プローブの引数の調査
<code>-l</code>に<code>-v</code>を組み合わせることで，プローブの引数を
調べることができる．この際，パターンマッチも可能．
```
bash$ sudo bpftrace -lv tracepoint:syscalls:sys_enter_open
tracepoint:syscalls:sys_enter_open
    int __syscall_nr;
    const char * filename;
    int flags;
    umode_t mode;
bash$ sudo bpftrace -lv '*sys_enter_open'
tracepoint:syscalls:sys_enter_open
    int __syscall_nr;
    const char * filename;
    int flags;
    umode_t mode;
bash$
```

### 引数の型の調査 (要調査)
公式サイトの[リファレンスガイド][ref-guide]によると，BTFが利用可能な環境ではkprobe等の引数の型を調べることができると
書いてあるものの，手元の環境(以下に示す)では，型の検索ができない．
```
bash$ uname -a
Linux ebpf 5.6.18 #1 SMP Wed Jul 1 00:05:32 UTC 2020 x86_64 x86_64 x86_64 GNU/Linux
bash$ grep -i btf /boot/config-5.6*
CONFIG_DEBUG_INFO_BTF=y
bash$
```

## デバッグ出力 : <code>-d</code>, <code>-dd</code>と<code>-v</code>
この分類に入るオプションは，かなり高度なプログラミングを行わないかぎり
必要にならない情報を出力するもので，
[公式リファレンスガイド][ref-guide]でも
「bpftraceのエンドユーザは通常使うことがないオプションなので，ここは飛ばして良い．」
と「太字」で記されている．

### 冗長出力 : <code>-v</code>
「<code>-v</code>」オプションは，実行前にスクリプトの詳細な情報を出力するが，
この中にはeBPFのVMにロードされるプログラムのバイトコードまで参照することができる．
このディレクトリにおさめている[HelloWorld][HelloWorld]を<code>-v</code>付きで
実行すると，以下のような出力が得られる．
```
bash$ cat HelloWorld
#!/usr/bin/env bpftrace

BEGIN { printf("Hello, World!\n"); }


bash$ sudo bpftrace -v HelloWorld
Attaching 1 probe...

Program ID: 67

Bytecode:
0: (bf) r6 = r1
1: (b7) r1 = 0
2: (7b) *(u64 *)(r10 -8) = r1
last_idx 2 first_idx 0
regs=2 stack=0 before 1: (b7) r1 = 0
3: (18) r7 = 0xffff93488a105e00
5: (85) call bpf_get_smp_processor_id#8
6: (bf) r4 = r10
7: (07) r4 += -8
8: (bf) r1 = r6
9: (bf) r2 = r7
10: (bf) r3 = r0
11: (b7) r5 = 8
12: (85) call bpf_perf_event_output#25
last_idx 12 first_idx 0
regs=20 stack=0 before 11: (b7) r5 = 8
13: (b7) r0 = 0
14: (95) exit
processed 14 insns (limit 1000000) max_states_per_insn 0 total_states 1 peak_states 1 mark_read 0

Attaching BEGIN
Running...
Hello, World!
^C

bash$
```
上のような情報を見る必要があるのは，よほど大規模もしくは凝ったスクリプトを作った場合に限られる．

### デバッグ情報 : <code>-d</code>, <code>-dd</code>
<code>-d</code>オプションをつけてbpftraceのスクリプトを実行すると，該当スクリプトは
実行されず，スクリプトを解析した結果の情報が出力される．
ただし，[公式リファレンスガイド][ref-guide]では，「ユーザスクリプトのデバッグではなく，
bpftrace自身のデバッグに役立つ情報である」と述べられている．
```
bash$ sudo bpftrace -d HelloWorld

AST
-------------------
Program
 BEGIN
  call: printf
   string: Hello, World!\n


AST after semantic analysis
-------------------
Program
 BEGIN
  call: printf :: type[none]
   string: Hello, World!\n :: type[string[64]]

; ModuleID = 'bpftrace'
source_filename = "bpftrace"
target datalayout = "e-m:e-p:64:64-i64:64-n32:64-S128"
target triple = "bpf-pc-linux"

%printf_t = type { i64 }

; Function Attrs: nounwind
declare i64 @llvm.bpf.pseudo(i64, i64) #0

define i64 @BEGIN(i8*) local_unnamed_addr section "s_BEGIN_1" {
entry:
  %printf_args = alloca %printf_t, align 8
  %1 = bitcast %printf_t* %printf_args to i8*
  call void @llvm.lifetime.start.p0i8(i64 -1, i8* nonnull %1)
  %2 = getelementptr inbounds %printf_t, %printf_t* %printf_args, i64 0, i32 0
  store i64 0, i64* %2, align 8
  %pseudo = tail call i64 @llvm.bpf.pseudo(i64 1, i64 1)
  %get_cpu_id = tail call i64 inttoptr (i64 8 to i64 ()*)()
  %perf_event_output = call i64 inttoptr (i64 25 to i64 (i8*, i64, i64, %printf_t*, i64)*)(i8* %0, i64 %pseudo, i64 %get_cpu_id, %printf_t* nonnull %printf_args, i64 8)
  call void @llvm.lifetime.end.p0i8(i64 -1, i8* nonnull %1)
  ret i64 0
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #1

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }
bash$
```

## プリプロセッサオプション

### インクルードファイルのディレクトリ指定 : <code>-I</code>
bpftraceのスクリプトでは，Cのデータ型の定義等をインクルードファイルとして別の
ファイルに定義しておき，それをロードすることでスクリプトの開発を容易にすることができる．
この際，インクルードファイルがスクリプトと別のディレクトリに存在する場合は，
このオプションを指定する必要がある．
以下の[例][include_example.bt]は，無用ではあるがスクリプト中でインクルードファイルを
要求している場合での動作を示している．
```
bash$ ls
HelloWorld  include_example.bt  printhelp  README.md
bash$ cat include_example.bt
#include <foo.h>
BEGIN { printf("Hello, World!\n"); }

bash$ sudo bpftrace ./include_example.bt
definitions.h:1:10: fatal error: 'foo.h' file not found
bash$ touch /tmp/foo.h
bash$ cat /tmp/foo.h
bash$ sudo bpftrace -I /tmp/ ./include_example.bt
Attaching 1 probe...
Hello, World!
^C

bash$
```

### インクルードするヘッダの指定 : <code>--include</code>
実行するbpftraceのスクリプトに<code>#include</code>がないが，スクリプト内で使われている構造体定義が外部で行われている場合に，
そのインクルードファイルを直接指定してロードする方法．

[公式リファレンスガイド][ref-guide]の例は，スクリプト部分にtypo(以下の例の<code>struct</code>が欠けている)があり，そのまま実行できないため
要注意(2020/7/02時点)．
```
bash$ sudo bpftrace --include linux/path.h --include linux/dcache.h -e 'kprobe:vfs_open { printf("open path: %s\n", str(((struct path *)arg0)->dentry->d_name.name)); }'
Attaching 1 probe...
open path: global.stat
open path: global.tmp
open path: db_0.tmp
open path: global.stat
open path: global.stat
open path: postmaster.pid
open path: oom_score_adj
open path: pg_filenode.map
open path: pg_internal.init
open path: PG_VERSION
open path: pg_filenode.map
open path: pg_internal.init
open path: 2601
open path: global.stat
open path: global.tmp
open path: db_1.tmp
open path: db_0.tmp
open path: global.stat
open path: global.stat
open path: db_1.stat
open path: db_0.stat
open path: 1262
open path: 1259
^C

bash$
```

## 環境変数

### bpftraceの環境変数

現在のバージョン(v0.10.0-184-g71754)のデフォルト値は以下の通り．
```
BPFTRACE_STRLEN             [default: 64] bytes on BPF stack per str()
BPFTRACE_NO_CPP_DEMANGLE    [default: 0] disable C++ symbol demangling
BPFTRACE_MAP_KEYS_MAX       [default: 4096] max keys in a map
BPFTRACE_CAT_BYTES_MAX      [default: 10k] maximum bytes read by cat builtin
BPFTRACE_MAX_PROBES         [default: 512] max number of probes
BPFTRACE_LOG_SIZE           [default: 1000000] log size in bytes
BPFTRACE_PERF_RB_PAGES      [default: 64] pages per CPU to allocate for ring buffer
BPFTRACE_NO_USER_SYMBOLS    [default: 0] disable user symbol resolution
BPFTRACE_CACHE_USER_SYMBOLS [default: auto] enable user symbol cache
BPFTRACE_VMLINUX            [default: none] vmlinux path used for kernel symbol resolution
BPFTRACE_BTF                [default: none] BTF file
```


詳細な環境変数の意味は[公式リファレンスガイド][ref-guide]を参照していただきたいが，
各環境変数の意味を簡単に紹介する．ただし，
以下の3種類は[公式リファレンスガイド][ref-guide]の現在(2020/07/02)バージョンでは，
説明が記載されていないので要注意．
```
BPFTRACE_CAT_BYTES_MAX
BPFTRACE_LOG_SIZE
BPFTRACE_NO_USER_SYMBOLS
```

#### BPFTRACE_STRLEN
Cのキャラクタ型の配列(通常文字列として利用)を実際の文字列に変換する関数で，この関数が取り扱える文字列の最大長を
定義する環境変数．

#### BPFTRACE_NO_CPP_DEMANGLE
この[解説記事][demangle]がわかりやすいが，コンパイラが同じ名前を広域に一意にするため，
名前を特別なアルゴリズムに従って変換する機能が「name mangling」と呼ばれる．
バイナリを解析する際にこの逆変換を行わないと，シンボル名が意味不明な文字列となり，
ソースとの対応がわからない．性能上の理由により，この逆変換を行わない設定を
この環境変数で実現できる．デフォルトは逆変換を行う．

#### BPFTRACE_MAP_KEYS_MAX
eBPFのmap機能で，HASH表などの大きさの最大値を定義する環境変数．

#### BPFTRACE_MAX_PROBES
bpftraceが同時に監視するカーネルのprobeの最大値．あまり増やすと
システムのメモリ利用量やシステムの負荷が増大し，最悪の場合
システムがクラッシュする危険がある．

#### BPFTRACE_CACHE_USER_SYMBOLS
メモリ上でのオブジェクトの配置をランダム化する機能(ASLR : address space layout randomization)が
使われる可能性があるため，アドレスからシンボルを解決する検索の結果を通常はキャッシュしないが，
性能面で不利．ASLRを使っていなければキャッシュをONにしても大丈夫なので，この環境変数の値を1に
すれば良い．

#### BPFTRACE_VMLINUX
カーネル内のシンボルを探すために，vmlinuxの置き場所が必要になるが，
通常この環境変数は定義がなく，その場合はいくつかのwell knownな
場所を探しに行く．この環境変数にvmlinuxの置き場所をセットすることで
探す必要がなくなるだけでなく，well knownではないパスにあっても
bpftraceが利用できる．

#### BPFTRACE_BTF
BTF用のvmlinuxファイルの置き場所を指定するための環境変数．

#### BPFTRACE_PERF_RB_PAGES
bpftraceがカーネル内で発生するイベントを示すデータを受信しきれなくなり，
ロスが起きている場合に，バッファサイズを変更(大きくする)ために用いる
環境変数で，2のべき乗の値でなければならない．

#### BPFTRACE_CAT_BYTES_MAX
bpftraceにはプログラム中で<code>cat()</code>関数を用いて，ファイル内容を
プリントアウトすることができる．この<code>cat()</code>で取り扱える
サイズの最大値を指定する環境変数．

#### BPFTRACE_NO_USER_SYMBOLS
巨大なバイナリを対象にbccを利用すると，ユーザシンボルをメモリ上に保持して様々な処理を行うため，
メモリを圧迫する可能性がある．bccのissueとして[報告][BPFTRACE_NO_USER_SYMBOLS]が上がっており，
これに対応するために，環境変数<code>BPFTRACE_NO_USER_SYMBOLS</code>が導入された．
この環境変数の値を変更することで，ユーザのシンボルの処理をしなくなるため
出力が非常に読みにくくなるが，メモリは圧迫しなくなる．
デフォルトは，シンボルを解析して保持する．

#### BPFTRACE_LOG_SIZE
カーネルからユーザ空間にメッセージやデータを送るためのeBPFのI/Fのサイズが小さいと，
データ量が多い場合にバッファがあふれるため，この環境変数でサイズを変更する．

### Clangの環境変数
詳細は[公式リファレンスガイド][ref-guide]を参照していただきたいが，bpftraceはバックエンドでClangを利用している．
そのため，Clangの動作を変更するために，Clangが参照する環境変数の
<code>CPATH</code>と<code>C_INCLUDE_PATH</code>は設定が有効な場合がある．

### 公式リファレンスガイドの環境変数部分に説明がないもの

#### BPFTRACE_KERNEL_SOURCE
[公式リファレンスガイド][ref-guide]では，オプション等を説明するところに記載がなく，
実行時エラーのところに紹介がされているのが，<code>BPFTRACE_KERNEL_SOURCE</code>である．

デフォルトで，カーネルのヘッダファイルを以下の場所に探しにいくので，別の場所に
存在する場合は，<code>BPFTRACE_KERNEL_SOURCE</code>にパスを設定する．
```
/lib/modules/$(uname -r)/build/include
```

<!-- 参考文献リスト -->
[ref-guide]: <https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md >  "公式リファレンスガイド"
[HelloWorld]: <HelloWorld> "HelloWorld"
[printhelp]: <printhelp> "printhelp"
[include_example.bt]: <include_example.bt> "include_example.bt"
[BPFTRACE_NO_USER_SYMBOLS]: <https://github.com/iovisor/bcc/issues/2421> "bcc_symcache_resolve uses too much memory #2421"

[demangle]: <http://0xcc.net/blog/archives/000095.html> "C++ demangle紹介"
