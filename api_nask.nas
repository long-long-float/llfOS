[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api_nask.nas"]

  GLOBAL _api_putc

[SECTION .text]

_api_putc:  ; void api_putc(int c)
  MOV EDX, 1
  MOV AL, [ESP+4]
  INT 0x40
  RET
