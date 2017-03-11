#include "bootpack.h"

Timer *task_timer;
int current_task;

void task_init() {
  task_timer = timer_alloc();
  timer_settime(task_timer, 2);
  task_timer->data = 2;
  current_task = 3;
}

void task_switch() {
  if (current_task == 3) {
    current_task = 4;
  } else {
    current_task = 3;
  }

  timer_settime(task_timer, 2);

  farjump(0, current_task << 3);
}


