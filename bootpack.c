#include "bootpack.h"
#include <stdio.h>

extern FIFO8 keybuf;
extern FIFO8 mousebuf;

void init_keyboard();
void enable_mouse();

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

  byte mbuf[128];
  fifo8_init(&mousebuf, 32, mbuf);

  init_keyboard();
  enable_mouse();

  while (1) {
    // keybuf.dataを読み取っている間に割り込みが来たら困るので
    io_cli();

    if (fifo8_count(&keybuf) > 0 || fifo8_count(&mousebuf) > 0) {
      if (fifo8_count(&keybuf) > 0) {
        // キーボードの処理
        int data = fifo8_pop(&keybuf);

        io_sti();

        boxfill8(info->vram, info->screenx, COLOR_BLACK, 0, FONT_HEIGHT * 2, info->screenx, FONT_HEIGHT * 3);

        char buf[20];
        sprintf(buf, "keyboard: %02X", data);
        putfonts8_asc(info->vram, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT * 2, buf);
      } else {
        // マウスの処理
        int data = fifo8_pop(&mousebuf);

        io_sti();

        boxfill8(info->vram, info->screenx, COLOR_BLACK, 0, FONT_HEIGHT * 3, info->screenx, FONT_HEIGHT * 4);

        char buf[20];
        sprintf(buf, "mouse: %02X", data);
        putfonts8_asc(info->vram, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT * 3, buf);
      }
    } else {
      // STIの後に割り込みが来るとキー情報があるのにHLTしてしまうので一緒に実行する
      io_stihlt();
    }
  }
}

void wait_KBC_sendready() {
  while (1) {
    if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
      break;
    }
  }
}

void init_keyboard() {
  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, KBC_MODE);
}

void enable_mouse() {
  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
}
