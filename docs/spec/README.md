# SRTrack 全体仕様書

SRTrack は AviUtl ExEdit2 上の字幕ソースを統合し、編集後のタイムラインに対応する SRT を出力するツールキットである。正規仕様は本ディレクトリの文書とする。

## コア・コンセプト

レンダリング済み動画だけから、元の字幕文字列とタイミングを正確に復元することは難しい。SRTrack は動画と対応する SRT をタイムラインへ置き、動画を連結・切抜き・分割・速度変更した後にも、元字幕に近い SRT を再出力できるようにする。

このための字幕 Object は二種類である。`SRTrack Object` は直上レイヤーの標準動画 Object を必要な時だけ参照する主用途の Object、`SRTrack Object (Standalone)` は動画に依存せず自身の再生開始位置と速度を保持する補助的な Object である。動画の状態をコピーし続ける常駐 Monitor は採用しない。

## 文書

| 文書 | 内容 |
| --- | --- |
| [01-overview.md](01-overview.md) | 構成、利用の流れ、対象外 |
| [02-srt-source.md](02-srt-source.md) | SRTrack Object／SRTrack Object (Standalone) と SRT 入力 |
| [03-timing.md](03-timing.md) | 時間変換と直上動画参照 |
| [04-export.md](04-export.md) | 字幕統合と SRT 出力 |
| [05-errors-and-validation.md](05-errors-and-validation.md) | エラー、互換性、検証計画 |
| [06-importer.md](06-importer.md) | Importer の初期配置 |
| [07-monitor.md](07-monitor.md) | Monitor 廃止方針 |
| [08-settings.md](08-settings.md) | SRTrack 全体に適用する利用者設定 |

## 実装状態

現行実装は、二つの Object、Monitor 廃止、直上動画参照型の Preview／Exporter を含む初期実装である。Preview は動画と SRTrack Object の開始位置差を反映する。分割・伸縮・速度変更・ループを含む標準動画との完全一致、および網羅的な受け入れ試験は未完了である。

完成時の配布物は `SRTrack.aux2`、`SRTrack.lua`、`SRTrackObject.obj2`、`SRTrackObject(Standalone).obj2`、`SRTrack.mod2` の5ファイルとする。`SRTrack.aux2` は Exporter と Importer、`SRTrack.lua` は GCMZDrops2 ハンドラー、二つの Object と共通 Preview モジュールは Script 側に配置する。

Object 追加メニューでは `SRTrack` の下に `SRTrack Object` と `SRTrack Object (Standalone)` を表示する。両方の Object は編集用の字幕 Preview に対応する。

SRTrack は未公開開発段階であり、旧単一 Object／Monitor を使う既存プロジェクトとの互換性は要求しない。次期実装では旧構成を置き換える。
