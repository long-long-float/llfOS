#include "bootpack.h"

const void *GDT_ADDR = (void*)0x00270000;
const void *IDT_ADDR = (void*)0x0026f800;

void init_gdtidt() {
  SegmentDescriptor *gdt = (SegmentDescriptor*)GDT_ADDR;
  GateDescriptor    *idt =    (GateDescriptor*)IDT_ADDR;

  for (int i = 0; i < 8192; i++) {
    set_segmdesc(&gdt[i], 0, 0, 0);
  }
  set_segmdesc(&gdt[1], 0xffffffff, 0x00000000, AR_DATA32_RW);
  set_segmdesc(&gdt[2], LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
  load_gdtr(0xffff, GDT_ADDR);

  for (int i = 0; i < 256; i++) {
    set_gatedesc(&idt[i], 0, 0, 0);
  }
  load_idtr(0x7ff, IDT_ADDR);

  set_gatedesc(&idt[0x20], (int)asm_inthandler20, 2 << 3, AR_INTGATE32);
  set_gatedesc(&idt[0x21], (int)asm_inthandler21, 2 << 3, AR_INTGATE32);
  set_gatedesc(&idt[0x2c], (int)asm_inthandler2c, 2 << 3, AR_INTGATE32);
  set_gatedesc(&idt[0x27], (int)asm_inthandler27, 2 << 3, AR_INTGATE32);
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
