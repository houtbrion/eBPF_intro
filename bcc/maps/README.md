# 各種Map
参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#maps


eBPFの環境でカーネル内のVM上で動作するプログラムから
ユーザ空間で動作するプログラムにデータを伝える方法として，
<code>bpf_trace_printk()</code>で文字列を渡す方法と，
<code>perf_submit()</code>で構造体データを
渡す方法を紹介した．
これらの方法では，監視対象のイベントが発生するたびに，
通知が発生し，ユーザ空間のアプリが動作してCPU負荷が上がるだけでなく，監視対象の
イベントの単位時間あたりの発生数が多いと，カーネルと
ユーザ空間のプログラムでデータをやり取りするための
バッファがあふれる可能性がある．

これに対して，MAP機能を使うと毎回のイベントのデータを必要としない
ようなeBPFのプログラムでは，MAPの特定のエントリをeBPFのプログラムで
更新しておき，周期的にユーザ空間のアプリからその表を読み取ることで
済むような用途では，CPU負荷やバッファ溢れの問題を回避することができる．

## BPF_TABLE および BPF_ARRAY
ここで対象となる機能:
- BPF_TABLE
- BPF_ARRAY
- map.lookup()
- map.increment()
- map.keys()

参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#1-bpf_table
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#3-bpf_array

本ディレクトリに収納している<a href="map_table_sample">map_table_sample</a>
で，
「<code>execve()</code>」がアプリから呼び出されたら，
eBPFのVM内のプログラムが実行され，eBPFプログラムで<code>execve</code>を呼び出した
プロセスのユーザ権限がrootか否かでテーブルをカウントアップする．

このプログラムの
C部分の先頭付近でのdefine文を有効にするか否かで<code>BPF_TABLE</code>と<code>BPF_ARRAY</code>
のいずれを用いるかを切り替えることができる作りになっている．
このような作りが可能であるのは，識別しているのがroot(uid=0)を
表のindex番号0とし，それ以外はすべてindex番号1としているので，
Arrayの場合とTableの場合でPython部分のプログラムが全く
同じにすることができるためである．

eBPFのC部分(12から18行目)で表を定義しているが，<code>BPF_TABLE</code>の場合の定義は<code>BPF_TABLE("array", uint32_t, long, uidcnt, 2)</code>で
表の型(array), キーの型(32bit非負整数), データの型(long型), 表の名前(uidcnt), キーの数を定義している．
<code>BPF_ARRAY</code>の場合は<code>
BPF_ARRAY(uidcnt, long, 2)
</code>
で表の名前とデータの型と要素の数だけを定義している．

このC部分(20から39行目)では，<code>execve()</code>を実行したプロセスのuidがrootか否かを判定し，
どちらの種類のユーザがexecveを呼び出したかをカウントするために，
表のエントリを<code>uidcnt.lookup(&idx)</code>で取得し，
存在すれば</code>uidcnt.increment(idx)
</code>でカウントアップしている．

Pythonから表にアクセスする前に，49行目の「<code>uidcnt = b.get_table("uidcnt")</code>」
でCで定義している表の名前を使って，Pythonの変数にマッピングしている．
あと，52行目以下の1秒に一回for文で表のキーを取得し(<code>uidcnt.keys()</code>)，各キーの値を読み出す(<code>val = uidcnt[k].value</code>)．
表の全部のキーの値を読みだしたあと， 表をクリア(<code>uidcnt.clear()</code>で表の各要素の値を0に)して1ラウンド終了となる．

### BPF_TABLEの表の型

なおサンプルプログラムでは，，<code>BPF_TABLE</code>で表の型として<code>"array"</code>を定義しているが，実際にどんな型が許されて，
その型によってどのような動作の違いがあるのかについて今のところ資料はないが，
bccのソースを検索すると以下のソースに定義が存在する．

- https://github.com/iovisor/bcc/blob/master/src/cc/frontends/clang/b_frontend_action.cc

上記ソースで<code>bool BTypeVisitor::VisitVarDecl(VarDecl *Decl)</code>の
部分で参考になる情報が存在する(以下に引用)．
```
    if (map_info_pos != std::string::npos) {
      std::string map_info = section_attr.substr(map_info_pos + 1);
      section_attr = section_attr.substr(0, map_info_pos);
      if (section_attr == "maps/array_of_maps" ||
          section_attr == "maps/hash_of_maps") {
        inner_map_name = map_info;
      }
    }

    bpf_map_type map_type = BPF_MAP_TYPE_UNSPEC;
    if (section_attr == "maps/hash") {
      map_type = BPF_MAP_TYPE_HASH;
    } else if (section_attr == "maps/array") {
      map_type = BPF_MAP_TYPE_ARRAY;
    } else if (section_attr == "maps/percpu_hash") {
      map_type = BPF_MAP_TYPE_PERCPU_HASH;
    } else if (section_attr == "maps/percpu_array") {
      map_type = BPF_MAP_TYPE_PERCPU_ARRAY;
    } else if (section_attr == "maps/lru_hash") {
      map_type = BPF_MAP_TYPE_LRU_HASH;
    } else if (section_attr == "maps/lru_percpu_hash") {
      map_type = BPF_MAP_TYPE_LRU_PERCPU_HASH;
    } else if (section_attr == "maps/lpm_trie") {
      map_type = BPF_MAP_TYPE_LPM_TRIE;
    } else if (section_attr == "maps/histogram") {
      map_type = BPF_MAP_TYPE_HASH;
      if (key_type->isSpecificBuiltinType(BuiltinType::Int))
        map_type = BPF_MAP_TYPE_ARRAY;
      if (!leaf_type->isSpecificBuiltinType(BuiltinType::ULongLong))
        error(GET_BEGINLOC(Decl), "histogram leaf type must be u64, got %0") << leaf_type;
    } else if (section_attr == "maps/prog") {
      map_type = BPF_MAP_TYPE_PROG_ARRAY;
    } else if (section_attr == "maps/perf_output") {
      map_type = BPF_MAP_TYPE_PERF_EVENT_ARRAY;
      int numcpu = get_possible_cpus().size();
      if (numcpu <= 0)
        numcpu = 1;
      table.max_entries = numcpu;
    } else if (section_attr == "maps/ringbuf") {
      map_type = BPF_MAP_TYPE_RINGBUF;
      // values from libbpf/src/libbpf_probes.c
      table.key_size = 0;
      table.leaf_size = 0;
    } else if (section_attr == "maps/perf_array") {
      map_type = BPF_MAP_TYPE_PERF_EVENT_ARRAY;
    } else if (section_attr == "maps/queue") {
      table.key_size = 0;
      map_type = BPF_MAP_TYPE_QUEUE;
    } else if (section_attr == "maps/stack") {
      table.key_size = 0;
      map_type = BPF_MAP_TYPE_STACK;
    } else if (section_attr == "maps/cgroup_array") {
      map_type = BPF_MAP_TYPE_CGROUP_ARRAY;
    } else if (section_attr == "maps/stacktrace") {
      map_type = BPF_MAP_TYPE_STACK_TRACE;
    } else if (section_attr == "maps/devmap") {
      map_type = BPF_MAP_TYPE_DEVMAP;
    } else if (section_attr == "maps/cpumap") {
      map_type = BPF_MAP_TYPE_CPUMAP;
    } else if (section_attr == "maps/xskmap") {
      map_type = BPF_MAP_TYPE_XSKMAP;
    } else if (section_attr == "maps/hash_of_maps") {
      map_type = BPF_MAP_TYPE_HASH_OF_MAPS;
    } else if (section_attr == "maps/array_of_maps") {
      map_type = BPF_MAP_TYPE_ARRAY_OF_MAPS;
    } else if (section_attr == "maps/sk_storage") {
      map_type = BPF_MAP_TYPE_SK_STORAGE;
    } else if (section_attr == "maps/sockmap") {
      map_type = BPF_MAP_TYPE_SOCKMAP;
    } else if (section_attr == "maps/sockhash") {
      map_type = BPF_MAP_TYPE_SOCKHASH;
    } else if (section_attr == "maps/cgroup_storage") {
      map_type = BPF_MAP_TYPE_CGROUP_STORAGE;
    } else if (section_attr == "maps/percpu_cgroup_storage") {
      map_type = BPF_MAP_TYPE_PERCPU_CGROUP_STORAGE;
    } else if (section_attr == "maps/extern") {
      if (!fe_.table_storage().Find(maps_ns_path, table_it)) {
        if (!fe_.table_storage().Find(global_path, table_it)) {
          error(GET_BEGINLOC(Decl), "reference to undefined table");
          return false;
        }
      }
      table = table_it->second.dup();
      table.is_extern = true;
    } else if (section_attr == "maps/export") {
      if (table.name.substr(0, 2) == "__")
        table.name = table.name.substr(2);
      Path local_path({fe_.id(), table.name});
      Path global_path({table.name});
      if (!fe_.table_storage().Find(local_path, table_it)) {
        error(GET_BEGINLOC(Decl), "reference to undefined table");
        return false;
      }
      fe_.table_storage().Insert(global_path, table_it->second.dup());
      return true;
    } else if(section_attr == "maps/shared") {
      if (table.name.substr(0, 2) == "__")
        table.name = table.name.substr(2);
      Path local_path({fe_.id(), table.name});
      Path maps_ns_path({"ns", fe_.maps_ns(), table.name});
      if (!fe_.table_storage().Find(local_path, table_it)) {
        error(GET_BEGINLOC(Decl), "reference to undefined table");
        return false;
      }
      fe_.table_storage().Insert(maps_ns_path, table_it->second.dup());
      return true;
    }
```

上のコードを見ると，<code>if</code>文の条件判定で，<code>section_attr == "maps/文字列"</code>
という表記が多数あり，この「文字列」の部分が<code>BPF_TABLE</code>の表の型として
利用可能なものにあたる．この部分はbccのバージョンが上がると変化する可能性もあるので，
ここでは，探す場所を紹介するに留める．

### PINNED_TABLE
また，公式リファレンスガイドでは<code>PINNED_TABLE</code>を用いることで/sysファイルシステムに
表が現れると記載しているが，手元の環境(ubuntu20.04LTS + カーネルバージョン5.4)では動かすことが
できなかった．

## BPF_HASH
- BPF_HASH
- map.lookup_or_try_init()
- map.delete()
- map.update()
- map.insert()

参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#2-bpf_hash


<code>BPF_HASH</code>は連想配列の一種で，一般的なプログラミング言語の連想配列はキーに
文字列を入れるのが一般的であるが，<code>BPF_HASH</code>の場合はメンバが文字列の構造体を用いるところに
注意が必要．

### 最も基本的な例
- BPF_HASH
- map.lookup_or_try_init()

BPF_HASHの非常にシンプルなプログラム例(
<a href="map_hash_simple">map_hash_simple</a>
)を本ディレクトリにおさめている．
このプログラムでは，<code>execve()</code>を実行したアプリのプログラム名をキーとして取り出し(29行目)，
そのアプリ名のプロセスが何回execveを実行したかを数えている(34から38行目)．
Cの部分におけるkeyの取り扱いやPython側でデータを取得する方法が<code>BPF_TABLE</code>や
<code>BPF_ARRAY</code>の場合と異なることに注意が必要．

まず，C言語部分17行目(<code>
BPF_HASH(uidcnt,struct key_t)
</code>)で表の宣言を行っている．
これは，<code>uidcnt</code>という<code>struct key_t</code>型の
キーを持つ表の定義である．23行目の<code>bpf_get_current_comm(&key.comm, sizeof(key.comm))</code>で
キーに<code>execve</code>を実行したプログラムの名前を取得する．
<code>value = uidcnt.lookup_or_try_init(&key, &zero)</code>で
取得したプログラムの名前をキーとして表のエントリを読み取るが，
エントリが存在しなければ変数<code>zero</code>の値(この場合は0)で
初期化する．最後に，取得した値(表のエントリ値もしくは初期化した値"0")を
ヘルパー関数(<code>map.increment()</code>)を用いず直接カウントアップしている(35から38行目)．

ユーザ空間で動作するPythonプログラムは，表のエントリを取得してソート(51行目)した上で
出力(52行目)し，53行目で表のデータをクリア(存在する要素の値を0に設定)している．

### <code>map.delete()</code>の利用
- map.delete()

本ディレクトリに収容したサンプルプログラム
<a href="map_hash_sample">map_hash_sample</a>
は，<code>map.delete()</code>を用いるプログラムで，Python側では表の
中身を変更することはなく，Cの側でカウントした回数が10を越えていたら(実際には11回目)エントリを消して
数え直すプログラムとなっている．

具体的には28行目から35行目の部分で，カウントアップした値が10より大きい(この場合11)になっていたら，
表のエントリ自体を削除する．
```
if (value) {
  // エントリが存在した(もしくは新規に作成できた)場合
  (*value)++;
  if ((*value) > 10 ) {
    // 該当キーの表のエントリを削除
    uidcnt.delete(&key);
  }
}
```

このプログラムを動作させると，あるプロセスが<code>execve()</code>を10回
実行するまでは正常に数えられ，12回目が1として数え直される動作をする．

### <code>map.update()</code>と<code>map.inset()</code>の利用
- map.update()
- map.insert()

最後の例は，<code>map.update()</code>と<code>map.inset()</code>を使う本ディレクトリの
<a href="map_hash_update">map_hash_update</a>
である．
この例は上のプログラムとまったく同じ結果となるが，mapの値の更新とエントリの
操作だけを変えている(28から40行目)．

```
    value = uidcnt.lookup_or_try_init(&key, &zero);
    if (value) {
        // エントリが存在した(もしくは新規に作成できた)場合
        long long val=*value;
        val++;
        uidcnt.update(&key,&val);      // 表のエントリを更新
        if (val > 10 ) {               // エントリの値が既定値を超えていた場合
            uidcnt.delete(&key);       // エントリを削除
            // 表のエントリを再度作成
            // これは無くても動くが，insetの練習のために追加 (lookup_or_try_init()が同じ働きをする)
            uidcnt.insert(&key,&zero); 
        }
    }
```
見比べるとわかるように，現在のエントリへのポインタを取得し，
値を別の変数に代入した上で，加算して<code>map.update()</code>で更新．
もし，値が10より大きくになっていた場合は上のプログラムと同じく
エントリを消去．このままでも，次回の実行時に34行目の<code>map.lookup_or_try_init()</code>で
消したエントリが再度作られるので問題ないが，この例ではわざわざ<code>map.inset()</code>で
エントリを値0で作ってから終了している．

## BPF_HISTOGRAM
- BPF_HISTOGRAM

参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#4-bpf_histogram

公式リポジトリで配布している以下のサンプルプログラムがわかりやすくておすすめ．
- https://github.com/iovisor/bcc/blob/master/examples/tracing/strlen_hist.py

元のサンプルプログラムは，コメントが非常に多いので，以下に冒頭の
コメント類を削除したものを示す．
```
#!/usr/bin/python

#
from __future__ import print_function
import bcc
import time

text = """
#include <uapi/linux/ptrace.h>
BPF_HISTOGRAM(dist);
int count(struct pt_regs *ctx) {
    dist.increment(bpf_log2l(PT_REGS_RC(ctx)));
    return 0;
}
"""

b = bcc.BPF(text=text)
sym="strlen"
b.attach_uretprobe(name="c", sym=sym, fn_name="count")

dist = b["dist"]

try:
    while True:
        time.sleep(1)
        print("%-8s\n" % time.strftime("%H:%M:%S"), end="")
        dist.print_log2_hist(sym + " return:")
        dist.clear()

except KeyboardInterrupt:
    pass
```

このプログラムでは<code>libc</code>の<code>strlen()</code>を監視し，
<code>strlen()</code>の返り値に対して<code>bpf_log2l()</code>で<code>log2</code>を計算し，
計算結果<code>x</code>を用いて
表のエントリをインクリメントする．
Python側では，1秒に一回ヒストグラムを出力する専用の関数<code>print_log2_hist()</code>を用いて
分布を出力した後，表の内容をクリア<code>clear()</code>する．

Pythonでヒストグラム用のテーブルを出力する関数には，log形式以外に線形形式で出力する
<code>print_linear_hist()</code>がある．これら2つをまとめたサンプルプログラムが
本ディレクトリの
<a href="map_hist">map_hist</a>
である．

このプログラムは，以下の文献を参考に，ブロックIOの書き込みデータ量をヒストグラムの表に
log形式と線形形式で別々に蓄積し，
同時に出力されるようにしたものである．

参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#1-trace_print
- https://books.google.co.jp/books?id=ihTADwAAQBAJ&pg=PT1010&lpg=PT1010&dq=kprobe__blk_account_io_completion&source=bl&ots=bySM8ABDIg&sig=ACfU3U01-2Ka06x1k_bC_5JBLk9Wdd6txA&hl=ja&sa=X&ved=2ahUKEwiR4MX--oDqAhWFGKYKHaq3Aj8Q6AEwAHoECAoQAQ#v=onepage&q=kprobe__blk_account_io_completion&f=false

このプログラムでは，10行目と11行目で線形形式用の表とlog2用の表の2つを定義している．
```
BPF_HISTOGRAM(dist);       // 線形の表
BPF_HISTOGRAM(log_dist);   // ログ形式の表
```
13行目でkprobeの<code>blk_account_io_completion()</code>にeBPFのプログラムを割り当てている．
```
int kprobe__blk_account_io_completion(struct pt_regs *ctx, void *req, unsigned int bytes)
```

15行目から17行目で引数として与えられた<code>bytes</code>をキロバイト単位に換算した上で，
両方の表を更新している．この際，log形式の表では<code>bpf_log2l()</code>
を用いてデータをログ形式に変換して更新していることに注意していただきたい．
```
// IOのデータサイズをキロバイト単位に変換した数値で更新
dist.increment(bytes / 1024);                 // 線形の表のエントリを更新
log_dist.increment(bpf_log2l(bytes / 1024));  // ログ形式の表のエントリを更新
```

Python側では両方の表を最後に出力しているが，線形形式の表は32行目からの部分で<code>print_linear_hist()</code>
を用いて出力している．
```
# 線形形式の表を出力
print("Linear histgram")
b["dist"].print_linear_hist("kbytes")
```

log形式の表は37行目からの部分で，<code>print_log2_hist()</code>を用いて出力している．
```
# ログ形式の表を出力
print("log2 histgram")
b["log_dist"].print_log2_hist("kbytes")
```

このサンプルプログラムを実際に動作させると以下のような出力が得られる．
```
# ./map_hist
Tracing block I/O... Hit Ctrl-C to end.
^C
Linear histgram
     kbytes        : count     distribution
        0          : 8        |********                                |
        1          : 0        |                                        |
        2          : 0        |                                        |
        3          : 0        |                                        |
        4          : 37       |****************************************|
        5          : 0        |                                        |
        6          : 0        |                                        |
        7          : 0        |                                        |
        8          : 4        |****                                    |
        9          : 0        |                                        |
        10         : 0        |                                        |
        11         : 0        |                                        |
        12         : 2        |**                                      |
        13         : 0        |                                        |
        14         : 0        |                                        |
        15         : 0        |                                        |
        16         : 1        |*                                       |
        17         : 0        |                                        |
        18         : 0        |                                        |
(中略)
        50         : 0        |                                        |
        51         : 0        |                                        |
        52         : 0        |                                        |
        53         : 0        |                                        |
        54         : 0        |                                        |
        55         : 0        |                                        |
        56         : 1        |*                                       |

log2 histgram
     kbytes              : count     distribution
         0 -> 1          : 8        |********                                |
         2 -> 3          : 0        |                                        |
         4 -> 7          : 37       |****************************************|
         8 -> 15         : 6        |******                                  |
        16 -> 31         : 1        |*                                       |
        32 -> 63         : 1        |*                                       |
#
```

## BPF_PERCPU_ARRAY
- BPF_PERCPU_ARRAY

参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#7-bpf_percpu_array

### 基本的なサンプルプログラム
<code>BPF_PERCPU_ARRAY</code>は監視対象のカーネル内の関数やsystem callが実行された
CPU毎に別のテーブルを用意するものであり，
本ディレクトリに格納してある
<a href="map_cpu_array_simple">map_cpu_array_simple</a>
がサンプルプログラムになっている．

このプログラムのC部分では，<code>
BPF_PERCPU_ARRAY(connect_cnt, long, 1)
</code>(13行目)により，<code>connect_cnt</code>という名前でlong型のデータを格納する
要素数1の表をCPU別に準備して，
<code>tcp_v4_connect()</code>が実行されると，
<code>bpf_trace_printk()</code>でユーザ空間に通知した上で，
該当CPUに対応した表の0番目の要素をカウントアップする．

Python側のプログラムでは，
<code>b.trace_fields()</code>(39行目)で
<code>bpf_trace_printk()</code>(17行目)の出力を受信して
それを出力する．
この際，38行目からの<code>try</code>文でキーボードインタラプトを受信すると
無限ループを抜け，<code>
connect_cnt.sum(0).value
</code>(51行目)で全CPU文のテーブルの0番目の要素の合計値を求めている．
最後に，<code>
val = connect_cnt.getvalue(0)
</code>で0番目の要素だけを集めた表を<code>val</code>
というPythonの変数に割り当てる．

また，
<code>
num_cpus = len(utils.get_online_cpus())
</code>
(62行目)で動作しているCPU数を取得して，64行目以降の
<code>
for
</code>文と
<code>
print("val[", i ,"] = ",val[i])
</code>でi番目のCPUで実行された
回数を表示させる．

このプログラムを2CPUの環境で実行すると以下のような出力が得られる．
```
# ./map_cpu_array_simple
PID    COMM         OUTPUT
1358   wget         start of tcp4connect
1358   wget         start of tcp4connect
1360   wget         start of tcp4connect
1360   wget         start of tcp4connect
1362   wget         start of tcp4connect
1362   wget         start of tcp4connect
1364   wget         start of tcp4connect
1364   wget         start of tcp4connect
^Ctcp connects total  8  times
val[ 0 ] =  6
val[ 1 ] =  2
#
```

### BPF_PERCPU_ARRAYの表をPythonからアクセスする場合の方法
<code>BPF_PERCPU_ARRAY</code>をPython側でアクセスする手法の
情報は公式リファレンスガイド等にも無く，
ソースを参照するしかない．
Pythonにおける．
<code>BPF_PERCPU_ARRAY</code>の定義(クラス名<code>PerCpuArray</code>)は以下のファイルに存在する．

- https://github.com/iovisor/bcc/blob/master/src/python/bcc/table.py

このクラス独自のメンバ関数の定義は以下の4種類存在し，
<a href="map_cpu_array_simple">上のサンプルプログラム</a>
では，
<code>
getvalue()
</code>と
<code>
sum()
</code>を利用している．

- <code>def getvalue(self, key)</code>
- <code>def sum(self, key)</code>
- <code>def max(self, key)</code>
- <code>def average(self, key)</code>

ただし，バージョンが変わると追加・削除があるかもしれないので
定期的に確認が必要．


## BPF_STACK_TRACE
- BPF_STACK_TRACE
- map.get_stackid()

参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#5-bpf_stack_trace

公式のサンプルプログラム:
- https://github.com/iovisor/bcc/blob/master/examples/tracing/mallocstacks.py


このプログラムでは，Pythonの以下のコードで<code>libc</code>の<code>malloc()</code>を
引数で与えた<code>pid</code>のプロセスが呼び出した場合に，eBPFのCのプログラムが
実行される．
```
b.attach_uprobe(name="c", sym="malloc", fn_name="alloc_enter", pid=pid)
```
Cのプログラムでは，<code>BPF_HASH</code>と<code>BPF_STACK_TRACE</code>の2種類の表を作って，
<code>int key = stack_traces.get_stackid(ctx, BPF_F_USER_STACK)</code>で
eBPFのプログラムが呼び出された時のユーザプログラムのstackのIDを取得し，
それを<code>BPF_HASH</code>のkeyとして利用し，該当keyに対応するエントリの値を
インクリメントしている．

Python側のプログラムは，99999999秒経過したか，Ctrl+Cを入力することで，
待ち状態から抜け出し，<code>BPF_HASH</code>の表のキーがstackの表のIDに対応しているため，
stack表から情報を抜き出して表示して終了する．

<a href="../OriginalSample/mallocstacks.py">公式のサンプルプログラム</a>
を手元の環境で/bin/shを対象に実行し，shellで数回lsコマンドを
実行すると以下のような結果が得られた．

```
# ./mallocstacks.py 1823
Attaching to malloc in pid 1823, Ctrl+C to quit.
^C672 bytes allocated at:
        __libc_malloc+0x0
120 bytes allocated at:
        __libc_malloc+0x0
        [unknown]
74 bytes allocated at:
        __libc_malloc+0x0
15 bytes allocated at:
        __libc_malloc+0x0
        [unknown]
9 bytes allocated at:
        __libc_malloc+0x0
        [unknown]
#
```

### 問題
stackの情報が取れたとして，これをどう使えば嬉しいことができるか不明なところ．スタックの中身まで見ることができめば，デバッグに役立つはずだが，
どうすれば実現できるか，リファレンスガイドの記述もほとんどない上に，サンプルプログラムを見てもよくわからない．

## BPF_PERF_ARRAY
- BPF_PERF_ARRAY
- map.perf_read()

参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#6-bpf_perf_array

公式サンプルプログラム
- https://github.com/iovisor/bcc/blob/master/tests/python/test_perf_event.py


使い方がわからない上に，サンプルプログラムが手元の環境ではうごかない．
もともと上記のサンプルプログラムは，セルフテスト用のプログラムで
読者の理解のためのものではない．
エラーで止まっていても理由がわからないため，少し改造したものを
本ディレクトリの
<a href="map_perf_array_simple">map_perf_array_simple</a>
として追加．

これ(改造版)を動かすと，以下のように「hardware events unsupported」と言われる．
```
$ sudo ./map_perf_array_simple
perf_event_open: No such file or directory
('error no = ', 2)
hardware events unsupported
s
----------------------------------------------------------------------
Ran 1 test in 0.649s

OK (skipped=1)
$
```

いろいろ調べたところ，
カーネルコンパイルの時に，以下の項目が有効になっていないといけない．
- CONFIG_PERF_EVENTS
- CONFIG_HW_PERF_EVENTS

手元のUbuntu20.04のカーネルでは，2番目の項目「CONFIG_HW_PERF_EVENTS」が
コンパイル時に無効になっている．

```
$ uname -a
Linux ebpf 5.4.0-33-generic #37-Ubuntu SMP Thu May 21 12:53:59 UTC 2020 x86_64 x86_64 x86_64 GNU/Linux
$ cd /boot
$ grep CONFIG_PERF_EVENTS config-5.4.0-33-generic
CONFIG_PERF_EVENTS=y
CONFIG_PERF_EVENTS_INTEL_UNCORE=y
CONFIG_PERF_EVENTS_INTEL_RAPL=m
CONFIG_PERF_EVENTS_INTEL_CSTATE=m
# CONFIG_PERF_EVENTS_AMD_POWER is not set
$ grep CONFIG_HW_PERF_EVENTS config-5.4.0-33-generic
$
```

検証に利用した環境のカーネルバージョン等はは以下のとおり．
```
# uname -a
Linux uvm2 5.6.0-1010-oem #10-Ubuntu SMP Thu Apr 30 08:44:50 UTC 2020 x86_64 x86_64 x86_64 GNU/Linux
#
```

カーネルのビルドに挑戦してみるも，CONFIG_HW_PERF_EVENTSはx86ではサポートしてていないらしく，
configで選択肢にでてこない．
そのためx86の環境では，「CONFIG_HW_PERF_EVENTS」のところがエラーとなり，テストが
1つスキップされるのが正常と思える．


物理サーバでやると20.04の公式版でも動く
```
noro@venus:~/devel/eBPF_intro/bcc/maps$ sudo ./map_perf_array_simple
/usr/lib/python3/dist-packages/bcc/__init__.py:296: DeprecationWarning: not a bytes object: '\nBPF_PERF_ARRAY(cnt1, NUM_CPUS);\nBPF_ARRAY(prev, u64, NUM_CPUS);\nBPF_HISTOGRAM(dist);\nint do_sys_getuid(void *ctx) {\n    u32 cpu = bpf_get_smp_processor_id();\n    u64 val = cnt1.perf_read(CUR_CPU_IDENTIFIER);\n\n    if (((s64)val < 0) && ((s64)val > -256))\n        return 0;\n\n    prev.update(&cpu, &val);\n    return 0;\n}\nint do_ret_sys_getuid(void *ctx) {\n    u32 cpu = bpf_get_smp_processor_id();\n    u64 val = cnt1.perf_read(CUR_CPU_IDENTIFIER);\n\n    if (((s64)val < 0) && ((s64)val > -256))\n        return 0;\n\n    u64 *prevp = prev.lookup(&cpu);\n    if (prevp)\n        dist.increment(bpf_log2l(val - *prevp));\n    return 0;\n}\n'
  text = _assert_is_bytes(text)
/usr/lib/python3/dist-packages/bcc/__init__.py:624: DeprecationWarning: not a bytes object: 'getuid'
  name = _assert_is_bytes(name)
/usr/lib/python3/dist-packages/bcc/__init__.py:639: DeprecationWarning: not a bytes object: 'do_sys_getuid'
  fn_name = _assert_is_bytes(fn_name)
/usr/lib/python3/dist-packages/bcc/__init__.py:665: DeprecationWarning: not a bytes object: 'do_ret_sys_getuid'
  fn_name = _assert_is_bytes(fn_name)
/usr/lib/python3/dist-packages/bcc/__init__.py:487: DeprecationWarning: not a bytes object: 'cnt1'
  name = _assert_is_bytes(name)
/usr/lib/python3/dist-packages/bcc/__init__.py:487: DeprecationWarning: not a bytes object: 'dist'
  name = _assert_is_bytes(name)
     value               : count     distribution
         0 -> 1          : 0        |                                        |
         2 -> 3          : 0        |                                        |
         4 -> 7          : 0        |                                        |
         8 -> 15         : 0        |                                        |
        16 -> 31         : 0        |                                        |
        32 -> 63         : 0        |                                        |
        64 -> 127        : 0        |                                        |
       128 -> 255        : 0        |                                        |
       256 -> 511        : 0        |                                        |
       512 -> 1023       : 99       |****************************************|
      1024 -> 2047       : 0        |                                        |
      2048 -> 4095       : 1        |                                        |
.
----------------------------------------------------------------------
Ran 1 test in 0.407s

OK
noro@venus:~/devel/eBPF_intro/bcc/maps$
```

### 問題
この機能の実用的な使い方が思い浮かばず，シンプルで実用的な例を実装するような
サンプルプログラムを作ることができないところ．

## BPF_PROG_ARRAY
- BPF_PROG_ARRAY
- map.call()

参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#9-bpf_prog_array

基本的にeBPFのVMにロードするプログラムはinline関数しか認められていないため，
サブルーチンのようなものを作ることができない．
また，1つの関数を2つのプログラムで監視するようなこともできないため，これを実現する
方法が表とtail callを用いて関数間で互いに呼び出す方法である
(ある意味，関数へのポインタのテーブルを用いて関数呼び出しをするようなイメージ)．

<code>BPF_PROG_ARRAY</code>はこのeBPFのtail call用の表を実現するためのもので，<code>map.call()</code>を
用いてmapの特定のエントリに登録されているCの関数を呼び出す．

### eBPFのVMにロードされる側のCプログラムのサンプル
元のサンプルプログラム
- https://github.com/iovisor/bcc/blob/master/examples/networking/tunnel_monitor/monitor.c

<a href="../OriginalSample/monitor.c">このプログラム</a>
は，カプセリングされたトンネルパケットを取り扱うためものプログラムで
出入り両方を見ている．
このプログラムでは，<code>parser</code>という名前の表(エントリ数10個)を作る．
あと，独立した2つの関数(<code>handle_outer()</code>と<code>handle_inner()</code>)を
定義している．
このうち，<code>handle_outer()</code>では，<code>parser.call(skb, 2);</code>で
<code>parser</code>という表の2番目のエントリに登録されている関数に引数<code>skb</code>を
与えて実行する．

```
BPF_PROG_ARRAY(parser, 10);
   :
(中略)
   :
// parse the outer vxlan frame
int handle_outer(struct __sk_buff *skb) {
   :
(中略)
   :
  parser.call(skb, 2);
   :
(中略)
   :
}

// Parse the inner frame, whatever it may be. If it is ipv4, add the inner
// source/dest ip to the key, for finer grained stats
int handle_inner(struct __sk_buff *skb) {
   :
(中略)
   :
}
```

### ユーザ空間側のPythonプログラム
元のサンプルプログラム:
- https://github.com/iovisor/bcc/blob/master/examples/networking/tunnel_monitor/monitor.py

<a href="../OriginalSample/monitor.py">このプログラム</a>
は，上のCのプログラムと連携して動作するユーザ空間側のプログラムである．
まず，<code>b = BPF(src_file="monitor.c", debug=0)</code>で，上のCプログラムを
eBPFのプログラムとしている．
```
b = BPF(src_file="monitor.c", debug=0)
```

<code>monitor.c</code>の中の
<code>handle_outer()</code>と<code>handle_inner()</code>を
eBPFのVMにロードする関数として定義している．

```
outer_fn = b.load_func("handle_outer", BPF.SCHED_CLS)
inner_fn = b.load_func("handle_inner", BPF.SCHED_CLS)
```
ただし，<code>load_func()</code>の第2引数にはプログラムの種類を
指定しているが，この値はカーネルソースのヘッダに定義されている．
```
/usr/src/linux-headers-5.4.0-37-generic/include/uapi/linux/bpf.h
```
具体的に定義されている部分は以下のようになっている．
```
enum bpf_prog_type {
        BPF_PROG_TYPE_UNSPEC,
        BPF_PROG_TYPE_SOCKET_FILTER,
        BPF_PROG_TYPE_KPROBE,
        BPF_PROG_TYPE_SCHED_CLS,
        BPF_PROG_TYPE_SCHED_ACT,
        BPF_PROG_TYPE_TRACEPOINT,
        BPF_PROG_TYPE_XDP,
        BPF_PROG_TYPE_PERF_EVENT,
        BPF_PROG_TYPE_CGROUP_SKB,
        BPF_PROG_TYPE_CGROUP_SOCK,
        BPF_PROG_TYPE_LWT_IN,
        BPF_PROG_TYPE_LWT_OUT,
        BPF_PROG_TYPE_LWT_XMIT,
        BPF_PROG_TYPE_SOCK_OPS,
        BPF_PROG_TYPE_SK_SKB,
        BPF_PROG_TYPE_CGROUP_DEVICE,
        BPF_PROG_TYPE_SK_MSG,
        BPF_PROG_TYPE_RAW_TRACEPOINT,
        BPF_PROG_TYPE_CGROUP_SOCK_ADDR,
        BPF_PROG_TYPE_LWT_SEG6LOCAL,
        BPF_PROG_TYPE_LIRC_MODE2,
        BPF_PROG_TYPE_SK_REUSEPORT,
        BPF_PROG_TYPE_FLOW_DISSECTOR,
        BPF_PROG_TYPE_CGROUP_SYSCTL,
        BPF_PROG_TYPE_RAW_TRACEPOINT_WRITABLE,
        BPF_PROG_TYPE_CGROUP_SOCKOPT,
};
```

最後に，tail call用の表(<code>parser</code>)に
<code>handle_outer()</code>と<code>handle_inner()</code>を登録する．
```
# using jump table for inner and outer packet split
parser = b.get_table("parser")
parser[c_int(1)] = c_int(outer_fn.fd)
parser[c_int(2)] = c_int(inner_fn.fd)
```

### kprobeで行う例
上記のサンプル(
<a href="../OriginalSample/monitor.c">C部分</a>と
<a href="../OriginalSample/monitor.py">Python側</a>)
では，分かりづらかったので，試行錯誤してkprobeで行う例を
作ってみた(本ディレクトリの
<a href="map_prog_array_simple">map_prog_array_simple</a>
)．
このプログラム自体は非常にわかりやすいもので，コメントも
大量に付いているので，読んで貰えればすぐに分かると思われるが，概要を説明しておく．

このプログラムは，Cの関数fooとbarを用意し，barは<code>BPF_PROG_ARRAY</code>
の先頭の要素を<code>call()</code>で呼び出す作りになっている．

Python側では，kprobeで
<code>execve()</code>を実行した場合にbarが呼び出されるように，
セットすると共に，
<code>BPF_PROG_ARRAY</code>の要素とて
fooを登録する．

<a href="map_prog_array_simple">このプログラム</a>
を動作させると，意図したとおり1回<code>execve()</code>が呼び出されるたびに，
関数<code>bar</code>が実行され，<code>bar</code>から<code>foo</code>が
呼び出されて実行されている．
```
# ./map_prog_array_simple
PID    COMM         OUTPUT
30814  bash         I'm bar
30814  bash         I'm foo
30815  <...>        I'm bar
30815  <...>        I'm foo
30816  sh           I'm bar
30816  sh           I'm foo
30816  bash         I'm bar
30816  bash         I'm foo
30817  <...>        I'm bar
30817  <...>        I'm foo
30821  lesspipe     I'm bar
30821  lesspipe     I'm foo
30825  <...>        I'm bar
30825  <...>        I'm foo
30826  less         I'm bar
30826  less         I'm foo
30827  sh           I'm bar
30827  sh           I'm foo
30827  bash         I'm bar
30827  bash         I'm foo
30828  <...>        I'm bar
30828  <...>        I'm foo
30832  <...>        I'm bar
30832  <...>        I'm foo
^C#
```


## 7. BPF_LPM_TRIE
- BPF_LPM_TRIE

参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#8-bpf_lpm_trie

リファレンスガイドによると，ロンゲストマッチを行うための表としか書いてなく，
表のキーとエントリの値のどちらに対してロンゲストマッチを行うのか不明確．

また，リファレンスガイドには，サンプルプログラムなどから「<code>BPF_LPM_TRIE</code>」を
検索する機能が付いているが，なにもヒットしない．

また，googleで検索しても以下の文献が<code>BPF_LPM_TRIE</code>をLinuxのiptablesの置き換えの実装の一部で
使っていますとしか書いていない．
そのため，何のためのもので，どうやって使うのか不明．

- https://mbertrone.github.io/documents/21-Securing_Linux_with_a_Faster_and_Scalable_Iptables.pdf

bccのリポジトリを<code>BPF_LPM_TRIE</code>でgrepすると，bccのセルフテストの
<a href="../OriginalSample/test_lpm_trie.py">プログラム</a>
が引っかかる．
- https://github.com/iovisor/bcc/blob/master/tests/python/test_lpm_trie.py

<a href="../OriginalSample/test_lpm_trie.py">この</a>うちの一部を下に引用する．
```
class KeyV4(ct.Structure):
    _fields_ = [("prefixlen", ct.c_uint),
                ("data", ct.c_ubyte * 4)]

@skipUnless(kernel_version_ge(4, 11), "requires kernel >= 4.11")
class TestLpmTrie(TestCase):
    def test_lpm_trie_v4(self):
        test_prog1 = """
        struct key_v4 {
            u32 prefixlen;
            u32 data[4];
        };
        BPF_LPM_TRIE(trie, struct key_v4, int, 16);
        """
        b = BPF(text=test_prog1)
        t = b["trie"]

        k1 = KeyV4(24, (192, 168, 0, 0))
        v1 = ct.c_int(24)
        t[k1] = v1

        k2 = KeyV4(28, (192, 168, 0, 0))
        v2 = ct.c_int(28)
        t[k2] = v2

        k = KeyV4(32, (192, 168, 0, 15))
        self.assertEqual(t[k].value, 28)

        k = KeyV4(32, (192, 168, 0, 127))
        self.assertEqual(t[k].value, 24)

        with self.assertRaises(KeyError):
            k = KeyV4(32, (172, 16, 1, 127))
            v = t[k]
```

C言語のeBPFのプログラムは以下部分のみで，構造体をキーとしてintの値を取る「<code>trie</code>」という
表を定義している．
```
struct key_v4 {
    u32 prefixlen;
    u32 data[4];
};
BPF_LPM_TRIE(trie, struct key_v4, int, 16);
```

この表に対して，以下のコードでPythonから値を代入している．１つ目は，
「192.168.0.0/24」のIPv4アドレスをキーとし，値として24のエントリを作成している．
```
k1 = KeyV4(24, (192, 168, 0, 0))
v1 = ct.c_int(24)
t[k1] = v1
```
2つ目のエントリとして，「192.168.0.0/28」のIPv4アドレスをキー，値が28を作成している．
```
k2 = KeyV4(28, (192, 168, 0, 0))
v2 = ct.c_int(28)
t[k2] = v2
```
次に，この表をPython側から読み出しており，最初は「192.168.0.15/32」をキー(<code>k</code>)として
表のエントリの値<code>t[k].value</code>を取得，その値が28かどうか確認している．
```
k = KeyV4(32, (192, 168, 0, 15))
self.assertEqual(t[k].value, 28)
```
「192.168.0.15/32」がキーの場合，最初に作成した2つのエントリのうち2番目のものが
ロンゲストマッチになるので，表のエントリの値は28となる．そのため，エントリの値の
検査は成功する．

同様に，以下のコードにより「192.168.0.127/32」をキーとして表のエントリの値を取得しているが，
この場合，「192.168.0.0/28」とは一致せず「192.168.0.0/24」が該当するため，エントリの値は24となる．
そのため，<code>assertEqual()</code>の検査も成功する．
```
k = KeyV4(32, (192, 168, 0, 127))
self.assertEqual(t[k].value, 24)
```

最後に，すべての表のエントリと一致しない検索を行い，エラーとなることを確認している．
```
with self.assertRaises(KeyError):
    k = KeyV4(32, (172, 16, 1, 127))
    v = t[k]
```

以上のことから，<code>BPF_LPM_TRIE</code>はIPアドレスのような場合は非常に便利な機能であることがわかる．
また， 元のセルフテストのプログラムは，IPv6の場合の使い方も示されており参考になる．

## BPF_ARRAY_OF_MAPS
- BPF_ARRAY_OF_MAPS

参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#13-bpf_array_of_maps
- https://prototype-kernel.readthedocs.io/en/latest/bpf/ebpf_maps.html

<code>BPF_ARRAY_OF_MAPS</code>は表を要素として持つ表であり，以下のURLのソースで定義されている．
- https://github.com/iovisor/bcc/blob/2fa54c0bd388898fdda58f30dcfe5a68d6715efc/src/cc/export/helpers.h#L291-L292

このソースを見るとわかるように，<code>BPF_ARRAY_OF_MAPS</code>は<code>BPF_TABLE</code>に変換するマクロとなっている．

また，2番目の参考文献を見ると，
<code>BPF_ARRAY_OF_MAPS</code>を操作する関数として以下の3つが紹介されている．
```
void bpf_map_lookup_elem(map, void *key. ...);
void bpf_map_update_elem(map, void *key, ..., __u64 flags);
void bpf_map_delete_elem(map, void *key);
```

この機能の公式サンプルプログラムとしては，以下のURLのものがあるが，本ディレクトリに
<a href="map_array_of_maps_sample">map_array_of_maps_sample</a>
という
サンプルプログラムを作り収容した．
- https://github.com/iovisor/bcc/blob/master/tests/python/test_map_in_map.py

 このプログラムはshとbashが<code>execve()</code>を実行した場合にeBPFのプログラムが実行され，
 rootユーザと一般ユーザのshが<code>execve()</code>を何回実行したか，bashも同じく
 rootか一般ユーザかで区別して勘定している．

 Pythonのプログラムの112から114行目でeBPFのプログラムを<code>execve()</code>を監視する指定を行っている．
 ```
 # eBPFのプログラムをexecveのkprobeに割当て
execve_fnname = b.get_syscall_fnname("execve")
b.attach_kprobe(event=execve_fnname, fn_name="syscall__execve")
```
次に，eBPFのCプログラムでは，18から21行目で<code>execve()</code>を実行したプロセス名を格納するための変数型を定義．
 ```
 // 監視対象のシステムコールを実行したプロセス名を格納するための変数型
struct key_t {
    char comm[TASK_COMM_LEN];
};
```
次に，23から26行目で<code>BPF_ARRAY_OF_MAPS</code>とその要素となる2つの表を定義している．
```
// 表の定義
BPF_TABLE("array", uint32_t, long, bashcnt, 2); // bash用の内部表
BPF_TABLE("array", uint32_t, long, shcnt, 2);   // shell用の内部表
BPF_ARRAY_OF_MAPS(map_array, "bashcnt", 2);     // 上記の2つの表を束ねる外部表
```
57行目で，<code>execve()</code>を実行したプロセス名を取得．
```
bpf_get_current_comm(&key.comm, sizeof(key.comm));
```

59行目から69行目で取得したプロセス名がbashもしくはshに一致するか否かを確認する．
この時の変数<code>idx</code>が表にアクセスするインデックスになる．
```
int  idx;
// execveを呼び出したプロセスがbashかshellか，それ以外かを判定
if (compare_with_bash(key.comm)) {
  idx = 0;
} else {
  if (compare_with_shell(key.comm)) {
    idx = 1;
  } else {
    return 0; // bashでもshellでもない場合は終了
  }
}
```
次に，71から78行目でプロセスのuidを取得し，rootかそれ以外かを識別．
```
    // execveを実行したプロセスのuidを取得
    u64 ugid = bpf_get_current_uid_gid();
    u32 uid = ugid; // 下32bitがuid
    // rootかそれ以外かに分別
    if (uid != 0) {
        uid = 1;
    }
```
80から83行目で<code>BPF_ARRAY_OF_MAPS</code>のインデックス番目の表を取得．インデックス(<code>idx</code>)の
値はshかbashかで異なっているため，sh用かbash用かの内部表を取得することができる．
```
void * inner_map;
// 外部表から内部表を取り出し
inner_map = map_array.lookup(&idx);
if (!inner_map) return 0;
```
最後に，85から94行目で内部表のuid番の要素を取得し，それをカウントアップする．
```
int *val;
long value;
// 内部表のuidに対応する要素を取得
val = bpf_map_lookup_elem(inner_map, &uid);
if (val) { // 取得した要素を更新
  value = *val + 1;
  // 更新処理本体． 末尾の引数は該当要素がなかった場合にどうするかなどを
  // 指定するフラグ(README.mdを参照)
  bpf_map_update_elem(inner_map, &uid, &value, BPF_ANY);
}
```
この際，内部表の要素を更新するため，93行目で<code>bpf_map_update_elem()</code>を
利用する．ここで，<code>bpf_map_update_elem()</code>の第4引数の意味は
リファレンスガイド等に記載がないが，カーネルソースのヘッダを読むと
定義と簡単な説明が書かれている．

手元の環境で確認すると，以下のファイルに記載がある．
```
/usr/src/linux-headers-5.4.0-42-generic/include/uapi/linux/bpf.h
```
その内容は以下の通り．
```
/* flags for BPF_MAP_UPDATE_ELEM command */
#define BPF_ANY         0 /* create new element or update existing */
#define BPF_NOEXIST     1 /* create new element if it didn't exist */
#define BPF_EXIST       2 /* update existing element */
#define BPF_F_LOCK      4 /* spin_lock-ed map_lookup/map_update */
```
サンプルプログラムでは，<code>BPF_ANY</code>が使われているが，これは
表の要素が存在すれば，それを更新し，存在しなければ要素を作る．

以上見たように，Cの側では3つの表を結びつける定義がないが，この操作は
Pythonの側(103から110行目)で行っている．

```
# 表の定義
bash_cnt = b.get_table("bashcnt")   # bash用の内部表
sh_cnt = b.get_table("shcnt")       # shell用の内部表
uid_cnt = b.get_table("map_array")  # 外部表の定義

#内部表を外側の表の要素として登録
uid_cnt[ct.c_int(0)] = ct.c_int(bash_cnt.get_fd())
uid_cnt[ct.c_int(1)] = ct.c_int(sh_cnt.get_fd())
```
上のコードの末尾2行にあるように，表のファイルディスクリプタを
取得し，その値を外部表の要素に代入している．

最後に表の内容を出力部分では，内部表のオブジェクトに対して，<code>keys()</code>
を与えて，すべての表のインデックス値を取り出し，<code>インデックスの要素.value</code>
で表の要素の値を取得している．
```
print("bashの結果:")
for k in bash_cnt.keys():    # 表のキーでループ
  val = bash_cnt[k].value  # 表のキーに対応する要素を取得
  i = k.value              # キーを値に変換
  if i == 0 :
    print("rootユーザ: {} 回execve呼び出し".format(val))
  else:
    print("一般ユーザ: {} 回execve呼び出し".format(val))
bash_cnt.clear()
```
<a href="map_array_of_maps_sample">このサンプルプログラム</a>
を動作させ，shとbashのウィンドウを
rootと一般ユーザの合計4つでlsを数を変えて実行した場合の出力が以下のものとなる．
```
# ./map_array_of_maps_sample
=================測定結果===================
bashの結果:
rootユーザ: 0 回execve呼び出し
一般ユーザ: 0 回execve呼び出し

shellの結果:
rootユーザ: 0 回execve呼び出し
一般ユーザ: 0 回execve呼び出し


=================測定結果===================
bashの結果:
rootユーザ: 1 回execve呼び出し
一般ユーザ: 2 回execve呼び出し

shellの結果:
rootユーザ: 3 回execve呼び出し
一般ユーザ: 4 回execve呼び出し


^C#
```

### 注意点

#### pythonでの内部表へのアクセス方法
<a href="map_array_of_maps_sample">このサンプルプログラム</a>で注意が必要なのが，shとbashの値を
保存している内部表にpython側プログラムでアクセスする手段として，
表の定義(103から106行目)で利用したオブジェクトをそのまま利用しているところである．
```
# 表の定義
bash_cnt = b.get_table("bashcnt")   # bash用の内部表
sh_cnt = b.get_table("shcnt")       # shell用の内部表
uid_cnt = b.get_table("map_array")  # 外部表の定義
```
リファレンスガイドやサンプルプログラムドキュメントを見ても，外部表の要素を取り出して，
それを内部表とて扱う方法が不明なため，このような作りとしている．
ちなみに，公式のサンプルプログラムも同じ方法で内部表にアクセスしている．

#### Cプログラムでの文字列比較
eBPFのVM内で<code>execve()</code>を実行したプロセス名を文字列"bash"もしくは"sh"と
一致するか否かを調べるために，inline関数2つを利用しているが，中をみると
非常に面倒な方法を使っている．これは，eBPFのVMでメモリをアクセスする場合に
セキュリティでブロックされることが多く，これをかいくぐるためにこのような
面倒な方法を使っている．

メモリの内容が一致しているか否かを判定するためのヘルパー関数があれば
多くの人が助かると思うが今のところ提供されていない．

## BPF_HASH_OF_MAPS
- BPF_HASH_OF_MAPS

参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#14-bpf_hash_of_maps


<code>BPF_HASH_OF_MAPS</code>は表を要素として持つハッシュ表であり，以下のURLのソースで定義されている．
- https://github.com/iovisor/bcc/blob/2fa54c0bd388898fdda58f30dcfe5a68d6715efc/src/cc/export/helpers.h#L294-L295

<code>BPF_ARRAY_OF_MAPS</code>と同じく<code>BPF_TABLE</code>に変換するマクロとなっている．

この機能を使うサンプルプログラムを本ディレクトリの
<a href="map_hash_of_maps_sample">map_hash_of_maps_sample</a>
に収容しているが，
このプログラムは上の<code>BPF_ARRAY_OF_MAPS</code>のサンプルプログラムと同じ機能を実現しており，
コードもほぼ同じである．

両者のプログラムのdiffをとったもののうち，コメントの違いなどのゴミ以外を除くと以下の部分だけが残る．
```
 // 表の定義
-BPF_TABLE("array", uint32_t, long, bashcnt, 2); // bash用の内部表
-BPF_TABLE("array", uint32_t, long, shcnt, 2);   // shell用の内部表
-BPF_ARRAY_OF_MAPS(map_array, "bashcnt", 2);     // 上記の2つの表を束ねる外部表
+BPF_ARRAY(bashcnt, long, 2);               // bash用の内部表
+BPF_ARRAY(shcnt, long, 2);                 // shell用の内部表
+BPF_HASH_OF_MAPS(map_array, "bashcnt", 2); // 上記の2つの表を束ねる外部表
```
これでサンプルプログラムが成立しているのは，内部，外部表ともに0,1のインデックスしか
使っていないため，ハッシュ，アレイ，テーブルの差がないためである．

<code>BPF_HASH_OF_MAPS</code>用のサンプルプログラム(
<a href="map_hash_of_maps_sample">map_hash_of_maps_sample</a>
)を実行した結果は以下の通り．

```
# ./map_hash_of_maps_sample
=================測定結果===================
bashの結果:
rootユーザ: 0 回execve呼び出し
一般ユーザ: 0 回execve呼び出し

shellの結果:
rootユーザ: 0 回execve呼び出し
一般ユーザ: 0 回execve呼び出し


=================測定結果===================
bashの結果:
rootユーザ: 1 回execve呼び出し
一般ユーザ: 2 回execve呼び出し

shellの結果:
rootユーザ: 3 回execve呼び出し
一般ユーザ: 4 回execve呼び出し


^C#
```

## 未作業
以下の部分はXDP専用の機能で動作の確認やサンプルプログラムの開発に
特殊な環境(複数の物理ネットワークインターフェースが必要など)
があるため，作業を保留している．

### 9. BPF_DEVMAP
- BPF_DEVMAP
- map.redirect_map()

xdpでネットワークインターフェースを示すエントリの表
https://github.com/torvalds/linux/blob/master/kernel/bpf/devmap.c


カーネルソース先頭のコメント
Devmaps primary use is as a backend map for XDP BPF helper call
bpf_redirect_map(). Because XDP is mostly concerned with performance we
spent some effort to ensure the datapath with redirect maps does not use
any locking. This is a quick note on the details.


https://github.com/iovisor/bcc/blob/master/examples/networking/xdp/xdp_redirect_map.py


XDPが動作する複数のネットワークインターフェースが存在する環境が必要

### 10. BPF_CPUMAP
- BPF_CPUMAP

- map.redirect_map()


CPUの番号とそのCPUに割当られたリングバッファのサイズの組み合わせを表す表で
XDP専用


- https://github.com/iovisor/bcc/blob/master/examples/networking/xdp/xdp_redirect_cpu.py

- https://lwn.net/Articles/736336/

複数のCPUが望ましい．


xdp_redirect_cpu

redirect_map()した結果，どうなるのが正しいのか不明．




### 11. BPF_XSKMAP
12. BPF_XSKMAP


map.redirect_map()
map.lookup()
参考文献
https://www.kernel.org/doc/html/latest/networking/af_xdp.html

https://gitlab.freedesktop.org/lima/linux/commit/fbfc504a24f53f7ebe128ab55cb5dba634f4ece8




