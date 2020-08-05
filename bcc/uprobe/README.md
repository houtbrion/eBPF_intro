# ユーザアプリの監視(uprobe/uretprobe)
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|


参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#4-uprobes
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#4-attach_uprobe
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#10-bpf_probe_read_user
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#11-bpf_probe_read_user_str
- https://github.com/iovisor/bcc/blob/master/examples/tracing/strlen_snoop.py

eBPFには，カーネル内部関数の開始と終了を追いかけるための機能としてkprobe/kretprobeが
存在するが，これと同様に名前や引数が既知の
ユーザアプリ内部関数の実行と終了を監視する機能がuprobeとuretprobeである．

## 監視対象の指定方法
eBPFの
kprobe/kretprobeは以下の方法でkprobe/kretprobeの監視対象の指定ができる．
```
int kprobe__監視対象関数名(struct pt_regs *ctx) {

}
int kretprobe__監視対象関数名(struct pt_regs *ctx) {

}
```
それに対して，
uprobe/uretprobeはeBPFのVMに与えるCコードで割り付ける場所を指定する方法は
提供されていないため，Python側で定義する必要がある．

### 監視対象が通常のアプリのバイナリである場合
```
bpf_test="Cコードの文字列"  # VMにロードするプログラムのソースを文字列として定義
b = BPF(text=bpf_text)     # 該当文字列がeBPFのVMに与えるものであることを宣言
b.attach_uprobe(name="プログラムのバイナリファイルのパス名", sym="foo", fn_name="printarg") # 監視対象プログラム中の関数名fooが呼び出された場合にeBPFのVMでprintarg()が実行されるべきものであることを指示
```
上は，監視対象関数(<code>foo</code>)が呼び出された場合に，eBPFのプログラム(文字列)のprintarg()を実行するように指定する(uprobe)場合であるが，実行の終わりを検出する場合は，以下のようにuretprobeとして定義する．
```
b.attach_uretprobe(name="プログラムのバイナリファイルのパス名", sym="foo", fn_name="printarg")
```

### 共有ライブラリ内の関数を監視する場合
一方，次に示すように記載することで，ダイナミックリンクライブラリ内の関数呼び出しが発生した場合をキャッチすることができる．
```
b.attach_uprobe(name="c", sym="strlen", fn_name="printarg") # libcのstrlenが呼び出された場合にeBPFのVMが実行されるべきものであることを指示
```

## ユーザアプリの監視

本ディレクトリに収容した「<a href="target-sample.c">target-sample.c</a>」は監視される側のアプリケーションのソースコードであるが，
大まかに以下のような構造となっており，ルインルーチンの無限ループから1秒に一回，関数「<code>func()</code>」を
呼び出し，「<code>func()</code>」は引数として与えられた数値をそのまま返り値として返す単純なプログラム．
```
unsigned int func(unsigned int counter) {
        return counter;
}

void main(){
        (中略)
        while(1) {
                counterをインクリメント;
                unsigned int ret=func(counter);
                printf("ret=%d\n",ret);
                1秒待機;
        }
}
```

そのアプリを監視する側のプログラムが「<a href="uprobe_simple">uprobe_simple</a>」である．
このプログラムでは，上記のソースをコンパイルした
バイナリファイルのパス名，監視対象の関数名<code>func</code>，監視対象の
関数実行(開始/終了)時に呼び出されるeBPFのプログラム(関数)<code>printarg()</code>
<code>printret()</code>を32行目と33行目で
<code>attach_uprobe()</code>と<code>attach_uretprobe()</code>を用いて指定している．

```
b.attach_uprobe(name="バイナリファイルのパス名", sym="func", fn_name="printarg")
b.attach_uretprobe(name="バイナリファイルのパス名", sym="func", fn_name="printret")
```
実際に使う際には，自分の環境に合わせてバイナリファイルのパス名は変更が必要．

ユーザアプリの監視対象関数(<code>func()</code>)の実行開始時に呼び出されるeBPFの
プログラムは，以下のようになっており，<code>PT_REGS_PARM1(ctx)</code>で
監視対象関数の第一引数が存在しなければそのまま終了し，引数が存在すれば，
引数の値(32bit非負整数)を印字する<code>bpf_trace_printk()</code>が実行される．
```
int printarg(struct pt_regs *ctx) {
    if (!PT_REGS_PARM1(ctx))
        return 0;
    bpf_trace_printk("arg = %u\\n", PT_REGS_PARM1(ctx));
    return 0;
};
```
同様に，監視対象の実行終了時も<code>PT_REGS_RC(ctx)</code>で返り値にアクセスし，
返り値が存在すれば，32bit非負整数を<code>bpf_trace_printk()</code>で
ユーザ空間に送る仕組みとなっている．
```
int printret(struct pt_regs *ctx) {
    if (!PT_REGS_RC(ctx))
        return 0;
    bpf_trace_printk("return value = %u\\n", PT_REGS_RC(ctx));
    return 0;
};
```

### 引数や返り値の参照
返り値の参照に関しては，kretprobeと同じ仕組みであるためここでは説明しないが，引数の
参照はkprobeと異なる仕組みを用いているため，簡単に説明する．

kprobeの例として収容しているサンプルプログラム<a href="../kprobe/tcpv4connect_args">tcpv4connect_args</a>では，
監視対象関数実行開始時に引数を参照する場合，参照する引数を監視する側の関数の引数として
定義している．
```
int kprobe__tcp_v4_connect(struct pt_regs *ctx, struct sock *sk, struct sockaddr *uaddr, int addr_len)
{
    (中略)
};
```

これに対してuprobeでは，監視される側の引数は特に定義せずに，
<code>PT_REGS_PARM1(ctx)</code>で第一引数を参照している．
<code>PT_REGS_PARM1()</code>はヘルパー関数として定義されているが，
バージョンによって定義されているファイルの場所が異なる．

- v5.4まで : カーネルソースベースディレクトリ/tools/testing/selftests/bpf/bpf_helpers.h
- v5.5以降 : カーネルソースベースディレクトリ/tools/lib/bpf/bpf_tracing.h

このヘルパー関数はCPUアーキやカーネルバージョンでいろいろ定義が変化しているが，
よく使われるのは以下の6つ．
最初の5つが，引数を参照するもので，数字の部分が第○引数を
参照する場合の○部分に相当している．
最後が，kretprobeやuretprobeで使っている，返り値を参照するもの．
```
PT_REGS_PARM1(x)
PT_REGS_PARM2(x)
PT_REGS_PARM3(x)
PT_REGS_PARM4(x)
PT_REGS_PARM5(x)
PT_REGS_RET(x)
```
この定義からわかるように，ヘルパー関数を用いて引数を見る場合，5番目以降の引数は
参照できない．

### 実行例
2つのshellウィンドウで監視対象のアプリと監視するeBPFプログラムを同時に動作させる．
どちらのプログラムを先に動作させても構わないが，アプリで発生した全ての
イベントをeBPFで掴むためには，先にeBPFのプログラムを動作させた方が良い．

以下は，アプリの動作ログであるが，「<code>ret=x</code>」の部分が
監視対象関数<code>func</code>の返り値(引数も同じ値)を表している．
```
$ ./target-sample
pid = 2562
ret=1
ret=2
ret=3
ret=4
^C
$
```

この動作を監視した結果の出力が以下の通りで，アプリのPIDが2562，
監視対象のプロセス名が「<code>target-sample</code>」で
引数と返り値を時刻付きで出力される．
```
# ./uprobe_simple
TIME(s)            COMM             PID    MESSAGE
51859.473296000    b'target-sample' 2562   b'arg = 1'
51859.473329000    b'target-sample' 2562   b'return value = 1'
51860.473877000    b'target-sample' 2562   b'arg = 2'
51860.473968000    b'target-sample' 2562   b'return value = 2'
51861.498029000    b'target-sample' 2562   b'arg = 3'
51861.498120000    b'target-sample' 2562   b'return value = 3'
51862.552934000    b'target-sample' 2562   b'arg = 4'
51862.552957000    b'target-sample' 2562   b'return value = 4'
^C#
```


## libcの関数が利用されたことをキャッチするシンプルなプログラム
本ディレクトリに収容しているプログラム
(<a href="strlen_snoop_simple">strlen_snoop_simple</a>)
は，参考文献にも上げたeBPFの開発環境であるbccのサンプルプログラム(<a href="../OriginalSample/strlen_snoop.py">strlen_snoop.py</a>)から
基本的な部分だけを残したものである．

このプログラムの24行目でlibcのstrlenが実行開始された場合に，8から18行目で定義されているCの関数<code>printarg()</code>が実行される．
```
b.attach_uprobe(name="c", sym="strlen", fn_name="printarg")
```

こちらの例でも，引数を参照するために，
<code>PT_REGS_PARM1(ctx)</code>を利用している．
stlenのmanページを参照するとわかるように，<code>strlen()</code>は
<code>size_t strlen(const char *s);</code>であるので，
第一引数が<code>char</code>の配列への
ポインタである．strlen_snoop_simpleでは，14行目でこの配列を<code>PT_REGS_PARM1(ctx)</code>で取得，そのサイズを
<code>sizeof()</code>で算出し，<code>bpf_trace_printk()</code>でユーザ空間へ通知している．

このプログラムを実行し，別のshellでこのディレクトリをlsすると，下のような結果が得られる．

```
$ sudo bash
# ./strlen_snoop_simple
TIME(s)            COMM             PID    MESSAGE
6152.677566000     b'ls'            1572   b'length of argument string = 8.'
6152.677668000     b'ls'            1572   b'length of argument string = 8.'
^C#
```



