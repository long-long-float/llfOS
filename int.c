#include "bootpack.h"

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
  BootInfo *info = (BootInfo*)ADR_BOOTINFO;
  boxfill8(info->vram, info->screenx, COLOR_LIGHT_BLUE, 0, 0, 32 * 8 - 1, 15);
  putfonts8_asc(info->vram, info->screenx, COLOR_WHITE, 0, 0, "INT 21 (IRQ-1) : PS/2 keyboard");
  while(1) io_hlt();
}

// PS/2マウスからの割り込み
void inthandler2c(int *esp) {
  BootInfo *info = (BootInfo*)ADR_BOOTINFO;
  boxfill8(info->vram, info->screenx, COLOR_LIGHT_BLUE, 0, 0, 32 * 8 - 1, 15);
  putfonts8_asc(info->vram, info->screenx, 0, 0, COLOR_WHITE, "INT 2C (IRQ-12) : PS/2 mouse");
  while(1) io_hlt();
}

// PIC0からの不完全割り込み対策
void inthandler27(int *esp) {
  io_out8(PIC0_OCW2, 0x67);
}
