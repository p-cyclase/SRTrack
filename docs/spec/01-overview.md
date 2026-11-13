# 1. 概要

## 1.1 構成

| 要素 | 責務 | 状態 |
| --- | --- | --- |
| Exporter | テキスト、外部字幕準備 Object、SRT Object を統合して SRT を保存 | 標準テキスト、セリフ準備、初期配置された SRTrack Object に対応。高度な直上参照は後続課題 |
| Importer | 動画ドロップ時に同名 SRT を検出し、動画と SRTrack Object を初期配置・グループ化 | 調査・安定化中 |
| SRTrack Object | 直上動画を参照して SRT を時間変換する主用途の Object | Object／Preview 初期版完成。Exporter の完全対応は後続課題 |
| SRTrack Object (Standalone) | 自身の再生設定で SRT を時間変換する補助的な Object | Object／Preview 初期版完成。Exporter 出力は未対応 |
| Preview モジュール | 編集画面専用の字幕・診断表示 | Object／Preview 初期版完成 |
| SRTrack 共通設定 | Importer の有効状態と単体 SRT ドロップの扱いを利用者単位で管理 | Importer 安定化まで保留 |

Monitor は旧構成の初期実装であり、採用しない。詳細は [07-monitor.md](07-monitor.md) を参照する。

## 1.2 利用の流れ

動画と同名の SRT がある場合、Importer は動画の直上に SRTrack Object を配置し、両者をグループ化することを目指す。Importer は現在安定化中であり、動作が確認できるまでは手動での Object 配置と SRT 指定を代替手段とする。

同名 SRT がない場合、または動画と別名の SRT を使う場合は、SRTrack Object の SRT ファイルを手動指定できる。動画と無関係の SRT は SRTrack Object (Standalone) として配置する。

Importer の全体オン・オフと単体 SRT ドロップの扱いは、Importer 本体が安定するまで保留する。現在の稼働版には `SRTrack 設定…` を設けない。

## 1.3 対象外

字幕の動画への焼き込み、音声認識、自動翻訳、YouTube への自動アップロード、動画ファイル名変更への SRT 自動追従は行わない。速度 0、逆再生、可変速度、中間点、時間制御、シーンをまたぐ利用は初期対象外である。
