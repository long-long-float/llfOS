#include "bootpack.h"

FIFO32 *keyboard_fifo;
int keydata0;

void wait_KBC_sendready() {
  while (1) {
    if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
      break;
    }
  }
}

void init_keyboard(FIFO32 *fifo32, int data0) {
  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, KBC_MODE);

  keyboard_fifo = fifo32;
  keydata0 = data0;
}

// PS/2キーボードからの割り込み
void inthandler21(int *esp) {
  io_out8(PIC0_OCW2, 0x61);
  byte data = io_in8(PORT_KEYDAT);

  fifo32_push(keyboard_fifo, keydata0 + data);
}
