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

アドレス情報は `config.txt` に持ちます。  
実行ファイルと同じディレクトリに置きます。

起動時の挙動:

- `config.txt` が存在しない場合
  内蔵の初期設定値から自動生成
- `config.txt` が存在する場合
  内容を読み込んで反映

形式はTSVです。

```text
mode	level_base	level_offsets	timer_base	timer_offsets	cursor_value	menu_cursor_position	theoretical_max_level	initial_timer_frames
NORMAL	0x00A7E528	0x8,0x30,0x10,0x10,0x10,0xC,0x98	0x00A7E528	0x8,0x30,0x10,0x10,0x10,0xC,0xA0	9	1	999	0
```

列の意味:

- `mode`
  モード名
- `level_base`
  `Level` 読取のベースアドレス
- `level_offsets`
  `Level` 用オフセット列。カンマ区切り
- `timer_base`
  ゲーム内タイマー読取のベースアドレス
- `timer_offsets`
  タイマー用オフセット列。カンマ区切り
- `cursor_value`
  モード判定に使うゲームモード値
- `menu_cursor_position`
  メニューカーソル位置
- `theoretical_max_level`
  そのモードの理論最大Lv
- `initial_timer_frames`
  タイマー初期値。60FPS基準フレーム数

### 初期タイマーフレームの考え方

- `ASUKA` は `7:00.00` 開始なので `25200`
- `ASUKAEASY` は `30:00.00` 開始なので `108000`
- それ以外は `0`

## 保存されるファイル

実行ファイルの横に `save` フォルダを作ります。

- `section_bests_*.txt`
  モードごとのベスト区間
- `max_level_*.txt`
  モードごとの最大到達Lv

履歴20件はメモリ内だけです。アプリ終了で消えます。

## 履歴表示

上部の `←` `→` ボタンで直近20回分を見返せます。

- `←`
  1つ前の履歴を見る
- `→`
  新しい履歴側へ戻る

履歴表示中の `Current Level` は、そのプレイ終了時のLvです。

## ビルド

### MinGW-w64

コンソールを出さずに起動するビルド:

```bat
gcc -mwindows -municode -O2 -Wall -Wextra -o tgm4_timer.exe main.c -lgdi32 -luser32
```

ランタイムDLL依存を減らしたい場合:

```bat
gcc -mwindows -municode -O2 -Wall -Wextra -static -static-libgcc -o tgm4_timer.exe main.c -lgdi32 -luser32
```

### MSVC

```bat
cl /W4 /O2 /DUNICODE /D_UNICODE main.c user32.lib gdi32.lib
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
