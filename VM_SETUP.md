# VMの利用
Ubuntu公式環境を構築した[VMイメージ][VM_image]を期間限定で
ダウンロード可能にしている．この[VMイメージ][VM_image]は
Windowsの[VitualBox][virtual_box]で構築したイメージファイルを
エクスポートしたものである．

そのため，[VitualBox][virtual_box]で[VMイメージ][VM_image]を
インポート
すれば，直ぐに利用可能となる．また，LinuxのKVMや
windowsやMacのVMwareでもイメージのコンバートで
利用可能であるが，それについては自分で調べていただきたい．

## virtual boxのインストール
[Virtual Boxのダウンロードページ][vitual_box_download]から
バイナリをダウンロード，インストールしてください．

## VMの取り込み手順
- [VMイメージ][VM_image]をダウンロード
- VitualBoxのインポート機能でVMをローカルの環境に取り込み
- VMが利用するCPU数やメモリ容量を調整

## VMのへのログイン
インポートしたVMを起動した上で，以下のIDでログイン．
|ユーザ名|パスワード|
|:--|:--|
|bpf|ebpf_intro|

VMにログインできたら，早急にパスワードは変更していただきたい．


<!-- 参考文献リスト -->
[VM_image]: <https://1drv.ms/u/s!AsD7a_l4Bpvng9VjSbW8cUD54SFCpQ?e=Hy3MFP> "VMイメージ"
[virtual_box]: <https://www.virtualbox.org/> "VirtualBox"
[vitual_box_download]: <https://www.virtualbox.org/wiki/Downloads> "VirtualBoxダウンロード"

