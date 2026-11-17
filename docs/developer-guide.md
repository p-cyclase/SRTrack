# SRTrack 開発ガイド

この文書は SRTrack の開発・検証・配布作業向けである。利用者向けの導入と使いかたは、リポジトリ直下の [README](../README.md) を参照する。正規仕様は [spec/](spec/README.md) とする。

## 開発構成

SRTrack は、動画追従型を主用途とし、補助用途を含む三つの Object を提供する。

| Object | 役割 |
| --- | --- |
| SRTrack Object | 自身より Layer 1 側で最も近い標準動画 Object を必要時に参照し、その再生位置・速度・ループに従って SRT を変換する。SRT パスと全長 `D` を保持する。 |
| SRTrack Object (fix) | 動画を参照せず、自身の再生開始位置・速度・ループ・全長とタイムライン範囲で SRT を変換する。 |
| SRTrack Object (txt) | 自由入力テキストを Object の範囲で Preview 表示し、SRT の補足字幕として出力する。動画書き出しには描画しない。 |

動画とのグループ化は、編集操作で上下関係を保ちやすくする補助である。字幕処理の条件はグループ ID ではなく、SRTrack Object より Layer 1 側で最も近い動画である。フィルタなどの非動画 Object は間に置ける。常駐 Monitor による値コピー・ファイル名追従は採用しない。

## ビルド

Windows SDK と MSVC の C++ 開発環境を準備し、Visual Studio Developer Command Prompt から次を実行する。

```bat
build.cmd
```

`build.cmd` はビルド後に単体テストも実行する。生成物は Git 管理しない `out/` に出力される。

```text
out/
  Plugin/SRTrack/SRTrack.aux2
  Plugin/SRTrack/SRTrack.lua
  Script/SRTrack/SRTrack.mod2
  Script/SRTrack/SRTrackObject.obj2
  Script/SRTrack/SRTrackObject(fix).obj2
  Script/SRTrack/SRTrackObject(txt).obj2
```

## 手動回帰テスト

- `tests/manual/SRTrack-Regression.aup2` は Preview／Exporter の正常系を確認する。
- `tests/diagnostics/SRTrack-Diagnostics.aup2` は、出力を継続できるエラー・警告・情報診断を確認する。
- Importer のドロップ操作は、空シーンで別途確認する。

`.aup2` と SRT fixture は Git 管理する。動画 fixture は容量と環境差のため Git 管理しない。共同開発者が手動回帰プロジェクトを使う場合は、各 `README.md` が示すファイル名・長さ・フレームレートに合うローカル動画を `fixtures/` へ用意し、AviUtl2 上で参照先を指定し直す必要がある。

テストの詳細と確認項目は [手動回帰テスト仕様](spec/09-manual-regression-test.md)、[診断回帰テスト仕様](spec/10-diagnostic-regression-test.md) を参照する。

## リリース

公開用 ZIP は [リリースとコミュニティ仕様](spec/11-release-and-community.md) に従う。`tools/package-release.ps1` は、ビルド済みの `out/` から版番号付きの ZIP を作る。GitHub Release にはその ZIP、該当する `CHANGELOG.md` の記録、タグを対応づける。

## 履歴資料

`docs/components/` と `docs/srtrack-design.md` は初期検討・履歴資料である。旧名称や旧設計を含むため、現行仕様・実装判断には用いない。読んで楽しむ資料として残す。
