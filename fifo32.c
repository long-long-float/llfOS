#include "bootpack.h"

void fifo32_init(FIFO32 *fifo32, int size, int *buf) {
  fifo32->size = size;
  fifo32->buf  = buf;
  fifo32->free = size;
  fifo32->flags = 0;
  fifo32->head = 0;
  fifo32->tail = 0;
}

int fifo32_push(FIFO32 *fifo32, int data) {
  if (fifo32->free > 0) {
    fifo32->buf[fifo32->tail] = data;
    fifo32->free--;
    fifo32->tail = (fifo32->tail + 1) % fifo32->size;
    return 0;
  } else {
    fifo32->flags |= FIFO32_OVERFLOW;
    return -1;
  }
}

int fifo32_pop(FIFO32 *fifo32) {
  if (fifo32->free < fifo32->size) {
    int data = fifo32->buf[fifo32->head];
    fifo32->head = (fifo32->head + 1) % fifo32->size;
    fifo32->free++;
    return data;
  } else {
    return -1;
  }
}

int fifo32_head(FIFO32 *fifo32) {
  return fifo32->buf[fifo32->head];
}

int fifo32_count(FIFO32 *fifo32) {
  return fifo32->size - fifo32->free;
}
