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
  MemoryMan *memman = (MemoryMan*)MEMORY_MAN_ADDRESS;

  int mousex = info->screenx / 2, mousey = info->screeny / 2;

  char buf[1024];

  // メモリ関連
  int memsize = memtest(0x00400000, 0xbfffffff);

  memory_man_init(memman);
  memory_man_free(memman, 0x00001000, 0x0009e000);
  memory_man_free(memman, 0x00400000, memsize - 0x00400000);

  // シート関連
  SheetControl *sheet_ctl = sheet_control_init(memman, info->vram, info->screenx, info->screeny);

  Sheet *sheet_back  = sheet_alloc(sheet_ctl);
  byte *buf_back = (byte*)memory_man_alloc_4k(memman, info->screenx * info->screeny);
  sheet_init(sheet_back, buf_back, info->screenx, info->screeny, -1);
  for (int i = 0; i < info->screenx * info->screeny; i++) buf_back[i] = -1;
  sheet_slide(sheet_back, 0, 0);

  byte buf_mouse[16 * 16];
  Sheet *sheet_mouse = sheet_alloc(sheet_ctl);
  sheet_init(sheet_mouse, buf_mouse, 16, 16, 99);
  init_mouse_cursor8(buf_mouse, 99);
  sheet_slide(sheet_mouse, mousex, mousey);

  sheet_updown(sheet_back, 0);
  sheet_updown(sheet_mouse, 1);

  sprintf(buf, "sheet %d %d %d %d", sheet_ctl->width, sheet_ctl->height, sheet_ctl->top_index, sheet_ctl->sheets[0]->color_inv);
  putfonts8_asc(info->vram, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT, buf);

  putfonts8_asc(buf_back, info->screenx, COLOR_WHITE, 0, 0, "Welcome to llfOS!");

  sprintf(buf, "screen %dx%d", info->screenx, info->screeny);
  putfonts8_asc(buf_back, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT, buf);

  io_out8(PIC0_IMR, 0xf9); // PIC1とキーボードを許可
  io_out8(PIC1_IMR, 0xef); // マウスを許可

  // 入力バッファの初期化
  byte kbuf[32];
  fifo8_init(&keybuf, 32, kbuf);

  byte mbuf[128];
  fifo8_init(&mousebuf, 32, mbuf);

  init_keyboard();
  enable_mouse();

  sprintf(buf, "memory %dMB, free %dKB", memsize / (1024 * 1024), memory_man_free_total(memman) / 1024);
  putfonts8_asc(buf_back, info->screenx, COLOR_WHITE, 0, info->screeny - FONT_HEIGHT, buf);

  bool received_0xfa = false;

  sheet_refresh(sheet_back, 0, 0, info->screenx, info->screeny);

  while (1) {
    // keybuf.dataを読み取っている間に割り込みが来たら困るので
    io_cli();

    if (fifo8_count(&keybuf) > 0 || fifo8_count(&mousebuf) > 0) {
      if (fifo8_count(&keybuf) > 0) {
        // キーボードの処理
        int data = fifo8_pop(&keybuf);

        io_sti();

        boxfill8(buf_back, info->screenx, COLOR_BLACK, 0, FONT_HEIGHT * 2, info->screenx, FONT_HEIGHT * 3);

        char buf[20];
        sprintf(buf, "keyboard: %02X", data);
        putfonts8_asc(buf_back, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT * 2, buf);

        sheet_refresh(sheet_back, 0, 0, info->screenx, FONT_HEIGHT * 4);
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

            mousex += dx;
            mousey += dy;

            if (mousex < 0) mousex = 0;
            if (mousex >= info->screenx) mousex = info->screenx - 1;
            if (mousey < 0) mousey = 0;
            if (mousey >= info->screeny) mousey = info->screeny - 1;

            boxfill8(buf_back, info->screenx, COLOR_BLACK, 0, FONT_HEIGHT * 3, info->screenx, FONT_HEIGHT * 4);

            char buf[20];
            char l = (button & 0x01) ? 'L' : '.',
                 r = (button & 0x02) ? 'R' : '.',
                 c = (button & 0x04) ? 'C' : '.';
            sprintf(buf, "mouse: ... (%d, %d)", mousex, mousey);
            buf[7] = l; // golibcのsprintfが%cに対応していなかったので
            buf[8] = c;
            buf[9] = r;
            putfonts8_asc(buf_back, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT * 3, buf);

            sheet_refresh(sheet_back, 0, 0, info->screenx, FONT_HEIGHT * 5);
            sheet_slide(sheet_mouse, mousex, mousey);
          }
        }
      }
    } else {
      // STIの後に割り込みが来るとキー情報があるのにHLTしてしまうので一緒に実行する
      io_stihlt();
    }
  }
}

