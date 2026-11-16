# SRTrack 診断用手動回帰テスト

`SRTrack-Diagnostics.aup2` は、Exporter が出力継続できる異常入力と診断表示を確認するための18秒・30fpsプロジェクトです。通常の Preview／Exporter 回帰は `../manual/SRTrack-Regression.aup2` を使います。

## 前提

- SRTrack の配布物を AviUtl2 へ導入済みであること。
- `../manual/fixtures/regression-video.mp4`、`regression-video.srt`、`fix.srt` を移動・改名しないこと。
- `fixtures/diagnostic-partial.srt` は Git 管理する。
- `fixtures/diagnostic-missing.srt` は意図的に存在しない。作成しないこと。

## 基準出力

初期設定でシーン全体を出力します。出力は継続し、結果モーダルにはエラー2件・警告1件を含む診断詳細が表示されます。SRTには `FIX-A`、`PARTIAL-VALID`、`DIAG-TXT` が含まれます。

| 区間 | ケース | 主な診断 |
| --- | --- | --- |
| 0〜3秒 | 正常なfix Object | なし |
| 3〜6秒 | 存在しないSRT | `EXP-SRT-READ` エラー |
| 6〜9秒 | 一部壊れたSRT | `EXP-SRT-PARSE` 警告 |
| 9〜12秒 | ループ動画と `D=0` | `EXP-VIDEO-DURATION` エラー |
| 12〜15秒 | 上側に動画なし | `EXP-VIDEO-END` 情報 |
| 15〜18秒 | txt Object | なし |

詳細な操作は [checklist.md](checklist.md) を参照してください。
