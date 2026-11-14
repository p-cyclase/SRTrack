# 1. 概要

## 1.1 構成

| 要素 | 責務 | 状態 |
| --- | --- | --- |
| Exporter | テキスト、外部字幕準備 Object、SRT Object を統合して SRT を保存 | 最小実装済み。標準テキスト、セリフ準備、二種類の SRTrack Object、候補統合、フレーム範囲選択出力に対応。実機検証とブラッシュアップ中 |
| Importer | 動画＋同名 SRT の組取込と SRT 単体取込を支援する | 初期版完成。設定で二つの入口を個別に停止できる |
| SRTrack Object | 自身の1つ上のレイヤーにある動画を参照して SRT を時間変換する主用途の Object | Object／Preview 初期版完成。Exporter 最小実装も対応済み |
| SRTrack Object (Standalone) | 自身の再生設定で SRT を時間変換する補助的な Object | Object／Preview 初期版完成。Exporter 最小実装も対応済み |
| Preview モジュール | 編集画面専用の字幕・診断表示 | Object／Preview 初期版完成 |
| SRTrack Importer 設定 | Importer の有効状態と単体 SRT ドロップの扱いを利用者単位で管理 | 初期版実装済み。再起動後に適用 |

Monitor は旧構成の初期実装であり、採用しない。詳細は [07-monitor.md](07-monitor.md) を参照する。

## 1.2 利用の流れ

動画と同名の SRT がある場合、Importer は同じ時刻の隣接する2レイヤーに、上側（Layer 1 側）へ動画、下側へ SRTrack Object を配置し、両者をグループ化する。動画ペア取込は GCMZDrops2 を経由し、SRT 単体取込は AviUtl2 SDK の直接配置経路を使う。

同名 SRT がない場合、または動画と別名の SRT を使う場合は、SRTrack Object の SRT ファイルを手動指定できる。動画と無関係の SRT は SRTrack Object (Standalone) として配置する。

`SRTrack Importer 設定…` では、動画ペア取込と SRT 単体取込をそれぞれオン・オフできる。SRT 単体取込は `SRTrack Object` または `SRTrack Object (Standalone)` を選べる。設定は AviUtl2 の再起動後に反映される。

## 1.3 対象外

字幕の動画への焼き込み、音声認識、自動翻訳、YouTube への自動アップロード、動画ファイル名変更への SRT 自動追従は行わない。逆再生、可変速度、中間点、シーンをまたぐ利用は初期対象外である。標準テキストの時間制御文字は Exporter の範囲で対応するが、文字送りを一文字ごとの SRT 字幕として再現しない。
