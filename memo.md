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
