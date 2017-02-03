CYLS EQU 10 ; どのシリンダまで読むか

  ORG 0x7c00 ; ブートセクタが読み込まれるアドレス(~0x7dff)
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

  MOV SI, msg
putloop:
  MOV AL, [SI]
  ADD SI, 1
  CMP AL, 0
  JE load
  MOV AH, 0x0e
  MOV BX, 0
  INT 0x10
  JMP putloop

load:
  ; ディスクを読む
  ; 10(シリンダ) x 2(ヘッド) x 18(セクタ) x 512(bytes/セクタ) = 180KB
  MOV AX, 0x0820 ; 0x8200に読み込む
  MOV ES, AX
  MOV CH, 0
  MOV DH, 0
  MOV CL, 2
readloop:
  MOV SI, 0    ; リトライ用
retry:
  MOV AH, 0x02 ; ディスク読み込み
  MOV AL, 1
  MOV BX, 0
  MOV DL, 0x00
  INT 0x13
  JNC next

  ADD SI, 1
  CMP SI, 5
  JAE  error

  MOV AH, 0x00 ; リセット
  MOV DL, 0x00
  INT 0x13
  JMP retry

next:
  MOV AX, ES
  ADD AX, 0x0020
  MOV ES, AX

  ADD CL, 1
  CMP CL, 18    ; CL <= 18(セクタ)
  JBE readloop
  MOV CL, 1

  ADD DH, 1
  CMP DH, 2     ; DH < 2(ヘッド)
  JB  readloop
  MOV DH, 0

  ADD CH, 1
  CMP CH, CYLS  ; CH < CYLS(シリンダ)
  JB  readloop

  JMP 0xc200 ; llfos.sysへ

error:
  HLT
  JMP error

msg:
  DB 0x0a, 0x0a
  DB "llfOS", 0x0d, 0x0a
  ; TODO: サイズオーバー...
  ; DB "888 888  .d888  .d88888b.   .d8888b. ", 0x0d, 0x0a
  ; DB "888 888 d88P'  d88P' 'Y88b d88P  Y88b", 0x0d, 0x0a
  ; DB "888 888 888    888     888 Y88b.     ", 0x0d, 0x0a
  ; DB "888 888 888888 888     888  'Y888b.  ", 0x0d, 0x0a
  ; DB "888 888 888    888     888     'Y88b.", 0x0d, 0x0a
  ; DB "888 888 888    888     888       '888", 0x0d, 0x0a
  ; DB "888 888 888    Y88b. .d88P Y88b  d88P", 0x0d, 0x0a
  ; DB "888 888 888     'Y88888P'   'Y8888P' ", 0x0d, 0x0a
  DB 0x0a
  DB 0

  RESB 0x7dfe-$

  DB 0x55, 0xaa

