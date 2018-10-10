■ 動作手順
  1. 回路を組む (LCD、RN42-EK、PIC16F1827をつなぐ。LCDの詳細はPIC16F1827_LCD_sampleリポジトリ参照)
  2. MPLAB X IDEでmain.cをコンパイル
  3. PICKIT3でPIC16F1827に書き込む
  4. RN42-EKにbluetoothで接続(android端末にS2 terminal for bluetoothをインストールすると簡単)
  5. 任意の文字をbluetooth(S2 terminal for bluetoothから)で送信すると、その文字がLCDに出力されたあと
     '*'(syncモードがonの場合は'-')がbluetoothで返却される(S2 terminal for bluetoothの画面に'*'または'-'が表示される)