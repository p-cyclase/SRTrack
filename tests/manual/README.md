# SRTrack 手動回帰テスト

`SRTrack-Regression.aup2` は、Preview と Exporter の手動回帰確認用プロジェクトである。Importer のドロップ試験は含めない。

## 前提

- SRTrack の配布物を AviUtl2 へ導入済みであること。
- `fixtures/regression-video.mp4`、`fixtures/regression-video.srt`、`fixtures/standalone.srt` を移動・改名しないこと。
- プロジェクトは 1920×1080、30fps、30秒である。

`regression-video.mp4` には、`regression-video.srt` の3秒ごとの区間に対応する基準表示を入れる。Previewの字幕と動画上の基準表示が同じ区間を指すことを、目視で確認するためである。動画を差し替える場合も、ファイル名・配置・30秒の長さ・30fpsと、SRTの区間に対応する基準表示を維持する。

## 配置済みのテストケース

| AviUtl2レイヤー | 区間 | 内容 |
| --- | --- | --- |
| Layer 1 | 0〜30秒 | 基準動画 `regression-video.mp4` |
| Layer 2 | 0〜30秒 | 動画追従型 SRTrack Object と `regression-video.srt` |
| Layer 3 | 12〜18秒 | `<r10>AB<c0.5>CD` による待機・クリア境界 |
| Layer 4〜5 | 20〜22秒 | 同時開始する `上`／`下` の本文順 |
| Layer 6 | 24〜28秒 | ルビ・絵文字・装飾・コメント・Lua の除去・変換 |
| Layer 7 | 28〜30秒 | Standalone Object と `standalone.srt` |

`.aup2` は AviUtl2 の保存形式に従い、現在の作業環境の絶対パスを保持する。別の場所で開く場合は、動画 Object の `ファイル`、SRTrack Object の `SRT ファイル`、Standalone Object の `SRT ファイル` をそれぞれ `fixtures/` 内の対応ファイルへ指定し直す。

詳細な項目は [checklist.md](checklist.md) を参照する。
