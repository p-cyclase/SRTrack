# SRTrack

AviUtl2 での SRT 字幕出力を支援するツールキットです。レンダリング済み動画を切り貼りして再編集する場合にも、元の字幕情報を引き継いで編集後の SRT を再構成できることを目指します。開発途中のプロジェクトで、Exporter、SRTrack Object、編集用 Preview モジュールを収録しています。動画と同名の SRT を対応づけて配置する SRTrack Importer を新設予定です。

## ツール一覧

- [SRTrack Exporter](tools/srtrack-exporter/README.md)：標準テキストや PSDToolKit の「セリフ準備」から SRT 字幕を出力する試作プラグイン。対応環境、ビルド方法、現在の制約は各ツールの README を参照してください。
- [SRTrack Object](tools/srtrack-object/README.md)：SRT と再生設定を保持するカスタムオブジェクトです。編集用の字幕プレビューは切り替え可能で、書き出し動画には反映されません。
- SRTrack Importer（新設予定）：動画ドロップ時に同名 SRT を検出し、動画と Object の二段分を置ける最初の後方時刻へ両者を配置・グループ化して、動画の総時間から参照範囲終端を初期化します。既存オブジェクトは動かしません。

## 開発状況

標準テキストからの字幕取得と SRT 出力、SRTrack Object の SRT 出力・編集用プレビュー・日本語パス読込・固定正速度への追従は AviUtl ExEdit2 2.1.13 で簡易確認済みです。厳密な網羅テストではありません。SRTrack Importer、動画との自動ペアリング、移動・分割・伸縮への自動追従、ループは未実装です。Importer による動画総時間からの終端初期化を設計済みですが、分割・伸縮時の Object 側の追従は実装・検証を保留しています。バイナリは収録していません。

## ライセンス

SRTrack Exporter、SRTrack Object、SRTrackPreview のコードは [MIT License](tools/srtrack-exporter/LICENSE) です。同梱 SDK とその他の依存関係については [第三者の権利表記](tools/srtrack-exporter/THIRD_PARTY_NOTICES.md) を参照してください。
