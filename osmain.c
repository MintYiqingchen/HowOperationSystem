#include <stdio.h>
#include "osmain.h"

/* 全局变量 */
struct BOOTINFO *botinfo=(struct BOOTINFO*)BOOTINFO_ADR;
struct SHTCTL *shtctl; // 全局画布控制器
extern int keydata,mousedata;
struct FIFO fifo, keycmd;
int fifobuf[128], keycmd_buf[32];
#define KEYCMD_LED 	0xed
//keycmd_wait表示键盘向控制器发送数据的状态。
// ==-1：通常状态可以发送指令
//！=-1：键盘控制器正在等待发送的数据（保存在keycmd_wait）
//key_leds设置指示灯的状态，这个数据是要送到键盘控制器的
static char keytable0[0x80] = {
        0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
    };
static char keytable1[0x80] = {
        0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
    };

void console_task(struct SHEET *sheet){
  
    struct TIMER *timer; //闪烁定时器
    struct TASK *task = task_now();//当前运行中任务地址

    int i, fifobuf[128], cursor_x = 24, cursor_c = COL8_BLACK;
    fifo_init(&task->fifo, 128, fifobuf, task);//当前任务写到缓冲区
    char s[2];

    timer = timer_alloc();//闪烁定时器设置
    timer_init(timer, &task->fifo, 1);
    timer_set(timer, 50);

    putfonts_asc_sht(shtctl, sheet,8,28,COL8_WHITE,COL8_BLACK,"$:",2);

    for (;;) {
        io_cli();//禁止中断
        if (fifo_status(&task->fifo) == 0) {
            task_sleep(task);//缓冲区没有内容，让当前运行中的任务睡眠
            io_sti();//允许中断
        } else {
            i = fifo_get(&task->fifo);
            io_sti();
            if (i <= 1) { /* 光标闪烁定时器超时 */
                if (i != 0) {
                    timer_init(timer, &task->fifo, 0);  /* i=1;此时重新设置光标定时器，写入FIFO中改为0 */
                    cursor_c = COL8_WHITE;//黑色背景
                } else {
                    timer_init(timer, &task->fifo, 1);  /* i=1;此时重新设置光标定时器，写入FIFO中改为0 */
                    cursor_c = COL8_BLACK;//白色背景
                }
                timer_set(timer, 50);//50个count计数的时间、文本显示框
               
            }

            if(256<=i && i<= 511){
            	//键盘输入
            	if (i== 0x0e +256){//退格
            		if(cursor_x>24){
            			putfonts_asc_sht(shtctl,sheet,cursor_x,28,COL8_WHITE,COL8_BLACK," ",1);
            			cursor_x-=8;
            		}
            	}else{
            		if(cursor_x<260){
            			s[0]=i-256;
            			s[1]=0;
            			putfonts_asc_sht(shtctl,sheet,cursor_x,28,COL8_WHITE,COL8_BLACK,s,1);
            			cursor_x+=8;
            		}
            	}
            }

            boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
			sheet_refresh(shtctl ,sheet,cursor_x,28,cursor_x+8,44);
        }
    }
}

void HariMain(void){
	//各种系统的初始化
	io_cli();
	init_gdtidt();
	init_pic();
	io_sti();
	init_pit();

	//内存管理器初始化
	struct MEMMAN *memman = (struct MEMMAN*)MEMMAN_ADDR;
	memman_init(memman);
	memtest(memman);
	
	// FIFO初始化:同时作为鼠标、键盘、计时器的缓冲区，同时禁用任务切换
	struct FIFO fifo;
	int fifobuf[128];
	fifo_init(&fifo, 128, fifobuf, 0);

	//鼠标、键盘初始化
	struct MOUSE_DEC mdec;
	init_keyboard(&fifo, 256);//256为键盘代表码
	enable_mouse(&fifo, 512, &mdec);//512为鼠标代表码
	io_out8(PIC0_IMR, 0xf8); /* 开放PIC1和键盘中断(11111000)和计时器中断 */
	io_out8(PIC1_IMR, 0xef); /* 开放鼠标中断(11101111) */

	//桌面初始化
	char s[40];//字符显示的临时存储串
	shtctl=NULL;
	struct SHEET *sht_back=NULL, *sht_mouse=NULL,*sht_win=NULL, *sht_cons=NULL;
	unsigned char *buf_back, buf_mouse[256],*buf_win, *buf_cons;
	
	init_palette();
	while(shtctl==NULL){//循环直到分配成功
		shtctl = shtctl_init(memman, botinfo->vram, botinfo->scrnx, botinfo->scrny);
	}

	sht_back  = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	sht_win = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, botinfo->scrnx * botinfo->scrny);
	buf_win = (unsigned char*) memman_alloc_4k(memman,320*200);
	sheet_setbuf(sht_back, buf_back, botinfo->scrnx, botinfo->scrny, -1); //没有透明色
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);//透明色号99
	sheet_setbuf(sht_win,buf_win, 320 , 200, -1);
	init_screen(buf_back,botinfo->scrnx,botinfo->scrny);
	init_mouse_cursor(buf_mouse,99);
	putfonts_asc(buf_back,botinfo->scrnx,0,0,COL8_WHITE,"Hello");
	make_window8(sht_win->buf , 320 , 200,"task_a", 1);
	int textX=8,textY=28;
	make_textbox(sht_win , textX , textY , 300 , 160 , COL8_WHITE);//添加输入文本框

	// 多任务管理器设置
	struct TASK *task_a, *task_cons;
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 0);
	
	 /* 设置控制台窗口 */
    sht_cons = sheet_alloc(shtctl);//命令行窗口图层、缓冲区
    buf_cons = (unsigned char *) memman_alloc_4k(memman, 288 * 200);
    sheet_setbuf(sht_cons, buf_cons, 288, 200, -1);/* 设置缓冲区，将CMD窗口图层写到缓冲区中 */
    make_window8(buf_cons, 288, 200, "console", 0);//用CMD缓冲区的内容画窗口
    make_textbox(sht_cons, 8, 28, 268, 160, COL8_BLACK);//CMD图层下的文本框
    task_cons = task_alloc();//CMD窗口的任务、分配空间
    task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
    task_cons->tss.eip = (int) &console_task;  //任务执行的函数（内容）
    task_cons->tss.es = 3 << 3;
    task_cons->tss.cs = 4 << 3;
    task_cons->tss.ss = 3 << 3;
    task_cons->tss.ds = 3 << 3;
    task_cons->tss.fs = 3 << 3;
    task_cons->tss.gs = 3 << 3;
    *((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;//将CMD图层地址放到CMD任务块首部
    task_run(task_cons, 2, 3);/* 设置LEVEL和优先级level=2, priority=2 */

	//控制命令行的fifo初始化
	int key_to = 0, key_shift = 0, key_leds = (botinfo->leds >> 4) & 7, keycmd_wait = -1;
	fifo_init(&keycmd, 32, keycmd_buf, 0);
	fifo_put(&keycmd, KEYCMD_LED);
    fifo_put(&keycmd, key_leds);
	
	//输出内存管理信息的函数
	int temp,posi=32;
	for(temp=0 ; temp < memman->frees;++temp){
		char c[20];
		sprintf(c,"%X",memman->free[temp].addr);
		putfonts_asc(buf_back,botinfo->scrnx,0,posi,COL8_WHITE,c);
		sprintf(c,"%X",memman->free[temp].size);
		putfonts_asc(buf_back,botinfo->scrnx,150,posi,COL8_WHITE,c);
		posi=posi+16;
	}

	sheet_slide(shtctl, sht_back, 0, 0);//还没画
	int mx = (botinfo->scrnx - 16) / 2; //按显示在画面中央来计算坐标
	int my = (botinfo->scrny - 28 - 16) / 2;
	
    sheet_slide(shtctl, sht_back,  0,  0);//图层开始显示的位置
    sheet_slide(shtctl, sht_cons, mx,  my);
    sheet_slide(shtctl, sht_win,  64, 56);
    sheet_slide(shtctl, sht_mouse, mx, my);
    sheet_updown(shtctl, sht_back,  0);//设置图层的高度
    sheet_updown(shtctl, sht_cons,  1);
    sheet_updown(shtctl, sht_win,   2);
    sheet_updown(shtctl, sht_mouse, 3);
	
	//假装有一个计时器
	//计时器初始化,用于任务的切换
	struct TIMER *timer;
	timer = timer_alloc();
	timer_init(timer, &fifo, 1);
	timer_set(timer, 50);

	//暂时记录光标位置
	int cursorX=textX+2;
	unsigned char cursor_c=COL8_DRKBLU;
	
	while(1){
		if (fifo_status(&keycmd) > 0 && keycmd_wait < 0) {
            /* 如果存在想键盘控制器发送的数据，则发送它 */
            keycmd_wait = fifo_get(&keycmd);//从KEYCMD缓冲区中获得要向键盘发送的数据
            wait_KBC_sendready();//等待ACK；等待键盘准备好
            io_out8(PORT_KEYDAT, keycmd_wait);//将keycmd_wait的值送到键盘控制器
        }
		io_cli();//记得在处理之前先关闭中断

		if (fifo_status(&fifo) <= 0) {
			task_sleep(task_a);
			io_sti();
		}
		else{
			int i = fifo_get(&fifo);
			io_sti();
			if (i <= 511 && i >= 256) {				
				sprintf(s, "%02X", i - 256);
                putfonts_asc_sht(shtctl, sht_back, 0, 16, COL8_WHITE, COL8_DRKPUREBLUE, s, 2);
                if (i < 0x80 + 256) { /* 按照键盘的编码进行转换 */
                    if (key_shift == 0) {
                        s[0] = keytable0[i - 256];
                    } else {
                        s[0] = keytable1[i - 256];
                    }
                } else {
                    s[0] = 0;
                }
				if ('A' <= s[0] && s[0] <= 'Z') {    /* 当输入字符为英文字母时 */
					if (((key_leds & 4) == 0 && key_shift == 0) ||  
							((key_leds & 4) != 0 && key_shift != 0)) {
						s[0] += 0x20;    /* 将大写字母转换成小写字母 */
					}
				}
                if (s[0] != 0) { /* 一般字符 */
                    if (key_to == 0) {    /* 想任务A中写 */
                        if (cursorX < 260) {
                            /* 光标后移 */
                            s[1] = 0;
                            putfonts_asc_sht(shtctl, sht_win, cursorX, 28, COL8_BLACK, COL8_WHITE, s, 1);
                            cursorX += 8;
                        }
                    } else {    /* 向CMD窗口中写 */
                        fifo_put(&task_cons->fifo, s[0] + 256);
                    }
				}
				if( 0x0e==i-keydata){
					if (key_to == 0) {    /* 发送信息到A任务 */
                        if (cursorX>=textX+10 ) {
                            /* 闪烁光标向左移动8个像素 */
                            putfonts_asc_sht(shtctl, sht_win, cursorX, 28, COL8_BLACK, COL8_WHITE, " ", 1);
                            cursorX -= 8;
                        }
                    } else {    /* 发送到任务CMD窗口 */
                        fifo_put(&task_cons->fifo, 0x0e + 256);
                    }
				}
				if (i == 256 + 0x0f) {  /* 键盘中断的键值是TAB键 */
					if (key_to == 0) {  //这里的原理和光标闪烁是一样的
						key_to = 1; //发送到CMD窗口
						make_wtitle8(buf_win,  sht_win->bxsize,  "task_a",  0);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
					} else {
						key_to = 0; //发送到任务A
						make_wtitle8(buf_win,  sht_win->bxsize,  "task_a",  1);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
					} //接着刷新CMD和窗口图层
					sheet_refresh(shtctl, sht_win,  0, 0, sht_win->bxsize,  21);
					sheet_refresh(shtctl, sht_cons, 0, 0, sht_cons->bxsize, 21);
				}
				if (i == 256 + 0x2a) {    /* 左shift ON */
                    key_shift |= 1;
                }
                if (i == 256 + 0x36) {    /* 右shift ON */
                    key_shift |= 2;
                }
                if (i == 256 + 0xaa) {    /* 左shift OFF */
                    key_shift &= ~1;
                }
                if (i == 256 + 0xb6) {    /* 右shift OFF */
                    key_shift &= ~2;
                }
				if (i == 256 + 0x3a) { /* CapsLock */
                    key_leds ^= 4; //设置CapsLock指示灯亮（ley_leds的第2位为1）
                    fifo_put(&keycmd, KEYCMD_LED);//先送入默认的KEYCMD_LED是为了下一次获得ACK（0xfa)
                    fifo_put(&keycmd, key_leds);  //接着再送入设置的指示灯状态信息
                }
                if (i == 256 + 0x45) {    /* NumLock */
                    key_leds ^= 2;    //设置NumLock 指示灯亮（ley_leds的第1位为1）
                    fifo_put(&keycmd, KEYCMD_LED);//同上
                    fifo_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x46) {    /* ScrollLock */
                    key_leds ^= 1;    //设置ScrollLock 指示灯亮（ley_leds的第0位为1）
                    fifo_put(&keycmd, KEYCMD_LED);//同上
                    fifo_put(&keycmd, key_leds);
                }
                if (i == 256 + 0xfa) {    /* 键盘成功接收数据返回一个ACK（0xfa） */
                    keycmd_wait = -1; //这是又可以继续向键盘控制器发送数据了
                }
                if (i == 256 + 0xfe) {    /* 键盘接收数据失败返回ACK（0xfe) */
                    wait_KBC_sendready();//等着（0xfa)键盘准备好
                    io_out8(PORT_KEYDAT, keycmd_wait);//重新把keycmd_wait的值发送到键盘控制器
                }
				//重新显示光标
				boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursorX, textY, cursorX + 7, textY+15);
				sheet_refresh(shtctl ,sht_win,cursorX,textY,cursorX+8,textY+16);
			}else if (i>=512 && i<=767) {
				if(mouse_decode(&mdec, i-mousedata) != 0){
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) {
						s[1] = 'L';
					}
					if ((mdec.btn & 0x02) != 0) {
						s[3] = 'R';
					}
					if ((mdec.btn & 0x04) != 0) {
						s[2] = 'C';
					}

					putfonts_asc_sht(shtctl, sht_back, 32, 16, COL8_WHITE, COL8_DRKPUREBLUE, s, 15);
					
					/* 鼠标移动 */
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					} 
					if (mx > botinfo->scrnx + 15) {
						mx = botinfo->scrnx + 15;
					}
					if (my > botinfo->scrny + 16) {
						my = botinfo->scrny + 16;
					}
					// 再次显示鼠标
					sprintf(s, "(%3d, %3d)", mx, my);
					putfonts_asc_sht(shtctl, sht_back, 0, 0, COL8_WHITE, COL8_DRKPUREBLUE, s, 10);
					sheet_slide(shtctl,sht_mouse, mx, my);//移动鼠标画布
					if((mdec.btn & 0x01) != 0){
						sheet_slide(shtctl,sht_win,mx-80,my-8);
					}
				}
			}
			else if (i <= 1){
				if(i==1){
					timer_init(timer,&fifo,0);
					cursor_c=COL8_DRKBLU;
				}else{
					timer_init(timer,&fifo,1);
					cursor_c=COL8_WHITE;
				}
				timer_set(timer,50);
				boxfill8(sht_win->buf,sht_win->bxsize,cursor_c,cursorX,textY,cursorX+8,textY+15);
				sheet_refresh(shtctl,sht_win,cursorX,textY,cursorX+8,textY+15);
			}
		}
	}
}
