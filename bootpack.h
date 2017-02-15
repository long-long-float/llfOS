typedef unsigned char byte;

// asmhead.nas

typedef struct {
  byte cycles, leds, vmode, reserve;
  short screenx, screeny;
  byte *vram;
} BootInfo;

// naskfunc.nas

void io_hlt();
void io_cli();
void io_out8(int port, int data);
int io_load_eflags();
void io_store_eflags(int eflags);
void load_gdtr(int limit, void *addr);
void load_idtr(int limit, void *addr);

// graphic.c

#define FONT_WIDTH  8
#define FONT_HEIGHT 16

enum Color {
  COLOR_BLACK     = 0,
  COLOR_LITE_BLUE = 4,
  COLOR_WHITE     = 7,

  COLOR_NONE      = 255, // 透明
};

void init_palette(void);
void set_palette(int start, int end, byte *rgb);
void boxfill8(byte* vram, int width, byte c, int x0, int y0, int x1, int y1);
void putfont8(byte *vram, int width, byte color, int left, int top, char *font);
void putfonts8_asc(byte *vram, int width, byte color, int left, int top, char *str);
void init_mouse_cursor8(byte *mouse, char bc);
void putblock8_8(byte *vram, int vxsize, int pxsize, int pysize, int px0, int py0, byte *buf, int bxsize);

// dsctbl.c

typedef struct {
  // base_* : セグメントの番地
  // limit_* : セグメントの大きさ(ページ(4KB)単位)
  // access_right : セグメントのアクセス権

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

