CYLS EQU 0x0ff0
LEDS EQU 0x0ff1
VMODE EQU 0x0ff2   ; 色数に関する情報。何ビットカラーか？
SCRNX EQU 0x0ff4   ; 解像度のX
SCRNY EQU 0x0ff6   ; 解像度のY
VRAM EQU 0x0ff8   ; グラフィックバッファの開始番地

  ORG 0xc200 ; = 0x8000 + 0x4200 にこのプログラムが読み込まれる

  ; VGAグラフィック
  MOV AL, 0x13
  MOV AH, 0x00
  INT 0x10

  ; 画面モード
  MOV BYTE [VMODE], 8
  MOV WORD [SCRNX], 320
  MOV WORD [SCRNY], 200
  MOV DWORD [VRAM], 0x00a0000

  ; キーボードの状態を取得
  MOV AH, 0x02
  INT 0x16
  MOV [LEDS], AL
fin:
  HLT
  JMP fin
