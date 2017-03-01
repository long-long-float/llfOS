#include "bootpack.h"
#include <stdio.h>

extern FIFO8 keybuf;
extern FIFO8 mousebuf;
extern TimerControl timerctl;

void make_window8(byte *buf, int xsize, int ysize, char *title);

void HariMain() {
  init_gdtidt();
  init_pic();
  io_sti();

  init_pit();

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

  const int wwidth = 160, wheight = 68;
  Sheet *sheet_win = sheet_alloc(sheet_ctl);
  byte *buf_win = (byte*)memory_man_alloc_4k(memman, wwidth * wheight);
  sheet_init(sheet_win, buf_win, wwidth, wheight, -1);
  make_window8(buf_win, wwidth, wheight, "window");
  putfonts8_asc(buf_win, wwidth, COLOR_BLACK, 24, 28, "Welcome to");
  putfonts8_asc(buf_win, wwidth, COLOR_BLACK, 24, 44, "  llfOS!");
  sheet_slide(sheet_win, 80, 72);

  sheet_updown(sheet_back, 0);
  sheet_updown(sheet_win, 1);
  sheet_updown(sheet_mouse, 2);

  // タイマー関連
  FIFO8 timer_fifo;
  byte timer_fifo_buf[8];
  fifo8_init(&timer_fifo, 8, timer_fifo_buf);
  Timer *timer = timer_alloc();
  timer_init(timer, &timer_fifo, 1);
  timer_settime(timer, 100);

  sprintf(buf, "sheet %d %d %d %d", sheet_ctl->width, sheet_ctl->height, sheet_ctl->top_index, sheet_ctl->sheets[0]->color_inv);
  putfonts8_asc(info->vram, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT, buf);

  putfonts8_asc(buf_back, info->screenx, COLOR_WHITE, 0, 0, "Welcome to llfOS!");

  sprintf(buf, "screen %dx%d", info->screenx, info->screeny);
  putfonts8_asc(buf_back, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT, buf);

  io_out8(PIC0_IMR, 0xf8); // タイマーとPIC1,キーボードを許可
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

  int count = 0;

  while (1) {
    // keybuf.dataを読み取っている間に割り込みが来たら困るので
    io_cli();

    if (fifo8_count(&keybuf) > 0 || fifo8_count(&mousebuf) > 0 || fifo8_count(&timer_fifo) > 0) {
      if (fifo8_count(&keybuf) > 0) {
        // キーボードの処理
        int data = fifo8_pop(&keybuf);

        io_sti();

        boxfill8(buf_back, info->screenx, COLOR_BLACK, 0, FONT_HEIGHT * 2, info->screenx, FONT_HEIGHT * 3);

        char buf[20];
        sprintf(buf, "keyboard: %02X", data);
        putfonts8_asc(buf_back, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT * 2, buf);

        sheet_refresh(sheet_back, 0, 0, info->screenx, FONT_HEIGHT * 4);
      } else if (fifo8_count(&mousebuf) > 0) {
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
      } else if (fifo8_count(&timer_fifo) > 0) {
        byte data = fifo8_pop(&timer_fifo);

        io_sti();

        count++;

        timer_settime(timer, 100);

        boxfill8(buf_win, wwidth, COLOR_LIGHT_GRAY, 40, 28, 119, 43);
        sprintf(buf, "%d", count);
        putfonts8_asc(buf_win, wwidth, COLOR_BLACK, 40, 28, buf);
        sheet_refresh(sheet_win, 40, 28, 120, 44);
      }
    } else {
      // STIの後に割り込みが来るとキー情報があるのにHLTしてしまうので一緒に実行する
      // io_stihlt();
      io_sti();
    }
  }
}

void make_window8(byte *buf, int xsize, int ysize, char *title) {
  static char closebtn[14][16] = {
    "OOOOOOOOOOOOOOO@",
    "OQQQQQQQQQQQQQ$@",
    "OQQQQQQQQQQQQQ$@",
    "OQQQ@@QQQQ@@QQ$@",
    "OQQQQ@@QQ@@QQQ$@",
    "OQQQQQ@@@@QQQQ$@",
    "OQQQQQQ@@QQQQQ$@",
    "OQQQQQ@@@@QQQQ$@",
    "OQQQQ@@QQ@@QQQ$@",
    "OQQQ@@QQQQ@@QQ$@",
    "OQQQQQQQQQQQQQ$@",
    "OQQQQQQQQQQQQQ$@",
    "O$$$$$$$$$$$$$$@",
    "@@@@@@@@@@@@@@@@"
  };

  boxfill8(buf , xsize , COLOR_LIGHT_GRAY , 0         , 0         , xsize - 1 , 0        );
  boxfill8(buf , xsize , COLOR_WHITE      , 1         , 1         , xsize - 2 , 1        );
  boxfill8(buf , xsize , COLOR_LIGHT_GRAY , 0         , 0         , 0         , ysize - 1);
  boxfill8(buf , xsize , COLOR_WHITE      , 1         , 1         , 1         , ysize - 2);
  boxfill8(buf , xsize , COLOR_DARK_GRAY  , xsize - 2 , 1         , xsize - 2 , ysize - 2);
  boxfill8(buf , xsize , COLOR_BLACK      , xsize - 1 , 0         , xsize - 1 , ysize - 1);
  boxfill8(buf , xsize , COLOR_LIGHT_GRAY , 2         , 2         , xsize - 3 , ysize - 3);
  boxfill8(buf , xsize , COLOR_DARK_BLUE  , 3         , 3         , xsize - 4 , 20       );
  boxfill8(buf , xsize , COLOR_DARK_GRAY  , 1         , ysize - 2 , xsize - 2 , ysize - 2);
  boxfill8(buf , xsize , COLOR_BLACK      , 0         , ysize - 1 , xsize - 1 , ysize - 1);
  putfonts8_asc(buf, xsize, COLOR_WHITE, 24, 4, title);

  for (int y = 0; y < 14; y++) {
    for (int x = 0; x < 16; x++) {
      char c = closebtn[y][x];
      if (c == '@') {
        c = COLOR_BLACK;
      } else if (c == '$') {
        c = COLOR_DARK_GRAY;
      } else if (c == 'Q') {
        c = COLOR_LIGHT_GRAY;
      } else {
        c = COLOR_WHITE;
      }
      buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
    }
  }
}
