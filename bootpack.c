#include "bootpack.h"
#include <stdio.h>
#include <string.h>

#define FIFO_KEYBORD_BEGIN 256
#define FIFO_KEYBORD_END   511
#define FIFO_MOUSE_BEGIN   (FIFO_KEYBORD_END+1)
#define FIFO_MOUSE_END     767

#define CONSOLE_ROW_MAX 8
#define CONSOLE_COL_MAX 30

typedef struct {
  byte name[8], ext[3], type;
  char reserved[10];
  unsigned short time, date, clustno;
  unsigned int size;
} FileInfo;

typedef struct {
  char buf[CONSOLE_ROW_MAX][CONSOLE_COL_MAX];
  int cursor_x, cursor_y;
} Console;

extern TimerControl timerctl;

void make_window8(byte *buf, int xsize, int ysize, char *title);

void putfonts8_asc_sht(Sheet *sht, int x, int y, int c, int b, char *s, int l) {
  boxfill8(sht->buf, sht->bwidth, b, x, y, x + l * 8 - 1, y + 15);
  putfonts8_asc(sht->buf, sht->bwidth, c, x, y, s);
  sheet_refresh(sht, x, y, x + l * 8, y + 16);

  return;
}

void console_print(Console *console, char *str);

void console_newline(Console *console, bool put_prompt) {
  console->cursor_y++;
  console->cursor_x = 0;

  if (console->cursor_y >= CONSOLE_ROW_MAX) {
    console->cursor_y--;
    // 下端まで達したらずらす
    for (int i = 1; i < CONSOLE_ROW_MAX; i++) {
      strcpy(console->buf[i - 1], console->buf[i]);
    }
    console->buf[console->cursor_y][0] = '\0';
  }

  if (put_prompt) {
    console_print(console, "> ");
  }
}

void console_putc(Console *console, char c) {
  if (c == '\n') {
    console_newline(console, false);
  } else if (c == '\t') {
    int pad = 4 - (console->cursor_x % 4);
    for (int i = 0; i < pad; i++) console_putc(console, ' ');
  } else {
    console->buf[console->cursor_y][console->cursor_x] = c;
    console->cursor_x++;
    console->buf[console->cursor_y][console->cursor_x] = '\0';

    if (console->cursor_x >= CONSOLE_COL_MAX - 1) {
      // 右端まで達したら改行
      console_newline(console, false);
    }
  }
}

void console_print(Console *console, char *str) {
  for (; *str; str++) console_putc(console, *str);
}

void console_puts(Console *console, char *str) {
  console_print(console, str);
  console_newline(console, false);
}

// 両端のスペースを取り除く
void strip(char *str) {
  int head = 0;
  for (; str[head] && str[head] == ' '; head++) ;
  int tail = strlen(str) - 1;
  for (; tail > head && str[tail] == ' '; tail--) ;

  for (int i = 0; i <= tail - head; i++) {
    str[i] = str[i + head];
  }
  str[tail - head + 1] = '\0';
}

// 大文字に変換
void upcase(char *str) {
  for (int i = 0; str[i]; i++) {
    if ('a' <= str[i] && str[i] <= 'z') {
      str[i] = 'A' + str[i] - 'a';
    }
  }
}

void make_file_name(FileInfo *fi, char *buf) {
  char *name = fi->name, *ext = fi->ext;

  int i = 0, j = 0;
  for (; i < 8 && name[i] != ' '; i++) buf[i] = name[i];

  if (ext[0] != ' ') {
    buf[i++] = '.';
  }

  for (; j < 3 && ext[j] != ' '; j++) buf[i + j] = ext[j];

  buf[i + j] = '\0';
}

void task_console_main(Sheet *sheet, int memsize) {
  Task *task = task_current();
  MemoryMan *memman = (MemoryMan*)MEMORY_MAN_ADDRESS;
  FileInfo *file_info = (FileInfo*)(ADR_DISKIMG + 0x002600);

  int fifobuf[128];
  FIFO32 *fifo = &task->fifo;
  fifo32_init(fifo, 128, fifobuf, task);

  Timer *timer = timer_alloc();
  timer_init(timer, fifo, 1);
  timer_settime(timer, 50);

  Console console;

  Color cursor_c = COLOR_WHITE;

  memset(console.buf, '\0', CONSOLE_ROW_MAX * CONSOLE_COL_MAX * sizeof(char));

  console_print(&console, "> ");

  putfonts8_asc(sheet->buf, sheet->bwidth, COLOR_WHITE, 8, 28, console.buf[console.cursor_y]);
  sheet_refresh(sheet, 0, 0, sheet->bwidth, sheet->bheight);

  while(true) {
    io_cli();

    if (fifo32_count(fifo) == 0) {
      task_sleep(task);
      io_sti();
    } else {
      int data = fifo32_pop(fifo);
      io_sti();
      if (data <= 1) { // カーソル用タイマ
        if (data != 0) {
          timer_init(timer, fifo, 0);
          cursor_c = COLOR_BLACK;
        } else {
          timer_init(timer, fifo, 1);
          cursor_c = COLOR_WHITE;
        }
        timer_settime(timer, 50);

        int cursor_x = 8 + cursor_x * FONT_WIDTH;
        int cursor_y = 28 + cursor_y * FONT_HEIGHT;
        int right = cursor_x + FONT_WIDTH, bottom = cursor_y + FONT_HEIGHT;
        boxfill8(sheet->buf, sheet->bwidth, cursor_c, cursor_x, cursor_y, right - 1, bottom - 1);
        sheet_refresh(sheet, cursor_x, cursor_y, right, bottom);
      }
      if (FIFO_KEYBORD_BEGIN <= data && data <= FIFO_KEYBORD_END) {
        data -= FIFO_KEYBORD_BEGIN;

        boxfill8(sheet->buf, sheet->bwidth, COLOR_BLACK, 8, 28, sheet->bwidth - 8, sheet->bheight - 8);

        if (data == 0x08) { // バックスペース
          // FIXME: はみ出してからのバックスペースに対応
          if (console.cursor_x > 2) {
            console.buf[console.cursor_y][--console.cursor_x] = '\0';
          }
        } else if (data == 0x0a) { // リターン
          char cmd[124], args[124];

          strcpy(cmd, &console.buf[console.cursor_y][2]);

          int args_head = 0;
          for (; cmd[args_head] && cmd[args_head] != ' '; args_head++) ;
          strcpy(args, &cmd[args_head]);
          strip(args);

          cmd[args_head] = '\0';

          if (strlen(cmd) > 0) {
            console_newline(&console, false);

            if (strcmp(cmd, "free") == 0) {
              char buf[128];
              sprintf(buf, "total: %dB, free: %dB", memsize, memory_man_free_size(memman));
              console_puts(&console, buf);

            } else if (strcmp(cmd, "ls") == 0) {
              console_puts(&console, "name         size");

              for (FileInfo *cur = file_info; cur->name[0] != 0; cur++) {
                if (cur->name[0] == 0xe5) continue; // deleted file

                if (!(cur->type & 0x18)) { // neither directory nor file information
                  char buf[128];
                  make_file_name(cur, buf);
                  sprintf(console.buf[console.cursor_y], "%s %d", buf, cur->size);
                  console_newline(&console, false);
                }
              }

            } else if (strcmp(cmd, "cat") == 0) {
              upcase(args);

              // FileInfoを探す
              FileInfo *cur = file_info;
              for (; cur->name[0] != 0; cur++) {
                if (cur->name[0] == 0xe5) continue; // deleted file

                char buf[124];
                make_file_name(cur, buf);
                if (strcmp(buf, args) == 0) break;
              }

              if (cur->name[0] == 0) {
                console_puts(&console, "file not found");
              } else {
                const byte *file_content_addr = (byte*)(ADR_DISKIMG + 0x003e00 + cur->clustno * 512);
                for (int i = 0; i < cur->size; i++) {
                  console_putc(&console, file_content_addr[i]);
                }
              }
            } else {
              char buf[124];
              sprintf(buf, "unknown command '%s'", cmd);
              console_puts(&console, buf);
            }
          }

          console_newline(&console, true);
        } else {
          console_putc(&console, data - FIFO_KEYBORD_BEGIN);
        }

        for (int i = 0; i <= console.cursor_y; i++) {
          putfonts8_asc(sheet->buf, sheet->bwidth, COLOR_WHITE, 8, 28 + i * FONT_HEIGHT, console.buf[i]);
        }

        sheet_refresh(sheet, 0, 0, sheet->bwidth, sheet->bheight);
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
  fifo32_init(&fifo, 128, fifo_buf, NULL);

  // メモリ関連
  int memsize = memtest(0x00400000, 0xbfffffff);

  memory_man_init(memman);
  memory_man_free(memman, 0x00001000, 0x0009e000);
  memory_man_free(memman, 0x00400000, memsize - 0x00400000);

  // シート関連
  SheetControl *sheet_ctl = sheet_control_init(memman, info->vram, info->screenx, info->screeny);

  Sheet *sheet_back = sheet_alloc(sheet_ctl);
  byte *buf_back = (byte*)memory_man_alloc_4k(memman, info->screenx * info->screeny);
  sheet_init(sheet_back, buf_back, info->screenx, info->screeny, -1);
  for (int i = 0; i < info->screenx * info->screeny; i++) buf_back[i] = -1;
  sheet_slide(sheet_back, 0, 0);

  byte buf_mouse[16 * 16];
  Sheet *sheet_mouse = sheet_alloc(sheet_ctl);
  sheet_init(sheet_mouse, buf_mouse, 16, 16, 99);
  init_mouse_cursor8(buf_mouse, 99);
  sheet_slide(sheet_mouse, mousex, mousey);

  const int wwidth = 256, wheight = 165;
  Sheet *sheet_win = sheet_alloc(sheet_ctl);
  byte *buf_win = (byte*)memory_man_alloc_4k(memman, wwidth * wheight);
  sheet_init(sheet_win, buf_win, wwidth, wheight, -1);
  make_window8(buf_win, wwidth, wheight, "terminal");
  boxfill8(sheet_win->buf, sheet_win->bwidth, COLOR_BLACK, 8, 28, wwidth - 8, wheight - 8);
  sheet_slide(sheet_win, 80, 72);

  sheet_updown(sheet_back, 0);
  sheet_updown(sheet_win, 1);
  sheet_updown(sheet_mouse, 2);

  // タスク関連
  Task *task_a = task_init(memman);
  fifo.task = task_a;
  task_run(task_a, 1, 0);

  Task *task_console = task_alloc();

  task_console->tss.eip = (int)&task_console_main;
  // task_console_mainに引数を渡している、ということにするために8を引く
  task_console->tss.esp = memory_man_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
  *((int*)(task_console->tss.esp + 4)) = (int)sheet_win;
  *((int*)(task_console->tss.esp + 8)) = memsize;
  task_console->tss.es = 1 * 8;
  task_console->tss.cs = 2 * 8;
  task_console->tss.ss = 1 * 8;
  task_console->tss.ds = 1 * 8;
  task_console->tss.fs = 1 * 8;
  task_console->tss.gs = 1 * 8;

  task_run(task_console, 2, 2);

  // タイマー関連
  Timer *timer = timer_alloc();
  timer_init(timer, &fifo, 1);
  timer_settime(timer, 100);

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
  static char KEY_TABLE_SHIFT[0x80] = {
    0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0,   0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0,   0,   'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
  };

  int pushed_shift = 0; // 0bit: 左シフト, 1bit: 右シフト
  int key_leds = (info->leds >> 4) & 7;

  // キーボードのLED関係
  FIFO32 keycmd;
  int keycmdbuf[32];
  int keycmd_wait = -1;
  fifo32_init(&keycmd, 32, keycmdbuf, NULL);

  fifo32_push(&keycmd, KEYCMD_LED);
  fifo32_push(&keycmd, key_leds);

  while (1) {
    if (fifo32_count(&keycmd) > 0 && keycmd_wait < 0) {
      keycmd_wait = fifo32_pop(&keycmd);
      wait_KBC_sendready();
      io_out8(PORT_KEYDAT, keycmd_wait);
    }

    // keybuf.dataを読み取っている間に割り込みが来たら困るので
    io_cli();

    if (fifo32_count(&fifo) > 0) {
      int data = fifo32_pop(&fifo);
      io_sti();

      if (FIFO_KEYBORD_BEGIN <= data && data <= FIFO_KEYBORD_END) {
        // キーボードの処理
        data -= FIFO_KEYBORD_BEGIN;

        boxfill8(buf_back, info->screenx, COLOR_BLACK, 0, FONT_HEIGHT * 2, info->screenx, FONT_HEIGHT * 3);

        char buf[128];
        sprintf(buf, "keyboard: %x", data);
        putfonts8_asc(buf_back, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT * 2, buf);

        sheet_refresh(sheet_back, 0, 0, info->screenx, FONT_HEIGHT * 4);

        int sent_data = -1;
        if (data < 0x80) { // 普通の文字
          if (pushed_shift == 0 && KEY_TABLE[data] != 0) {
            sent_data = KEY_TABLE[data];
          } else if (KEY_TABLE_SHIFT[data] != 0) {
            sent_data = KEY_TABLE_SHIFT[data];
          }
        }
        if (data == 0x0e) { // バックスペース
          sent_data = 0x08;
        }
        if (data == 0x0f) { // タブ
          sent_data = 0x09;
        }
        if (data == 0x1c) { // リターン
          sent_data = 0x0a;
        }
        if (data == 0x2a) { // 左シフト
          pushed_shift |= 1;
        }
        if (data == 0x36) { // 右シフト
          pushed_shift |= 1 << 1;
        }
        if (data == 0xaa) { // 左シフト(off)
          pushed_shift &= ~1;
        }
        if (data == 0xb6) { // 右シフト(off)
          pushed_shift &= ~(1 << 1);
        }
        if ('A' <= sent_data && sent_data <= 'Z') { // 小文字に変換
          bool caps = key_leds & 1 << 2;
          if ((caps && pushed_shift) || (!caps && !pushed_shift)) {
            sent_data = 'a' + (sent_data - 'A');
          }
        }
        if (sent_data > 0) {
          fifo32_push(&task_console->fifo, sent_data + FIFO_KEYBORD_BEGIN);
        }

        bool changed_led = false;
        if (data == 0x3a) { // CapsLock
          key_leds ^= 1 << 2;
          changed_led = true;
        }
        if (data == 0x45) { // NumLock
          key_leds ^= 1 << 1;
          changed_led = true;
        }
        if (data == 0x46) { // ScrollLock
          key_leds ^= 1 << 0;
          changed_led = true;
        }
        if (changed_led) {
          fifo32_push(&keycmd, KEYCMD_LED);
          fifo32_push(&keycmd, key_leds);
        }
        if (data == 0xfa) {
          keycmd_wait = -1;
        }
        if (data == 0xfe) {
          wait_KBC_sendready();
          io_out8(PORT_KEYDAT, keycmd_wait);
        }
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
      }
    } else {
      // task_sleep中に割り込みが来たら困るのでsleepから復帰したらフラグを戻す(task bの方は別の割り込みレジスタになるので問題なく割り込みが来る)
      task_sleep(task_a);
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
