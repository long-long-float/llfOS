#include "bootpack.h"

SheetControl *sheet_control_init(MemoryMan *mm, byte *vram, int width, int height) {
  SheetControl *ctl = (SheetControl*)memory_man_alloc(mm, sizeof(SheetControl));
  if (ctl == NULL) return ctl;

  ctl->vram = vram;
  ctl->width = width;
  ctl->height = height;
  ctl->top_index = -1;
  for (int i = 0; i < MAX_SHEET_NUM; i++) {
    ctl->sheets0[i].flags = 0;
  }

  return ctl;
}

Sheet *sheet_alloc(SheetControl *ctl) {
  for (int i = 0; i < MAX_SHEET_NUM; i++) {
    if (ctl->sheets0[i].flags == 0) {
      Sheet *sheet = &ctl->sheets0[i];
      sheet->flags = SHEET_FLAGS_USED;
      sheet->index = -1;
      return sheet;
    }
  }
  return NULL;
}

void sheet_init(Sheet *sheet, byte *buf, int width, int height, int color_inv) {
  sheet->buf       = buf;
  sheet->bwidth    = width;
  sheet->bheight   = height;
  sheet->color_inv = color_inv;
}

void sheet_lift_up(SheetControl *ctl, int index, int dest_index) {
  for (int i = index; i > dest_index; i--) {
    ctl->sheets[i] = ctl->sheets[i - 1];
    ctl->sheets[i]->index = i;
  }
}

void sheet_lift_down(SheetControl *ctl, int index, int dest_index) {
  for (int i = index; i < dest_index; i++) {
    ctl->sheets[i] = ctl->sheets[i + 1];
    ctl->sheets[i]->index = i;
  }
}

void sheet_updown(SheetControl *ctl, Sheet *sheet, int index) {
  int old_index = sheet->index;

  if (index > ctl->top_index + 1) index = ctl->top_index + 1;
  if (index < -1) index = -1;

  sheet->index = index;

  if (index < old_index) { // 低くなる
    if (index >= 0) {
      sheet_lift_up(ctl, old_index, index);
      ctl->sheets[index] = sheet;
    } else { // 非表示化
      if (ctl->top_index > old_index) {
        sheet_lift_down(ctl, old_index, ctl->top_index);
      }
      ctl->top_index--;
    }
    sheet_refresh(ctl);
  } else if (index > old_index) {
    if (old_index >= 0) {
      sheet_lift_down(ctl, old_index, index);
      ctl->sheets[index] = sheet;
    } else {
      sheet_lift_up(ctl, ctl->top_index + 1, index);
      ctl->sheets[index] = sheet;
      ctl->top_index++;
    }
    sheet_refresh(ctl);
  }
}

void sheet_refresh(SheetControl *ctl) {
  for (int i = 0; i <= ctl->top_index; i++) {
    Sheet *sheet = ctl->sheets[i];
    for (int y = 0; y < sheet->bheight; y++) {
      int vy = sheet->vy0 + y;
      for (int x = 0; x < sheet->bwidth; x++) {
        int vx = sheet->vx0 + x;
        byte c = sheet->buf[y * sheet->bwidth + x];
        if (c != sheet->color_inv) {
          ctl->vram[vy * ctl->width + vx] = c;
        }
      }
    }
  }
}

void sheet_slide(SheetControl *ctl, Sheet *sheet, int vx0, int vy0) {
  sheet->vx0 = vx0;
  sheet->vy0 = vy0;
  if (sheet->index >= 0) sheet_refresh(ctl);
}

void sheet_free(SheetControl *ctl, Sheet *sheet) {
  if (sheet->index >= 0) sheet_updown(ctl, sheet, -1);

  sheet->flags = 0;
}

