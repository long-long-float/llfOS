#include "bootpack.h"

#define TASK_SWITCH_INTERVAL 2

Timer *task_timer;
TaskControl *task_control;

void task_add(Task *task);
void task_switch_sub();

Task *task_init(MemoryMan *mm) {
  SegmentDescriptor *gdt = (SegmentDescriptor*)ADR_GDT;
  task_control = (TaskControl*)memory_man_alloc_4k(mm, sizeof(TaskControl));

  for (int i = 0; i < MAX_TASK_NUM; i++) {
    Task *t = &task_control->tasks0[i];
    t->flags = 0;
    t->gdt_number = (TASK_GDT0 + i) << 3;
    set_segmdesc(&gdt[TASK_GDT0 + i], 103, (int)&t->tss, AR_TSS32);
  }

  for (int i = 0; i < MAX_TASK_LEVEL; i++) {
    task_control->levels[i].running_num = 0;
    task_control->levels[i].current_task = 0;
  }

  Task *task = task_alloc();
  task->flags = TASK_FLAGS_USED;
  task->priority = 2;
  task->level = 0;

  task_add(task);
  task_switch_sub();

  load_tr(task->gdt_number);

  task_timer = timer_alloc();
  timer_settime(task_timer, task->priority);

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

Task *task_current() {
  TaskLevel *l = &task_control->levels[task_control->current_level];
  return l->tasks[l->current_task];
}

void task_add(Task *task) {
  TaskLevel *level = &task_control->levels[task->level];
  if (level->running_num < MAX_TASK_NUM_PER_LEVEL) {
    level->tasks[level->running_num++] = task;
    task->flags = TASK_FLAGS_USED;
  }
}

void task_remove(Task *task) {
  if (task->flags == TASK_FLAGS_USED) {
    TaskLevel *level = &task_control->levels[task->level];

    int index;
    for (index = 0; index < level->running_num; index++) {
      if (level->tasks[index] == task) break;
    }

    level->running_num--;
    if (index < level->current_task) {
      level->current_task--;
    }

    for (; index < level->running_num; index++) {
      level->tasks[index] = level->tasks[index + 1];
    }

    task->flags = TASK_FLAGS_ALLOC;
  }
}

void task_run(Task *task, int level, int priority) {
  if (level < 0) {
    level = task->level; // レベルを変更しない
  }
  if (priority > 0) {
    task->priority = priority;
  }

  if (task->flags == TASK_FLAGS_USED && task->level != level) {
    // レベルの変更
    task_remove(task);
  }

  if (task->flags != TASK_FLAGS_USED) {
    // スリープからの復帰
    task->level = level;
    task_add(task);
  }

  task_control->will_change_level = true;
}

void task_switch_sub() {
  int i;
  for (i = 0; i < MAX_TASK_LEVEL; i++) {
    if (task_control->levels[i].running_num > 0) break;
  }

  task_control->current_level = i;
  task_control->will_change_level = false;
}

void task_switch() {
  TaskLevel *level = &task_control->levels[task_control->current_level];
  Task *current_task = task_current();

  level->current_task++;
  if (level->current_task >= level->running_num) {
    level->current_task = 0;
  }

  if (task_control->will_change_level) {
    task_switch_sub();
    level = &task_control->levels[task_control->current_level];
  }

  Task *task = level->tasks[level->current_task];

  timer_settime(task_timer, task->priority);

  if (task != current_task) {
    farjump(0, task->gdt_number);
  }
}

void task_sleep(Task *task) {
  if (task->flags == TASK_FLAGS_USED) {
    Task *current_task = task_current();
    task_remove(task);

    if (task == current_task) {
      task_switch_sub();
      current_task = task_current();
      farjump(0, current_task->gdt_number);
    }
  }
}

