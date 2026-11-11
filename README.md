# SRTrack

AviUtl2 での SRT 字幕出力を支援するツールキットです。レンダリング済み動画を切り貼りして再編集する場合にも、元の字幕情報を引き継いで編集後の SRT を再構成できることを目指します。開発途中のプロジェクトで、現在は字幕出力プラグインの試作版を収録しています。

## ツール一覧

- [SRTrack Exporter](tools/srtrack-exporter/README.md)：標準テキストや PSDToolKit の「セリフ準備」から SRT 字幕を出力する試作プラグイン。対応環境、ビルド方法、現在の制約は各ツールの README を参照してください。
- SRTrack Object：SRT と再生設定を保持する非描画カスタムオブジェクトです。設計検討中で、まだ実装されていません。[全体仕様書](docs/spec/README.md)に目的、連携案、未決定事項をまとめています。

## 開発状況

字幕出力プラグインは AviUtl ExEdit2 2.1.13 を対象としています。標準テキストからの字幕取得と SRT 出力は同バージョンで確認済みです。PSDToolKit セリフ準備、レイヤー指定、オフセット、設定保存を含むすべての機能と SDK 互換性は継続して検証中です。バイナリは収録していません。

## ライセンス

SRTrack Exporter のライセンスは [MIT License](tools/srtrack-exporter/LICENSE) です。同梱 SDK とその他の依存関係については [第三者の権利表記](tools/srtrack-exporter/THIRD_PARTY_NOTICES.md) を参照してください。ツールキット全体の設計文書や今後追加するツールのライセンスは、別途定めます。
