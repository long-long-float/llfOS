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
  timerctl.using_index = 0;
  for (int i = 0; i < MAX_TIMERS; i++) {
    timerctl.timers0[i].flags = 0;
  }
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
}

void timer_free(Timer *timer) {
  timer->flags = 0;
}

void timer_settime(Timer *timer, unsigned timeout) {
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USED;

  int eflags = io_load_eflags();
  io_cli();

  int index;
  for (index = 0; index < timerctl.using_index; index++) {
    if (timerctl.timers[index]->timeout >= timer->timeout) {
      break;
    }
  }

  for (int j = timerctl.using_index; j > index; j++) {
    timerctl.timers[j] = timerctl.timers[j - 1];
  }
  timerctl.using_index++;

  timerctl.timers[index] = timer;
  timerctl.next_timeout = timerctl.timers[0]->timeout;

  io_store_eflags(eflags);
}

void inthandler20(int *esp) {
  io_out8(PIC0_OCW2, 0x60);

  timerctl.count++;
  if (timerctl.next_timeout > timerctl.count) {
    return;
  }

  int i;
  for (i = 0; i < timerctl.using_index; i++) {
    Timer *timer = timerctl.timers[i];

    if (timer->timeout > timerctl.count) {
      break;
    }
    // タイムアウト
    timer->flags = TIMER_FLAGS_ALLOC;
    fifo32_push(timer->fifo, timer->data);
  }

  timerctl.using_index -= i;
  for (int j = 0; j < timerctl.using_index; j++) {
    timerctl.timers[j] = timerctl.timers[i + j];
  }

  if (timerctl.using_index > 0) {
    timerctl.next_timeout = timerctl.timers[0]->timeout;
  } else {
    timerctl.next_timeout = MAX_TIMEOUT;
  }
}


