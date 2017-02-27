#include "bootpack.h"

bool next_out = false;

void logger_mark_next_out() {
  next_out = true;
}

void logger_log(byte *vram, int width, const char *str) {
  if (next_out) {
    putfonts8_asc(vram, width, COLOR_WHITE, 0, FONT_HEIGHT * 4, str);
    next_out = false;
  }
}
