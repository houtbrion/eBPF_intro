# システムコールトレースポイント
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|


## 参考文献
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#8-system-call-tracepoints

## kprobeとの違い
kprobeで使っていた<a href="../kprobe/tcpv4connect_simple">tcpv4connectを利用するサンプル</a>(本リポジトリのkprobeの説明を参照)を書き換えて，syscall_tracepointの書式に合わせると，
「tcpv4connectにトレースポイントはない」というエラーがでて，実行が止まる．

そのため，参考文献に取り上げられている「execve」を対象にするよう，
修正すると(本ディレクトリの<a href="execve_simple">execve_simple</a>)
正常に動作する．

このことから，システムコールのうちトレースポイントを定義しているものだけが
対象となることがわかるが，通常のトレースポイントとの違いは不明確．実際に，
トレースポイントの情報が取れる/sysファイルシステムを見ると，「execve」のフォーマット
情報が取得できる．

```
# cd /sys/kernel/debug/tracing/events/syscalls/sys_enter_execve
# cat format
name: sys_enter_execve
ID: 700
format:
        field:unsigned short common_type;       offset:0;       size:2; signed:0;
        field:unsigned char common_flags;       offset:2;       size:1; signed:0;
        field:unsigned char common_preempt_count;       offset:3;       size:1; signed:0;
        field:int common_pid;   offset:4;       size:4; signed:1;

        field:int __syscall_nr; offset:8;       size:4; signed:1;
        field:const char * filename;    offset:16;      size:8; signed:0;
        field:const char *const * argv; offset:24;      size:8; signed:0;
        field:const char *const * envp; offset:32;      size:8; signed:0;

print fmt: "filename: 0x%08lx, argv: 0x%08lx, envp: 0x%08lx", ((unsigned long)(REC->filename)), ((unsigned long)(REC->argv)), ((unsigned long)(REC->envp))
#
```

一方，execveをkprobeで捉えようとするプログラムも本ディレクトリに
<a href="execve_kprobe_simple">execve_kprobe_simple</a>
として収納している．

これを実行すると下のようにエラーとなることから，execveはkprobeでは捕まえることができず，syscall_tracepointを用いる必要がある．
```
# ./execve_kprobe_simple
cannot attach kprobe, probe entry may not exist
Traceback (most recent call last):
  File "./execve_kprobe_simple", line 30, in <module>
    b = BPF(text=bpf_text)
  File "/usr/lib/python3/dist-packages/bcc/__init__.py", line 364, in __init__
    self._trace_autoload()
  File "/usr/lib/python3/dist-packages/bcc/__init__.py", line 1179, in _trace_autoload
    self.attach_kprobe(
  File "/usr/lib/python3/dist-packages/bcc/__init__.py", line 667, in attach_kprobe
    raise Exception("Failed to attach BPF program %s to kprobe %s" %
Exception: Failed to attach BPF program b'kprobe__execve' to kprobe b'execve'
#     
```

これまでのことから，外部に公開しているシステムコールは「syscall_tracepoint」で取り扱い，内部関数はkprobeで扱う必要があるように見受けられる．

## 利用可能なシステムコールの数
```
# cd /sys/kernel/debug/tracing/events/syscalls
# file * |wc -l
666
# ls
enable                            sys_enter_writev
filter                            sys_exit_accept
sys_enter_accept                  sys_exit_accept4
sys_enter_accept4                 sys_exit_access
sys_enter_access                  sys_exit_acct
sys_enter_acct                    sys_exit_add_key
(以下略)
```
2つほど，システムコールに関係ないファイルがあり，システムコールの入り口と出口にトレースポイントが仕掛けられているので，実数は332個のシステムコールの入り口と出口をキャッチすることができる．


## 使い方
基本的な使い方を<a href="execve_simple">execve_simple</a>
に基づいて説明する．

システムコールトレースポイントを利用することを宣言するため，プログラムの20行目で
監視対象システムコールのオブジェクトを取り出し，23行目で8行目から14行目のC関数が
呼び出されるように指定している．


