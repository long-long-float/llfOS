#include <stdio.h>

#define FONT_WIDTH  8
#define FONT_HEIGHT 16

enum Color {
  COLOR_BLACK     = 0,
  COLOR_LITE_BLUE = 4,
  COLOR_WHITE     = 7,

  COLOR_NONE      = 255, // 透明
};

extern char hankaku[4096];

typedef unsigned char byte;

typedef struct {
  byte cycles, leds, vmode, reserve;
  short screenx, screeny;
  byte *vram;
} BootInfo;

typedef struct {
  short limit_low, base_low;
  byte  base_mid, access_right;
  byte  limit_high, base_high;
} SegmentDescriptor;

typedef struct {
  short offset_low, selector;
  byte  dw_count, access_right;
  short offset_high;
} GateDescriptor;

void init_gdtidt();
void set_segmdesc(SegmentDescriptor *sd, unsigned int limit, int base, int ar);
void set_gatedesc(GateDescriptor *gd, int offset, int selector, int ar);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);

void io_hlt();
void io_cli();
void io_out8(int port, int data);
int io_load_eflags();
void io_store_eflags(int eflags);

void init_palette(void);
void set_palette(int start, int end, byte *rgb);
void boxfill8(byte* vram, int width, byte c, int x0, int y0, int x1, int y1);
void putfont8(byte *vram, int width, byte color, int left, int top, char *font);
void putfonts8_asc(byte *vram, int width, byte color, int left, int top, char *str);
void init_mouse_cursor8(byte *mouse, char bc);
void putblock8_8(byte *vram, int vxsize, int pxsize, int pysize, int px0, int py0, byte *buf, int bxsize);

void HariMain() {
  init_palette();

  BootInfo* info = (BootInfo*)0x0ff0;

  byte* vram = info->vram;

  putfonts8_asc(vram, info->screenx, COLOR_WHITE, 0, 0, "Welcome to llfOS!");

  char buf[1024];
  sprintf(buf, "screen %dx%d", info->screenx, info->screeny);
  putfonts8_asc(vram, info->screenx, COLOR_WHITE, 0, FONT_HEIGHT, buf);

  byte mcursor[16 * 16];
  init_mouse_cursor8(mcursor, COLOR_NONE);
  putblock8_8(vram, info->screenx, 16, 16, info->screenx / 2, info->screeny / 2, mcursor, 16);

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

void boxfill8(byte* vram, int width, byte color, int x0, int y0, int x1, int y1) {
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      vram[y * width + x] = color;
    }
  }
}

void putfont8(byte *vram, int width, byte color, int left, int top, char *font) {
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 8; x++) {
      if ((font[y] >> (7 - x)) & 0x01) {
        vram[(top + y) * width + (left + x)] = color;
      }
    }
  }
}

void putfonts8_asc(byte *vram, int width, byte color, int left, int top, char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    putfont8(vram, width, color, left + i * FONT_WIDTH, top, &hankaku[str[i] *16]);
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
      if (c != COLOR_NONE) {
        vram[(py0 + y) * vxsize + (px0 + x)] = c;
      }
    }
  }
}

const void *GDT_ADDR = (void*)0x00270000;
const void *IDT_ADDR = (void*)0x0026f800;

void init_gdtidt() {
  SegmentDescriptor *gdt = (SegmentDescriptor*)GDT_ADDR;
  GateDescriptor    *idt =    (GateDescriptor*)IDT_ADDR;

  for (int i = 0; i < 8192; i++) {
    set_segmdesc(&gdt[i], 0, 0, 0);
  }
  set_segmdesc(&gdt[1], 0xffffffff, 0x00000000, 0x4092);
  set_segmdesc(&gdt[2], 0x0007ffff, 0x00280000, 0x409a);
  load_gdtr(0xffff, GDT_ADDR);

  for (int i = 0; i < 256; i++) {
    set_gatedesc(&idt[i], 0, 0, 0);
  }
  load_idtr(0x7ff, IDT_ADDR);
}

void set_segmdesc(SegmentDescriptor *sd, unsigned int limit, int base, int ar) {
  if (limit > 0xfffff) {
    ar |= 0x8000; /* G_bit = 1 */
    limit /= 0x1000;
  }
  sd->limit_low    = limit & 0xffff;
  sd->base_low     = base & 0xffff;
  sd->base_mid     = (base >> 16) & 0xff;
  sd->access_right = ar & 0xff;
  sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
  sd->base_high    = (base >> 24) & 0xff;
}

void set_gatedesc(GateDescriptor *gd, int offset, int selector, int ar) {
  gd->offset_low   = offset & 0xffff;
  gd->selector     = selector;
  gd->dw_count     = (ar >> 8) & 0xff;
  gd->access_right = ar & 0xff;
  gd->offset_high  = (offset >> 16) & 0xffff;
}
