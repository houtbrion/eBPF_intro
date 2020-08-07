# はじめの一歩

|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

下のプログラムは，本ディレクトリに収容しているサンプルプログラム(
<a href="printk_sample">printk_sample</a>)である．

```
 1:  #!/usr/bin/python3
 2:  # -*- coding: utf-8 -*-
 3:  
 4:  from bcc import BPF
 5:  from bcc.utils import printb
 6:  
 7:  # eBPFのVMにロードさせるプログラムのCコードを文字列として定義
 8:  bpf_text = """
 9:  #include <linux/sched.h>
10:  
11:  // 取得したデータを一時的に可能するための構造体定義
12:  struct data_t {
13:      u32 pid;
14:      char comm[TASK_COMM_LEN];
15:      u64 time;
16:  };
17:  
18:  int syscall__execve(struct pt_regs *ctx)
19:  {
20:      struct data_t data = {};
21:      // カーネル内部時刻を取得
22:      data.time = bpf_ktime_get_ns();
23:  
24:      // プロセスIDを取得
25:      u64 pgid = bpf_get_current_pid_tgid();
26:      data.pid = pgid;    // 下32bitがpid
27:  
28:      // execveを実行したプロセスの名前を取得
29:      bpf_get_current_comm(&data.comm, sizeof(data.comm));
30:  
31:      // 取得した情報をユーザ空間に通知
32:      bpf_trace_printk("start of execve time = %ld , pid = %u, comm = %s\\n",data.time, data.pid, data.comm);
33:      return 0;
34:  };
35:  """
36:  
37:  # 上で定義した文字列のCコードをeBPFのVMに与える
38:  b = BPF(text=bpf_text)
39:  
40:  # eBPFのVMのプログラムをkprobeのexecveに割付け
41:  execve_fnname = b.get_syscall_fnname("execve")
42:  b.attach_kprobe(event=execve_fnname, fn_name="syscall__execve")
43:  
44:  # ヘッダの出力
45:  print("%-6s %-12s %s" % ("PID", "COMM", "OUTPUT"))
46:  
47:  # eBPFのプログラムが出力する内容(bpf_trace_printk())を無限ループで受信して出力
48:  while 1:
49:          # Read messages from kernel pipe
50:          try:
51:              (task, pid, cpu, flags, ts, msg) = b.trace_fields()
52:          except ValueError:
53:              # Ignore messages from other tracers
54:              continue
55:          except KeyboardInterrupt:
56:              # Ctrl-Cが入力された場合はループから抜け出す
57:              exit()
58:  
59:          printb(b"%-6d %-12.12s %s" % (pid, task, msg))
```

Pythonに馴染みのある方は既にご存知かと思われるが，2行目でファイルのエンコーディングを
UTF-8に指定している．
```
 2:  # -*- coding: utf-8 -*-
 ```

次に，必要なpythonのライブラリをロードしている．
```
 4:  from bcc import BPF
 5:  from bcc.utils import printb
 ```
 ここでロードしているのは，eBPFを使うためのライブラリbccから
 BPFのVMとやり取りするためのモジュール(BPF)とeBPFのVMの出力を印字するために用いているprint文の拡張(<code>printb</code>)である．

eBPFのVMで実行するプログラムのソースコードをPythonの文字列(<code>bpf_text</code>)として8から35行目までで定義し，その文字列が
eBPFのソースであることを38行目で宣言している．これで，eBPFのプログラムをロードしたVMが変数<code>b</code>としてPythonプログラムから参照できる．
```
38:  b = BPF(text=bpf_text)
```

41行目でカーネル内のシステムコールのシンボルを変数<code>execve_fnname</code>に代入する．
```
41:  execve_fnname = b.get_syscall_fnname("execve")
```
また，eBPFのVMを<code>execve</code>を監視するカーネル内の<code>kprobe</code>モジュールに割り付けて，
なんらかのプロセスが<code>execve</code>を実行した場合に，<code>bpf_text</code>のC関数(ソース18行目の<code>syscall__execve</code>)が
実行される．ここは，eBPFの第一歩なのでkprobeの詳細については説明しない．具体的な中身について詳しく知りたい場合は，kprobeの章を参照のこと．
```
18:  int syscall__execve(struct pt_regs *ctx)
(中略)
42:  b.attach_kprobe(event=execve_fnname, fn_name="syscall__execve")
```

Pythonのメインルーチンは48行目以下の部分で，無限ループを回り，51行目の<code>b.trace_fields()</code>で32行目の<code>bpf_trace_printk()</code>の
出力を受信している．51行目を詳しく見ると，<code>trace_fields()</code>の出力が6個組となっていて，6個組の最後の<code>msg</code>が
<code>bpf_trace_printk()</code>の出力で，<code>pid</code>は<code>execve</code>を実行したプロセスのPID, <code>ts</code>が
eBPFのVMのコードが実行されたカーネルの内部時計の時刻(カレンダー時刻ではなく，uptimeに近いもの)，<code>cpu</code>が該当プロセスが
実行されていたCPU，<code>task</code>で該当プロセスのプロセス名が格納されている．
なお，詳細については，公式リファレンスガイドを参照のこと．
```
51:  (task, pid, cpu, flags, ts, msg) = b.trace_fields()
```

## bpf_trace_printk()
bpf_trace_printk()の使い方で注意が必要なのは，
C,C++の通常のprintf()とは異なり，引数は最大3個，かつ，文字列は3個のうち
1個しかダメという厳しい規制がある．
他にも，マルチスレッドの環境では2つのbpf_trace_printk()の出力が混じってしまう可能性があるなど，問題が多いが．
しかし，使いやすいことから簡易なプログラムを作る場合や，開発中のデバッグの際には多用される．


## 実行例
本ディレクトリに添付したサンプルプログラム(printk_sample)を実行し，同じホストの別の
ウィンドウでlsやlessを実行すると，以下のような出力が得られる．
下の出力のOUTPUT部分がeBPFのCのプログラムからの出力(<code>bpf_trace_printk()</code>部分)に相当する．
```
bash$ sudo ./printk_sample
PID    COMM         OUTPUT
1236   <...>        start of execve time = 242359092433 , pid = 1236, comm = bash
1237   <...>        start of execve time = 244982436391 , pid = 1237, comm = bash
1238   <...>        start of execve time = 245847801497 , pid = 1238, comm = bash
1241   bash         start of execve time = 250183089532 , pid = 1241, comm = bash
1243   less         start of execve time = 250191036820 , pid = 1243, comm = less
1244   <...>        start of execve time = 250192138879 , pid = 1244, comm = sh
1244   bash         start of execve time = 250193960378 , pid = 1244, comm = bash
1245   lesspipe     start of execve time = 250195431447 , pid = 1245, comm = lesspipe
1249   lesspipe     start of execve time = 250198258391 , pid = 1249, comm = lesspipe
^Cbash$
```

