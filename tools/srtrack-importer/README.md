# SRTrack Importer for AviUtl2

GCMZDrops2 の公開ハンドラースクリプト API を通じ、動画ファイルのドラッグ＆ドロップ時に同じフォルダの同名 SRT を検出して SRTrack Object を追加する試作コンポーネントです。GCMZDrops2 と L-SMASH Works を前提プラグインとし、いずれも同梱しません。

対応 SRT がある場合、Importer は元の動画ファイルを、標準 AviUtl2 の複数オブジェクト形式の一時 `.object` へ置換します。このエイリアスには動画ファイルと SRTrack Object を含め、両者へ同じグループ値を設定します。動画は下側、Object はその直上に作成されます。Object には SRT パス、再生位置 `0`、再生範囲終端、動画全長、再生速度 `100` を設定します。SRT がなければファイルリストを変更せず、動画は通常どおり配置され、通知もしません。

初期版は、動画と Object を同じドロップ操作で作成し、グループ化するところまでを実装しています。分割・伸縮への自動追従は未実装です。動画直下への配置と二段分の空き区間への後方移動は、実機検証が必要です。

## ビルドと導入

GCMZDrops2、L-SMASH Works、SRTrack Object、SRTrackPreview を導入した環境が必要です。`build.cmd` を実行して生成した `SRTrackImporter.aux2` と、`SRTrackImporter.lua` を、AviUtl2 の `Plugin\\SRTrack` など **SRTrack 専用の同じフォルダ**へコピーして AviUtl2 を再起動してください。GCMZDrops2 のフォルダへ SRTrack のファイルをコピーする必要はありません。

`SRTrackImporter.aux2` は、GCMZDrops2 が公開している `AddHandlerScriptFile` だけを使い、同じ SRTrack フォルダ内の Lua ファイルを起動時に登録します。GCMZDrops2 が導入・有効化されていない場合は、通常の動画ドロップに介入しません。

対象は、L-SMASH Works で一般に扱われる動画コンテナと `.avi` です。実際に動画トラックを持つことを GCMZDrops2 の公開メディア情報 API で確認してから Object を追加します。L-SMASH Works 側で独自に追加した拡張子は、初期版の候補一覧には含まれない場合があります。

## ライセンス

コードは [MIT License](../srtrack-exporter/LICENSE) です。SDK の権利表記は [SRTrack Exporter の第三者表記](../srtrack-exporter/THIRD_PARTY_NOTICES.md) を参照してください。
