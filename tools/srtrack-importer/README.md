# SRTrack Importer for AviUtl2

GCMZDrops2 の公開ハンドラースクリプト API を通じ、動画ファイルのドラッグ＆ドロップ時に同じフォルダの同名 SRT を検出して SRTrack Object を追加する試作コンポーネントです。GCMZDrops2 を前提プラグインとし、GCMZDrops2 自身は同梱しません。

対応 SRT がある場合も、動画ファイルは最終ドロップリストに残します。したがって L-SMASH Works 等の既存入力プラグインが従来どおり動画を読み込みます。Importer は SRTrack Object を定義した一時 `.object` を追加し、動画と Object を同じ長さで作成します。配置は、動画直下のレイヤーと二段分の区間が空く最初の後方時刻へ、ペア全体をずらして行います。既存オブジェクトは動かしません。Object には SRT パス、再生位置 `0`、再生範囲終端、動画全長、再生速度 `100` を設定します。SRT がなければファイルリストを変更せず、動画は通常どおり配置され、通知もしません。

初期版では、動画と Object のグループ化、および分割・伸縮への自動追従は未実装です。対応する動画と Object を同じ区間に配置するところまでを扱います。

## ビルドと導入

GCMZDrops2、L-SMASH Works、SRTrack Object、SRTrackPreview を導入した環境が必要です。GCMZDrops2 の公開 API により、SRTrack Importer 側からハンドラースクリプトを登録する予定です。具体的な導入手順は実装・実機検証後に確定します。

現在の `SRTrackImporter.aux2` は、直接 MP4 を入力担当にする試作版です。この方式は L-SMASH Works の読込と競合することが確認されたため、利用しないでください。GCMZDrops2 連携版が完成するまで、動画と SRTrack Object は手動配置してください。

## ライセンス

コードは [MIT License](../srtrack-exporter/LICENSE) です。SDK の権利表記は [SRTrack Exporter の第三者表記](../srtrack-exporter/THIRD_PARTY_NOTICES.md) を参照してください。
