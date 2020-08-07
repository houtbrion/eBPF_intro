# スクリプトの構成要素

<a href="../FirstStep">はじめの一歩</a>でも一部紹介したように，bpftraceのスクリプトは以下のような
構造になっている．
```
[
  インクルード文
]+
[
  BEGIN{
    [プログラム文;]+
  }
]
{
  パターン
  [/フィルタ文/]
  {
    [プログラム文;]+
  }
}+
[
  END{
    [プログラム文;]+
  }
]
```
ここでは，以下のリストの構文構成要素と一部のプログラム文の内容について説明するが，以下のリスト(構文上の登場順)と説明の順番は異なるのでご注意ください．
- インクルード文
- プローブ : 上の例の<code>BEGIN</code>,<code>END</code>や<code>パターン</code>に相当する部分
- フィルタ : 上の例の<code>/フィルタ文/</code>の部分
- アクションブロック : 上の例のカッコ<code>{</code>と<code>}</code>で囲まれた部分

プローブでは，監視対象とするイベントを定義しているため，監視対象のイベントが発生した場合に
フィルタでイベントの内容を検査して，アクションブロックを実行するか否かを判定する．
もし，判定結果が<code>true</code>であった場合，アクションが実行される．

## アクションブロック <code>{...}</code>
この部分の構文は基本的にC言語と同じになる．ただし，Cのすべての構文の構成要素が利用できるわけではない．
例えば，bpftraceのバックエンドで動くbccの制約になりますが，if文の<code>else if</code>がサポートされたのが，
つい最近であるため，使えない環境が多かったりする．
他にも，変数(これについては別の節で説明)で特殊な構文のもの(<code>@</code>や<code>$</code>で始まるもの)が存在
するので注意．

## インクルード文
[公式リファレンスガイド][ref-guide]では構文の中で紹介されていないが，bpftraceでもCと同じく
ヘッダファイルのインクルードが可能．
```
#include <なにか.h>
```
アクションブロックで説明したように，各アクションの中に出てくるCの変数を取り扱うために，
型定義のヘッダを読み込むためにこの構文が存在する．

## コメント
Cと同じく，以下の構文でbpftraceのスクリプト内にコメントを付加することが可能．
```
// single-line comment

/*
 * multi-line comment
 */
 ```

## フィルタ <code>/.../</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

[公式リファレンスガイド][ref-guide]では，以下のような2種類の例を紹介している．


### 監視対象関数の引数でアクションを実行するか否かを判定

この例では，<code>vfs_read()</code>の第2引数(読み取りデータのサイズ)が
16以下の場合にアクションが実行される．
```
# bpftrace -e 'kprobe:vfs_read /arg2 < 16/ { printf("small read: %d byte buffer\n", arg2); }'
Attaching 1 probe...
small read: 8 byte buffer
small read: 8 byte buffer
small read: 8 byte buffer
small read: 8 byte buffer
small read: 8 byte buffer
small read: 12 byte buffer
^C
```

### 監視対象関数を実行したプロセスの名前でアクションを実行するか否かを判定

次の例では，<code>vfs_read()</code>を実行したプロセスの名前がbashの場合だけ，
アクションを実行する．
```
# bpftrace -e 'kprobe:vfs_read /comm == "bash"/ { printf("read by %s\n", comm); }'
Attaching 1 probe...
read by bash
read by bash
read by bash
read by bash
^C
```

### 複合判定

フィルタの条件式は複合条件も可能．一例(本ディレクトリの
[filter_example.bt][filter_example.bt])を以下に示す．
```
//#define FILTER_SIZE 1024
//#define FILTER_SIZE 100

BEGIN{
  printf("I will watch vfs_read().\n");
  printf("Please input Ctrl-C to finish this program.\n");
}

kprobe:vfs_read
/ (comm == "bash") && (arg2 < FILTER_SIZE) /
{
  printf("%s read %d bytes\n", comm, arg2);
}

END{
  printf("\nterminating to watch vfs_read().\n");
}
```
まず，最初にコメントアウトされている定数定義のうち，<code>#define FILTER_SIZE 1024</code>を
生かして実行し，別ウィンドウのbashで<code>ls</code>を入力すると以下のような出力が得られる．
```
# bpftrace filter_example.bt
Attaching 3 probes...
I will watch execve() system call.
Please input Ctrl-C to finish this program.
bash read 1 bytes
bash read 1 bytes
bash read 1 bytes
bash read 1 bytes
bash read 256 bytes
bash read 728 bytes
bash read 28 bytes
bash read 64 bytes
bash read 616 bytes
^C
terminating to watch execve().


#
```
次に，2番目の定義<code>#define FILTER_SIZE 100</code>を有効にすると，以下のような
出力となる．この出力は，1番目の出力から100より大きい数字のデータを削除すると同じになる．
```
# bpftrace filter_example.bt
Attaching 3 probes...
I will watch execve() system call.
Please input Ctrl-C to finish this program.
bash read 1 bytes
bash read 1 bytes
bash read 1 bytes
bash read 1 bytes
bash read 28 bytes
bash read 64 bytes
^C
terminating to watch execve().


#
```

## 構造体の定義 : <code>struct</code>
bpftraceはアクションでCの構造体を取り扱う必要があるため，Cの構造体定義が利用可能．
[公式リファレンスガイド][ref-guide]では以下のようなサンプルが紹介されている．

```
struct nameidata {
        struct path     path;
        struct qstr     last;
        // [...]
};
```

ただし，Cの構造体定義構文の全てが使える訳ではないので注意が必要．

## 構造体メンバへのアクセス <code>-></code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

[vfs_open()の定義][vfs_open]を以下に引用．
```
extern int vfs_open(const struct path *, struct file *);
```
この第1引数(<code>arg0</code>>)は構造体へのポインタであり，そのメンバへのアクセスはC言語と同じく<code>-></code>を用いる．
[公式リファレンスガイド][ref-guide]の例を以下に引用する．
```
# cat path.bt
#include <linux/path.h>
#include <linux/dcache.h>

kprobe:vfs_open
{
	printf("open path: %s\n", str(((struct path *)arg0)->dentry->d_name.name));
}

# bpftrace path.bt
Attaching 1 probe...
open path: dev
open path: if_inet6
open path: retrans_time_ms
[...]
```

## 3項演算子
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

Cの3項演算子がbpftraceのアクションとして利用可能．
```
<条件式> ? <真の場合> : <偽の場合>
```
[公式リファレンスガイド][ref-guide]では，2種類の例が挙げられている．
1つめは，Cの条件式をそのまま用いるもの．
```
# bpftrace -e 'tracepoint:syscalls:sys_exit_read { @error[args->ret < 0 ? - args->ret : 0] = count(); }'
Attaching 1 probe...
^C

@error[11]: 24
@error[0]: 78
```
この例では，配列の要素を表す数字を3項演算子で計算する仕組みとなっており，<code>args->ret < 0</code>が成立していれば
アクセスする要素の番号は<code>args->ret</code>の符号を反転させた正の整数となり，そうでない場合は<code>0</code>となる．

次の例はビット操作の例で，<code>pid & 1</code>の一番下のビットを検査している．
```
# bpftrace -e 'BEGIN { pid & 1 ? printf("Odd\n") : printf("Even\n"); exit(); }'
Attaching 1 probe...
Odd
```
pidの最下位ビットが立っている(つまり1)の場合，<code>pid & 1</code>の値は1となり，最下位ビットが0の場合は0となる．
出力部分を見ると，真の場合が「奇数」を返し，偽の場合に「偶数」を返していることからも
条件式の計算結果が0の場合が偽で，それ以外が真となる(C言語と同じ)ことがわかる．

## if文
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

本ディレクトリに収容している[if_example.bt][if_example.bt]を以下に示すが，基本的にCのif文と同じ．
別のところで述べたように，bccやbpftraceのバージョンによっては<code>else if</code>が使えないかも
しれないので注意．
```
BEGIN{
  printf("I will watch vfs_read().\n");
  printf("Please input Ctrl-C to finish this program.\n");
}

kprobe:vfs_read
/ comm == "bash" /
{
  if (arg2 < 100) {
    printf("%s read %d bytes(x<100)\n", comm, arg2);
  } else if (arg2 < 300) {
    printf("%s read %d bytes(100<=x<300)\n", comm, arg2);
  } else {
    printf("%s read %d bytes(x>=300)\n", comm, arg2);
  }
}

END{
  printf("\nterminating to watch vfs_read().\n");
}
```
上の[サンプルを][if_example.bt]実行し，別のbashウィンドウでlsすると以下のような結果となる．
```
# bpftrace if_example.bt
Attaching 3 probes...
I will watch vfs_read().
Please input Ctrl-C to finish this program.
bash read 1 bytes(x<100)
bash read 1 bytes(x<100)
bash read 1 bytes(x<100)
bash read 1 bytes(x<100)
bash read 256 bytes(100<=x<300)
bash read 728 bytes(x>=300)
bash read 28 bytes(x<100)
bash read 64 bytes(x<100)
bash read 616 bytes(x>=300)
^C
terminating to watch vfs_read().


#
```

## <code>unroll()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

C++のプラグマとは異なり，構文として存在している．
<code>unroll(n) {...}</code>と表記すると，</code>....</code>の状態がn回実行すると元に戻る．

[公式リファレンスガイド][ref-guide]の例は実行回数が短すぎて効果がよくわからないので，
実行時間を少し延ばしたものを以下に示す．
```
# bpftrace -e 'kprobe:do_nanosleep { $i = 1; unroll(5) { printf("i: %d\n", $i); $i = $i + 1; } }'
Attaching 1 probe...
i: 1
i: 2
i: 3
i: 4
i: 5
i: 1
i: 2
i: 3
i: 4
i: 5
i: 1
i: 2
i: 3
i: 4
i: 5
i: 1
i: 2
i: 3
i: 4
i: 5
^C

#
```
上のように，5回実行されると変数<code>$i</code>の値が元の1に戻っている．

## インクリメントとデクリメント : <code>++</code> , <code>--</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

C/C++の場合と同じなので，説明は不要だと思われる．ただ，C/C++と異なり，
適用可能な多少がスカラーな関数だけでなく，mapやmapのあるエレメントにも
適用できる．
mapについては，別途説明するのでそちらを参照のこと．

以下に，[公式リファレンスガイド][ref-guide]の例を以下に示す．

### 変数
```
# bpftrace -e 'BEGIN { $x = 0; $x++; $x++; printf("x: %d\n", $x); }'
Attaching 1 probe...
x: 2
^C
#
```

### map
```
# bpftrace -e 'k:vfs_read { @++ }'
^C
@: 12807
#
```

### キーありのmap
```
# bpftrace -e 'k:vfs_read { @[probe]++ }'
^C
 @[kprobe:vfs_read]: 13369
#
```

## 配列 : <code>[]</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|×|
|Ubuntu最新|×|

[公式リファレンスガイド][ref-guide]の例はbpftaceを用い，ユーザアプリの
監視をするスクリプトであるにも関わらず，監視される側の情報が無いため，
非常に分かりづらい．
そのため，[公式リファレンスガイド][ref-guide]の例を少しアレンジしたものを
以下に示す．

まず，監視される側のプログラム[array_access.c][array_access.c]は以下の通りで，
bpftraceで監視されるのは関数<code>test_struct()</code>の第1引数．
```
#include <stdio.h>
#include <unistd.h>

struct MyStruct {
        int y[4];
};

void test_struct(struct MyStruct *var){
        printf(" y[] = %d, %d, %d, %d\n",
                        var->y[0],
                        var->y[1],
                        var->y[2],
                        var->y[3]
                        );
}

void main(){
        struct MyStruct myStruct;
        for (int i=0; i<4 ; i++) {
                myStruct.y[i]=i+1;
        }
        while (1) {
                for (int i=0; i<4 ; i++) {
                        test_struct(&myStruct);
                        sleep(1);
                }
        }
}
```

これに対して監視する側のスクリプト([array_access.bt][array_access.bt])は以下のもの．
```
struct MyStruct { int y[4]; }
uprobe:./array_access:test_struct
{
  printf("y[]= %d, %d, %d, %d\n",
    ((struct MyStruct *) arg0)->y[0],
    ((struct MyStruct *) arg0)->y[1],
    ((struct MyStruct *) arg0)->y[2],
    ((struct MyStruct *) arg0)->y[3]);
}
```
この監視スクリプトは，<code>test_struct()</code>の第1引数のメンバ<code>y</code>の
各要素をプリントアウトする．

まず，監視される側のプログラムを動かすと，<code>test_struct()</code>が
呼び出されるたびに，第1引数がダンプされる．
```
bash$ ./array_access
 y[] = 1, 2, 3, 4
 y[] = 1, 2, 3, 4
 y[] = 1, 2, 3, 4
 y[] = 1, 2, 3, 4
^C
bash$
```



監視する側のスクリプトの出力として以下のようなものが期待される．
```
 y[] = 1, 2, 3, 4
```

Ubuntu公式の環境では期待どおり動作するが，他の2つの環境は以下のような出力となり，配列のインデックスに関係なく0番目の要素ばかりが出力される．
```
# bpftrace array_access.bt
Attaching 1 probe...
y[]= 1, 1, 1, 1
y[]= 1, 1, 1, 1
y[]= 1, 1, 1, 1
y[]= 1, 1, 1, 1
^C

#
```

原因は今の所不明だが，bfptraceが怪しい．



## Integerのキャスト
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|


[公式リファレンスガイド][ref-guide]に整数のキャストの表が挙げられており，Cのユーザは一見して
意味がわかるはず．一応利用可能な型の一覧は以下にリストアップしておく．

- uint8
- int8
- uint16
- int16
- uint32
- int32
- uint64
- int64

上のキャストを利用することで，値を操作することができる．
下の例は[公式リファレンスガイド][ref-guide]のものであるが，1を左に16bitシフトしたものと，
そこから下16bitだけ取り出したものを同時に印刷するため，<code>0</code>と<code>2^16</code>が
並んでいる．
```
# bpftrace -e 'BEGIN { $x = 1<<16; printf("%d %d\n", (uint16)$x, $x); }'
Attaching 1 probe...
0 65536
^C
```

## ループ : <code>while</code>, <code>break</code>, <code>continue</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|×|
|CentOS公式|○|
|Ubuntu最新|○|


ループ命令のうちサポートされるているのは<code>while</code>文のみ．[公式リファレンスガイド][ref-guide]の例では，
<code>while</code>しか出てこないので，以下のサンプルスクリプト([loop_example.bt][loop_example.bt])で
説明する．
```
i:ms:100
{
  $i = 1;
  while ($i <= 200) {
    $x = (uint8) $i/3;
    $y = $i;
    printf("%d ", $i);
    $i++;
    if ($y == (int64) $x*3) {
      if ($y != 0) {
        printf(", ");
      }
      continue;
    }
    if ($i >= 100) {
      break;
    }
  }
  exit();
}
```
上のスクリプトのプローブ部分<code>i:ms:100</code>の意味は，100ms間隔でprobeが発火する．
この<code>i</code>は<code>interval</code>の省略形．
もし，probeが発火すると，<code>while</code>で200回ループするが，
3回に一回はコンマを出力し，カウンタ変数<code>$i</code>が100になっていると，ループを抜け出す．下がその実行例．

```
# bpftrace loop_example.bt
Attaching 1 probe...
1 2 3 , 4 5 6 , 7 8 9 , 10 11 12 , 13 14 15 , 16 17 18 , 19 20 21 , 22 23 24 , 25 26 27 , 28 29 30 , 31 32 33 , 34 35 36 , 37 38 39 , 40 41 42 , 43 44 45 , 46 47 48 , 49 50 51 , 52 53 54 , 55 56 57 , 58 59 60 , 61 62 63 , 64 65 66 , 67 68 69 , 70 71 72 , 73 74 75 , 76 77 78 , 79 80 81 , 82 83 84 , 85 86 87 , 88 89 90 , 91 92 93 , 94 95 96 , 97 98 99 , 100

#
```

## return文
|環境|動作|
|:--|:--|
|Ubuntu公式|×|
|CentOS公式|○|
|Ubuntu最新|○|


[公式リファレンスガイド][ref-guide]は説明だけで例がないので，以下に
サンプルスクリプト([return_example.bt][return_example.bt])を示す．
```
BEGIN{
  printf("I will watch vfs_read().\n");
  printf("Please input Ctrl-C to finish this program.\n");
}

kprobe:vfs_read
{
  if (comm!="bash") {
    return;
  }
  printf("%s read %d bytes\n", comm, arg2);
}

END{
  printf("\nterminating to watch vfs_read().\n");
}
```
上のスクリプトは<code>vfs_read()</code>を実行したのが，bash
である場合に，アクションが実行されるものであるが，通常はフィルタで
実現する．あくまで<code>return</code>を用いる例として同じ機能を
<code>if</code>文と<code>return</code>の組み合わせで実現している．
下は，このスクリプトの実行結果である．

```
# bpftrace return_example.bt
Attaching 3 probes...
I will watch vfs_read().
Please input Ctrl-C to finish this program.
bash read 1 bytes
bash read 1 bytes
bash read 1 bytes
bash read 1 bytes
bash read 256 bytes
bash read 728 bytes
bash read 28 bytes
bash read 64 bytes
bash read 616 bytes
^C
terminating to watch vfs_read().


#
```

## タプル : <code>( , )</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|×|
|CentOS公式|○|
|Ubuntu最新||

bpftraceではNタプル(Nは1以上)をサポートしている．
タプルのn番目を指定する場合は，<code>.</code>を用いる．
以下は[公式リファレンスガイド][ref-guide]の例である．
```
# bpftrace -e 'BEGIN { $t = (1, 2, "string"); printf("%d %s\n", $t.1, $t.2); }'
Attaching 1 probe...
2 string
^C

#
```
上のスクリプトにおいて，<code>$t</code>は<code>1,2,"string"</code>の3個組のタプルである．
<code>printf()</code>の中で<code>$t</code>の2番目の要素を<code>$t.1</code>
でアクセスし，3番目の要素が<code>$t.2</code>となる．

<!-- 参考文献リスト -->
[ref-guide]: <https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md >  "公式リファレンスガイド"
[filter_example.bt]: <filter_example.bt> "filter_example.bt"
[vfs_open]: <https://elixir.bootlin.com/linux/latest/source/fs/internal.h#L134> "vfs_open()"
[if_example.bt]: <if_example.bt> "filter_example.bt"
[array_access.bt]: <array_access.bt> "array_access.bt"
[array_access.c]: <array_access.c> "array_access.c"
[loop_example.bt]: <loop_example.bt> "loop_example.bt"
[return_example.bt]: <return_example.bt> "return_example.bt"