# 11. リリースとコミュニティ

## 11.1 目的

SRTrack は初期版完成後も、互換性確認、境界条件、利用者からのフィードバックを通じて継続的に改善する。公開リポジトリでは、ソースと利用者向け情報を分け、各版の配布物を GitHub Releases に保全する。

初回の正式公開版は **v0.1.0** とする。配布 ZIP、Git タグ、GitHub Release、診断ログ、各 Object の情報欄は、この版番号を利用者が確認できる場所として扱う。

## 11.2 確認済み環境と前提プラグイン

確認済みの動作環境は Windows x64、AviUtl2 / ExEdit2 2.1.13 とする。SRTrack の前提プラグインは GCMZDrops2 と L-SMASH Works とし、いずれも SRTrack には同梱しない。利用者は各権利者の配布条件に従って導入する。

## 11.3 GitHub Release

各公開版には Git タグと対応する GitHub Release を作成し、次を添付する。

- `SRTrack-vMAJOR.MINOR.PATCH.zip`
- その版の変更内容

公開済みの Release asset は置き換えず、後続版は新しいタグと ZIP を追加する。これにより利用者が過去版へ戻れるよう、版ごとの配布物を保全する。

## 11.4 配布 ZIP の構成

ZIP のルートには、次だけを置く。

```text
Plugin/
  SRTrack/
    SRTrack.aux2
    SRTrack.lua
Script/
  SRTrack/
    SRTrack.mod2
    SRTrackObject.obj2
    SRTrackObject(fix).obj2
    SRTrackObject(txt).obj2
readme.txt
```

テストプロジェクト、SRT fixture、動画 fixture、ビルド中間物、ソースコードは配布 ZIP に含めない。利用者は ZIP を AviUtl2 のデータフォルダへ展開する。更新・ロールバックの際は AviUtl2 を終了してから対象版の ZIP を展開する。

## 11.5 生成と公開前確認

リリース作業は次の順で行う。

1. `build.cmd` を実行し、単体テストを通す。
2. 正常系・診断系の手動回帰テストと、Importer の空シーン確認を行う。
3. `CHANGELOG.md` に版の変更内容を記録し、公開日を確定する。
4. README の現在のバージョンと最終更新日を確定した公開内容へ更新する。
5. `tools/package-release.ps1 -Version X.Y.Z` を実行して ZIP を生成する。
6. ZIP の内容が本仕様の構成だけであることを確認する。
7. Git タグと GitHub Release を作成し、生成した ZIP と変更内容を公開する。

生成先 `dist/` は Git 管理しない。公開済みの ZIP は GitHub Releases を正本とする。

## 11.6 不具合報告と診断

不具合報告には GitHub Issue のテンプレートを用いる。報告者には、再現手順、期待した結果、実際の結果、SRTrack 版、AviUtl2 版、前提プラグインの版、可能なら診断ログを求める。

Exporter の警告・エラーは結果画面の `共通ログをコピー`、Importer・Preview を含む共通ログは `SRTrack 設定` の `診断ログ` タブから取得できる。診断ログは絶対パス、SRT 本文、動画名を記録しない。投稿者には、送信前に内容を確認し、公開したくない情報は伏せるよう案内する。

## 11.7 開発協力

協力者向けのビルド、テスト、提案・不具合報告の手順はリポジトリ直下の `CONTRIBUTING.md` に置く。テスト用 `.aup2` は開発資料であり、ローカル動画 fixture が必要であることを明記する。初期検討資料は履歴として残し、現行の判断は `docs/spec/` を正とする。
