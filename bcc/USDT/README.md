# User Statically-Defined Tracing
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

User Statically-Defined Tracingは元々DTraceで導入された技術で，
ユーザアプリのソースコードに外部からアプリを監視するための機能を
実装しておく機能である．

## 参考文献
参考文献というか，オフィシャルの説明．
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#6-usdt-probes
- https://github.com/iovisor/bcc/blob/master/examples/usdt_sample/usdt_sample.md

USDTの元であるSun(現Oracle)が公開していた日本語ドキュメント
- https://docs.oracle.com/cd/E19253-01/819-0395/index.html

上記のOracleのドキュメントはググると見つかるが，Oracleの
公式サイトから手繰ってもたどり着けないので，
「なかったことになりつつあるのでは」と心配している．

## 参考文献の問題
リファレンスガイドも，exampleの説明もほとんど中身のある事が書いてないので，結局自分で勉強するしかない．初心者向けのドキュメントがどこにも無い．さらに，example配下のプログラムはサンプルプログラムがリンクするshared library内の関数を監視するものであるため，理解が難しい．
それに加えて，説明書に書いてある実行例(全部で4つ)のうち，1つは例の通りに
実行しても，動かない．

### 監視される対象のプログラムを起動．
参考文献の手順に従い，コンパイルした状態で監視される側のプログラムを起動する．
```
bash$ examples/usdt_sample/build/usdt_sample_app1/usdt_sample_app1 "pf" 1 30 10 1 50
Applying the following parameters:
Input prefix: pf.
Input range: [1, 30].
Calls Per Second: 10.
Latency range: [1, 50] ms.
You can now run the bcc scripts, see usdt_sample.md for examples.
pid: 20112
Press ctrl-c to exit.
```
ここで，プロセスID(20112)を記録．

### 監視する側のプログラムを起動
次に，別のshellで監視する側のプログラムをroot権限で起動するが，この際，引数に上でメモしたプロセスIDを与えると，説明文では以下のような出力が得られることになっている．

```
# python tools/argdist.py -p (プロセスID) -i 5 -C 'u:usdt_sample_lib1:operation_start():char*:arg2#input' -z 32
[11:18:29]
input
	COUNT      EVENT
	1          arg2 = pf_10
	1          arg2 = pf_5
	1          arg2 = pf_12
	1          arg2 = pf_1
	1          arg2 = pf_11
```

しかし，手元の環境で実行するとエラーで止まる．このエラーメッセージを見ると，監視対象のバイナリパスの
指定方法が違うと読めるが，公式サイトで配布されるプログラムを修正するのはためらわれたため，
追求していない．
```
# python tools/argdist.py -p 20112 -i 5 -C 'u:usdt_sample_lib1:operation_start():char*:arg2#input' -z 32
HINT: Binary path should be absolute.

USDT failed to instrument PID 20112
#
```

## 非常にシンプルな使い方の例
仕方がないので，DTraceの参考文献をいろいろググって自分でアプリ(本ディレクトリの<a href="target-sample.c">target-sample.c</a>)と監視用の
プログラム(<a href="probe-sample">probe-sample</a>)を作ってみた．

### 監視されるアプリ
```
#include <stdio.h>
#include <sys/sdt.h>
#include <unistd.h>
void main(){
        unsigned int counter=0;
        pid_t pid=getpid();
        printf("pid = %d\n",pid);
        while(1) {
                counter++;
                printf("counter=%d\n",counter);
                // probeの引数として2つあげるので，DTRACE_PROBE2を使う
                DTRACE_PROBE2(target-sample, test-probe, counter, "hello");
                sleep(1);
        }
}
```
元々のDTraceではDTrace用の観測スクリプトを作成し，そのスクリプトを
ツールに与えて生成されたインクルードファイルをアプリのソースに
インクルードさせてからコンパイルする必要があるが，eBPFの場合は
そのような手順は不要で，上のアプリをgccを通すだけでコンパイルと実行が
可能となる．
```
bash$ gcc -o target-sample target-sample.c
bash$ ./target-sample
pid = 2698
counter=1
^Cbash$
```

アプリのソースの12行目を見ると，2引数のDTraceの関数を呼び出し，
第一引数非負整数，第二引数に文字列を与えている．
```
DTRACE_PROBE2(target-sample, test-probe, counter, "hello");
```

### 監視するアプリ

このプログラムのDTraceの部分を追いかけるeBPFのプログラム(本ディレクトリの<a href="probe-sample">probe-sample</a>)は
29行目の「<code>u = USDT(pid=int(pid))</code>」で特定のPIDのプロセスに埋め込まれた
Dtraceのトレースポイントを追いかけることを指定．次の行の「<code>u.enable_probe(probe="test-probe", fn_name="do_trace")</code>」で「test-probe」という名前トレースポイントが実行された
場合に，eBPFの<code>do_trace()</code>関数が呼び出されるよう指示している．
最後に，35行目でCのコードとUSDTの変数を与えて，eBPFを有効にしている．

メインルーチンでは，<code>bpf_trace_printk()</code>の出力を無限ループで読み取り続ける
仕組みとなっている．

以上のようなプログラムのうち，監視される側のアプリを動作させると，
以下のような出力となる．

```
bash$ ./target-sample
pid = 1627
counter=1
counter=2
counter=3
counter=4
counter=5
counter=6
counter=7
counter=8
counter=9
counter=10
counter=11
counter=12
counter=13
counter=14
counter=15
counter=16
counter=17
^Cbash$
```

アプリ動作中に監視する側のeBPFのプログラムを動作させる(プロセスID
を引数として与える)
と以下のような出力となる．

```
# ./probe-sample 1627
2707.557453000     target-sample    1627   counter:11  text:hello
2708.557558000     target-sample    1627   counter:12  text:hello
2709.557669000     target-sample    1627   counter:13  text:hello
2710.557950000     target-sample    1627   counter:14  text:hello
2711.559205000     target-sample    1627   counter:15  text:hello
2712.560740000     target-sample    1627   counter:16  text:hello
^C#
```

### アプリに埋め込むDTrace関数
アプリに埋め込むUSDTのプローブ関数の定義がわからないといけないので，
手元の環境(Ubuntu20.04)でインクルードファイルを探してみた．
```
bash$ cd /usr/include
bash$ find . -name sdt.h -print
./x86_64-linux-gnu/sys/sdt.h
bash$
```
該当のインクルードファイルを見てみると，以下のように定義されていた．
```
#define DTRACE_PROBE(provider,probe)            \
  STAP_PROBE(provider,probe)
#define DTRACE_PROBE1(provider,probe,parm1)     \
  STAP_PROBE1(provider,probe,parm1)
#define DTRACE_PROBE2(provider,probe,parm1,parm2)       \
  STAP_PROBE2(provider,probe,parm1,parm2)
(中略)
#define DTRACE_PROBE12(provider,probe,parm1,parm2,parm3,parm4,parm5,parm6,parm7,parm8,parm9,parm10,parm11,parm12) \
  STAP_PROBE12(provider,probe,parm1,parm2,parm3,parm4,parm5,parm6,parm7,parm8,parm9,parm10,parm11,parm12)
```
これから，わかるようにUSDTのトレースポイントを通過したことだけを見るためのものが，<code>DTRACE_PROBE()</code>で
<code>DTRACE_PROBE数字()</code>の数字部分が引数の数となっている．
そのため，
監視プログラムに通知する内容(引数の数)に合わせて，PROBE関数を選べば良い．
