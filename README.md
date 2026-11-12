# SRTrack

SRTrack は、AviUtl2 で編集後の動画に対応する SRT 字幕を出力するためのツールキットです。

最大の目的は、いったん書き出した動画を別プロジェクトで連結・切り抜き・速度変更しても、元の字幕本文とタイミングを引き継ぎ、編集結果に対応する SRT を再構成できるようにすることです。

## コア・コンセプト

動画ファイルだけから、元の字幕を正確に復元することはできません。SRTrack は、動画と同時に出力された SRT を `SRTrack Object` として動画へ対応づけます。動画と字幕側 Object を一組として扱い、動画の編集結果を字幕の時間情報へ反映して、最終的に SRT を出力します。

SRTrack Object は字幕を動画へ焼き込むための Object ではありません。対応する動画の字幕情報を保持する「字幕側の相方」です。編集用字幕プレビューは補助機能であり、書き出し動画には反映されません。

SRTrack Exporter も動画を書き出す機能ではなく、通常の保存ダイアログを使う SRT エクスポーターです。保存済み `.aup2` の現在のシーンに記録された動画書き出し先を読める場合だけ、同名 `.srt` を保存先の初期値として提案します。これは候補の提示に限り、保存先を強制しません。

## 構成

開発上の責務は、次の4つに分けます。

| 構成要素 | 役割 | 状態 |
| --- | --- | --- |
| Exporter | タイムライン上の字幕ソースを集め、SRT を出力する | 実装済み |
| Importer | 動画ドロップ時に同名 SRT を検出し、動画と SRTrack Object を生成・グループ化する | 実装済み |
| Monitor | 動画 Object の編集結果を読み取り、SRTrack Object の再生値へ反映する | 初期実装済み・概略確認済み |
| Object | SRT ファイルと再生情報を保持し、編集用字幕プレビューを提供する | 一部実装済み |

最終配布では、論理上は区別したまま、次の4ファイルにまとめます。

```text
SRTrack.aux2
├─ Exporter
├─ Importer
└─ Monitor

SRTrack.lua
└─ GCMZDrops2 用の Importer ハンドラスクリプト

SRTrack.obj2
└─ SRT の参照・時間情報を保持するカスタム Object

SRTrack.mod2
└─ Object の字幕プレビュー用補助モジュール
```

リポジトリも完成時の配布名に合わせ、`src/aux2`、`src/lua`、`src/object`、`src/mod2` へ実装を分けています。`build.cmd` は4ファイルを `out/` に生成します。

## リポジトリ構成

```text
src/
├─ aux2/       Exporter・Importer・将来の Monitor を収録する DLL の実装
├─ lua/        GCMZDrops2 に登録する SRTrack.lua
├─ object/     SRTrack.obj2
└─ mod2/       SRTrack.mod2 の実装

tests/         変換・入力処理の単体テスト
third_party/   AviUtl2 Plugin SDK の必要なヘッダーとライセンス
docs/          全体仕様、コンポーネント別資料、初期設計メモ
build.cmd      out/ へ配布用4ファイルを生成するビルドスクリプト
```

## 基本的な利用の流れ

1. AviUtl2 プロジェクト内の標準テキストなどから、Exporter で SRT を出力する。
2. 書き出した動画と同名の SRT を同じフォルダへ置く。
3. 動画をタイムラインへドロップする。
4. Importer が同名 SRT を見つけた場合、動画と SRTrack Object を同時に配置・グループ化する。
5. 編集後、Exporter でタイムライン全体の字幕を SRT として出力する。

同名 SRT がない動画は通常どおり配置され、通知は表示しません。

## SRTrack Object

SRTrack Object は、SRT ファイル、再生位置 `P`、再生範囲終端 `E`、動画全長 `D`、再生速度、ループ再生、編集用字幕プレビューを持ちます。

`D` は元動画の全長であり、移動・分割・伸縮によって変化させません。ループ再生時だけ、SRT 元時刻を戻す周期として使います。編集用字幕プレビューのループは簡易確認済みです。ループ時の Exporter 出力は未確認です。

## Importer と Monitor

Importer は GCMZDrops2 の公開ハンドラー API を利用します。同名 SRT がある場合、動画 Object の直上に SRTrack Object を置き、両者をグループ化します。複数動画を同時にドロップした場合は、ファイル名順に同じ二層へ並べ、動画ごとに別のグループを作成します。

Monitor は常駐コンポーネントです。Importer が生成した管理対象 Object について、動画 Object の再生位置、再生範囲終端、再生速度、ループ、ファイル参照、タイムライン範囲を SRTrack Object へ反映します。Object 側の手動変更は動画側の値へ差し戻します。候補を一意に決められない場合は何も変更しません。移動・伸縮・分割とリネームは開発者による概略確認済みです。

## 動作確認状況

AviUtl2 2.1.13 で、開発者による概略確認を行っています。

- 標準テキストおよび SRTrack Object からの SRT 出力
- 日本語を含むパスの SRT 読み込み
- 編集用字幕プレビューと、そのループ
- 統合後の `SRTrack.aux2` による SRTrack Object 由来字幕の出力
- 同名 SRT の検出、動画と Object の二層配置・グループ化
- エクスプローラーおよび AviUtl2 内メディアエクスプローラーからの動画追加
- 複数動画のファイル名順配置と、動画・Object の初期終端値の一致

これは網羅的な受け入れ試験ではありません。Monitor の網羅検証と、ループ時の Exporter 出力は今後の検証対象です。

## 前提プラグイン

Importer を使う場合は GCMZDrops2 と L-SMASH Works が必要です。いずれも SRTrack には同梱しません。SRTrack は GCMZDrops2 の公開 API のみを利用し、AviUtl2 標準 Object 以外の外部ファイルやプラグインのリバースエンジニアリングは行いません。

## ライセンス

SRTrack のコードは [MIT License](LICENSE) です。AviUtl2 SDK および第三者コンポーネントについては、[第三者の権利表記](THIRD_PARTY_NOTICES.md) を参照してください。
