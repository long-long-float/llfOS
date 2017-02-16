#include "bootpack.h"

FIFO8 keybuf;
FIFO8 mousebuf;

void init_pic() {
  io_out8(PIC0_IMR,  0xff  ); /* 全ての割り込みを受け付けない */
  io_out8(PIC1_IMR,  0xff  ); /* 全ての割り込みを受け付けない */

  io_out8(PIC0_ICW1, 0x11  ); /* エッジトリガモード */
  io_out8(PIC0_ICW2, 0x20  ); /* IRQ0-7は、INT20-27で受ける */
  io_out8(PIC0_ICW3, 1 << 2); /* PIC1はIRQ2にて接続 */
  io_out8(PIC0_ICW4, 0x01  ); /* ノンバッファモード */

  io_out8(PIC1_ICW1, 0x11  ); /* エッジトリガモード */
  io_out8(PIC1_ICW2, 0x28  ); /* IRQ8-15は、INT28-2fで受ける */
  io_out8(PIC1_ICW3, 2     ); /* PIC1はIRQ2にて接続 */
  io_out8(PIC1_ICW4, 0x01  ); /* ノンバッファモード */

  io_out8(PIC0_IMR,  0xfb  ); /* 11111011 PIC1以外は全て禁止 */
  io_out8(PIC1_IMR,  0xff  ); /* 11111111 全ての割り込みを受け付けない */
}

// PS/2キーボードからの割り込み
void inthandler21(int *esp) {
  io_out8(PIC0_OCW2, 0x61);
  byte data = io_in8(PORT_KEYDAT);

  fifo8_push(&keybuf, data);
}

// PS/2マウスからの割り込み
void inthandler2c(int *esp) {
  io_out8(PIC1_OCW2, 0x64); // スレーブ
  io_out8(PIC0_OCW2, 0x62); // マスター
  byte data = io_in8(PORT_KEYDAT);

  fifo8_push(&mousebuf, data);
}

// PIC0からの不完全割り込み対策
void inthandler27(int *esp) {
  io_out8(PIC0_OCW2, 0x67);
}
