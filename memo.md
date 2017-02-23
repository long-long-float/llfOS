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
