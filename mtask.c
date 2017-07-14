#include "osmain.h"
#include <stdio.h>

struct Timer* tasktm;

void task_idle(void)	//限制任务idle
{
    for (;;) {
        io_hlt();	//让CPU不断的HLT等待中断的到来
    }
}

struct TASKCTL *taskctl;
struct TASK * task_init(struct MEMMAN *memman) {
	struct TASK *task,*idle;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)GDT_ADR;
	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof(struct TASKCTL));
	int i;
	for(i=0;i<MAX_TASKS;i++){
		taskctl->task[i].flags=0;
		taskctl->task[i].sel=(TASK_GDT0+i)<<3;
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->task[i].tss, AR_TSS);
	}
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		taskctl->level[i].runnings = 0;
		taskctl->level[i].now = 0;
	}
	task = task_alloc();
	task->flags = 2;  // 活动中的标志
	task->priority = 2; 
	task->level = 0;  // 最高LEVEL
	task_add(task);
	task_switchsub();
	load_tr(task->sel);
	tasktm = timer_alloc();
	timer_set(tasktm, task->priority);

	idle = task_alloc();	//为dile分配空间
    idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
    idle->tss.eip = (int) &task_idle;	//dile任务做的事，执行这个函数
    idle->tss.es = 3 * 8;
    idle->tss.cs = 4 * 8;
    idle->tss.ss = 3 * 8;
    idle->tss.ds = 3 * 8;
    idle->tss.fs = 3 * 8;
    idle->tss.gs = 3 * 8;
    //idle放在最后一层，FLAG置1
    task_run(idle, MAX_TASKLEVELS - 1, 1);
	return task;
} 

struct TASK *task_alloc(void) {
	struct TASK *task;
	int i;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->task[i].flags == 0) {
			task = &taskctl->task[i];
			task->flags = 1; // 正在使用的标志
			task->tss.eflags = 0x00000202; //
			task->tss.eax = 0;
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
	return 0;
}

void task_run(struct TASK *task, int level, int priority) {
	if(level < 0) {
		level = task->level;  // 不改变LEVEL
	}
	
	if(priority > 0) {
		task->priority = priority;
	}
	
	if (task->flags == 2 && task->level != level) {  // 改变活动中的Level
		task_remove(task);
	}
	
	if(task->flags != 2) {
		task->level = level;
		task_add(task);
	}
	
	taskctl->lv_change = 1; // 下次任务切换时检查Level
	return;
}

void task_switch(void) {
	struct TASKLEVEL *t1 = &taskctl->level[taskctl->now_level];
	struct TASK *new_task, *now_task = t1->tasks[t1->now];
	t1->now ++;
	if (t1->now == t1->runnings) {
		t1->now = 0;
	}
	if (taskctl->lv_change != 0) {
		task_switchsub();
		t1 = &taskctl->level[taskctl->now_level];
	}
	new_task = t1->tasks[t1->now];
	timer_set(tasktm, new_task->priority);
	if (new_task != now_task) {
		farjmp(0, new_task->sel);
	}
	return;
}

void task_sleep(struct TASK *task) {
	struct TASK *now_task;
	if (task->flags == 2) {  // 指定任务处于唤醒状态：flags=2
		now_task = task_now();
		task_remove(task);  // 使得flags变为1
		if (task == now_task) {
			task_switchsub();
			now_task = task_now(); // 在设定后获取当前任务的值
			farjmp(0, now_task->sel);
		}
	}
	return;
}

struct TASK *task_now(void) {
	struct TASKLEVEL *t1 = &taskctl->level[taskctl->now_level];
	return t1->tasks[t1->now];
}

void task_add(struct TASK *task) {
	struct TASKLEVEL *t1 = &taskctl->level[task->level];
	t1->tasks[t1->runnings] = task;
	t1->runnings ++;
	task->flags = 2;
	return;
}

void task_remove(struct TASK *task) { 
	struct TASKLEVEL *t1 = &taskctl->level[task->level];
	
	int i;
	for (i = 0; i < t1->runnings; i++) {
		if (t1->tasks[i] == task) {
			break;
		}
	}
	t1->runnings --;
	if (i < t1->now) {
		t1->now --;
	}
	if (t1->now >= t1->runnings) {
		t1->now = 0;
	}
	task->flags = 1;
	
	for (; i < t1->runnings; i++) {
		t1->tasks[i] = t1->tasks[i + 1];
	}
	return;
}

void task_switchsub (void) {
	int i;
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		if (taskctl->level[i].runnings > 0) {
			break;
		}
	}
	taskctl->now_level = i;
	taskctl->lv_change = 0;
	return;
}