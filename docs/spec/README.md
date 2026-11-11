# SRTrack 全体仕様書

SRTrack は、AviUtl ExEdit2 上で字幕を扱い、編集後のタイムラインに対応する SRT を出力するためのツールキットである。本仕様書は、実装済みの SRTrack Exporter と、今後実装する SRTrack Object を共通の目的に沿って整理する。

## 文書の読み方

- **実装済み**：現行ソースに存在する機能。ただし実機確認の範囲は別記する。
- **初期版の方針**：実装の基準にしたい提案。未実装または実機未検証であり、変更できる。
- **未決定**：実装前に決める必要がある事項。ここを推測で固定しない。

| 文書 | 内容 |
| --- | --- |
| [01-overview.md](01-overview.md) | 目的、構成、利用の流れ、用語、対象外 |
| [02-srt-source.md](02-srt-source.md) | SRTrack Object と入力データ |
| [03-timing.md](03-timing.md) | シーン時刻と SRT 時刻の変換 |
| [04-export.md](04-export.md) | 字幕ソースの統合と SRT 出力 |
| [05-errors-and-validation.md](05-errors-and-validation.md) | エラー、互換性、検証計画 |

検討時の原文は [srtrack-design.md](../srtrack-design.md) に残す。この仕様書と矛盾が見つかった場合は、実装済みの挙動、既存の SRTrack Exporter 仕様書、利用者の判断の順に確認する。

## 現在の実装状況

| 要素 | 状態 |
| --- | --- |
| SRTrack Exporter | 実装済み。標準テキストからの字幕取得と SRT 出力は AviUtl2 2.1.13 で確認済み。 |
| PSDToolKit のセリフ準備の読取り | 実装済み。実機での互換性・設定値の保存形式は未検証。 |
| SRTrack Object | 未実装。 |
| SRTrack Object からの出力 | 未実装。 |
| 字幕プレビュー／描画 | 初期版の対象外。 |

## 初期版の到達点

初期版は、UTF-8 SRT を参照する非描画の読み込みオブジェクトをタイムラインに置き、固定の正の速度・ループなしの範囲で、元 SRT の区間を編集後のシーン時刻に変換して出力できることを目標とする。標準テキスト、PSDToolKit のセリフ準備、SRT 由来の字幕を混在できることを目指す。
