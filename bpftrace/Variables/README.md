# 変数


## 組み込み変数

|変数名|内容|
|:---|:---|
|<code>pid</code> | プロセスID (kernel tgid)|
|<code>tid</code> | スレッドID (kernel pid)|
|<code>uid</code> | ユーザID|
|<code>gid</code> | グループID|
|<code>nsecs</code> | ナノ秒単位のカーネル内部時刻(64bit非負整数) |
|<code>elapsed</code> | bpftraceの初期化後に経過した時刻(ナノ秒単位)|
|<code>cpu</code> | プロセッサID|
|<code>comm</code> | プロセス名|
|<code>kstack</code> | Kernel stack trace|
|<code>ustack</code> | User stack trace|
|<code>arg0, arg1, ..., argN</code> | 監視対象の関数の引数で64bitの変数とみなしてアクセスされる|
|<code>sarg0, sarg1, ..., sargN</code> | 監視対象の関数の引数でスタックに積まれているものを64bitの変数とみなしてアクセスされる |
|<code>retval</code> | 監視対象関数の返り値|
|<code>func</code> | 監視対象関数の名前 |
|<code>probe</code> | プローブの名前(例 kprobe:do_sys_open )|
|<code>curtask</code> | Current task struct as a u64|
|<code>rand</code> | 32bit非負整数の乱数|
|<code>cgroup</code> | カレントプロセスのCgroup ID|
|<code>cpid</code> | bpftrace実行時に-cオプションを使って，プログラムを起動した際のプログラムのpid(u32)|
|<code>$1, $2, ..., $N, $#</code> | bpftrace用スクリプトの引数を表す変数|


### <code>func</code>の使用例
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

```
# bpftrace -e 'kprobe:do_sys_open { printf("func = %s\n", func); }'
Attaching 1 probe...
func = do_sys_open
func = do_sys_open
func = do_sys_open
func = do_sys_open
func = do_sys_open
func = do_sys_open
^C

#
```

### <code>probe</code>の使用例
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

```
# bpftrace -e 'kprobe:do_sys_open { printf("probe = %s\n", probe); }'
Attaching 1 probe...
probe = kprobe:do_sys_open
probe = kprobe:do_sys_open
probe = kprobe:do_sys_open
probe = kprobe:do_sys_open
probe = kprobe:do_sys_open
^C

#
```

### <code>rand</code>の使用例
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

```
# bpftrace -e 'kprobe:do_sys_open { printf("rand = %u\n", rand); }'
Attaching 1 probe...
rand = 3855028744
rand = 1310463167
rand = 2036790611
rand = 2170011707
rand = 1182509454
rand = 3954550731
rand = 2337070903
rand = 2572606336
rand = 4019234747
^C

#
```

### <code>cgroup</code>の使用例
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

```
# bpftrace -e 'kprobe:do_sys_open { printf("cgroup = %u\n", cgroup); }'
Attaching 1 probe...
cgroup = 871
cgroup = 871
cgroup = 871
cgroup = 871
cgroup = 871
cgroup = 871
cgroup = 871
cgroup = 871
cgroup = 871
cgroup = 931
cgroup = 931
cgroup = 931
cgroup = 931
^C

#
```


### <code>cpid</code>の使用例
|環境|動作|
|:--|:--|
|Ubuntu公式|×|
|CentOS公式|○|
|Ubuntu最新|○|

Ubuntu公式の環境では動作しない．
```
# bpftrace -c ./usdt_sample -e 'usdt:./usdt_sample:foo:bar { printf("cpid = %u \n", cpid); }'
Attaching 1 probe...
Error finding or enabling probe: usdt:./usdt_sample:foo:bar
#
```
一方，他の環境では動作する．
```
# bpftrace -c ./usdt_sample -e 'usdt:./usdt_sample:foo:bar { printf("cpid = %u \n", cpid); }'
Attaching 1 probe...
pid = 10590
counter=1
cpid = 10590
counter=2
cpid = 10590
counter=3
cpid = 10590
counter=4
cpid = 10590
counter=5
cpid = 10590
counter=6
cpid = 10590
counter=7
cpid = 10590
^C

#
```

### <code>$1, $2, ..., $N, $#</code>の使用例
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

bpftraceの引数を参照するための機能を使う例を以下に示す．
```
# cat posv.bt
#!/usr/bin/env bpftrace
#

BEGIN{
  printf("$1 = %u, $2 = %u, $# = %u\n",$1,$2,$#);
}
# bpftrace posv.bt
Attaching 1 probe...
$1 = 0, $2 = 0, $# = 0
^C

# bpftrace posv.bt 1
Attaching 1 probe...
$1 = 1, $2 = 0, $# = 1
^C

#  bpftrace posv.bt 1 2
Attaching 1 probe...
$1 = 1, $2 = 2, $# = 2
^C

# bpftrace posv.bt 1 2 3
Attaching 1 probe...
$1 = 1, $2 = 2, $# = 3
^C

#
```

### <code>nsecs, elapsed</code>の使用例
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

<code>nsecs</code>は
ナノ秒単位のカーネル内部時刻(64bit非負整数)を表す．この時刻はuptimeに似たもので，詳細は<code>man clock_gettime</code>でCLOCK_MONOTONICの部分を参照．また，<code>elapsed</code>はbpftraceの初期化後からの経過時間(ナノ秒)を表す．

```
# bpftrace -e 'BEGIN {printf("nsecs = %u, elapsed = %u\n",nsecs, elapsed);}'
Attaching 1 probe...
nsecs = 3610106795, elapsed = 1327847
^C

#
```

### <code>kstack</code>と<code>ustack</code>


<code>kstack</code>と<code>ustack</code>はそれぞれ<code>kstack()</code>と<code>ustack()</code>の
aliasであるので，関数の<code>kstack()</code>と<code>ustack()</code>を参照．

## ユーザ定義変数 : <code>@</code>, <code>$</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
@global_name
@thread_local_variable_name[tid]
$scratch_name
```


### グローバル変数

文法:
```
@name
```

bpftraceのプログラムは，プローブの定義からアクションを記述するプログラム文の列までの組み合わせ(節)が，
何個か並ぶ形となる．グローバル変数はこのbpftraceのプログラム全体で有効な変数である．
```
{
  プローブの定義
  [/フィルタ文/]
  {
    [プログラム文;]+
  }
}+
```

下の実行例は[公式リファレンスガイド][ref-guide]と同じものであるが，グローバル変数「<code>@start</code>」を
「<code>BEGIN</code>」のところで現在時刻で初期化し，kprobeで「<code>do_nanosleep</code>」の実行を監視し，
<code>do_nanosleep</code>が実行された時刻がbpftrace実行からどの提示時間が経過したのかを出力している．
```
# bpftrace -e 'BEGIN { @start = nsecs; }
kprobe:do_nanosleep /@start != 0/ { printf("at %d ms: sleep\n", (nsecs - @start) / 1000000); }'
Attaching 2 probes...
at 145 ms: sleep
at 1146 ms: sleep
at 2146 ms: sleep
^C

@start: 481368456795

#
```

### ローカル変数
文法:
```
$name
```
ローカル変数は，プローブの定義から始まる節をまたいで参照することができない変数で，節をまたいで参照すると「未定義エラー」となる．
```
# bpftrace -e 'BEGIN { $start = nsecs; } kprobe:do_nanosleep /$start != 0/ { printf("at %d ms: sleep\n", (nsecs - $start) / 1000000); }'
stdin:1:47-54: ERROR: Undefined or undeclared variable: $start
BEGIN { $start = nsecs; } kprobe:do_nanosleep /$start != 0/ { printf("at %d ms: sleep\n", (nsecs - $start) / 1000000); }
                                              ~~~~~~~
stdin:1:100-106: ERROR: Undefined or undeclared variable: $start
BEGIN { $start = nsecs; } kprobe:do_nanosleep /$start != 0/ { printf("at %d ms: sleep\n", (nsecs - $start) / 1000000); }
                                                                                                   ~~~~~~
#
```
[公式リファレンスガイド][ref-guide]の例は，節の中でローカル変数「<code>$delta</code>」を利用して，時刻の差分を参照している．
```
# bpftrace -e 'kprobe:do_nanosleep { @start[tid] = nsecs; }
>     kretprobe:do_nanosleep /@start[tid] != 0/ { $delta = nsecs - @start[tid];
>         printf("slept for %d ms\n", $delta / 1000000); delete(@start[tid]); }'
Attaching 2 probes...
slept for 1000 ms
slept for 1001 ms
slept for 1000 ms
^C

@start[571]: 2234709522993

#
```

## 連想配列
文法:
```
@associative_array_name[key_name] = value
@associative_array_name[key_name, key_name2, ...] = value
```
ローカル変数の連想配列存在しないため，下のような構文エラーとなる．
```
# bpftrace -e 'kprobe:do_nanosleep { @start[tid] = nsecs; }
    kretprobe:do_nanosleep /@start[tid] != 0/ { $delta[tid] = nsecs - @start[tid];
        printf("slept for %d ms\n", $delta[tid] / 1000000); delete(@start[tid]); }'
stdin:2:61-62: ERROR: syntax error, unexpected =, expecting }
    kretprobe:do_nanosleep /@start[tid] != 0/ { $delta[tid] = nsecs - @start[tid];
                                                            ~
#
```

[公式リファレンスガイド][ref-guide]の例では，スレッド毎に時刻を分けて連想配列に記録することで，
特定のスレッドが該当関数を実行した時刻を算出している．
ただし，<code>tid</code>が出力されていないため，出力された時刻情報がどのプログラムのものであるかは，
下の例では不明である．
```
# bpftrace -e 'kprobe:do_nanosleep { @start[tid] = nsecs; }
     kretprobe:do_nanosleep /@start[tid] != 0/ {
         printf("slept for %d ms\n", (nsecs - @start[tid]) / 1000000); delete(@start[tid]); }'
Attaching 2 probes...
slept for 1000 ms
slept for 1000 ms
^C

@start[571]: 2438845069928

#
```

[公式リファレンスガイド][ref-guide]に示されているように，名無しの連想配列も可能である．
```
# bpftrace -e 'BEGIN { @[1,2] = 3; printf("%d\n", @[1,2]); clear(@); }'
Attaching 1 probe...
3
^C

#
```


<!-- 参考文献リスト -->
[ref-guide]: <https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md>  "公式リファレンスガイド"