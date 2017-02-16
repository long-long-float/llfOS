#include "bootpack.h"
#include <stdio.h>

extern FIFO8 keybuf;

void HariMain() {
  init_gdtidt();
  init_pic();
  io_sti();

  init_palette();

  BootInfo* info = (BootInfo*)ADR_BOOTINFO;

  byte* vram = info->vram;

  putfonts8_asc(vram, info->screenx, COLOR_WHITE, 0, 0, "Welcome to llfOS!");

  char buf[1024];
  sprintf(buf, "screen %dx%d", info->screenx, info->screeny);
  putfonts8_asc(vram, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT, buf);

  byte mcursor[16 * 16];
  init_mouse_cursor8(mcursor, COLOR_NONE);
  putblock8_8(vram, info->screenx, 16, 16, info->screenx / 2, info->screeny / 2, mcursor, 16);

  io_out8(PIC0_IMR, 0xf9); // PIC1とキーボードを許可
  io_out8(PIC1_IMR, 0xef); // マウスを許可

  byte kbuf[32];
  fifo8_init(&keybuf, 32, kbuf);

  while (1) {
    // keybuf.dataを読み取っている間に割り込みが来たら困るので
    io_cli();

    if (fifo8_count(&keybuf) > 0) {
      int data = fifo8_pop(&keybuf);

      io_sti();

      boxfill8(info->vram, info->screenx, COLOR_LIGHT_BLUE, 0, 0, 32 * 8 - 1, 15);

      char buf[20];
      sprintf(buf, "keyboard: %02X", data);
      putfonts8_asc(info->vram, info->screenx, COLOR_WHITE, 0, 0, buf);
    } else {
      // STIの後に割り込みが来るとキー情報があるのにHLTしてしまうので一緒に実行する
      io_stihlt();
    }
  }
}

