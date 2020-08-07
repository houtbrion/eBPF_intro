# はじめの一歩

|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

## bpftrace用スクリプト全体のイメージ
ものすごく大雑把なイメージで言うと，awkに似た使い方ができるもので，
引数にスクリプトを与えてそのまま実行することも，別のファイルに
スクリプトを保存しておいて実行することもできる．

bpftrace用のスクリプトの構造を正規表現を用いて表すと，大まかに以下のようなイメージとなる．
これを見ていただければ，フィルタ文の部分を除くとawkに非常に
似ていることに納得していただけると思う．
```
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

## Hello World

下の例は，awkで"Hello, World!"を出力するワンライナーの例であり，
スクリプト部分は<code>BEGIN</code>で始まる部分(節)だけで構成されている．
このコマンドを投入すると直ちに"Hello, World!"が出力され，awkの
実行は終了する．
```
$ awk -e '
> BEGIN { print "Hello, World!";}
> '
Hello, World!
$
```
BEGIN節はawk実行開始直後に内容が実行され，次のステップに移るが，
このスクリプトは他になにもプログラムがないため，即座に終了している．

これに対して，
"Hello World."をbpftraceのワンライナーで
実行する例が[公式リファレンスガイド][ref-guide]に紹介されている．
これを実行すると(以下に示す)，自動的に終了しないため，Ctrl-Cで強制終了する必要がある．
```
# bpftrace -e 'BEGIN { printf("Hello, World!\n"); }'
Attaching 1 probe...
Hello, World!
```
awkの場合と同じく，ワンライナーとして実行する場合は引数<code>-e</code>を
用いる必要がある．

## Good bye
awkと同じくEND節は終了時に実行するためのもので，bpftraceのワンライナーとして
実行すると以下のようになる．awkと異なりbpftraceは，プログラムが空でも
実行が終わらないため，Ctrl-Cを入力してEND節を実行させる
必要がある．
```
# bpftrace -e 'END { printf("Good bye!"); }'
Attaching 1 probe...
^CGood bye!

#
```

## プログラム節の書き方
プログラム節の構造は以下の通り．まず最初はフィルタ文の部分は簡単にするために
省略する．
```
パターン
[/フィルタ文/]
{
  [プログラム文;]+
}

```

下の例はあるプロセスが他のプログラムを実行する場合に使われるシステムコール<code>execve()</code>
を監視するスクリプトである．
なお，ここではパターン部分とプログラム文の詳細説明は省く．
```
# bpftrace -e 'tracepoint:syscalls:sys_enter_execve { printf("%s call execve()\n", comm);}'
Attaching 1 probe...
```

この状態で，別のウィンドウで以下のようにコマンドを入力する．
```
bash$ ls
README.md
bash$ sh
$ ls
README.md
$ exit
bash$
```

上のようにコマンドを入力すると，以下のような出力が得られる．
```
# bpftrace -e 'tracepoint:syscalls:sys_enter_execve { printf("%s call execve()\n", comm);}'
Attaching 1 probe...
bash call execve()
bash call execve()
sh call execve()
^C

#
```

## ファイルに保存したスクリプトの実行
以下の
[サンプルスクリプト][hello_bye_sample.bt]は，今までの説明の内容を
1つのスクリプトにまとめて，ファイルに収めたものである．
```
bash$ cat hello_bye_sample.bt
BEGIN{
  printf("I will watch execve() system call.\n");
  printf("Please input Ctrl-C to finish this program.\n");
}

tracepoint:syscalls:sys_enter_execve {
  printf("%s call execve()\n", comm);
}

END{
  printf("\nterminating to watch execve().\n");
}
bash$
```

この[ファイル][hello_bye_sample.bt]
をbpftraceで実行する場合，スクリプトのファイル名<code>hello_bye_sample.bt</code>を引数で与えれば良い．
```
# bpftrace hello_bye_sample.bt
Attaching 3 probes...
I will watch execve() system call.
Please input Ctrl-C to finish this program.
```
ここで，先程と同じく別のウィンドウで以下のコマンドを入力する．
```
bash$ ls
hello_bye_sample.bt  README.md
bash$ sh
$ ls
hello_bye_sample.bt  README.md
$ exit
bash$
```
この状態でのbpftraceのウィンドウでCtrl-Cを入力し，実行を止めた場合の出力は以下の通り．
```
# bpftrace hello_bye_sample.bt
Attaching 3 probes...
I will watch execve() system call.
Please input Ctrl-C to finish this program.
bash call execve()
bash call execve()
sh call execve()
^C
terminating to watch execve().


#
```

## 自動で実行を終了させる
今までの例では，bpftraceの実行を止めるためにユーザがCtrl-Cを入力する必要があった．
これは面倒なので，あるイベントが発生したら自動的に実行を終わらせる方法がある．

### 一定時間経過で終了
下の[スクリプト][hello_bye_auto.bt]は，一定時間間隔でbpftraceにイベントを上げる
機能(<code>interval</code>)とプログラムを終了する機能(<code>exit()</code>)を用いて，
実行から10秒で終わるようにプログラミングしたものである．
```
BEGIN{
  printf("Hello, World!\n");
}

interval:s:10 {
  printf("timeout (10sec)\n");
  exit();
}

END{
  printf("Good bye.\n");
}
```

この[スクリプト][hello_bye_auto.bt]を実行すると，以下のような結果が得られる．
```
# bpftrace hello_bye_auto.bt
Attaching 3 probes...
Hello, World!
timeout (10sec)
Good bye.


#
```

### 時間経過を表示
下の[スクリプト][hello_bye_timerup.bt]は，時間経過を出力するため，
1秒間隔でイベントが上がる機能を追加 したものである．
```
BEGIN{
  printf("Hello, World!\n");
}

interval:s:1 {
  printf("1sec passed\n");
}

interval:s:10 {
  printf("timeout (10sec)\n");
  exit();
}

END{
  printf("Good bye.\n");
}
```

この[スクリプト][hello_bye_timerup.bt]を実行すると以下のような結果となる．
```
# bpftrace hello_bye_timerup.bt
Attaching 4 probes...
Hello, World!
1sec passed
1sec passed
1sec passed
1sec passed
1sec passed
1sec passed
1sec passed
1sec passed
1sec passed
1sec passed
timeout (10sec)
Good bye.


#
```

<!-- 参考文献リスト -->
[ref-guide]: <https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md>  "公式リファレンスガイド"
[hello_bye_sample.bt]: <hello_bye_sample.bt> "hello_bye_sample.bt"
[hello_bye_auto.bt]: <hello_bye_auto.bt> "hello_bye_auto.bt"
[hello_bye_timerup.bt]: <hello_bye_timerup.bt> "hello_bye_timerup.bt"


