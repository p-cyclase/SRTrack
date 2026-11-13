# SRTrack

SRTrack は AviUtl2 で編集後の動画に対応する SRT 字幕を出力するツールキットです。

## コア・コンセプト

動画ファイルだけから元の字幕を正確に復元することはできません。SRTrack は、動画と同時に用意した SRT をタイムラインへ対応づけ、レンダリング済み動画の連結・切抜き・分割・速度変更後にも、元の文字列とタイミングに近い SRT を再出力することを目指します。

SRT を動画へ焼き込むことは目的ではありません。編集用字幕プレビューは補助機能であり、動画書き出しには反映されません。Exporter は動画を書き出さない通常の SRT エクスポーターで、保存済み `.aup2` の `output.file` を読める場合だけ同名 `.srt` を保存先候補として提案します。

## Object モデル

SRTrack は、旧単一 Object の役割を二つの Object に分けています。

| Object | 役割 |
| --- | --- |
| SRTrack Object | 直上レイヤーの標準動画 Object を必要時に参照し、その再生位置・速度・ループに従って SRT を変換する。SRT パスと動画全長 `D` を保持する。 |
| SRTrack Object (Standalone) | 動画を参照せず、自身の再生開始位置・速度とタイムライン範囲で SRT を変換する。 |

動画とのグループ化は、両者の上下関係を編集操作で保ちやすくする補助です。字幕処理の条件はグループ ID ではなく、SRTrack Object の直上レイヤーにある動画です。常駐 Monitor による値コピー・ファイル名追従は廃止します。

## 構成

| 要素 | 役割 | 状態 |
| --- | --- | --- |
| Exporter | タイムライン上の字幕ソースを集め、SRT を出力する | 標準テキスト、セリフ準備、初期配置された SRTrack Object を扱う。高度な直上動画参照は後続課題 |
| Importer | 動画ドロップ時に同名 SRT を検出し、動画と SRTrack Object を配置・グループ化する | 調査・安定化中 |
| Object / Preview | SRT の参照、編集用字幕プレビュー | 二 Object と両方の Preview を実装済み |

配布物は `SRTrack.aux2`、`SRTrack.lua`、`SRTrackObject.obj2`、`SRTrackObject(Standalone).obj2`、`SRTrack.mod2` の5ファイルです。二つの Object は同じ Preview モジュールを共有します。

## ドキュメント

現行・次期の正規仕様は [全体仕様書](docs/spec/README.md) を参照してください。[初期検討メモ](docs/srtrack-design.md) と `docs/components/` の一部は履歴資料であり、正規仕様ではありません。

## 前提プラグイン

Importer を使う場合は GCMZDrops2 と L-SMASH Works が必要です。いずれも SRTrack には同梱しません。SRTrack は GCMZDrops2 の公開 API のみを利用し、AviUtl2 標準 Object 以外の外部ファイルやプラグインのリバースエンジニアリングは行いません。

## ライセンス

SRTrack のライセンスは MIT License です。AviUtl2 Plugin SDK の同梱ヘッダーは SDK 付属の MIT License に従います。外部前提プラグインは各権利者の条件に従います。
