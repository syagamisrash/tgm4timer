# TGM4 Section Timer

Windows用の `Tetris The Grand Master 4` 補助ツールです。  
ゲームプロセスのメモリから `Level` とゲーム内タイマーを読み取り、100区切りのセクション記録を別ウィンドウに表示します。

## 主な機能

- `tgm4.exe` に自動アタッチ
- メニュー状態からモードを自動判定
- `Level 0` を検出すると走行開始
- セクション到達時に `GameTime` と `Delta` を記録
- `Back` と `Tet` をセクションごとに記録
- `Run Time` に現在のゲーム内タイムを表示
- `Run Time` の右に直近1分ベースの `lv/min` と、その最大値を表示
- モードごとの `Max Level` を保存
- モードごとのベスト区間記録を保存
- 実行中メモリ内に直近20回分の履歴を保持
- `NORMAL(1.1)`, `NORMAL(2.1)`, `NORMAL(3.1)` ではGM条件を下部表示

## 対応モード

- `NORMAL`
- `NORMAL(1.1)`
- `NORMAL(2.1)`
- `NORMAL(3.1)`
- `NORMAL(4.1)`
- `ASUKA`
- `ASUKAEASY`
- `MASTER`

## 表示内容

- `Section`
  各100区間。理論最大Lvが `999` のモードでは最終区間は `900-999`
- `GameTime`
  そのセクション到達時のゲーム内タイマー値。形式は `m:ss.cc`
- `Delta`
  そのセクションのゲーム内タイム差分とベスト区間との差
- `Best`
  そのモードの保存済みベスト区間
- `Back`
  1だけLevelが減った回数
- `Tet`
  1回の更新でLevelが4以上増えた回数

## 動作ルール

- 走行開始は `Level 0`
- `Level 0 -> 1以上` になった瞬間に表を新走行用へクリア
- 走行中に再び `Level 0` へ戻った場合はリトライ扱い
- 理論最大Lvを超える値は誤読として無視
- 2秒未満のセクションは誤読扱いで記録しない
- モード未判定時は表を出さない
- 一度有効なモードを検出した後は、関係ないカーソル値に一時的に変わっても直前モードを維持

## 設定ファイル

メモリアドレス情報は実行ファイルと同じディレクトリの `config.txt` にあります。
全ゲームモードで共通の値だけを持つため、ゲーム更新後は5項目だけを修正すれば反映されます。

```text
# Shared pointer settings for every game mode.
base_address	0x00A7FBC8
game_mode_offsets	0x8,0x30,0x30,0x1c
menu_cursor_y_offsets	0x8,0x30,0x30,0x15
level_offsets	0x8,0x30,0x28,0x1a4
timer_offsets	0x8,0x30,0x28,0x1dc
```

- `base_address`: `tgm4.exe` のモジュールベースからの共通ベースアドレス
- `game_mode_offsets`: ゲームモード値を読むポインタチェイン
- `menu_cursor_y_offsets`: メニューカーソルY値を読むポインタチェイン
- `level_offsets`: 現在Lvを読むポインタチェイン
- `timer_offsets`: ゲーム内タイマーのフレーム値を読むポインタチェイン

各オフセットは `0x...` をカンマで区切って指定します。`config.txt` が存在しない場合は、同じ5項目の空テンプレートを生成します。アドレス値は `config.txt` のみに保持し、ソースコードには持ちません。
## ビルド

### MinGW-w64

コンソールを出さずに起動するビルド:

```bat
gcc -mwindows -municode -O2 -Wall -Wextra -o tgm4_timer.exe main.c ui.c config_store.c -lgdi32 -luser32
```

ランタイムDLL依存を減らしたい場合:

```bat
gcc -mwindows -municode -O2 -Wall -Wextra -static -static-libgcc -o tgm4_timer.exe main.c ui.c config_store.c -lgdi32 -luser32
```

### MSVC

```bat
cl /W4 /O2 /DUNICODE /D_UNICODE main.c ui.c config_store.c user32.lib gdi32.lib
```

## ファイル構成

- `main.c`
  本体
- `config.h`
  ポーリング間隔などの共通定数
- `config.txt`
  メモリアドレス設定。起動時に自動生成可

## 補足

- `Level` とゲーム内タイマーは32bit値前提で読んでいます
- メモリレイアウトが変わった場合は `config.txt` を修正してください
- 画面表示や履歴はアプリ実行中だけの補助用途を想定しています
