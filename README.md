# llfOS

[「30日でできる！ OS自作入門」](http://hrb.osask.jp/)で作成したOS

## 動作環境

* Ubuntu 16.04
* QEMU 2.5.0
* wine 1.6.2
* コンパイラ等は「OS自作入門」の付録CDの物を使用

## ビルド

「OS自作入門」の付録CDから`tolset`ディレクトリをこのディレクトリと同じ所に置いて、

```sh
$ patch -u -p0 < haribote.rul.patch
```

でパッチを当ててください。

```sh
$ make run
```

でビルドされた後、QEMUが立ち上がります。
