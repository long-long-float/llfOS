typedef unsigned char byte;

void io_hlt();
void io_cli(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

void init_palette(void);
void set_palette(int start, int end, byte *rgb);
void boxfill8(byte* vram, int width, byte c, int x0, int y0, int x1, int y1);

void HariMain() {
  init_palette();

  byte* vram = (byte*)0xa0000;
  int width = 320, height = 200;

  boxfill8(vram, width, 4, 0, height - 20, 20, height);

  while(1) io_hlt();
}

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

void boxfill8(byte* vram, int width, byte c, int x0, int y0, int x1, int y1) {
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      vram[y * width + x] = c;
    }
  }
}
