#include "bootpack.h"
#include <stdio.h>

extern FIFO8 keybuf;
extern FIFO8 mousebuf;

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

  io_out8(PIC0_IMR, 0xf9); // PIC1とキーボードを許可
  io_out8(PIC1_IMR, 0xef); // マウスを許可

  byte kbuf[32];
  fifo8_init(&keybuf, 32, kbuf);

  byte mbuf[128];
  fifo8_init(&mousebuf, 32, mbuf);

  init_keyboard();
  enable_mouse();

  bool received_0xfa = false;
  int mousex = info->screenx / 2, mousey = info->screeny / 2;

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
        if (!received_0xfa) {
          int data = fifo8_pop(&mousebuf);
          if (data == 0xfa) {
            io_sti();

            received_0xfa = true;
          }
        } else {
          if (!(fifo8_head(&mousebuf) & 0x08)) {
            fifo8_pop(&mousebuf); // エラーなので読みとばす
            io_sti();
          } else if (fifo8_count(&mousebuf) >= 3) {
            int mouse_info[3];
            for (int i = 0; i < 3; i++) mouse_info[i] = fifo8_pop(&mousebuf);

            io_sti();

            int button = mouse_info[0] & 0x07;
            int dx = mouse_info[1], dy = mouse_info[2];
            if (mouse_info[0] & 0x10) {
              dx |= 0xffffff00;
            }
            if (mouse_info[0] & 0x20) {
              dy |= 0xffffff00;
            }
            dy = -dy;

            boxfill8(info->vram, info->screenx, COLOR_BLACK, mousex, mousey, mousex + 16, mousey + 16);

            mousex += dx;
            mousey += dy;

            if (mousex < 0) mousex = 0;
            if (mousex >= info->screenx) mousex = info->screenx - 1;
            if (mousey < 0) mousey = 0;
            if (mousey >= info->screeny) mousey = info->screeny - 1;

            boxfill8(info->vram, info->screenx, COLOR_BLACK, 0, FONT_HEIGHT * 3, info->screenx, FONT_HEIGHT * 4);

            char buf[20];
            char l = (button & 0x01) ? 'L' : '.',
                 r = (button & 0x02) ? 'R' : '.',
                 c = (button & 0x04) ? 'C' : '.';
            sprintf(buf, "mouse: ... (%d, %d)", mousex, mousey);
            buf[7] = l; // golibcのsprintfが%cに対応していなかったので
            buf[8] = c;
            buf[9] = r;
            putfonts8_asc(info->vram, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT * 3, buf);

            putblock8_8(vram, info->screenx, 16, 16, mousex, mousey, mcursor, 16);
          }
        }
      }
    } else {
      // STIの後に割り込みが来るとキー情報があるのにHLTしてしまうので一緒に実行する
      io_stihlt();
    }
  }
}

