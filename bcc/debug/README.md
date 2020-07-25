# デバッグ用機能

参考文献は今のところ1つのみ，また機能も現状は1つ(<code>bpf_override_return()</code>)のみ
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#debugging

## bpf_override_return()
参考文献:
- https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#1-bpf_override_return

この機能はとても危険で，カーネル内の関数の返り値を強制的に書き換えるもので，むやみに使うと怖いことが起きるため，本ディレクトリに収めたサンプルプログラム(
<a href="bpf_override_return">bpf_override_return</a>
)は
特定のアプリ(bash)に対してのみ実行される仕組みにしている．

### 使い方
<a href="bpf_override_return">bpf_override_return</a>
は64,65行目を見てわかるように，execveが実行された場合に
eBPFのVMで監視関数が動作する仕組みとなっている．
```
execve_fnname = b.get_syscall_fnname("execve")
b.attach_kprobe(event=execve_fnname, fn_name="syscall__execve")
```

eBPFのプログラム(関数)を見ると，45行目でexecveを実行したプロセスの名前を取得．
```
bpf_get_current_comm(&data.comm, sizeof(data.comm));
```
次に，独自に作成した文字列比較関数(<code>compare_with_bash()</code>)で
execveを実行したプロセスがbashか否かを判定．51行目からのif文で，
execveを実行したのがbashの場合だけ，返り値を<code>-EACCES</code>に書き換え．
```
bpf_override_return(ctx, -EACCES);
```

execveのmanページで<code>EACCES</code>を探すと，
パーミッションの不正で実行できないことを表すエラーと記載がある．

### 実行例
<a href="bpf_override_return">bpf_override_return</a>
をroot権限で動作させた状態で，別のbashウィンドウでコマンドを
実行すると，パーミッションで実行できないことがわかる．
```
bash$ ls
-bash: /usr/bin/ls: Permission denied
bash$
```

### 各種のディストリビューションでの利用可能性
ヘルパー関数の表(以下のURL)で<code>bpf_override_return()</code>がどのカーネルバージョンから
利用可能かを見ることができる．
- https://github.com/iovisor/bcc/blob/master/docs/kernel-versions.md#helpers

これによると，<code>BPF_FUNC_override_return()</code>はカーネルバージョン4.16
以降で利用できるとあるが，
カーネル4.18を搭載している
CentOS8.1では使えない．

この機能はカーネルコンパイル時のコンフィグレーションで
<code>
CONFIG_BPF_KPROBE_OVERRIDE
</code>を有効にする必要があるので，
CentOS8.1でカーネルのコンフィグファイルを/boot/でgrepしてみる．

```
[root@centos boot]# grep KPROBE_OVERRIDE config*
# CONFIG_BPF_KPROBE_OVERRIDE is not set
[root@centos boot]#
```

以上の結果から，カーネルコンパイル時にOFFにされているため，
CentOS8.1はカーネルを入れ替えないかぎり使えないことがわかる．

同様に，Ubuntu20.04で確認してみる．
```
# grep KPROBE_OVERRIDE config*
config-5.4.0-31-generic:CONFIG_BPF_KPROBE_OVERRIDE=y
config-5.4.0-33-generic:CONFIG_BPF_KPROBE_OVERRIDE=y
config-5.4.0-37-generic:CONFIG_BPF_KPROBE_OVERRIDE=y
#
```
インストール済みの3つのカーネル(Ubuntu標準品)共に，利用可能な状態であることがわかる．


