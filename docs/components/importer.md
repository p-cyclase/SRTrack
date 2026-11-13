# Importer

> **履歴資料**：以下の詳細は旧単一 Object の初期実装記録である。次期仕様は [Importer](../spec/06-importer.md) を正とし、Monitor への言及は採用しない。

> Importer は `SRTrack.aux2` 内の論理コンポーネントです。GCMZDrops2 用ハンドラースクリプトは `SRTrack.lua` として同じ Plugin フォルダへ配布します。

GCMZDrops2 の公開ハンドラースクリプト API を通じ、動画ファイルのドラッグ＆ドロップ時に同じフォルダの同名 SRT を検出して SRTrack Object を追加する試作コンポーネントです。GCMZDrops2 と L-SMASH Works を前提プラグインとし、いずれも同梱しません。

対応 SRT がある場合、Importer は元の動画ファイルを、標準 AviUtl2 の複数オブジェクト形式の一時 `.object` へ置換します。このエイリアスには動画ファイルと SRTrack Object を含め、両者へ同じグループ値を設定します。動画は下側、Object はその直上に作成されます。複数動画はファイル名順に、同じ二層へ時間方向に並べます。Object には SRT パス、再生位置 `0`、動画と同一のフレーム境界終端を `再生範囲終端` と `動画全長`、再生速度 `100`、ループ再生オフを設定します。SRT がなければファイルリストを変更せず、動画は通常どおり配置され、通知もしません。

単体・複数動画のドロップ、動画直下への二層配置、ペアごとのグループ化、エクスプローラーおよび AviUtl2 内メディアエクスプローラーからの追加を AviUtl2 2.1.13 で簡易確認済みです。分割・伸縮への自動追従は `SRTrack.aux2` 内の Monitor が担当します。Monitor の移動・伸縮・分割とリネームは概略確認済みで、網羅検証は未実施です。

## ビルドと導入

GCMZDrops2、L-SMASH Works、SRTrack Object を導入した環境が必要です。`build.cmd` を実行して生成した `out\Plugin\SRTrack\SRTrack.aux2` と `SRTrack.lua` を、AviUtl2 の `Plugin\\SRTrack` など **SRTrack 専用の同じフォルダ**へコピーして AviUtl2 を再起動してください。GCMZDrops2 のフォルダへ SRTrack のファイルをコピーする必要はありません。

完成時の配布では、`SRTrack.aux2` と `SRTrack.lua` を同じ SRTrack 専用フォルダへ置きます。Object 側は `SRTrack.obj2` と `SRTrack.mod2` を Script フォルダへ置きます。

`SRTrack.aux2` は、GCMZDrops2 が公開している `AddHandlerScriptFile` だけを使い、同じ SRTrack フォルダ内の `SRTrack.lua` を起動時に登録します。GCMZDrops2 が導入・有効化されていない場合は、通常の動画ドロップに介入しません。

固定の対応拡張子一覧は持ちません。同名 SRT があるファイルについて、GCMZDrops2 の公開メディア情報 API が動画トラックを取得できた場合だけ Object を追加します。そのため、対応範囲は導入済みの L-SMASH Works と標準 AVI 読込が実際に扱える動画形式に従います。

## ライセンス

コードは [MIT License](../../LICENSE) です。SDK の権利表記は [第三者の権利表記](../../THIRD_PARTY_NOTICES.md) を参照してください。
