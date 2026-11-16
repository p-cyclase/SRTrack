# SRTrack

SRTrack は、AviUtl2 で編集した動画に対応する SRT 字幕を出力するためのツールキットです。動画と同時に用意した SRT をタイムラインへ対応づけ、動画の連結・切抜き・分割・速度変更後にも、元の文字列とタイミングに近い SRT を再出力します。

字幕を動画へ焼き込む機能ではありません。編集中の確認用に字幕 Preview を表示できますが、動画書き出しには描画しません。

## できること

- 標準テキスト、PSDToolkit のセリフ準備、SRTrack Object を一つの SRT へ出力する。
- 動画に追従する `SRTrack Object`、動画に依存しない `SRTrack Object (fix)`、補足字幕用の `SRTrack Object (txt)` を使い分ける。
- 短い字幕の結合、フレーム範囲選択、同時開始時の本文順、AviUtl2 テキスト制御文字の変換に対応する。
- 動画と同名の SRT を検出して、動画と SRTrack Object をまとめて取り込む。
- 問題が起きたとき、Exporter の結果画面または `SRTrack 設定` の診断ログから、報告に必要な情報をコピーできる。

## 確認済みの動作環境

- Windows x64
- AviUtl2 / ExEdit2 2.1.13
- GCMZDrops2
- L-SMASH Works

SRTrack は上記の環境で確認しています。GCMZDrops2 と L-SMASH Works は SRTrack に同梱しません。各プラグインは配布元の案内に従って導入してください。

## インストール

1. [Releases](../../releases) から利用したい版の `SRTrack-vX.Y.Z.zip` を取得します。
2. AviUtl2 を終了します。
3. ZIP を AviUtl2 のデータフォルダへ展開し、既存ファイルがある場合は置き換えます。
4. AviUtl2 を起動します。

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
readme.txt
```

更新時も同じ手順です。以前の版へ戻したい場合は、GitHub Releases に保管されているその版の ZIP を展開してください。

## 使いかたの入口

### 動画と SRT を対応づける

動画と同名の `.srt` を同じフォルダに置いて動画をドロップすると、Importer が動画と `SRTrack Object` を配置します。Importer の有効／無効と、SRT 単体ドロップ時の Object 種別は `SRTrack 設定` で変更できます。取込設定は AviUtl2 を再起動した後のファイルドロップから反映されます。

### 手動で配置する

- `SRTrack Object`：上側にある最も近い動画 Object を参照して SRT を時間変換します。
- `SRTrack Object (fix)`：動画を参照せず、Object 自身の再生位置・速度で SRT を時間変換します。
- `SRTrack Object (txt)`：自由入力したテキストを、Object の範囲だけ SRT に出力します。

### SRT を出力する

Exporter を開き、出力対象と字幕の整形条件を指定して `エクスポート…` を選びます。保存先は Windows の保存ダイアログで選択します。

## 困ったとき・不具合報告

Exporter で警告またはエラーが表示された場合は、結果画面の `共通ログをコピー` を使ってください。Importer や Preview を含む診断は `SRTrack 設定` の `診断ログ` タブからコピーできます。

GitHub Issue では、表示されたメッセージ、再現手順、期待した結果、実際の結果、コピーした診断情報を添えてください。診断ログには絶対パス、SRT 本文、動画名を記録しない設計ですが、投稿前に内容を確認し、公開したくない情報は伏せてください。詳しくは [不具合報告と貢献](CONTRIBUTING.md) を参照してください。

## 開発・仕様

- 現在の仕様：[`docs/spec/`](docs/spec/README.md)
- 開発・ビルド・手動回帰テスト：[`docs/developer-guide.md`](docs/developer-guide.md)
- 変更履歴：[`CHANGELOG.md`](CHANGELOG.md)

初期版は完成しています。今後は、互換性・境界条件・利用者フィードバックに基づく改善として育てていきます。

## ライセンス

SRTrack は [MIT License](LICENSE) で提供します。AviUtl2 Plugin SDK の同梱ヘッダーは SDK 付属の MIT License に従います。第三者の権利表記は [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) を参照してください。
