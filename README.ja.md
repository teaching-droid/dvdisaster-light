# dvdisaster Light

[English](README.md) | [Deutsch](README.de.md) | **日本語** | [Italiano](README.it.md)

**dvdisaster Light** は [dvdisaster](https://dvdisaster.jcea.es) を大胆に絞り込んだコマンドライン専用フォークです。目的はただひとつ: **光学メディア向け RS03 エラー訂正を、ハードウェアの限界速度で**。

dvdisaster はディスクイメージをリード・ソロモン方式のパリティで保護します。後日メディアが劣化しても、損傷が追加したパリティより小さい限り修復できます。保護はイメージ単位で働くため、ファイルシステムの損傷さえ乗り越えられます。

## dvdisaster との違い

維持(バイト単位で互換):

* **RS03 コーデック**の両方式: 独立したエラー訂正ファイル(`-mRS03 -o file`)と拡張イメージ(`-mRS03 -o image`)
* イメージの作成・検証・修復、拡張イメージからの訂正データ除去
* 物理ドライブからのイメージ読み込み(リニア方式)
* 上流の回帰テスト(RS03 部分)。下記の互換性の約束を保証します

削除:

* RS01 と RS02 コーデック(これらで保護したメディアはオリジナルの dvdisaster をご利用ください。形式が重なる範囲では両者は互いのディスクを修復できます)
* GTK グラフィカルインターフェース。本フォークはコマンドラインツールです(GUI は将来、独立プログラムとして提供する可能性があります)
* アダプティブ読み込み方式

予定(バージョン番号のとおり、これは大きな計画の初期リリースです):

* GPU 選択機能と CPU フォールバックを備えた **OpenCL RS03 エンコーダ**
* 既存の SSE2 に加えて AVX2 CPU パス
* Windows 7 SP1 以降のサポート継続

## 互換性の約束

dvdisaster Light が生成するファイルと拡張イメージは、同じ入力と設定であれば dvdisaster 0.79.10-pl6 の出力と**ビット単位で同一**です。また、どの dvdisaster バージョンで RS03 保護したメディアも本フォークで修復でき、その逆も成り立ちます。継承した回帰テストと、実物の 41.5 GB Blu-ray イメージでのオリジナルバイナリとのバイト一致比較が、すべての変更でこの約束を守ります。

## バージョン体系

`dvdisaster Light 0.1.0 (based on dvdisaster 0.79.10-pl6)`: Light 側の番号は本フォークのリリースを数え、ベース側の番号はコーデックの由来となった上流の正確な状態を示します。ファイル形式内のバージョン欄はベース版に固定されているため、他の dvdisaster バージョンもファイルを正しく解釈できます。

## ビルド

Linux など:

```
./configure && make -j$(nproc)
```

Windows(MSYS2、MINGW64 環境):

```
pacman -S --needed git diffutils make pkg-config mingw-w64-x86_64-glib2 mingw-w64-x86_64-gcc
./configure && make -j16
```

回帰テストは `cd regtest && ./runtests.sh` で実行します(`/var/tmp/regtest` ディレクトリが必要です)。

## 謝辞とライセンス

本フォークは、dvdisaster を生み出し長年保守してきた **Carsten Gnoerlich** 氏、**dvdisaster 開発チーム**、そして本コードの直接の土台である 0.79.10-pl6 を保守し、このようなフォークの維持を可能にする回帰テストを整備した **speed47** 氏の仕事の上に成り立っています。

dvdisaster Light は **GNU General Public License v3**([COPYING](COPYING) 参照)のフリーソフトウェアです。dvdisaster の改変版であり、オリジナルの作者とは関係ありません。各リリースの完全なソースコードはバイナリと共に本リポジトリで公開されます。
