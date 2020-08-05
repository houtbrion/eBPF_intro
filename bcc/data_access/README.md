# データへのアクセス
|環境|動作|
|:--|:--|
|Ubuntu公式|△|
|CentOS公式|△|
|Ubuntu最新|○|

UbuntuとCentOSの公式はカーネルが5.5未満であるため，一部動かない機能がある．

カーネル内のさまざまなデータを取得するヘルパー関数の主要な物のリストが
リファレンスガイドの以下のURLに存在する．

参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#data
- https://github.com/iovisor/bcc/blob/master/docs/kernel-versions.md#helpers

これを表にまとめたものを下に示す．

|関数名 | サポートされたカーネルバージョン | 内容 |
|:------|------|:-------|
|bpf_ktime_get_ns()|4.1|カーネル内部時計(uptimeのようなもの)を取得|
|bpf_get_current_pid_tgid()|4.2|監視対象を引き起こした原因となっているプロセスのpidやtgidを取得|
|bpf_get_current_uid_gid()|4.2|監視対象を引き起こした原因となっているプロセスのuidやgidを取得|
|bpf_get_current_comm()|4.2|監視対象を引き起こした原因となっているプロセスの名前を取得|
|bpf_get_current_task()|4.8|監視対象を引き起こした原因となっているプロセスのいろいろな情報を構造体としてまとめて一回で取得|
|bpf_log2l()|不明|引数のlog2を計算|
|bpf_get_prandom_u32()|4.1|疑似乱数を取得|
|bpf_probe_read_user()|5.5|eBPF VM内の変数間でデータをコピーするために用いる関数|
|bpf_probe_read_user_str()|5.5|上記関数で文字列専用のもの|
|bpf_probe_read_kernel()|5.5||
|bpf_probe_read_kernel_str()|5.5||


表の関数すべてを使うサンプルプログラムが本ディレクトリの<a href="execve_simple">execve_simple</a>である．
このサンプルプログラムを元に，各ヘルパー関数の使い方を紹介する．

## bpf_ktime_get_ns()
この機能は，上の表にあるようにカーネル内部で保持するナノ秒単位の内部時計の時刻を取得する機能で，64bit非負整数の
値を取得することができる(24行目)．
```
u64 time = bpf_ktime_get_ns();
```
この時計は年月日や時分秒を表すものではなく，uptimeコマンドの出力に近いものである．
気をつける必要があるのは，この時計はクロックの進み具合を数えているため，
ntpを実行していると，時計のクロックの進み方が早くなったり，遅くなったりするため，
正確な時刻経過を示さない可能性があることである．

## bpf_get_current_pid_tgid()
このヘルパー関数は，eBPFで実行されるプログラムが呼び出される原因となったプロセスのPID，tgidを
取得する機能である．サンプルプログラム中の
26～28行目のように，64bit非負整数に2つの32bit非負整数がまぜられているため，
取得した64bitの整数を32bitの変数に代入して，下32bitを取り出してから，
元の64bit整数を32bitシフトして，上32bitを取り出している．
```
u64 pgid = bpf_get_current_pid_tgid();
u32 pid = pgid;
u32 tgid = pgid >> 32;
```

## bpf_get_current_uid_gid()
<code>bpf_get_current_uid_gid()</code>は，先程と同じく，原因となったプロセスの
uid,gidを取得する．これも64bit非負整数に2つのデータがまとめられているため，
30から32行目にあるように，下32bitをuidとして取り出し，上32bitをgidとして取得する．
```
u64 ugid = bpf_get_current_uid_gid();
u32 uid = ugid;
u32 gid = ugid >> 32;
```

## bpf_get_current_comm()
このヘルパー関数は，eBPFのプログラムが実行される原因となったプロセスの名前を取得する機能であり，
34,35行目がこのヘルパー関数の利用例となる．
まず，プロセス名を可能するための場所(文字列)を定義し，その場所に<code>bpf_get_current_comm()</code>
を用いてデータをコピーする．
```
char comm[TASK_COMM_LEN];
bpf_get_current_comm(&comm, sizeof(comm));
```

ここで，プロセス名の最大値が必要になるため，上で定数<code>TASK_COMM_LEN</code>を
利用しているが，この定数を利用可能にするため，
15行目でインクルードしている．
```
#include <linux/sched.h>
```

## bpf_get_current_task()
このヘルパー関数は，カーネルのスケジューラが管理するプロセスの構造体を取得するための
もので，サンプルプログラムの37行目がそれに相当する．
```
struct task_struct *t = (struct task_struct *)bpf_get_current_task();
```

この部分が実行されることで，eBPFのプログラムが実行される原因となった
プロセスをカーネルのスケジューラが管理しているデータを取得することが
できる．

参考までに，「<code>bpf_get_current_task()</code>」で取得される構造体の一部を以下に示す．
```
struct task_struct {
#ifdef CONFIG_THREAD_INFO_IN_TASK
	/*
	 * For reasons of header soup (see current_thread_info()), this
	 * must be the first element of task_struct.
	 */
	struct thread_info		thread_info;
#endif
	/* -1 unrunnable, 0 runnable, >0 stopped: */
	volatile long			state;
```
本サンプルプログラムでは，46行目で上記構造体のうち，<code>state</code>だけを利用している．
```
bpf_trace_printk("state of comm = %d\\n", t->state);
```

## bpf_get_prandom_u32()とbpf_log2l()
<code>bpf_get_prandom_u32()</code>と
<code>bpf_log2l()</code>は，eBPFのVM内で整数データを加工する場合に便利な機能を提供する．

<code>bpf_get_prandom_u32()</code>は，32bit非負整数の模擬乱数を生成する機能で，
39行目のように，引数無しで変数に代入すれば良い．

```
u32 rval = bpf_get_prandom_u32();
```

<code>bpf_log2l()</code>は，ある整数データのlog2を計算するもので，
<a href="execve_simple">サンプルプログラム</a>
の
48行目で利用している．下に引用するように，出力はintとなる．
```
bpf_trace_printk("randval = %u , log2l(randval) = %d\\n", rval, bpf_log2l(rval));
```


## bpf_probe_read_kernel()とbpf_probe_read_kernel_str()
これらのヘルパー関数は，カーネルバージョン5.5以降で利用可能な機能であり，
カーネルが保持するデータをeBPFのプログラム内で利用するため，一時的に
eBPFのメモリ空間にコピーするためのものである．
以前のバージョンでは，VMのメモリセキュリティ機構を
かいくぐるため，forは使わないとか，if文で事前にアクセスする対象の
アドレスを検査する等の対策を行っていたが，この関数を利用することで，
1行で可能となった．<a href="execve_simple">本サンプルプログラム</a>
では，
カーネルスケジューラの構造体が保持するpidとtgidを
51,52行目で取得している．
```
bpf_probe_read_kernel(&tgid, sizeof(tgid), &t->tgid);
bpf_probe_read_kernel(&pid, sizeof(pid), &t->pid);
```
<a href="execve_simple">本サンプルプログラム</a>では，bpf_probe_read_kernel_str()は用いていない(申し訳無い)．

## bpf_probe_read_user()とbpf_probe_read_user_str()
これらのヘルパー関数は，先程と同じくカーネルバージョン5.5以降で利用可能な機能であり，
eBPFのプログラムが保持するデータを別の領域にコピーするためのものである．
この機能も，VMのメモリセキュリティ機構を
かいくぐるための面倒なプログラミングが不要になった．
<a href="execve_simple">本サンプルプログラム</a>では，<code>execve</code>の
引数となっている，起動を要求されたプログラム名(文字列)を
32～35行目で文字列に格納している．
```
// 「execve」を実行したプロセスのプログラム名を取得
char comm[TASK_COMM_LEN];
bpf_get_current_comm(&comm, sizeof(comm));
```
その文字列を
<code>
bpf_probe_read_user()
</code>と
<code>
bpf_probe_read_user_str()
</code>の両方の方法で別の文字列の領域にコピー(
54行目から)している．
```
// 「execve」を実行したプロセスのプログラム名(文字列)を別の変数にコピー(構造体など一般的な方法)
char comm2[TASK_COMM_LEN];
if (0 == bpf_probe_read_user(comm2, sizeof(comm2), (void *) filename)) {
    bpf_trace_printk("comm2 = %s\\n", comm2);
} else {
    bpf_trace_printk("fail to exec : bpf_probe_read_user()\\n");
}
// 「execve」を実行したプロセスのプログラム名(文字列)を別の変数にコピー(文字列専用の方式)
char comm3[TASK_COMM_LEN];
int rst = bpf_probe_read_user_str(comm3, sizeof(comm3), (void *) filename);
```
上の例からわかるように，文字列ならどちらの方法でも取得できるが，アクセスする先が
文字列でない場合は，<code>
bpf_probe_read_user_str()
</code>を用いる必要がある．

## カーネルバージョンで機能を切り替える方法

現状，想定している利用環境は，Ubuntu20.04(カーネル5.4)とCentOS8系(カーネル4.18)に
加えて，Ubuntu20.04のカーネルだけをバージョンアップしたもの(v5.6)であるため，
カーネルバージョンが5.5以上，5.4，それ以下の3種類に分類し，5.5以上だけで
対応しているヘルパー関数を使う部分を有効にするか否かを切り替えている．

### カーネルバージョンの識別
カーネルバージョンの識別は74～88行目で行っており，Pythonのosモジュールを使って，
リリース番号の文字列を切り出し，それを小数点に変換して判別している．
```
# OSカーネルのバージョンを取得する関数
def kernelVersion():
    releaseNum = os.uname().release.split('.')
    release = float(releaseNum[0] + '.' + releaseNum[1])
    return release

# OSカーネルのバージョンで，eBPFの対応レベルを判定する関数
def ebpfLevel():
    num = kernelVersion()
    if  num >= 5.5 :
        return 2
    if  num >= 5.4 :
        return 1
    else:
        return 0
```

### 機能の切り替え
eBPFのVMに与えるC言語のプログラムは，<code>NEW_KERNEL</code>が
defineされているか否かで変化するため，
8から12行目でそのための定義部分を文字列として保持している．
```
# カーネルバージョンが5.5以上の時に，eBPFプログラムの
# 先頭に付加する「define文」
header = """
#define NEW_KERNEL
"""
```

次に，取得したカーネルバージョンが5.5以上の場合は，元々の
eBPFのプログラムであるCソースの文字列の先頭に先程の
define文の文字列を付加する
(90から94行目)ことで，機能を有効にしている．
```
# OSカーネルのバージョンで，5.5以降で利用可能な機能を有効にするか否かを切り替え
level = ebpfLevel()
if level >= 2 :
    bpf_text= header + bpf_text
```


