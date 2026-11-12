# SRTrack

AviUtl2 での SRT 字幕出力を支援するツールキットです。レンダリング済み動画を切り貼りして再編集する場合にも、元の字幕情報を引き継いで編集後の SRT を再構成できることを目指します。開発途中のプロジェクトで、Exporter、SRTrack Object、編集用 Preview モジュール、GCMZDrops2 連携の SRTrack Importer を収録しています。

## ツール一覧

- [SRTrack Exporter](tools/srtrack-exporter/README.md)：タイムライン上の対応字幕オブジェクトから SRT 字幕を出力する試作プラグイン。対応環境、ビルド方法、現在の制約は各ツールの README を参照してください。
- [SRTrack Object](tools/srtrack-object/README.md)：SRT と再生設定を保持するカスタムオブジェクトです。編集用の字幕プレビューは切り替え可能で、書き出し動画には反映されません。
- [SRTrack Importer](tools/srtrack-importer/README.md)：GCMZDrops2 の公開ハンドラー API を通じて動画ドロップ時に同名 SRT を検出し、動画と SRTrack Object をグループ化して追加します。複数動画はファイル名順に同じ二層へ並べます。移動・分割・伸縮への追従は未実装です。

## 開発状況

標準テキストからの字幕取得と SRT 出力、SRTrack Object の SRT 出力・編集用プレビュー・日本語パス読込・固定正速度への追従は AviUtl ExEdit2 2.1.13 で簡易確認済みです。SRTrack Object の `動画全長` を周期とするループは実装済みですが、実機検証は未完了です。SRTrack Importer は、エクスプローラーとメディアエクスプローラーからの単体・複数動画ドロップ、同名 SRT の検出、二層配置、ペアごとのグループ化、終端値の一致を簡易確認済みです。いずれも厳密な網羅テストではありません。移動・分割・伸縮への自動追従は未実装です。バイナリは収録していません。

## ライセンス

SRTrack Exporter、SRTrack Object、SRTrackPreview、SRTrack Importer のコードは [MIT License](tools/srtrack-exporter/LICENSE) です。SRTrack Importer は GCMZDrops2 と L-SMASH Works を前提プラグインとして想定しますが、いずれも同梱しません。同梱 SDK とその他の依存関係については [第三者の権利表記](tools/srtrack-exporter/THIRD_PARTY_NOTICES.md) を参照してください。
