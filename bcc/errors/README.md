# 実行時に発生するエラーについて

## 参考文献
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#bpf-errors
- https://github.com/iovisor/bcc/blob/master/docs/kernel-versions.md#helpers


## メモリアクセス
詳細は参考文献に記載されているが，eBPFでは様々なC用のヘルパー関数が
提供されている．これを用いず，Cプログラム内でむやみとメモリ空間にアクセスすると，
VMのセキュリティ機構でアクセスがブロックされてエラーとなる．

その一例が参考文献にも記載されているが，先頭にあるように「<code>R7 invalid mem access 'inv'</code>」となっている．
```
bpf: Permission denied
0: (bf) r6 = r1
1: (79) r7 = *(u64 *)(r6 +80)
2: (85) call 14
3: (bf) r8 = r0
[...]
23: (69) r1 = *(u16 *)(r7 +16)
R7 invalid mem access 'inv'

Traceback (most recent call last):
  File "./tcpaccept", line 179, in <module>
    b = BPF(text=bpf_text)
  File "/usr/lib/python2.7/dist-packages/bcc/__init__.py", line 172, in __init__
    self._trace_autoload()
  File "/usr/lib/python2.7/dist-packages/bcc/__init__.py", line 612, in _trace_autoload
    fn = self.load_func(func_name, BPF.KPROBE)
  File "/usr/lib/python2.7/dist-packages/bcc/__init__.py", line 212, in load_func
    raise Exception("Failed to load BPF program %s" % func_name)
Exception: Failed to load BPF program kretprobe__inet_csk_accept
```

## eBPFプログラム(C部分)のライセンス定義
eBPFはLinuxカーネルのライセンス(GPLv2)と互換性があるプログラムしか
ロードしてくれない場合がある．ライセンスの問題をデモするためのプログラムが
本ディレクトリの<a href="error_sample">error_sample</a>である．

このプログラムの10行目のコメントを見るとわかるように，カーネルソースの特定のファイルに
互換性ありと認められたライセンスの一覧がある．
この情報に基づいて<a href="error_sample">error_sample</a>は作られており，<a href="error_sample">error_sample</a>の12行目が
GPLに非互換なライセンス宣言で，14行目からがGPLに互換性があるライセンス宣言となっている．
なにも宣言しない場合はGPLv2として取り扱われるが，明示的に宣言する場合はいずれかの
行のコメントを取り除けばよい．

このプログラム自体は<code>execve</code>を監視するプログラムとなっており，
ライセンス宣言に問題がない場合は，以下のような出力が得られる．
```
# ./error_sample
PID    COMM         OUTPUT
3569   bash         start of execve
3571   <...>        start of execve
^C#
```

これに対して，12行目を生かして実行すると以下のようなエラーとなる．
```
# ./error_sample
bpf: Failed to load program: Invalid argument
0: (79) r6 = *(u64 *)(r1 +112)
1: (bf) r3 = r6
2: (07) r3 += 112
3: (bf) r1 = r10
4: (07) r1 += -8
5: (b7) r2 = 8
6: (85) call bpf_probe_read#4
cannot call GPL-restricted function from non-GPL compatible program
processed 7 insns (limit 1000000) max_states_per_insn 0 total_states 0 peak_states 0 mark_read 0

Traceback (most recent call last):
  File "./error_sample", line 35, in <module>
    b.attach_kprobe(event=execve_fnname, fn_name="syscall__execve")
  File "/usr/lib/python3/dist-packages/bcc/__init__.py", line 663, in attach_kprobe
    fn = self.load_func(fn_name, BPF.KPROBE)
  File "/usr/lib/python3/dist-packages/bcc/__init__.py", line 403, in load_func
    raise Exception("Failed to load BPF program %s: %s" %
Exception: Failed to load BPF program b'syscall__execve': Invalid argument
#
```
上の実行結果に<code>cannot call GPL-restricted function from non-GPL compatible program</code>が
存在することから，ライセンスの問題が存在することがわかる．

eBPFの機能のうち，どれがGPLを要求するかは参考文献のヘルパー関数のURLを見て，
ヘルパー関数の表のライセンス欄にGPLと書いてあるかで判別することができる．
上のサンプルプログラムでは，<code>bpf_trace_printk()</code>を使っているが，
この機能は<code>BPF_FUNC_trace_printk()</code>の欄を見れば良く，カーネル
バージョン4.1以降で利用可能で，GPLを要求すると読み取れる．

