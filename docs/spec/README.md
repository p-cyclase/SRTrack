# SRTrack 全体仕様書

SRTrack は AviUtl ExEdit2 上の字幕ソースを統合し、編集後のタイムラインに対応する SRT を出力するツールキットである。正規仕様は本ディレクトリの文書とする。

## コア・コンセプト

レンダリング済み動画だけから、元の字幕文字列とタイミングを正確に復元することは難しい。SRTrack は動画と対応する SRT をタイムラインへ置き、動画を連結・切抜き・分割・速度変更した後にも、元字幕に近い SRT を再出力できるようにする。

このための字幕 Object は二種類である。`SRTrack Link Object` は直上レイヤーの標準動画 Object を必要な時だけ参照する。`SRTrack Clip Object` は動画に依存せず、自身の再生開始位置と速度を保持する。動画の状態をコピーし続ける常駐 Monitor は採用しない。

## 文書

| 文書 | 内容 |
| --- | --- |
| [01-overview.md](01-overview.md) | 構成、利用の流れ、対象外 |
| [02-srt-source.md](02-srt-source.md) | Link／Clip Object と SRT 入力 |
| [03-timing.md](03-timing.md) | 時間変換と直上動画参照 |
| [04-export.md](04-export.md) | 字幕統合と SRT 出力 |
| [05-errors-and-validation.md](05-errors-and-validation.md) | エラー、互換性、検証計画 |
| [06-importer.md](06-importer.md) | Importer の初期配置 |
| [07-monitor.md](07-monitor.md) | Monitor 廃止方針 |

## 実装状態

現行実装は旧単一 `SRTrack Object` と旧 Monitor を含む初期実装である。新しい Link／Clip Object、直上動画参照型 Preview／Exporter、Monitor 廃止は次期の大幅変更であり、まだ実装済みではない。既存機能の概略動作確認は、新方式の受け入れ試験を意味しない。

完成時の配布物は当面 `SRTrack.aux2`、`SRTrack.lua`、`SRTrack.obj2`、`SRTrack.mod2` の4ファイルを維持する。`SRTrack.aux2` は Exporter と Importer、`SRTrack.lua` は GCMZDrops2 ハンドラー、Object と Preview モジュールは Script 側に配置する。複数 Object の最終ファイル構成は実装時に確定する。
