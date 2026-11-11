# SRTrack Object for AviUtl2

SRTrack Object は、レンダリング済み動画に対応する SRT の参照と時間変換設定を保持する非描画カスタムオブジェクトです。字幕を画面へ描画せず、SRTrack Exporter が読み取る字幕ソースとして使います。

## インストール

`SRTrack Object.obj2` を AviUtl2 の `Script` フォルダ、またはその直下のサブフォルダへコピーして AviUtl2 を再起動します。通常の配置先は `C:\ProgramData\aviutl2\Script\SRTrack\SRTrack Object.obj2` です。

## 設定項目

- `SRT ファイル`：対応する元 SRT を指定します。
- `SRT 再生開始（ミリ秒）`：オブジェクト開始時に参照する元 SRT の時刻です。
- `SRT 再生速度`：元 SRT の時間の進み方です。初期実装は正の固定値だけを対象にします。

動画のドロップ時に同名 SRT を見つけ、対応する SRTrack Object を自動作成・グループ化する処理は SRTrack Exporter 側で実装します。

## ライセンス

SRTrack Object を含む今後追加するツールのライセンスは未確定です。現時点では利用条件を付与していません。
