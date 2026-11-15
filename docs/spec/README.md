# SRTrack 全体仕様書

SRTrack は AviUtl ExEdit2 上の字幕ソースを統合し、編集後のタイムラインに対応する SRT を出力するツールキットである。正規仕様は本ディレクトリの文書とする。

## コア・コンセプト

レンダリング済み動画だけから、元の字幕文字列とタイミングを正確に復元することは難しい。SRTrack は動画と対応する SRT をタイムラインへ置き、動画を連結・切抜き・分割・速度変更した後にも、元字幕に近い SRT を再出力できるようにする。

このための字幕 Object は二種類である。`SRTrack Object` は自身の1つ上のレイヤー（Layer 1 側）の標準動画 Object を必要な時だけ参照する主用途の Object、`SRTrack Object (Standalone)` は動画に依存せず自身の再生開始位置と速度を保持する補助的な Object である。動画の状態をコピーし続ける常駐 Monitor は採用しない。

## 文書

| 文書 | 内容 |
| --- | --- |
| [01-overview.md](01-overview.md) | 構成、利用の流れ、対象外 |
| [02-srt-source.md](02-srt-source.md) | SRTrack Object／SRTrack Object (Standalone) と SRT 入力 |
| [03-timing.md](03-timing.md) | 時間変換と1つ上のレイヤーの動画参照 |
| [04-export.md](04-export.md) | 字幕統合と SRT 出力 |
| [05-errors-and-validation.md](05-errors-and-validation.md) | エラー、互換性、検証計画 |
| [06-importer.md](06-importer.md) | Importer の初期配置 |
| [07-monitor.md](07-monitor.md) | Monitor 廃止方針 |
| [08-settings.md](08-settings.md) | SRTrack 全体に適用する利用者設定 |
| [09-manual-regression-test.md](09-manual-regression-test.md) | Preview／Exporter 用の手動回帰テストプロジェクト計画 |

## 実装状態

現行実装は、二つの Object、Monitor 廃止、1つ上のレイヤーの動画を参照する Preview、動画＋同名 SRT の組取込、SRT 単体取込、標準テキスト・セリフ準備・二種類の SRTrack Object を扱う Exporter を含む。Object／Preview、Importer、Exporter のすべてを初期版完成とし、SRTrack の実装初期版も完成とする。Preview と Exporter は、動画と SRTrack Object の開始位置差、1つ上のレイヤーの動画の途中切替・不在・再出現、Standalone の出力に対応する。Exporter は候補統合、短字幕結合、フレーム範囲選択、時間制御文字の出力を含む。

SRTrack の初期版完成は、要求済みの機能を実装し、代表的な実機確認を終えたことを意味する。Object／Preview はSRT参照と編集用表示、Importerは動画ペア取込・SRT単体取込・通常入力への委譲・設定の再起動後反映、Exporterは字幕候補の統合・短字幕結合・出力範囲・時間制御文字・動画不在後の再出現を実装・概略確認済みとする。

分割・伸縮・速度変更・ループを含む標準動画との完全一致、対応環境・他プラグインとの網羅試験、異常入力、表示上の細部は、今後のバグフィクスと互換性確認として扱う。これらは初期版の未実装項目ではなく、公開後も手動回帰プロジェクトと常設診断を用いて育てる対象である。

完成時の配布物は `SRTrack.aux2`、`SRTrack.lua`、`SRTrackObject.obj2`、`SRTrackObject(Standalone).obj2`、`SRTrack.mod2` の5ファイルとする。`SRTrack.aux2` は Exporter と Importer、`SRTrack.lua` は GCMZDrops2 ハンドラー、二つの Object と共通 Preview モジュールは Script 側に配置する。

Object 追加メニューでは `SRTrack` の下に `SRTrack Object` と `SRTrack Object (Standalone)` を表示する。両方の Object は編集用の字幕 Preview に対応する。

SRTrack は未公開開発段階であり、旧単一 Object／Monitor を使う既存プロジェクトとの互換性は要求しない。
