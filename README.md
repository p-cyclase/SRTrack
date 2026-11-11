# SRTrack

AviUtl2 での SRT 字幕出力を支援するツールキットです。レンダリング済み動画を切り貼りして再編集する場合にも、元の字幕情報を引き継いで編集後の SRT を再構成できることを目指します。開発途中のプロジェクトで、Exporter、SRTrack Object、編集用 Preview モジュールを収録しています。

## ツール一覧

- [SRTrack Exporter](tools/srtrack-exporter/README.md)：標準テキストや PSDToolKit の「セリフ準備」から SRT 字幕を出力する試作プラグイン。対応環境、ビルド方法、現在の制約は各ツールの README を参照してください。
- [SRTrack Object](tools/srtrack-object/README.md)：SRT と再生設定を保持するカスタムオブジェクトです。編集用の字幕プレビューは切り替え可能で、書き出し動画には反映されません。動画との自動生成・同期は実装中です。[全体仕様書](docs/spec/README.md)に目的、連携案、未決定事項をまとめています。

## 開発状況

標準テキストからの字幕取得と SRT 出力、SRTrack Object の SRT 出力・編集用プレビュー・日本語パス読込・固定正速度への追従は AviUtl ExEdit2 2.1.13 で簡易確認済みです。厳密な網羅テストではありません。動画との自動ペアリング、移動・分割・伸縮への自動追従、ループは未実装です。特に分割・伸縮は、対応する動画の再生範囲終端を安全に扱う仕組みが未実装であるため保留しています。バイナリは収録していません。

## ライセンス

SRTrack Exporter、SRTrack Object、SRTrackPreview のコードは [MIT License](tools/srtrack-exporter/LICENSE) です。同梱 SDK とその他の依存関係については [第三者の権利表記](tools/srtrack-exporter/THIRD_PARTY_NOTICES.md) を参照してください。
