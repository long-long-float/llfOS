#include "bootpack.h"
#include <string.h>

void file_make_name(FileInfo *fi, char *buf) {
  char *name = fi->name, *ext = fi->ext;

  int i = 0, j = 0;
  for (; i < 8 && name[i] != ' '; i++) buf[i] = name[i];

  if (ext[0] != ' ') {
    buf[i++] = '.';
  }

  for (; j < 3 && ext[j] != ' '; j++) buf[i + j] = ext[j];

  buf[i + j] = '\0';
}

FileInfo *file_find(FileInfo *head, char *name) {
  FileInfo *cur = head;
  for (; cur->name[0] != 0; cur++) {
    if (cur->name[0] == 0xe5) continue; // deleted file

    char buf[124];
    file_make_name(cur, buf);
    if (strcmp(buf, name) == 0) return cur;
  }

  return NULL;
}

// 返り値: ファイルが全て読めたか
bool file_read(FileInfo *file, char *buf, int buf_size) {
  int idx_file = 0, current_clustno = file->clustno;
  int idx_buf = 0;

  while(idx_file < file->size) {
    const byte *file_content_addr = (byte*)(ADR_DISKIMG + 0x003e00 + current_clustno * 512);

    for (int j = 0; j < 512 && idx_file < file->size; idx_file++, j++) {
      if (idx_buf < buf_size) {
        buf[idx_buf] = file_content_addr[j];
        idx_buf++;
      } else {
        return false;
      }
    }

    // FATから次のセクタを取る
    // FATのセクタは3バイトずつにまとめることで読めるようになる
    // ab cd ef -> dab efc
    // ||
    //  - head_addr
    byte *head_addr = (byte*)(ADR_DISKIMG + 0x000200 + current_clustno / 2 * 3);
    int next_clustno = 0;
    if (current_clustno % 2 == 0) {
      next_clustno = *head_addr | ((*(head_addr + 1) & 0xf) << 4);
    } else {
      next_clustno = ((*(head_addr + 1) & 0xf0) >> 4) | (*(head_addr + 2) << 4);
    }

    if ((next_clustno & 0xff8) == 0xff8) break; // 0xff8 ~ 0xfffは続きはない

    current_clustno = next_clustno;
  }

  return true;
}

