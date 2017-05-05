#define NULL 0

typedef unsigned char byte;

// boolean
typedef _Bool bool;
#define true  1
#define false 0

// asmhead.nas

typedef struct {
  byte cycles, leds, vmode, reserve;
  short screenx, screeny;
  byte *vram;
} BootInfo;

#define ADR_BOOTINFO 0x00000ff0
#define ADR_DISKIMG  0x00100000

// naskfunc.nas

void io_hlt();
void io_sti();
void io_cli();
void io_stihlt();
void io_out8(int port, int data);
int io_in8(int port);
int io_load_eflags();
void io_store_eflags(int eflags);
void load_gdtr(int limit, void *addr);
void load_idtr(int limit, void *addr);
void load_tr(int tr);
void farjump(int eip, int cs);
int load_cr0();
void store_cr0(int cr0);
void asm_inthandler20();
void asm_inthandler21();
void asm_inthandler27();
void asm_inthandler2c();
unsigned memtest_sub(unsigned start, unsigned end);

// graphic.c

#define FONT_WIDTH  8
#define FONT_HEIGHT 16

typedef enum {
  COLOR_BLACK     = 0,
  COLOR_LIGHT_BLUE = 4,
  COLOR_WHITE     = 7,
  COLOR_LIGHT_GRAY = 8,
  COLOR_DARK_BLUE = 12,
  COLOR_DARK_GRAY = 15,
} Color;

void init_palette(void);
void set_palette(int start, int end, byte *rgb);
void boxfill8(byte* vram, int width, Color color, int x0, int y0, int x1, int y1);
void putfont8(byte *vram, int width, Color color, int left, int top, char *font);
void putfonts8_asc(byte *vram, int width, Color color, int left, int top, char *str);
void init_mouse_cursor8(byte *mouse, char bc);
void putblock8_8(byte *vram, int vxsize, int pxsize, int pysize, int px0, int py0, byte *buf, int bxsize);

// dsctbl.c

#define ADR_IDT       0x0026f800
#define LIMIT_IDT     0x000007ff
#define ADR_GDT       0x00270000
#define LIMIT_GDT     0x0000ffff
#define ADR_BOTPAK    0x00280000
#define LIMIT_BOTPAK  0x0007ffff
#define AR_DATA32_RW  0x4092
#define AR_CODE32_ER  0x409a
#define AR_TSS32      0x0089
#define AR_INTGATE32  0x008e

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

// int.c

// PIC0 : マスターPIC
// PIC1 : スレーブPIC
#define PIC0_ICW1   0x0020
#define PIC0_OCW2   0x0020
#define PIC0_IMR    0x0021
#define PIC0_ICW2   0x0021
#define PIC0_ICW3   0x0021
#define PIC0_ICW4   0x0021
#define PIC1_ICW1   0x00a0
#define PIC1_OCW2   0x00a0
#define PIC1_IMR    0x00a1
#define PIC1_ICW2   0x00a1
#define PIC1_ICW3   0x00a1
#define PIC1_ICW4   0x00a1

#define PORT_KEYDAT 0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE    0x60
#define KBC_MODE    0x47
#define KEYCMD_SENDTO_MOUSE  0xd4
#define MOUSECMD_ENABLE      0xf4
#define KEYCMD_LED           0xed

void init_pic();

// fifo8.c

typedef struct {
  byte *buf;
  int  size, free;
  int  head, tail;
  int  flags;
} FIFO8;

typedef enum {
  FIFO8_OVERFLOW = 1,
} FIFO8Flags;

void fifo8_init(FIFO8 *fifo8, int size, byte *buf);
int fifo8_push(FIFO8 *fifo8, byte data);
int fifo8_pop(FIFO8 *fifo8);
int fifo8_head(FIFO8 *fifo8);
int fifo8_count(FIFO8 *fifo8);

// fifo32.c

typedef struct {
  int *buf;
  int  size, free;
  int  head, tail;
  int  flags;

  // sleepしたタスクをrunする用
  struct _Task *task;
} FIFO32;

typedef enum {
  FIFO32_OVERFLOW = 1,
} FIFO32Flags;

void fifo32_init(FIFO32 *fifo32, int size, int *buf, struct _Task *task);
int fifo32_push(FIFO32 *fifo32, int data);
int fifo32_pop(FIFO32 *fifo32);
int fifo32_head(FIFO32 *fifo32);
int fifo32_count(FIFO32 *fifo32);

// keyboard.c

void wait_KBC_sendready();
void init_keyboard(FIFO32 *fifo32, int data0);
void inthandler21(int *esp);

// mouse.c

void enable_mouse(FIFO32 *fifo32, int data0);
// PS/2マウスからの割り込み
void inthandler2c(int *esp);

// memory.c

#define MEMORY_MAN_FREE_SIZE 4090
#define MEMORY_MAN_ADDRESS   0x003c0000

typedef struct {
  unsigned address, size;
} FreeInfo;

typedef struct {
  int free_count;     // FreeInfoの個数
  int max_free_count; // free_countの最大値
  int lost_size;      // 解放に失敗したサイズ
  int lost_count;     // 解放に失敗した回数
  FreeInfo frees[MEMORY_MAN_FREE_SIZE];
} MemoryMan;

unsigned memtest(unsigned start, unsigned end);
void memory_man_init(MemoryMan *mm);
unsigned memory_man_free_total(MemoryMan *mm);
unsigned memory_man_alloc(MemoryMan *mm, unsigned size);
bool memory_man_free(MemoryMan *mm, unsigned address, unsigned size);
unsigned memory_man_alloc_4k(MemoryMan *mm, unsigned size);
unsigned memory_man_free_4k(MemoryMan *mm, unsigned address, unsigned size);
unsigned memory_man_free_size(MemoryMan *mm);

// sheet.c

#define MAX_SHEET_NUM 256
#define SHEET_FLAGS_USED 1

struct _SheetControl;
typedef struct _SheetControl SheetControl;

typedef struct {
  SheetControl *ctl;

  byte *buf;
  int bwidth, bheight; // bufの幅、高さ
  int vx0, vy0;        // vram上の座標
  int color_inv;       // 透明色
  int index;
  int flags;
} Sheet;

struct _SheetControl {
  byte *vram;
  byte *map;
  int width, height;
  int top_index;
  Sheet *sheets[MAX_SHEET_NUM];
  Sheet sheets0[MAX_SHEET_NUM];
};

SheetControl *sheet_control_init(MemoryMan *mm, byte *vram, int width, int height);
Sheet *sheet_alloc(SheetControl *ctl);
void sheet_init(Sheet *sheet, byte *buf, int width, int height, int color_inv);
void sheet_updown(Sheet *sheet, int index);
void sheet_refresh(Sheet *sheet, int bx0, int by0, int bx1, int by1);
void sheet_slide(Sheet *sheet, int vx0, int vy0);
void sheet_free(Sheet *sheet);
bool sheet_in(Sheet *sheet, int x, int y);

// logger.c

void logger_mark_next_out();
void logger_log(byte *vram, int width, char *str);

// timer.c

#define MAX_TIMERS 500

typedef struct _Timer {
  unsigned timeout;
  unsigned flags;
  FIFO32 *fifo;
  unsigned data;

  struct _Timer *next;
} Timer;

typedef struct {
  unsigned count;
  unsigned next_timeout;

  Timer *timer_head;
  Timer timers0[MAX_TIMERS];
} TimerControl;

void init_pit();
Timer *timer_alloc();
void timer_init(Timer *timer, FIFO32 *fifo, byte data);
void timer_free(Timer *timer);
void timer_settime(Timer *timer, unsigned timeout);
void inthandler20(int *esp);

// task.c

#define MAX_TASK_NUM_PER_LEVEL 100
#define MAX_TASK_LEVEL         10
#define MAX_TASK_NUM           (MAX_TASK_NUM_PER_LEVEL * MAX_TASK_LEVEL)
#define TASK_GDT0              3

#define TASK_FLAGS_ALLOC 1
#define TASK_FLAGS_USED  2

typedef struct {
  // タスクの設定
  int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
  // 32bitレジスタ
  int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
  // 16bitレジスタ
  int es, cs, ss, ds, fs, gs;
  // タスクの設定
  int ldtr, iomap;
} TSS32; // task status segment

typedef struct _Task {
  int gdt_number, flags;
  TSS32 tss;
  int priority;
  int level;

  FIFO32 fifo;
} Task;

typedef struct {
  int running_num;
  int current_task;
  Task *tasks[MAX_TASK_NUM_PER_LEVEL];
} TaskLevel;

typedef struct {
  int current_level;
  bool will_change_level;
  TaskLevel levels[MAX_TASK_LEVEL];
  Task tasks0[MAX_TASK_NUM];
} TaskControl;

Task *task_init(MemoryMan *mm);
Task *task_alloc();
void task_run(Task *task, int level, int priority);
void task_switch();
Task *task_current();
void task_sleep(Task *task);
