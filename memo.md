* Ubuntu 16.04

## 1日目

helloosの起動

```
$ qemu-system-i386 -fda helloos.img
```

naskのアセンブル

```
$ wine tolset/z_tools/nask.exe helloos.nas helloos.img
```

## 10日目

2のべき乗を使うことでAND, OR命令を使えるようにする(結果的に10進数とかを使うより速くなる)

## 12日目

割り込みを禁止する処理で

* `io_load_eflags`でフラグを保存してから`io_cli`して、`io_store_eflags`でフラグを戻す方法
* 単に`io_cli`してから`io_sti`で割り込みを許可する方法

の違いが不明
