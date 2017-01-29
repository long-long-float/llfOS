  ORG 0x7c00
  JMP entry
  DB 0x90 ; ブートストラップ プログラムへのジャンプ命令(x86命令)

  DB "HELLOIPL"
  DW 512 ; セクタサイズ
  DB 1   ; クラスタサイズ
  DW 1   ; FATがどこから始まるか
  DB 2
  DW 224
  DW 2880
  DB 0xf0
  DW 9
  DW 18
  DW 2
  DD 0
  DD 2880
  DB 0,0,0x29
  DD 0xffffffff
  DB "HELLO-OS   "
  DB "FAT12   "
  RESB 18

entry:
  MOV AX, 0
  MOV SS, AX
  MOV SP, 0x7c00
  MOV DS, AX
  MOV ES, AX

  MOV SI, msg
putloop:
  MOV AL, [SI]
  ADD SI, 1
  CMP AL, 0
  JE fin
  MOV AH, 0x0e
  MOV BX, 0
  INT 0x10
  JMP putloop
fin:
  HLT
  JMP fin

msg:
  DB 0x0a, 0x0a
  DB "888 888  .d888  .d88888b.   .d8888b. ", 0x0d, 0x0a
  DB "888 888 d88P'  d88P' 'Y88b d88P  Y88b", 0x0d, 0x0a
  DB "888 888 888    888     888 Y88b.     ", 0x0d, 0x0a
  DB "888 888 888888 888     888  'Y888b.  ", 0x0d, 0x0a
  DB "888 888 888    888     888     'Y88b.", 0x0d, 0x0a
  DB "888 888 888    888     888       '888", 0x0d, 0x0a
  DB "888 888 888    Y88b. .d88P Y88b  d88P", 0x0d, 0x0a
  DB "888 888 888     'Y88888P'   'Y8888P' ", 0x0d, 0x0a
  DB 0x0a
  DB 0

  RESB 0x7dfe-$

  DB 0x55, 0xaa

