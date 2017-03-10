#include "bootpack.h"
#include <stdio.h>

#define FIFO_KEYBORD_BEGIN 256
#define FIFO_KEYBORD_END   511
#define FIFO_MOUSE_BEGIN   (FIFO_KEYBORD_END+1)
#define FIFO_MOUSE_END     767

extern TimerControl timerctl;

void make_window8(byte *buf, int xsize, int ysize, char *title);

void putfonts8_asc_sht(Sheet *sht, int x, int y, int c, int b, char *s, int l) {
  boxfill8(sht->buf, sht->bwidth, b, x, y, x + l * 8 - 1, y + 15);
  putfonts8_asc(sht->buf, sht->bwidth, c, x, y, s);
  sheet_refresh(sht, x, y, x + l * 8, y + 16);

  return;
}

void task_b_main() {
  FIFO32 fifo;
  int fifobuf[128];

  fifo32_init(&fifo, 128, fifobuf);
  Timer *timer_ts = timer_alloc();
  timer_init(timer_ts, &fifo, 1);
  timer_settime(timer_ts, 2);

  Sheet *sht_back = (Sheet*) *((int *) 0x0fec);

  int count = 0;

  while (1) {
    count++;

    char s[128];
    sprintf(s, "%10d", count);
    putfonts8_asc_sht(sht_back, 0, 144, COLOR_WHITE, COLOR_BLACK, s, 30);

    io_cli();

    if (fifo32_count(&fifo) == 0) {
      io_sti();
    } else {
      int i = fifo32_pop(&fifo);

      io_sti();
      if (i == 1) {
        farjump(0, 3 * 8);
        timer_settime(timer_ts, 2);
      }
    }
  }
}

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

  int fifo_buf[128];
  FIFO32 fifo;
  fifo32_init(&fifo, 128, fifo_buf);

  // タスク関連
  TSS32 tss_a, tss_b;
  tss_a.ldtr = 0;
  tss_a.iomap = 0x40000000;

  tss_b.ldtr = 0;
  tss_b.iomap = 0x40000000;
  tss_b.eip = (int)&task_b_main;
  tss_b.eflags = 0x00000202;
	tss_b.eax = 0;
	tss_b.ecx = 0;
	tss_b.edx = 0;
	tss_b.ebx = 0;
	tss_b.esp = memory_man_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	tss_b.ebp = 0;
	tss_b.esi = 0;
	tss_b.edi = 0;
	tss_b.es = 1 * 8;
	tss_b.cs = 2 * 8;
	tss_b.ss = 1 * 8;
	tss_b.ds = 1 * 8;
	tss_b.fs = 1 * 8;
	tss_b.gs = 1 * 8;

  SegmentDescriptor *gdt = (SegmentDescriptor*)ADR_GDT;
  set_segmdesc(&gdt[3], 103, &tss_a, AR_TSS32);
  set_segmdesc(&gdt[4], 103, &tss_b, AR_TSS32);

  load_tr(3 << 3);

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

  *((int*)0x0fec) = (int) sheet_back;

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
  Timer *timer = timer_alloc();
  timer_init(timer, &fifo, 1);
  timer_settime(timer, 100);

  Timer *task_timer = timer_alloc();
  timer_init(task_timer, &fifo, 2);
  timer_settime(task_timer, 2);

  sprintf(buf, "sheet %d %d %d %d", sheet_ctl->width, sheet_ctl->height, sheet_ctl->top_index, sheet_ctl->sheets[0]->color_inv);
  putfonts8_asc(info->vram, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT, buf);

  putfonts8_asc(buf_back, info->screenx, COLOR_WHITE, 0, 0, "Welcome to llfOS!");

  sprintf(buf, "screen %dx%d", info->screenx, info->screeny);
  putfonts8_asc(buf_back, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT, buf);

  io_out8(PIC0_IMR, 0xf8); // タイマーとPIC1,キーボードを許可
  io_out8(PIC1_IMR, 0xef); // マウスを許可

  init_keyboard(&fifo, FIFO_KEYBORD_BEGIN);
  enable_mouse(&fifo, FIFO_MOUSE_BEGIN);

  sprintf(buf, "memory %dMB, free %dKB", memsize / (1024 * 1024), memory_man_free_total(memman) / 1024);
  putfonts8_asc(buf_back, info->screenx, COLOR_WHITE, 0, info->screeny - FONT_HEIGHT, buf);

  bool received_0xfa = false;
  int mouse_info[3];
  int mouse_info_count = 0;

  sheet_refresh(sheet_back, 0, 0, info->screenx, info->screeny);

  static char KEY_TABLE[0x54] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.'
  };

  int count = 0;

  while (1) {
    // keybuf.dataを読み取っている間に割り込みが来たら困るので
    io_cli();

    if (fifo32_count(&fifo) > 0) {
      int data = fifo32_pop(&fifo);
      io_sti();

      if (FIFO_KEYBORD_BEGIN <= data && data <= FIFO_KEYBORD_END) {
        // キーボードの処理
        data -= FIFO_KEYBORD_BEGIN;

        boxfill8(buf_back, info->screenx, COLOR_BLACK, 0, FONT_HEIGHT * 2, info->screenx, FONT_HEIGHT * 3);

        char buf[] = "keyboard: .";
        buf[10] = KEY_TABLE[data];
        putfonts8_asc(buf_back, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT * 2, buf);

        sheet_refresh(sheet_back, 0, 0, info->screenx, FONT_HEIGHT * 4);
      } else if (FIFO_MOUSE_BEGIN <= data && data <= FIFO_MOUSE_END) {
        // マウスの処理
        data -= FIFO_MOUSE_BEGIN;
        if (!received_0xfa) {
          if (data == 0xfa) {
            received_0xfa = true;
          }
        } else {
          if (mouse_info_count == 0 && !(data & 0x08)) {
            // エラーなので読みとばす
          } else {
            mouse_info[mouse_info_count] = data;
            mouse_info_count++;

            if (mouse_info_count >= 3) {
              mouse_info_count = 0;

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

              if (l == 'L' && sheet_in(sheet_win, mousex, mousey)) {
                sheet_slide(sheet_win, sheet_win->vx0 + dx, sheet_win->vy0 + dy);
              }
            }
          }
        }
      } else if (data == 1) { // timer
        count++;

        timer_settime(timer, 100);

        boxfill8(buf_win, wwidth, COLOR_LIGHT_GRAY, 40, 28, 119, 43);
        sprintf(buf, "%d", count);
        putfonts8_asc(buf_win, wwidth, COLOR_BLACK, 40, 28, buf);
        sheet_refresh(sheet_win, 40, 28, 120, 44);
      } else if (data == 2) {
        farjump(0, 4 << 3);
        timer_settime(task_timer, 2);
      }
    } else {
      // STIの後に割り込みが来るとキー情報があるのにHLTしてしまうので一緒に実行する
      io_stihlt();
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
