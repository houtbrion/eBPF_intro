# eBPMのVMからユーザ空間のプログラムへのデータ出力


## BPF_PERF_OUTPUTとperf_submit()
「最初の一歩」で説明したように，<code>bpf_trace_printk()</code>には問題があるため，より複雑なデータをeBPFのVMとユーザ空間のプログラム間でやり取りするための方法が，
<code>BPF_PERF_OUTPUT</code>と<code>perf_submit()</code>となる．これを本ディレクトリに収容したサンプル
プログラム
(<a href="perf_output_sample">perf_output_sample</a>)で説明する．
<a href="perf_output_sample">perf_output_sample</a>のリンクを別のウィンドウで開いて，そちらのプログラムソースの行番号を見ながら以下の説明を見ていただきたい．

このプログラムは，「はじめの一歩」のサンプルプログラムのeBPFのVMからユーザ空間に通知を上げるインターフェースを
なにかと問題が多い<code>bpf_trace_printk()</code>から安定的に使える<code>perf_submit()</code>に
変更したものである．

まず，<a href="perf_output_sample">perf_output_sample</a>の20行目で，ユーザ空間に通知するI/F(イベント名)として<code>events</code>を定義している．
```
BPF_PERF_OUTPUT(events);
```
また，36行目でユーザ空間に通知を上げている．
```
events.perf_submit(ctx, &data, sizeof(data));
```
ここで，<code>perf_submit()</code>の第2,3引数でユーザ空間へ通知するイベントの内容のデータを与えているが，
このデータはCの構造体となっており，型を13行目から17行目までで構造体を定義し，24行目で変数<code>data</code>を
定義，構造体の中身は26から33行目までで代入している．

26行目で構造体メンバ<code>time</code>にカーネル内の時刻，29と30行目で<code>execve</code>を実行したプロセスのPIDを
取得，33行目で<code>execve</code>を実行したプロセスの名前を取得し，構造体の各メンバに代入している．
なお，C関数内で各データを取得するために利用した関数については，データアクセスの章を参照していただきたい．

Python側では，57行目の「<code>b["events"].open_perf_buffer(print_event)</code>」でC側のBPF_PERF_OUTPUTで定義した名前をオブジェクトを開き，該当I/Fにデータが届いた時に
飛ぶ処理(このプログラムでは49行目からの<code>print_event()</code>)と結びつけている．

Pythonのメインルーチンでは無限ループを周り，66行目の「<code>b.perf_buffer_poll()</code>」で
データのインターフェースを読み取る．この時にデータが届いていた場合は，
57行目の定義に従い，「<code>print_event()</code>」が実行される．

<code>print_event()</code>では，51行目の「<code>event = b["events"].event(data)</code>」で
Cの20行目(<code>BPF_PERF_OUTPUT(events)</code>)の名前「<code>events</code>」をPython変数の<code>event</code>に
マッピングして，event変数の中身を読み出してSTDOUTに出力している．


## 実行例
<a href="perf_output_sample">perf_output_sample</a>を実行し，別のウィンドウでlsとlessを一回づつ実行した場合の出力は以下のとおり．
```
$ sudo ./perf_output_sample
TIME(s)            COMM             PID    MESSAGE
829305575433       bash             1273   Hello, perf_output!
832298483658       bash             1276   Hello, perf_output!
832299767695       less             1277   Hello, perf_output!
832300419786       sh               1278   Hello, perf_output!
832301221961       bash             1278   Hello, perf_output!
832301768421       lesspipe         1279   Hello, perf_output!
832302941546       lesspipe         1283   Hello, perf_output!
^C$
```
ここで，出力されているのは<a href="perf_output_sample">perf_output_sample</a>の53と54行目の部分に相当する．
```
printb(b"%-18ld %-16s %-6d %s" % (event.time, event.comm, event.pid,
       b"Hello, perf_output!"))
```

