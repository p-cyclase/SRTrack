# SRTrack 手動回帰テスト

`SRTrack-Regression.aup2` は、Preview と Exporter の手動回帰確認用プロジェクトである。Importer のドロップ試験は含めない。

## 前提

- SRTrack の配布物を AviUtl2 へ導入済みであること。
- `fixtures/regression-video.mp4`、`fixtures/regression-video.srt`、`fixtures/standalone.srt` を移動・改名しないこと。
- プロジェクトは 1920×1080、30fps、210秒である。動画fixture自体は30秒であり、複数回参照する。

`regression-video.mp4` には、`regression-video.srt` の3秒ごとの区間に対応する基準表示を入れる。Previewの字幕と動画上の基準表示が同じ区間を指すことを、目視で確認するためである。動画を差し替える場合も、ファイル名・配置・30秒の長さ・30fpsと、SRTの区間に対応する基準表示を維持する。

## 配置済みのテストケース

| AviUtl2レイヤー | 区間 | 内容 |
| --- | --- | --- |
| Layer 1〜2 | 0〜30秒 | 通常速度の動画追従と基準SRT |
| Layer 1〜2 | 30〜45秒 | 同じ動画・SRTによる2倍速ケース |
| Layer 1〜2 | 45〜60秒 | 再生開始位置 `P=10` 秒のケース |
| Layer 1〜2 | 60〜75秒 | 動画より3秒遅れて始まるObjectの開始位置差 |
| Layer 2 | 75〜90秒 | 1つ上に動画がない動画追従Object |
| Layer 2 | 90〜105秒 | Standalone Object と `standalone.srt` |
| Layer 3〜6 | 105〜110秒 | 同時開始する `SIM-A`〜`SIM-D` |
| Layer 3 | 110〜128秒 | `<r>`、`<w>`、`<w*>`、`<c>`、`<c*>` |
| Layer 3 | 128〜134秒 | ルビ・絵文字・装飾・コメント・Lua の変換 |
| Layer 1〜3 | 134〜140秒 | 動画追従SRTを `INTERRUPT` で中断 |
| Layer 1〜2 | 140〜150秒 | 動画より早く終わるObject |
| Layer 1〜2 | 150〜180秒 | 同一Object内で動画が終了・不在・再出現するケース |
| Layer 1〜2 | 180〜210秒 | 同一Object内で通常速度から2倍速へ動画を切り替えるケース |

`.aup2` は AviUtl2 の保存形式に従い、現在の作業環境の絶対パスを保持する。別の場所で開く場合は、動画 Object の `ファイル`、SRTrack Object の `SRT ファイル`、Standalone Object の `SRT ファイル` をそれぞれ `fixtures/` 内の対応ファイルへ指定し直す。

詳細な項目は [checklist.md](checklist.md) を参照する。
