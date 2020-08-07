# Map関数

マップはeBPFの特別なデータの格納するための機能で<code>@</code>で始まる機能．
以下の例で使われている<code>count()</code>は該当のMAPのデータをカウントアップする
(詳細はのちほど説明)．
```
# bpftrace -e 'kprobe:vfs_read { @[comm] = count(); }'
Attaching 1 probe...
^C

@[systemd]: 2
@[sshd]: 8
@[bash]: 9
@[ls]: 15

#
```

|変数名|内容|
|:---|:---|
|<code>count()</code> | Count the number of times this function is called|
|<code>sum(int n)</code> | Sum the value|
|<code>avg(int n)</code> | Average the value|
|<code>min(int n)</code> | Record the minimum value seen|
|<code>max(int n)</code> | Record the maximum value seen|
|<code>stats(int n)</code> | Return the count, average, and total for this value|
|<code>hist(int n)</code> | Produce a log2 histogram of values of n|
|<code>lhist(int n, int min, int max, int step)</code> | Produce a linear histogram of values of n|
|<code>delete(@x[key])</code> | Delete the map element passed in as an argument|
|<code>print(@x[, top [, div]])</code> | Print the map, optionally the top entries only and with a divisor|
|<code>print(value)</code> | Print a value|
|<code>clear(@x)</code> | Delete all keys from the map|
|<code>zero(@x)</code> | Set all map values to zero|


## <code>count()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
@counter_name[optional_keys] = count()
```

<code>count()</code>は<code>@counter_name[optional_keys]</code>の
データ(整数)をカウントアップする．普通の言語なら，<code>increment()</code>
という名前のものにするが，なぜか<code>count()</code>になっている．
```
# bpftrace -e 'kprobe:vfs_read { @reads = count();  }'
Attaching 1 probe...
^C

@reads: 145

# bpftrace -e 'kprobe:vfs_read { @reads[comm] = count(); }'
Attaching 1 probe...
^C

@reads[systemd]: 2
@reads[basename]: 7
@reads[tr]: 9
@reads[sh]: 12
@reads[ls]: 15
@reads[snapd]: 16
@reads[less]: 21
@reads[lesspipe]: 24
@reads[sshd]: 31
@reads[bash]: 34

#
```


## <code>sum()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
@counter_name[optional_keys] = sum(value)
```

<code>@counter_name[optional_keys] = sum(value)</code>の意味はC言語の
記法で書くと<code>@counter_name[optional_keys] += value</code>の意味．

[公式リファレンスガイド][ref-guide]のサンプルを手元の環境で動作させた結果を下に示す．
```
# bpftrace -e 'kprobe:vfs_read { @bytes[comm] = sum(arg2); }'
Attaching 1 probe...
^C

@bytes[systemd]: 2048
@bytes[basename]: 2600
@bytes[sh]: 4292
@bytes[ls]: 8112
@bytes[bash]: 9604
@bytes[tr]: 18984
@bytes[lesspipe]: 31072
@bytes[less]: 83206
@bytes[sshd]: 409600

#
```

## <code>avg()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
@counter_name[optional_keys] = avg(value)
```



ファイルを読み取るカーネル内関数<code>vfs_read()</code>の
第3引数(<code>arg2</code>)はファイルの読み取りサイズであり，
[公式リファレンスガイド][ref-guide]の例は，
その平均値を計算するもの．ただし，何回の平均かまったく
不明なので，<code>vfs_read()</code>を実行した回数も
集計できるようにしたものを実行した例は以下の通り．
```
# bpftrace -e 'kprobe:vfs_read { @bytes[comm] = avg(arg2); @[comm] = count();}'
Attaching 1 probe...
^C

@[basename]: 7
@[tr]: 9
@[sh]: 12
@[ls]: 15
@[less]: 21
@[lesspipe]: 24
@[bash]: 32
@[sshd]: 61

@bytes[bash]: 300
@bytes[sh]: 357
@bytes[basename]: 371
@bytes[ls]: 540
@bytes[lesspipe]: 1294
@bytes[tr]: 2109
@bytes[less]: 3962
@bytes[sshd]: 16384

#
```

## <code>min()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|


文法:
```
@counter_name[optional_keys] = min(value)
```

<code>min()</code>は最小値を取るものであるので，
[公式リファレンスガイド][ref-guide]の例は，
1回のファイル読み取りの最小値を見つけるためのもの．
ただし，<code>avg()</code>の例と同じく
何回のうちの最小値であるかを表示できる例を下に示す．


```
# bpftrace -e 'kprobe:vfs_read { @bytes[comm] = min(arg2); @[comm] = count();}'
Attaching 1 probe...
^C

@[basename]: 7
@[tr]: 9
@[sh]: 12
@[ls]: 15
@[lesspipe]: 24
@[bash]: 34
@[less]: 54
@[sshd]: 180

@bytes[less]: 1
@bytes[bash]: 1
@bytes[sh]: 28
@bytes[lesspipe]: 28
@bytes[ls]: 32
@bytes[tr]: 32
@bytes[basename]: 32
@bytes[sshd]: 16384

#
```

## <code>max()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
@counter_name[optional_keys] = max(value)
```

<code>max()</code>は最大値を取るもの．<code>avg()</code>や<code>min()</code>の
例と同じく，ファイルの読み取りしたサイズのうち最大値と何回実行した
結果であるかを表示する例を下に示す．

```
# bpftrace -e 'kprobe:vfs_read { @bytes[comm] = max(arg2); @[comm] = count();}'
Attaching 1 probe...
^C

@[basename]: 7
@[locale]: 7
@[tr]: 9
@[sh]: 12
@[ls]: 15
@[tbl]: 19
@[preconv]: 20
@[nroff]: 21
@[groff]: 22
@[lesspipe]: 24
@[bash]: 46
@[less]: 54
@[pager]: 67
@[grotty]: 80
@[troff]: 116
@[man]: 146
@[sshd]: 722

@bytes[basename]: 832
@bytes[bash]: 832
@bytes[locale]: 832
@bytes[sh]: 832
@bytes[ls]: 1024
@bytes[groff]: 4096
@bytes[troff]: 4096
@bytes[preconv]: 4096
@bytes[tbl]: 4096
@bytes[grotty]: 4096
@bytes[lesspipe]: 8192
@bytes[tr]: 8192
@bytes[nroff]: 8192
@bytes[sshd]: 16384
@bytes[less]: 32768
@bytes[pager]: 32768
@bytes[man]: 65536

#
```

## <code>stats()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|


文法:
```
@counter_name[optional_keys] = stats(value)
```

下の例を見るとわかるように，<code>stats()</code>は
回数，平均，合計値を出力する．
```
# bpftrace -e 'kprobe:vfs_read { @bytes[comm] = stats(arg2); }'
Attaching 1 probe...
^C

@bytes[systemd-udevd]: count 1, average 8, total 8
@bytes[bash]: count 56, average 232, total 13001
@bytes[sh]: count 12, average 357, total 4292
@bytes[locale]: count 14, average 371, total 5200
@bytes[basename]: count 7, average 371, total 2600
@bytes[ls]: count 15, average 540, total 8112
@bytes[groff]: count 44, average 757, total 33344
@bytes[systemd]: count 2, average 1024, total 2048
@bytes[nroff]: count 42, average 1077, total 45248
@bytes[lesspipe]: count 24, average 1294, total 31072
@bytes[less]: count 54, average 1693, total 91430
@bytes[tr]: count 9, average 2109, total 18984
@bytes[preconv]: count 40, average 2139, total 85584
@bytes[tbl]: count 38, average 2208, total 83920
@bytes[pager]: count 81, average 2648, total 214565
@bytes[grotty]: count 162, average 3653, total 591824
@bytes[troff]: count 232, average 3786, total 878544
@bytes[man]: count 289, average 4661, total 1347030
@bytes[sshd]: count 837, average 16384, total 13713408

#
```

## <code>lhist()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
@histogram_name[optional_key] = lhist(value, min, max, step)
```
<code>lhist()</code>は，データを線形形式のヒストグラムで表示させるためのもので，
最小値，最大値，1つの区間のサイズを指定する．[公式リファレンスガイド][ref-guide]の例
(下に実行例を示す)は，最低値0,最大値1万で，千が区間のヒストグラムのデータとして，
カーネル内の関数の返り値を入れている．
```
# bpftrace -e 'kretprobe:vfs_read { @bytes = lhist(retval, 0, 10000, 1000); }'
Attaching 1 probe...
^C

@bytes:
[0, 1000)            666 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[1000, 2000)           6 |                                                    |
[2000, 3000)           1 |                                                    |
[3000, 4000)           3 |                                                    |
[4000, 5000)          26 |@@                                                  |
[5000, 6000)           0 |                                                    |
[6000, 7000)           0 |                                                    |
[7000, 8000)           0 |                                                    |
[8000, 9000)           5 |                                                    |

#
```


## <code>hist()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
@histogram_name[optional_key] = hist(value)
```
<code>hist()</code>は底が2のlog形式のヒストグラムを表示させるためのもの．

下の例は[公式リファレンスガイド][ref-guide]と同じもので，ファイルリードの
カーネル内関数の返り値を溜め込んで，Ctrl-Cを入力(実行終了)するとためたデータを
log2形式で出力するものである．
```
# bpftrace -e 'kretprobe:vfs_read { @bytes = hist(retval); }'
Attaching 1 probe...
^C

@bytes:
[0]                   21 |@@                                                  |
[1]                  104 |@@@@@@@@@@@@@                                       |
[2, 4)               392 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[4, 8)                29 |@@@                                                 |
[8, 16)               12 |@                                                   |
[16, 32)              63 |@@@@@@@@                                            |
[32, 64)             102 |@@@@@@@@@@@@@                                       |
[64, 128)            223 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                       |
[128, 256)            22 |@@                                                  |
[256, 512)            38 |@@@@@                                               |
[512, 1K)             56 |@@@@@@@                                             |
[1K, 2K)               4 |                                                    |
[2K, 4K)               8 |@                                                   |
[4K, 8K)              24 |@@@                                                 |
[8K, 16K)              7 |                                                    |

#
```

上の例では，全てのアプリのデータを1つの変数の貯めていたが，
アプリ別の変数に貯めることも可能(下を参照)．
```
# bpftrace -e 'kretprobe:do_sys_open { @bytes[comm] = hist(retval); }'
Attaching 1 probe...
^C

@bytes[tbl]:
[2, 4)                 5 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|

@bytes[preconv]:
(..., 0)               1 |@@@@@@@                                             |
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                 7 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|

@bytes[sh]:
(..., 0)              16 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                 0 |                                                    |
[4, 8)                 2 |@@@@@@                                              |

@bytes[basename]:
(..., 0)              16 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                 0 |                                                    |
[4, 8)                 3 |@@@@@@@@@                                           |

@bytes[locale]:
(..., 0)              16 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                 3 |@@@@@@@@@                                           |

@bytes[tr]:
(..., 0)              16 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                 0 |                                                    |
[4, 8)                 3 |@@@@@@@@@                                           |

@bytes[lesspipe]:
(..., 0)              16 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                 0 |                                                    |
[4, 8)                 4 |@@@@@@@@@@@@@                                       |

@bytes[nroff]:
(..., 0)              16 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                 4 |@@@@@@@@@@@@@                                       |

@bytes[pager]:
(..., 0)              28 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                 7 |@@@@@@@@@@@@@                                       |

@bytes[less]:
(..., 0)              28 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                 7 |@@@@@@@@@@@@@                                       |
[4, 8)                 1 |@                                                   |

@bytes[bash]:
(..., 0)              34 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                 0 |                                                    |
[4, 8)                 7 |@@@@@@@@@@                                          |

@bytes[groff]:
(..., 0)              44 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                 6 |@@@@@@@                                             |

@bytes[grotty]:
(..., 0)              47 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                10 |@@@@@@@@@@@                                         |

@bytes[ls]:
(..., 0)              52 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                 9 |@@@@@@@@@                                           |

@bytes[troff]:
(..., 0)              49 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[0]                    0 |                                                    |
[1]                    0 |                                                    |
[2, 4)                17 |@@@@@@@@@@@@@@@@@@                                  |

```




## <code>print()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

文法:
```
print(@map [, top [, divisor]])
```

<code>print()</code>プリント関数はmapに対しても利用できる．
[公式リファレンスガイド][ref-guide]の例は説明が冗長なので，シンプルに纏めた例を下に示す．
```
# bpftrace -e 'kprobe:vfs_read { @start[tid] = nsecs; }
>    kretprobe:vfs_read /@start[tid]/ {@ms[pid] = sum(nsecs - @start[tid]); delete(@start[tid]); }
>    END { print(@ms, 3,1000000); clear(@ms); clear(@start); }'
Attaching 3 probes...
^C@ms[3456]: 8
@ms[3466]: 76
@ms[3467]: 825




#
```
上の例は，あるプロセスによる<code>vfs_read()</code>の実行開始時間(ナノ秒単位)を
<code>kprobe:vfs_read</code>のところで<code>@start[tid]</code>に
覚えさせておき，<code>kretprobe:vfs_read</code>で現在時刻と実行開始時刻の
差分を計算して，<code>@ms[pid]</code>に加算する．

END節の部分で，実行終了時に<code>@ms</code>を<code>print()</code>で出力するが，
1番目の引数で<code>@ms[キー]</code>のうち，データが大きいものトップ3つ(<code>print()</code>の第2引数で指定)を
取り出し，値を<code>10^6</code>(<code>print()</code>の第3引数で指定)で割り算してナノ秒をミリ秒に変換して
出力している．

## <code>clear()</code>と<code>zero()</code>
|環境|動作|
|:--|:--|
|Ubuntu公式|○|
|CentOS公式|○|
|Ubuntu最新|○|

MAPは内容をクリアできるので，END節の中でclearすると中身が空になる．
```
# bpftrace -e 'kprobe:vfs_read { @foo[comm] = count(); } END{clear(@foo);}'
Attaching 2 probes...
^C


#
```
同じく，<code>clear()</code>ではなく，<code>zero()</code>で値を0にできる．
```
# bpftrace -e 'kprobe:vfs_read { @foo[comm] = count(); } END{zero(@foo);}'
Attaching 2 probes...
^C

@foo[tr]: 0
@foo[less]: 0
@foo[ls]: 0
@foo[bash]: 0
@foo[basename]: 0
@foo[systemd-resolve]: 0
@foo[sh]: 0
@foo[sshd]: 0
@foo[systemd]: 0
@foo[lesspipe]: 0

#
```



<!-- 参考文献リスト -->
[ref-guide]: <https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md>  "公式リファレンスガイド"

