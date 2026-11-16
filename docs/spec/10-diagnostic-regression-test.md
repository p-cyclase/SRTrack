# 10. Exporter 診断用手動回帰テスト

## 10.1 目的

`tests/diagnostics/SRTrack-Diagnostics.aup2` は、正常系の手動回帰プロジェクトとは別に、Exporter が出力を継続できるエラー・警告・情報診断を確認するためのプロジェクトである。診断が期待どおり表示・記録されることを確認するため、基準出力ではエラーと警告が発生することを正とする。

Importer のドロップ失敗、保存先への書込み失敗、権限や外部プラグインの有無に依存する事象は、このプロジェクトの常設ケースに含めない。

## 10.2 管理方針

```text
tests/
  diagnostics/
    SRTrack-Diagnostics.aup2
    README.md
    checklist.md
    fixtures/
      diagnostic-partial.srt
```

- プロジェクトは Git 管理する。生成される `SRTrack-Diagnostics.srt` は Git 管理しない。
- 動画 fixture は新設せず、`tests/manual/fixtures/regression-video.mp4` と `regression-video.srt`、`fix.srt` を読み取り専用で再利用する。
- `diagnostic-missing.srt` は**作成しない**。存在しない参照として `EXP-SRT-READ` を再現する。
- fixture の本文は、個人情報・第三者著作物を含まない識別用文字列だけを使う。

## 10.3 基準プロジェクト

プロジェクトは1920×1080、30fps、18秒とする。Exporter は初期設定（全レイヤー、シーン全体、最小表示時間1.000秒、SRTrack Object を対象）で実行する。

| 区間 | ケース | 期待する診断と出力 |
| --- | --- | --- |
| 0〜3秒 | 正常な SRTrack Object (fix) | `FIX-A` を出力し、後続の異常があっても出力が継続する基準になる |
| 3〜6秒 | 存在しない SRT を指定した fix Object | `EXP-SRT-READ` エラー。該当 Object だけを除外 |
| 6〜9秒 | 有効区間と不正区間を併せ持つ SRT を指定した fix Object | `EXP-SRT-PARSE` 警告。有効な `PARTIAL-VALID` だけを出力 |
| 9〜12秒 | ループ再生の動画と、`動画全長 D=0` の SRTrack Object | `EXP-VIDEO-DURATION` エラー。該当区間を除外 |
| 12〜15秒 | 上側に動画がない SRTrack Object | `EXP-VIDEO-END` 情報。警告・エラーを増やさず該当区間を出力しない |
| 15〜18秒 | 正常な SRTrack Object (txt) | `DIAG-TXT` を一つの字幕として出力し、異常後も処理継続を確認 |

基準出力の想定診断数は、エラー2件、警告1件、情報1件以上である。出力 SRT には少なくとも `FIX-A`、`PARTIAL-VALID`、`DIAG-TXT` が含まれる。診断数だけを固定の正解とせず、エラーコード・対象 Object・レイヤー・フレーム・出力継続を確認する。

## 10.4 確認項目

1. Exporter の結果モーダルに、出力継続の結果と診断詳細が表示される。
2. モーダルの「共通ログをコピー」と Importer 設定パネルの「診断情報をコピー」で、同じセッションの診断内容を取得できる。
3. `EXP-SRT-READ`、`EXP-SRT-PARSE`、`EXP-VIDEO-DURATION` は、それぞれ該当 Object だけを除外して出力を継続する。
4. `EXP-VIDEO-END` は情報であり、正常系プロジェクトでは警告モーダルを出さないことを [09-manual-regression-test.md](09-manual-regression-test.md) と合わせて確認する。
5. 手動追加確認として、存在しないレイヤー名による `EXP-LAYER-NOT-FOUND` の続行確認、フレーム範囲選択未指定時の `EXP-RANGE-MISSING` を確認する。

保存先の書込み失敗は、利用者の権限・ファイルを不必要に変更するおそれがあるため、再現手順を固定しない。実際の報告で発生したときは、常設診断ログを採取して個別に調査する。
