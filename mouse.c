#include "bootpack.h"

FIFO32 *mouse_fifo;
int mousedata0;

void enable_mouse(FIFO32 *fifo32, int data0) {
  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);

  mouse_fifo = fifo32;
  mousedata0 = data0;
}

// PS/2マウスからの割り込み
void inthandler2c(int *esp) {
  io_out8(PIC1_OCW2, 0x64); // スレーブ
  io_out8(PIC0_OCW2, 0x62); // マスター
  byte data = io_in8(PORT_KEYDAT);

  fifo32_push(mouse_fifo, data + mousedata0);
}
