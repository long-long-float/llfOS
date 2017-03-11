#include "bootpack.h"

#define TASK_FLAGS_ALLOC 1
#define TASK_FLAGS_USED  2

#define TASK_SWITCH_INTERVAL 2

Timer *task_timer;
TaskControl *task_control;

Task *task_init(MemoryMan *mm) {
  SegmentDescriptor *gdt = (SegmentDescriptor*)ADR_GDT;
  task_control = (TaskControl*)memory_man_alloc_4k(mm, sizeof(TaskControl));

  for (int i = 0; i < MAX_TASK_NUM; i++) {
    Task *t = &task_control->tasks0[i];
    t->flags = 0;
    t->gdt_number = (TASK_GDT0 + i) << 3;
    set_segmdesc(&gdt[TASK_GDT0 + i], 103, (int)&t->tss, AR_TSS32);
  }

  Task *task = task_alloc();
  task->flags = TASK_FLAGS_ALLOC;
  task_control->running_num = 1;
  task_control->current_task = 0;
  task_control->tasks[0] = task;

  load_tr(task->gdt_number);

  task_timer = timer_alloc();
  timer_settime(task_timer, TASK_SWITCH_INTERVAL);

  return task;
}

Task *task_alloc() {
  for (int i = 0; i < MAX_TASK_NUM; i++) {
    if (task_control->tasks0[i].flags == 0) {
      Task *task = &task_control->tasks0[i];
      task->flags = TASK_FLAGS_ALLOC;
      task->tss.eflags = 0x00000202; // IF = 1;
      task->tss.eax = 0; // とりあえず0にしておくことにする
      task->tss.ecx = 0;
      task->tss.edx = 0;
      task->tss.ebx = 0;
      task->tss.ebp = 0;
      task->tss.esi = 0;
      task->tss.edi = 0;
      task->tss.es = 0;
      task->tss.ds = 0;
      task->tss.fs = 0;
      task->tss.gs = 0;
      task->tss.ldtr = 0;
      task->tss.iomap = 0x40000000;
      return task;
    }
  }

  return NULL;
}

void task_run(Task *task) {
  task->flags = TASK_FLAGS_USED;
  task_control->tasks[task_control->running_num] = task;
  task_control->running_num++;
}

void task_switch() {
  timer_settime(task_timer, TASK_SWITCH_INTERVAL);

  if (task_control->running_num >= 2) {
    task_control->current_task++;
    if (task_control->current_task == task_control->running_num) {
      task_control->current_task = 0;
    }
    farjump(0, task_control->tasks[task_control->current_task]->gdt_number);
  }
}


