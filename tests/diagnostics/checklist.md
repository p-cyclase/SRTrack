# 診断用手動回帰チェックリスト

結果欄には、確認日、AviUtl2版、SRTrackコミット、結果を記録する。

| # | 操作 | 期待結果 | 結果 |
| --- | --- | --- | --- |
| P1 | 0〜3秒を再生・シークする | fix Objectが `[SRTrack] FIX-A` を表示する | |
| P2 | 3〜6秒を再生・シークする | fix Objectが `[SRTrack: 読み込みに失敗]` を付けてSRT読込失敗を表示する | |
| P3 | 6〜9秒を再生・シークする | 部分破損SRTの有効な `[SRTrack] PARTIAL-VALID` を表示する | |
| P4 | 9〜12秒を再生・シークする | 動画追従Objectが `[SRTrack: 設定]` を付けてループ用の全長不足を表示する | |
| P5 | 12〜15秒を再生・シークする | 動画追従Objectが `[SRTrack: 注意]` を付けて上側の動画不在を表示する | |
| P6 | 15〜18秒を再生・シークする | txt Objectが `[SRTrack] DIAG-TXT` を表示する | |
| E1 | 初期設定でシーン全体をExporter出力する | 出力が継続し、結果モーダルにエラー2件・警告1件が表示される | |
| E2 | E1の結果モーダルと共通ログを確認する | `EXP-SRT-READ`、`EXP-SRT-PARSE`、`EXP-LOOP-DURATION`、`EXP-VIDEO-END` に、Object・レイヤー・フレームが記録される | |
| E3 | E1の出力SRTを確認する | `FIX-A`、`PARTIAL-VALID`、`DIAG-TXT` を含み、欠損・不正・D不足の各区間は出力しない | |
| E4 | 結果モーダルの「共通ログをコピー」とImporter設定の「診断情報をコピー」を使う | 同じ起動中の診断内容をコピーできる | |
| E5 | Exporterで存在しないレイヤー名を指定する | `EXP-LAYER-NOT-FOUND` の続行確認が表示され、共通ログに残る | |
| E6 | フレーム範囲選択を選び、選択範囲を解除して出力する | `EXP-RANGE-MISSING` が表示され、共通ログに残る | |
