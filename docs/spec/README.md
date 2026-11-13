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

## 実装状態

現行実装は旧単一 `SRTrack Object` と旧 Monitor を含む初期実装である。新しい主用途の SRTrack Object、SRTrack Object (Standalone)、直上動画参照型 Preview／Exporter、Monitor 廃止は次期の大幅変更であり、まだ実装済みではない。既存機能の概略動作確認は、新方式の受け入れ試験を意味しない。

完成時の配布物は `SRTrack.aux2`、`SRTrack.lua`、`SRTrack.obj2`、`SRTrackStandalone.obj2`、`SRTrack.mod2` の5ファイルとする。`SRTrack.aux2` は Exporter と Importer、`SRTrack.lua` は GCMZDrops2 ハンドラー、二つの Object と共通 Preview モジュールは Script 側に配置する。

SRTrack は未公開開発段階であり、旧単一 Object／Monitor を使う既存プロジェクトとの互換性は要求しない。次期実装では旧構成を置き換える。
