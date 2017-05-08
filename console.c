#include "bootpack.h"
#include <string.h>

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

