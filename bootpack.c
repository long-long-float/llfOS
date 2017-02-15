#include "bootpack.h"
#include <stdio.h>

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

  while(1) io_hlt();
}

