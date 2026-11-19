# SRTrack

SRTrack は、AviUtl2 で編集した動画に対応する SRT 字幕を出力するためのツールキットです。

動画を書き出すたびに対応する SRTファイルを保存しておけば、後から別プロジェクトに動画ファイルとセットでSRTファイル（SRTrack Object）を配置できます。複数の動画を結合・切り抜き・速度変更した場合にも、編集結果に対応する字幕を再出力できます。

SRTrack Objectは編集画面でプレビューを表示でき、レンダリング時は動画に出力されない安心設計です。

## バージョン

- 現在の開発バージョン：v0.1.1（未リリース）
- 最終更新：2026/11/19

## 試験版について

SRTrack は v0 の試験段階にあるプラグインです。製作者自身の動画投稿上の必要に基づき、Codex によるバイブコーディングを中心に制作されました。

導入・利用は自己責任で行ってください。本プラグインの利用によって生じたいかなる不利益についても、作者は責任を負いません。導入前には、AviUtl2 のデータフォルダと大切なプロジェクトファイルをバックアップしておくことを推奨します。

バグの改善や機能追加は、v1.0.0 の完成を目標に継続します。以降の更新・保守を約束するものではありません。不具合報告、改善案、利用時の感想は [GitHub Issues](https://github.com/p-cyclase/SRTrack/issues) で受け付けます。設計の整理、テストの充実、コード改善を含む開発協力も歓迎します。報告・協力の方法は [開発・不具合報告について](CONTRIBUTING.md) を参照してください。

## 前提プラグイン

SRTrack は AviUtl2 / ExEdit2 2.1.13 で動作確認しています。利用前に、次のプラグインを導入してください。いずれも SRTrack には同梱しません。

- GCMZDrops2
- L-SMASH Works

各プラグインは配布元の案内に従って導入してください。

## 導入方法

1. [Releases](https://github.com/p-cyclase/SRTrack/releases) から利用したい版の `SRTrack-vX.Y.Z.zip` を取得します。
2. AviUtl2 を終了します。
3. ダウンロードした ZIP を任意の場所へ解凍します。
4. 解凍してできたフォルダ内の `Pluginフォルダ` と `Scriptフォルダ` を、AviUtl2 のデータフォルダへコピーします。既存ファイルの置換を確認された場合は置き換えます。
5. AviUtl2 を起動します。初回はプラグインやスクリプトを信頼するか確認されるため、内容を確認したうえで SRTrack の読み込みを信頼します。

ZIP は次の構成です。フォルダ構造を保ったまま展開してください。

```text
Plugin/
  SRTrack/
    SRTrack.aux2
    SRTrack.lua
Script/
  SRTrack/
    SRTrack.mod2
    SRTrackObject.obj2
    SRTrackObject(fix).obj2
    SRTrackObject(txt).obj2
readme.txt  （読むだけ。コピー不要）
```

`readme.txt` は導入手順の控えです。AviUtl2 のデータフォルダへコピーする必要はありません。

更新時も同じ手順です。以前の版へ戻したい場合は、GitHub Releases に保管されているその版の ZIP を解凍し、`Pluginフォルダ` と `Scriptフォルダ` をコピーしてください。

## 主な機能

### Exporter

タイムライン上のテキスト、セリフ準備、SRTrack Object を集め、編集結果に対応する SRT を出力します。出力時間、対象レイヤー、字幕の整形を指定できます。

### Importer

動画と同名の SRT がある場合、動画と SRTrack Object をまとめて配置します。SRT ファイル単体のドロップにも対応します。他プラグインのドロップ処理と干渉する可能性があるため、各取込機能は `SRTrack 設定` から個別にオフにできます。

### Object

`SRTrack Object` は SRT ファイルを保持し、上位レイヤの動画 Object の再生時間を参照して字幕を出力する Object です。編集画面でプレビューでき、動画には出力されません。

詳しい操作と設定は [ユーザーガイド](docs/user-guide.md) を参照してください。

現在の制約と、今後検討している機能は [既知の制約と今後の予定](docs/roadmap.md) にまとめています。

## 困ったとき・不具合報告

字幕が表示されない、期待した SRT にならないなどのときは、まず [ユーザーガイドの「困ったとき」](docs/user-guide.md#5-困ったとき) を確認してください。

解決しない場合は、[GitHub Issues](https://github.com/p-cyclase/SRTrack/issues) で報告を受け付けています。可能であれば、再現手順、期待した結果、実際の結果、SRTrack・AviUtl2・前提プラグインの版を記入してください。

Exporter の問題は結果画面の `共通ログをコピー`、Importer の問題は `SRTrack 設定` の `診断ログ` から診断情報をコピーして添えると、調査に役立ちます。Preview の問題は、現時点では編集画面のメッセージと再現手順を添えてください。Preview 用の診断表示は次回以降の更新で追加を予定しています。詳しくは [開発・不具合報告について](CONTRIBUTING.md) を参照してください。

## 開発・仕様

- 現在の仕様：[`docs/spec/`](docs/spec/README.md)
- 利用者向けの操作説明：[`docs/user-guide.md`](docs/user-guide.md)
- 開発・ビルド・手動回帰テスト：[`docs/developer-guide.md`](docs/developer-guide.md)
- 変更履歴：[`CHANGELOG.md`](CHANGELOG.md)

初期公開版は **v0.1.0** です。今後は、互換性・境界条件・利用者フィードバックに基づく改善として育てていきます。

## ライセンス

SRTrack は [MIT License](LICENSE) で提供します。AviUtl2 Plugin SDK の同梱ヘッダーは SDK 付属の MIT License に従います。第三者の権利表記は [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) を参照してください。
