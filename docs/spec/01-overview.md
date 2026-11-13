# 1. 概要

## 1.1 構成

| 要素 | 責務 | 状態 |
| --- | --- | --- |
| Exporter | テキスト、外部字幕準備 Object、SRT Object を統合して SRT を保存 | 実装済み、新 Object 対応は次期実装 |
| Importer | 動画ドロップ時に同名 SRT を検出し、動画と SRTrack Object を初期配置・グループ化 | 初期実装済み、項目構成の更新待ち |
| SRTrack Object | 直上動画を参照して SRT を時間変換する主用途の Object | 次期実装 |
| SRTrack Object (Standalone) | 自身の再生設定で SRT を時間変換する補助的な Object | 次期実装 |
| Preview モジュール | 編集画面専用の字幕・診断表示 | 初期実装済み、直上参照方式へ更新予定 |
| SRTrack 共通設定 | Importer の有効状態と単体 SRT ドロップの扱いを利用者単位で管理 | 実装予定 |

Monitor は旧構成の初期実装であり、採用しない。詳細は [07-monitor.md](07-monitor.md) を参照する。

## 1.2 利用の流れ

動画と同名の SRT がある場合、動画をドロップすると Importer が動画の直上に SRTrack Object を配置し、両者をグループ化する。利用者は通常の編集操作で両者を扱い、Exporter は出力時に直上動画の現在状態から SRT 時刻を求める。

同名 SRT がない場合、または動画と別名の SRT を使う場合は、SRTrack Object の SRT ファイルを手動指定できる。動画と無関係の SRT は SRTrack Object (Standalone) として配置する。

Importer の動作は `SRTrack 設定…` から止められる。単体 SRT のドロップは、SRTrack Object、Standalone、または他のプラグインへ委譲する処理なしを選べる。これは動画ドロップ時の同名 SRT 自動取込とは別の設定である。

## 1.3 対象外

字幕の動画への焼き込み、音声認識、自動翻訳、YouTube への自動アップロード、動画ファイル名変更への SRT 自動追従は行わない。速度 0、逆再生、可変速度、中間点、時間制御、シーンをまたぐ利用は初期対象外である。
