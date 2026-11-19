# 1. 概要

## 1.1 構成

| 要素 | 責務 | 状態 |
| --- | --- | --- |
| Exporter | テキスト、外部字幕準備 Object、SRTrack Object を統合して SRT を保存 | 初期版完成。標準テキスト、セリフ準備、三種の SRTrack Object、候補統合、フレーム範囲選択出力に対応。細部は今後のバグフィクス・互換性確認で扱う |
| Importer | 動画＋同名 SRT の組取込と SRT 単体取込を支援する | 初期版完成。設定で二つの入口を個別に停止できる |
| SRTrack Object | 自身よりLayer 1側で最も近い動画（上側の直前動画Object）を参照して SRT を時間変換する主用途の Object | 初期版完成。Objectプロパティ・Importer配置は維持 |
| SRTrack Object (fix) | 自身の再生設定で SRT を時間変換する補助的な Object | 初期版完成。Object／Preview・Exporter の対象 |
| SRTrack Object (txt) | 自由入力テキストを Object 範囲の字幕として扱う補助的な Object | 初期版完成。Preview と Exporter の対象、動画書き出しには非表示 |
| Preview モジュール | 編集画面専用の字幕・診断表示 | Object／Preview 初期版完成 |
| SRTrack 設定 | Importer の有効状態、Preview の全体表示、共通診断ログを利用者単位で管理 | 2タブUI、Preview 全体設定、Importer・診断ログを実装。取込設定は再起動後に適用 |

Monitor は旧構成の初期実装であり、採用しない。詳細は [07-monitor.md](07-monitor.md) を参照する。

> 上側にある最も近い動画 Object を参照する規則により、フィルタ Object を動画と SRTrack Object の間へ置ける。Preview と Exporter に実装し、非動画 Object を間へ置いた実機確認も完了した。Importerの二層初期配置、グループ化、Objectのプロパティは変更しない。

## 1.2 利用の流れ

動画と同名の SRT がある場合、Importer は同じ時刻の隣接する2レイヤーに、上側（Layer 1 側）へ動画、下側へ SRTrack Object を配置し、両者をグループ化する。動画ペア取込は GCMZDrops2 を経由し、SRT 単体取込は AviUtl2 SDK の直接配置経路を使う。

同名 SRT がない場合、または動画と別名の SRT を使う場合は、SRTrack Object の SRT ファイルを手動指定できる。動画と無関係の SRT は SRTrack Object (fix) として配置する。

`SRTrack 設定…` では、動画ペア取込と SRT 単体取込をそれぞれオン・オフでき、Preview の全体表示を切り替え、共通診断ログを確認できる。SRT 単体取込は `SRTrack Object` または `SRTrack Object (fix)` を選べる。`txt` 型はテキスト専用のためドロップ対象にしない。取込設定は AviUtl2 の再起動後に反映され、Preview 表示はその場で切り替える。

## 1.3 対象外

字幕の動画への焼き込み、音声認識、自動翻訳、YouTube への自動アップロード、動画ファイル名変更への SRT 自動追従は行わない。逆再生、複雑な時間制御、シーンをまたぐ利用は初期対象外である。動画 Object の再生速度に中間点がある場合は、Exporter が区間ごとの一定速度近似で対応し、Preview も同じ近似へそろえる実装を予定する。標準テキストの時間制御文字は Exporter の範囲で対応するが、文字送りを一文字ごとの SRT 字幕として再現しない。

## 1.4 開発・検証の共通環境

`tests/manual/SRTrack-Regression.aup2` は、Preview と Exporter の代表的な正常系を一度に確認する手動回帰プロジェクトである。`tests/diagnostics/SRTrack-Diagnostics.aup2` は、出力を継続できるエラー・警告・情報診断を確認する別プロジェクトである。実装変更後は、標準設定で出力した SRT とテストマップを照合し、機能ごとの実際の結果を観察する。Importer のドロップ操作はこれらのプロジェクトに含めず、空シーンで別途確認する。

テスト用の `.aup2` と SRT fixture は Git 管理する一方、動画 fixture と Exporter 等の生成物は Git 管理しない。常設診断の設計方針と、テストプロジェクトの詳細な扱いはそれぞれ [05-errors-and-validation.md](05-errors-and-validation.md)、[09-manual-regression-test.md](09-manual-regression-test.md)、[10-diagnostic-regression-test.md](10-diagnostic-regression-test.md) を正とする。
