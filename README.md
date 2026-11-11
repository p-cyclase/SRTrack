# SRTrack

AviUtl2 での SRT 字幕出力を支援するツールキットです。開発途中のプロジェクトで、現在は字幕出力プラグインの試作版を収録しています。

## ツール一覧

- [SubtitleExport](tools/subtitle-export/README.md)：標準テキストや PSDToolKit の「セリフ準備」から SRT 字幕を出力する試作プラグイン。対応環境、ビルド方法、現在の制約は各ツールの README を参照してください。
- SRT 読み込みカスタムオブジェクト：設計検討中です。まだ実装されていません。[設計メモ](docs/srtrack-design.md)に目的、連携案、未決定事項をまとめています。

## 開発状況

字幕出力プラグインは AviUtl ExEdit2 2.1.13 を対象としていますが、同バージョンでの全機能の動作と SDK 互換性は未検証です。テスト用プロジェクトで使用し、出力結果を確認してください。バイナリは収録していません。

## ライセンス

字幕出力プラグインのライセンスは [MIT License](tools/subtitle-export/LICENSE) です。同梱 SDK とその他の依存関係については [第三者の権利表記](tools/subtitle-export/THIRD_PARTY_NOTICES.md) を参照してください。ツールキット全体の設計文書や今後追加するツールのライセンスは、別途定めます。
