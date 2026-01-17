# CrossPoint Reader CJK版

[English](./README.md) | [中文](./README-ZH.md) | **[日本語](./README-JA.md)**

> [daveallie/crosspoint-reader](https://github.com/daveallie/crosspoint-reader) をベースにした **Xteink X4** 電子ペーパーリーダー用ファームウェアのCJK対応版です。

本プロジェクトは、オリジナルのCrossPoint Readerに中国語/日本語のローカライズサポートとCJKフォントレンダリング機能を追加しています。

![](./docs/images/cover.jpg)

## ✨ CJK版の新機能

### 🌏 多言語UIサポート (I18n)
- **中国語**、**英語**、**日本語** のインターフェース言語をサポート
- 設定でいつでも言語を切り替え可能
- すべてのメニュー、プロンプト、設定項目が完全にローカライズ済み

### 📝 CJKフォントサポート
- **外部フォント読み込み**：SDカードの `/fonts/` ディレクトリからカスタムCJKフォントを読み込み
- Xteink標準 `.bin` フォント形式をサポート
- LRUキャッシュ最適化によりCJKレンダリングパフォーマンスを向上
- サンプルフォント同梱：
  - `SourceHanSansCN-Medium_20_20x20.bin` (源ノ角ゴシック)
  - `KingHwaOldSong_38_33x39.bin` (京華老宋体)

## 📦 機能一覧

オリジナルCrossPoint Readerのすべての機能を継承：

- [x] EPUB解析とレンダリング (EPUB 2 および EPUB 3)
- [x] EPUB画像altテキスト表示
- [x] TXTプレーンテキストリーダーサポート
- [x] 読書進捗の保存
- [x] ファイルブラウザ（ネストされたフォルダをサポート）
- [x] カスタムスリープ画面（表紙表示サポート）
- [x] WiFiファイルアップロード
- [x] WiFi OTAファームウェア更新
- [x] フォント、レイアウト、表示オプションのカスタマイズ
- [x] 画面回転
- [x] Calibreワイヤレス接続とウェブライブラリ統合
- [x] 表紙画像表示

詳細な操作方法は[ユーザーガイド](./USER_GUIDE.md)をご覧ください。

## 📥 インストール方法

### Webフラッシュ（推奨）

1. USB-CでXteink X4をコンピュータに接続
2. https://xteink.dve.al/ にアクセスし、「Flash CrossPoint firmware」をクリック

> **ヒント**：公式ファームウェアに戻すには同じURLからフラッシュするか、https://xteink.dve.al/debug でブートパーティションを切り替えてください

### 手動ビルド

下記の[開発](#開発)セクションをご覧ください。

## 🔧 開発

### 前提条件

* **PlatformIO Core** (`pio`) または **VS Code + PlatformIO IDE**
* Python 3.8+
* USB-Cケーブル
* Xteink X4デバイス

### コードの取得

```bash
git clone --recursive https://github.com/aBER0724/crosspoint-reader-cn

# サブモジュールなしでクローン済みの場合：
git submodule update --init --recursive
```

### ビルドとフラッシュ

デバイスを接続して実行：

```bash
pio run --target upload
```

### CJKフォント設定

1. SDカードのルートディレクトリに `/fonts/` フォルダを作成
2. `.bin` 形式のCJKフォントファイルを配置
3. 設定で「外部フォント」オプションを選択

フォントファイル名形式：`FontName_size_WxH.bin`
例：`SourceHanSansCN-Medium_20_20x20.bin`

## 📚 技術詳細

### データキャッシング

書籍の章は初回読み込み時にSDカードにキャッシュされます。以降の読み込みはキャッシュから行われます。キャッシュディレクトリはSDカードルートの `.crosspoint/` にあります：

```
.crosspoint/
├── epub_12471232/       # 各EPUBはサブディレクトリ (epub_<hash>)
│   ├── progress.bin     # 読書進捗
│   ├── cover.bmp        # 書籍カバー
│   ├── book.bin         # 書籍メタデータ
│   └── sections/        # 章データ
│       ├── 0.bin
│       ├── 1.bin
│       └── ...
└── epub_189013891/
```

`.crosspoint` ディレクトリを削除するとすべてのキャッシュがクリアされます。

詳細な技術情報は[ファイル形式ドキュメント](./docs/file-formats.md)をご覧ください。

## 🤝 コントリビューション

IssueやPull Requestを歓迎します！

### コントリビューションプロセス

1. このリポジトリをフォーク
2. フィーチャーブランチを作成 (`feature/your-feature`)
3. 変更をコミット
4. Pull Requestを提出

## 📜 謝辞

- [daveallie/crosspoint-reader](https://github.com/daveallie/crosspoint-reader) - オリジナルCrossPoint Reader

---

**免責事項**：本プロジェクトはXteinkまたはX4ハードウェアメーカーとは一切関係ありません。コミュニティプロジェクトです。
