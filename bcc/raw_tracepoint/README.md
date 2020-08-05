# Rawトレースポイント

|環境|動作|備考|
|:--|:--|:--|
|Ubuntu公式|△|<code>attach_raw_tracepoint()</code>が動かない|
|CentOS公式|△|<code>attach_raw_tracepoint()</code>が動かない|
|Ubuntu最新|△|<code>attach_raw_tracepoint()</code>が動かない|



## 参考文献
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#7-raw-tracepoints
- https://lwn.net/Articles/748352/

tracepointとraw_tracepointの違いが明確に記載されているドキュメントが
発見できないが，以前ちらりと見たドキュメント(どこに書いてあったか
わからなくなった)には，「普通のtracepointの方がいろいろな
ヘルパーfunctionを通してからVM内のコードが走るから多少
CPU処理量が多い」という記載を見た．

これが正しいとすると，実行時間を極力削りたい
用途では，raw_tracepointを使った方が有利と言える．

## ヘルパー関数
Referenceガイドに記載がないのは不親切だが，ユーザが使っている
カーネルがraw_tracepointをサポートしているか否かを判定するヘルパー関数が
提供されており，本ディレクトリの<a href="check_raw_tracepoint">check_raw_tracepoint</a>はカーネルの機能をチェックするだけの
Pythonプログラムとなっている．


このプログラムの7行目でカーネルがRawトレースポイントをサポートしているか否かが判別できる．
```
is_support_raw_tp = BPF.support_raw_tracepoint()
```

## raw_tracepointが使えない場合の回避策
リファレンスガイド上のリンクからbccと一緒に
配布されているtoolの中でraw_tracepointを使っているものが
参照できるが，それらを見ると，kprobeで代替するコードが
セットで用意されているので，それを見ると
raw_tracepointと同じことをkprobeで行う方法を読み取ることが
できる．

ただし，toolのプログラムは少し長いので，読むのに
気合を入れる必要がある．


## シンプルな利用例
リファレンスガイドにリンクされている，toolは読むのに
気合が必要なため，非常にシンプルな例(本ディレクトリの
<a href="sched_switch_simple">sched_switch_simple</a>)を収納している．

このサンプルでは，リファレンスガイドが例として
挙げている「<code>sched_switch</code>」関数を引っ掛ける
例になっている．

このプログラムでは，<code>tgid</code>を参照している(29,30行目)が，
<code>struct task_struct</code>の中身は使っているカーネルの
ヘッダファイルをインストールすると，
「/usr/src/linux-headers-カーネルのバージョン/include/linux/sched.h」の中に
定義されているので，必要な構造体メンバを参照するように
書き換えて貰えれば他の構造体メンバを使う例も実現できる．

## attach_raw_tracepoint()の利用
0.9.4はだめ
<code>RAW_TRACEPOINT_PROBE</code>マクロを使わずに，<code>attach_raw_tracepoint()</code>を使おうと
すると(本ディレクトリの
<a href="sched_switch_attach_tracepoint">sched_switch_attach_tracepoint</a>)，
手元のbccのバージョンのせいか
動かない．

```
# ./sched_switch_attach_trac
epoint
your kernel supports raw_tracepint()
bpf_attach_raw_tracepoint (sched_swtich): No such file or directory
Traceback (most recent call last):
  File "./sched_switch_attach_tracepoint", line 33, in <module>
    b.attach_raw_tracepoint("sched_swtich", "do_trace")
  File "/usr/lib/python3/dist-packages/bcc/__init__.py", line 860, in attach_raw_tracepoint
    raise Exception("Failed to attach BPF to raw tracepoint")
Exception: Failed to attach BPF to raw tracepoint
#
```

