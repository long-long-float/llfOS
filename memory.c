#include "bootpack.h"

#define EFLAGS_AC_BIT     0x00040000
#define CR0_CACHE_DISABLE 0x60000000

unsigned memtest(unsigned start, unsigned end) {
  bool flag486 = false;

  // 386か486以降かの確認
  unsigned eflag = io_load_eflags();
  eflag |= EFLAGS_AC_BIT;
  io_store_eflags(eflag);

  eflag = io_load_eflags();
  if (eflag & EFLAGS_AC_BIT) { // 386ではACが0になる
    flag486 = true;
  }
  eflag &= ~EFLAGS_AC_BIT;
  io_store_eflags(eflag);

  if (flag486) {
    unsigned cr0 = load_cr0();
    cr0 |= CR0_CACHE_DISABLE; // ギャッシュを禁止
    store_cr0(cr0);
  }

  unsigned res = memtest_sub(start, end);

  if (flag486) {
    unsigned cr0 = load_cr0();
    cr0 &= ~CR0_CACHE_DISABLE; // ギャッシュを許可
    store_cr0(cr0);
  }

  return res;
}

void memory_man_init(MemoryMan *mm) {
  mm->free_count = 0;
  mm->max_free_count = 0;
  mm->lost_size = 0;
  mm->lost_count = 0;
}

unsigned memory_man_free_total(MemoryMan *mm) {
  unsigned sum = 0;
  for (int i = 0; i < mm->free_count; i++) {
    sum += mm->frees[i].size;
  }
  return sum;
}

unsigned memory_man_alloc(MemoryMan *mm, unsigned size){
  for (int i = 0; i < mm->free_count; i++) {
    FreeInfo *fi = &mm->frees[i];
    if (fi->size >= size) {
      unsigned addr = fi->address;
      fi->address += size;
      fi->size    -= size;

      if (fi->size == 0) {
        // 一つづつずらす
        mm->free_count--;
        for (; i < mm->free_count; i++) {
          mm->frees[i] = mm->frees[i + 1];
        }
      }

      return addr;
    }
  }
  return 0;
}

bool memory_man_free(MemoryMan *mm, unsigned address, unsigned size) {
  int index;

  for (index = 0; index < mm->free_count; index++) {
    if (mm->frees[index].address > address) break;
  }

  if (index > 0) {
    FreeInfo *prev = &mm->frees[index - 1];
    if (prev->address + prev->size == address) {
      // 前のFreeInfoとまとめる
      prev->size += size;

      if (index < mm->free_count && prev->address + prev->size == mm->frees[index].address) {
        // 後ろのFreeInfoとまとめる
        prev->size += mm->frees[index].size;

        mm->free_count--;
        for (int i = index; i < mm->free_count; i++) {
          mm->frees[i] = mm->frees[i + 1];
        }
      }
      return true;
    }
  }

  if (index < mm->free_count) {
    FreeInfo *next = &mm->frees[index];
    if (address + size == next->address) {
      // 後ろのFreeInfoとまとめる
      next->address -= size;
      next->size    += size;

      return true;
    }
  }

  if (mm->free_count < MEMORY_MAN_FREE_SIZE) {
    // 1つづつ後ろにずらす
    for (int i = mm->free_count; i > index; i--) {
      mm->frees[i] = mm->frees[i - 1];
    }
    mm->free_count++;

    if (mm->max_free_count < mm->free_count) {
      mm->max_free_count = mm->free_count;
    }

    FreeInfo *cur = &mm->frees[index];
    cur->address = address;
    cur->size    = size;

    return true;
  }

  mm->lost_count++;
  mm->lost_size += size;

  return false;
}

unsigned memory_man_alloc_4k(MemoryMan *mm, unsigned size) {
  return memory_man_alloc(mm, (size + 0xfff) & 0xfffff000); // 4k単位で切り上げ
}

unsigned memory_man_free_4k(MemoryMan *mm, unsigned address, unsigned size) {
  return memory_man_free(mm, address, (size + 0xfff) & 0xfffff000); // 4k単位で切り上げ
}

