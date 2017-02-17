#include "bootpack.h"

extern char hankaku[4096];

void init_palette() {
  static byte table_rgb[16 * 3] = {
    0x00, 0x00, 0x00, /*  0:黒 */
    0xff, 0x00, 0x00, /*  1:明るい赤 */
    0x00, 0xff, 0x00, /*  2:明るい緑 */
    0xff, 0xff, 0x00, /*  3:明るい黄色 */
    0x00, 0x00, 0xff, /*  4:明るい青 */
    0xff, 0x00, 0xff, /*  5:明るい紫 */
    0x00, 0xff, 0xff, /*  6:明るい水色 */
    0xff, 0xff, 0xff, /*  7:白 */
    0xc6, 0xc6, 0xc6, /*  8:明るい灰色 */
    0x84, 0x00, 0x00, /*  9:暗い赤 */
    0x00, 0x84, 0x00, /* 10:暗い緑 */
    0x84, 0x84, 0x00, /* 11:暗い黄色 */
    0x00, 0x00, 0x84, /* 12:暗い青 */
    0x84, 0x00, 0x84, /* 13:暗い紫 */
    0x00, 0x84, 0x84, /* 14:暗い水色 */
    0x84, 0x84, 0x84  /* 15:暗い灰色 */
  };

  set_palette(0, 15, table_rgb);
}

void set_palette(int start, int end, byte *rgb) {
  int eflags = io_load_eflags(); // 割り込み許可フラグを保存
  io_cli();                      // 割り込み禁止

  io_out8(0x03c8, start);
  for (int i = start; i <= end; i++) {
    io_out8(0x03c9, rgb[0] / 4);
    io_out8(0x03c9, rgb[1] / 4);
    io_out8(0x03c9, rgb[2] / 4);
    rgb += 3;
  }
  io_store_eflags(eflags);
}

void boxfill8(byte* vram, int width, Color color, int x0, int y0, int x1, int y1) {
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      vram[y * width + x] = color;
    }
  }
}

void putfont8(byte *vram, int width, Color color, int left, int top, char *font) {
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 8; x++) {
      if ((font[y] >> (7 - x)) & 0x01) {
        vram[(top + y) * width + (left + x)] = color;
      }
    }
  }
}

void putfonts8_asc(byte *vram, int width, Color color, int left, int top, char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    putfont8(vram, width, color, left + i * FONT_WIDTH, top, &hankaku[str[i] * 16]);
  }
}

void init_mouse_cursor8(byte *mouse, char bc) {
  static char cursor[16][16] = {
    "***.............",
    "*O*.............",
    "*OO*............",
    "*OOO*...........",
    "*OOOO*..........",
    "*OOOOO*.........",
    "*OOOOOO*........",
    "*OOOOOO*........",
    "*OOOOO*.........",
    "*OOOOO*.........",
    "*OO*OO*.........",
    "*O**OO*.........",
    "**..*OO*........",
    "*...*OO*........",
    ".....*O*........",
    ".......*........"
  };

  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      if (cursor[y][x] == '*') {
        mouse[y * 16 + x] = COLOR_BLACK;
      }
      if (cursor[y][x] == 'O') {
        mouse[y * 16 + x] = COLOR_WHITE;
      }
      if (cursor[y][x] == '.') {
        mouse[y * 16 + x] = bc;
      }
    }
  }
}

void putblock8_8(byte *vram, int vxsize, int pxsize, int pysize, int px0, int py0, byte *buf, int bxsize) {
  for (int y = 0; y < pysize; y++) {
    for (int x = 0; x < pxsize; x++) {
      byte c = buf[y * bxsize + x];
      int xx = px0 + x, yy = py0 + y;
      if (c != COLOR_NONE && 0 <= xx && xx < vxsize && 0 <= yy /* && yy < vysize */) {
        vram[yy * vxsize + xx] = c;
      }
    }
  }
}

