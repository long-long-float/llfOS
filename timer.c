#include "bootpack.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

#define TIMER_FLAGS_ALLOC 1
#define TIMER_FLAGS_USED  2

#define MAX_TIMEOUT 0xffffffff

TimerControl timerctl;

void init_pit() {
  int interval = 11932;

  io_out8(PIT_CTRL, 0x34);
  io_out8(PIT_CNT0,  interval & 0x00ff);
  io_out8(PIT_CNT0, (interval & 0xff00) >> 8);

  timerctl.count = 0;
  timerctl.next_timeout = MAX_TIMEOUT;
  for (int i = 0; i < MAX_TIMERS; i++) {
    timerctl.timers0[i].flags = 0;
  }

  // 番兵
  Timer *stop = timer_alloc();
  stop->timeout = MAX_TIMEOUT;
  stop->flags   = TIMER_FLAGS_USED;
  stop->next    = NULL;
  timerctl.timer_head = stop;
}

Timer *timer_alloc() {
  for (int i = 0; i < MAX_TIMERS; i++) {
    if (timerctl.timers0[i].flags == 0) {
      timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
      return &timerctl.timers0[i];
    }
  }
  return NULL;
}

void timer_init(Timer *timer, FIFO32 *fifo, byte data) {
  timer->fifo = fifo;
  timer->data = data;
  timer->next = NULL;
}

void timer_free(Timer *timer) {
  timer->flags = 0;
}

void timer_settime(Timer *timer, unsigned timeout) {
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USED;

  int eflags = io_load_eflags();
  io_cli();

  Timer *current = timerctl.timer_head, *prev = NULL;
  while (current) {
    if (current->timeout >= timer->timeout) {
      break;
    }
    prev = current;
    current = current->next;
  }

  if (prev) {
    prev->next = timer;
    timer->next = current;
  } else {
    Timer *head = timerctl.timer_head;
    timerctl.timer_head = timer;
    timer->next = head;
  }

  timerctl.next_timeout = timerctl.timer_head->timeout;

  io_store_eflags(eflags);
}

void inthandler20(int *esp) {
  io_out8(PIC0_OCW2, 0x60);

  timerctl.count++;
  if (timerctl.next_timeout > timerctl.count) {
    return;
  }

  int i;
  Timer *current = timerctl.timer_head;
  for (; current ; current = current->next) {
    if (current->timeout > timerctl.count) {
      break;
    }
    // タイムアウト
    current->flags = TIMER_FLAGS_ALLOC;
    fifo32_push(current->fifo, current->data);
  }

  timerctl.timer_head = current;

  timerctl.next_timeout = timerctl.timer_head;
}

