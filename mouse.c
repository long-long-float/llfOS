#include "bootpack.h"

FIFO8 mousebuf;

void enable_mouse() {
  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
}

// PS/2マウスからの割り込み
void inthandler2c(int *esp) {
  io_out8(PIC1_OCW2, 0x64); // スレーブ
  io_out8(PIC0_OCW2, 0x62); // マスター
  byte data = io_in8(PORT_KEYDAT);

  fifo8_push(&mousebuf, data);
}
