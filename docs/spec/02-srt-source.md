# 2. SRT Object と入力データ

> **公開前の参照規則変更**：SRTrack Object の公開仕様は「上側の直前動画Object」を参照する方式へ変更する。現行実装は直上レイヤーだけを参照しており、この規則は Preview／Exporter の実装変更後に適用する。Object のプロパティ、Importer の初期配置、グループ化は変更しない。

## 2.1 二つの Object

SRTrack は、一つの Object に動画追従と手動再生設定を同居させない。SRT を字幕ソースとして扱う共通の役割を、時間変換の基準が異なる二つの Object に分ける。

| Object | 時間変換の基準 | 主な用途 | 状態 |
| --- | --- | --- | --- |
| SRTrack Object | 同じ時刻に自身よりLayer 1側で最も近い標準 `動画ファイル` Object（上側の直前動画Object） | レンダリング済み動画と対応する SRT | `SRTrackObject.obj2`、初期実装・簡易確認済み |
| SRTrack Object (Standalone) | Object 自身の再生開始位置と速度 | 動画と独立した SRT の配置 | `SRTrackObject(Standalone).obj2`、初期実装 |

両者は SRT ファイルを読み、Exporter の字幕ソースになり、編集用字幕プレビューを任意に表示する。プレビューは確認専用であり、動画書き出し中は描画しない。標準テキスト等との統合・結合・オフセット後の最終 SRT と Preview 表示の一致は要件にしないが、各 Object が単独で変換する SRT 時刻は Exporter と Preview で一致させる。

## 2.2 SRTrack Object

SRTrack Object は動画に付随する主用途の字幕側 Object である。保持する項目は `SRT ファイル`、`動画全長 D`、`字幕プレビュー` とする。`D` は画面へ表示し、Importer が初回配置時に元動画の全長として設定する。取得不能時は `0` を設定する。`D` はループの周期だけに使い、移動・分割・伸縮では変更しない。AviUtl2 の UI 上で手動変更できる場合は動画との整合性が崩れうるため、変更は非推奨とする。

再生位置 `P`、再生速度、ループ再生は SRTrack Object に保存しない。Object が存在する各時刻で、自身よりLayer 1側を順に探索し、最初に見つかる標準 `動画ファイル` Object の現在値を直接参照する。動画以外のObjectは読み飛ばし、SRTrack Objectより下側のObjectは参照しない。有効区間の終端は、参照動画 Object がタイムライン上に存在する終端とする。動画と SRTrack Object をグループ化することは、移動・分割・伸縮時にこの上下関係を保ちやすくする補助であり、参照の成立条件ではない。

`SRT ファイル` は利用者が指定する独立した参照である。Importer は同名 SRT を初期値として設定するだけで、動画ファイルの変更後に SRT パスを自動追従・差し戻ししない。別名の動画と SRT を組み合わせる手動利用を妨げない。

## 2.3 SRTrack Object (Standalone)

SRTrack Object (Standalone) は動画を参照しない補助的な Object である。`SRT ファイル`、再生開始位置 `P`、再生速度、`字幕プレビュー` と Object 自身のタイムライン範囲を保持する。`動画全長 D`、ループ再生、動画とのグループ関係は持たない。SRT の最終字幕を過ぎた後は Preview を表示せず、途中で終了したい場合は Object の終端を使う。Exporter も `SRTrack Object` と同じ対象設定で Standalone を字幕ソースとして扱う。

## 2.4 SRTrack Object の参照失敗とプレビュー

SRTrack Object の存在する時刻に、上側のレイヤー群から標準動画を見つけられない場合、その時刻は有効な字幕ソースではない。Preview は編集画面に専用エラーを表示する。Exporter は当該区間を単に字幕がない区間として扱い、警告や失敗にせず出力を継続する。SRT の読込失敗や不正な設定も、空字幕として黙って扱わず編集画面に診断を表示する。診断は動画書き出し中には描画しない。

Preview は描画時に、公開Lua APIでSRTrack ObjectよりLayer 1側を探索し、上側の直前動画Objectの設定値と開始フレームを直接参照する。SRTrack Object自身の開始フレームとの差をフレームレートで秒へ換算し、動画の途中から置かれた場合も動画内の対応位置から表示する。`SRTrack.mod2` はUnicodeを含むSRTパスの読込だけを担う。常時監視やプロジェクト全走査は行わない。

### 公開 Lua API による上側探索

[AviUtl2 Lua リファレンス](https://docs.aviutl2.jp/lua/)の `obj.getvalue(layer, effect, item)` は、指定レイヤー（1から）の現在時刻にあるObjectの設定値を取得し、対象がない場合は `nil` を返す。Preview は `obj.layer - 1` からLayer 1側へ順に、標準 `動画ファイル` の `再生位置` を取得できる最初のレイヤーを参照先とする。同じレイヤーから再生速度・ループを取得し、`obj.getvalue("layerN.frame_s")` で動画開始フレームを得る。

この経路は現行Previewが使う直上レイヤー取得の一般化であり、Objectの描画ごとに上側レイヤーだけを確認する。プロジェクト全体の走査やMonitorを必要としない。公開API上の実現性は確認済みであり、残る確認は実装後にフィルタ等の非動画Objectを間へ置いた実機動作と、Exporterとの一致を確認することである。

Lua API で標準動画の再生範囲として直接取得できるのは開始位置 `P` である。Preview の終端は、同じタイムライン範囲へ配置される SRTrack Object 自身の終端で表す。したがって Preview の時間変換には `P`、再生速度、ループ、`D` を使用する。

## 2.5 SRT の入力形式

初期版は UTF-8 を対象とする。BOM の有無、CRLF／LF、末尾空行、複数行本文を検証対象にする。各字幕は開始時刻、終了時刻（整数ミリ秒）、本文、元ファイル内順序へ解析する。内部区間は `[開始, 終了)` とする。

不正 UTF-8、不正時刻、番号欠落・重複、時刻順でない字幕、重複区間、本文の空行やタグ、大きいファイル、相対パス、外部更新の扱いは継続検討する。曖昧な入力を黙って変更しない。

## 2.6 追加メニューと Preview

Object 追加メニューでは、二つの Object を `SRTrack` の下へまとめる。

- `SRTrack` → `SRTrack Object`
- `SRTrack` → `SRTrack Object (Standalone)`

両方とも `字幕プレビュー` を持つ。`SRTrack Object` は上側の直前動画Objectの開始位置・速度・ループと開始位置差を用いる。`SRTrack Object (Standalone)` は自身が保持する再生位置・速度と Object 内時刻だけを用いる。いずれも Preview は編集画面専用で、動画出力には描画しない。
