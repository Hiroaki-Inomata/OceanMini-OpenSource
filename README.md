# OceanMini OpenSource Version

[OceanMini](https://phazor.info/OpenOcean/?page_id=592) のオープンソース版です。

今回は組み込 Web サーバに [Crow](https://github.com/CrowCpp/Crow) を使ってみました。

## 動作環境

- CMake 3.15 以上
- C++17 対応コンパイラ
- MacOS / Windows / Linux

開発環境は MacOS ですが、機種依存コードは使ってないので Win/Linux でも動くと思います。

## ビルド方法

```bash
mkdir build
cmake -S . -B build
cmake --build build
```
ビルドに成功すると、`build` フォルダ内に `oceanmini` が作成されます。

## 起動方法

```bash
./build/oceanmini
```
などで起動後、デフォルトブラウザで次の URL が開きます。

```text
http://localhost:18080/
```

`index.html` が表示されれば成功です。

## ライセンス

OceanMini OpenSource Version は GNU General Public License version 3 で公開しています。

詳細は `LICENSE` を参照してください。

## Third Party

This project uses Crow, licensed under the BSD 3-Clause License.

See `LICENSE_third_party`.
