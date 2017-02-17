#include "bootpack.h"

void fifo8_init(FIFO8 *fifo8, int size, byte *buf) {
  fifo8->size = size;
  fifo8->buf  = buf;
  fifo8->free = size;
  fifo8->flags = 0;
  fifo8->head = 0;
  fifo8->tail = 0;
}

int fifo8_push(FIFO8 *fifo8, byte data) {
  if (fifo8->free > 0) {
    fifo8->buf[fifo8->tail] = data;
    fifo8->free--;
    fifo8->tail = (fifo8->tail + 1) % fifo8->size;
    return 0;
  } else {
    fifo8->flags |= FIFO8_OVERFLOW;
    return -1;
  }
}

int fifo8_pop(FIFO8 *fifo8) {
  if (fifo8->free < fifo8->size) {
    int data = fifo8->buf[fifo8->head];
    fifo8->head = (fifo8->head + 1) % fifo8->size;
    fifo8->free++;
    return data;
  } else {
    return -1;
  }
}

int fifo8_head(FIFO8 *fifo8) {
  return fifo8->buf[fifo8->head];
}

int fifo8_count(FIFO8 *fifo8) {
  return fifo8->size - fifo8->free;
}
