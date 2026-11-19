# 変更履歴

この変更履歴は [Keep a Changelog](https://keepachangelog.com/ja/1.1.0/) の形式を参考にする。各公開版の正確な配布物は GitHub Releases のタグと ZIP を正とする。

## [Unreleased]

## [0.1.1] - Unreleased

### Added

- 動画 Object の中間点ごとに再生位置・速度を再取得する、Exporter と Preview の時間変換。
- 指定値未満の短い空白を直前の字幕で補間する `空白補間時間`。

### Changed

- 最小表示時間を `最小字幕時間` へ改称し、最小字幕時間と空白補間時間の入力下限を `0.000` 秒へ変更。
- ルビ・絵文字を含む時間制御文字の文字数計算を、AviUtl2 の表示上の文字単位へ近づけた。

## [0.1.0] - 2026-11-18

### Added

- `SRTrack Object`、`SRTrack Object (fix)`、`SRTrack Object (txt)`。
- 通常 Object と fix Object のループ再生、および全長を周期とする字幕時刻の変換。
- 標準テキスト、セリフ準備、三種の SRTrack Object を統合する SRT Exporter。
- 動画＋同名 SRT の組取込と SRT 単体取込を支援する Importer。
- 編集用字幕 Preview、共通診断ログ、正常系・診断系の手動回帰プロジェクト。
- 公開リポジトリ向けの利用者 README、リリース手順、診断を添えた不具合報告導線。
