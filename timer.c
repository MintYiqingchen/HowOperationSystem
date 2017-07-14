//#define TIMER_FLAG_ALLOC 1 //定时器已配置
//#define TIMER_FLAG_USING 2 //定时器正在运行
#include "osmain.h"
#include <stdio.h>

#define PIT_CTRL 0X43
#define PIT_CNT0 0x40

struct TIMECTL timerctl;
void init_pit(void){
	struct TIMER *t;
	//设置计数器0的工作方式2
	io_out8(PIT_CTRL,0x34);
	//设置保持寄存器的值，约等于1秒10次中断
	io_out8(PIT_CNT0,0x9c);//低八位
	io_out8(PIT_CNT0,0x2d);//高八位
	int i=0;
	for(;i<MAX_TIMER;++i){
		timerctl.timer[i].flags=0;//全部标记为未使用状态
	}
	t = timer_alloc(); // 取得一个
	t->timeout = 0xffffffff;
	t->flags = TIMER_FLAG_USING;
	t->next_timer = 0; 
	timerctl.count=0;
	timerctl.t0 = t;
	timerctl.next_time=0xffffffff;//没有正在运行的定时器
	return ;
}

struct TIMER* timer_alloc(void){//用计时器管理器分配一个计时器
	int i=0;
	for(;i<MAX_TIMER;++i){
		if(timerctl.timer[i].flags==0){
			timerctl.timer[i].flags=TIMER_FLAG_ALLOC;
			return &timerctl.timer[i];
		}
	}
	return NULL;//没找到
}

void timer_free(struct TIMER* timer){
	timer->flags=0;
	return ;
}

void timer_init(struct TIMER* timer,struct FIFO* fifo, int data){
	timer->fifo=fifo;
	timer->data=data;
	return;
}

void timer_set(struct TIMER* timer,unsigned int timeout){//设置了超时后，计时器开始正式工作
	int e;
	e=io_load_eflags();
	struct TIMER *t, *s; 
	io_cli();
	timer->timeout=timeout+timerctl.count;
	timer->flags=TIMER_FLAG_USING;
	
	t = timerctl.t0;
	if (timer->timeout <= t->timeout) {
		// 插入最前面
		timerctl.t0 = timer;
		timer->next_timer = t; 
		timerctl.next_time = timer->timeout;
		io_store_eflags(e);
		return;
	}
	// 搜寻插入位置
	while (1) {
		s = t;
		t = t->next_timer;
		if (t == 0) {
			io_store_eflags(e);
			break; 
		}
		if (timer->timeout <= t->timeout) {
			// 插入s和t之间
			s->next_timer = timer; // s 的下一个是timer
			timer->next_timer = t; // timer的下一个是t
			io_store_eflags(e);
			return;
		}
	}
	return;
}
extern struct Timer* tasktm;
void int20handle(int *esp){
	struct TIMER *timer;
    io_out8(PIC0_OCW2,0x60);
    timerctl.count++;
    if(timerctl.next_time > timerctl.count){//先比较下一个超时有没有到来
        return;
    }
    timerctl.next_time=0xffffffff;
    
    int tc=0;
	timer = timerctl.t0;// 指向最近的计时器
    for(;;){
        if(timer->timeout<=timerctl.count){
			// 超时
		    timer->flags=TIMER_FLAG_ALLOC;
		    if(timer==tasktm){
		    	tc=1;
		    }else
            	fifo_put(timer->fifo,timer->data);

			timer = timer->next_timer;
        }else{
            break;
        }
    }
	timerctl.t0 = timer;
	timerctl.next_time = timer->timeout;
	
	// 中断处理完成后再进行任务切换
	if(tc==1){
		task_switch();
	}
    return ;
}