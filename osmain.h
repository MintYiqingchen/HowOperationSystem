//颜色定义
#define COL8_BLACK      0
#define COL8_LITRED     1
#define COL8_LITGRE     2
#define COL8_LITYEL     3
#define COL8_LITBLU     4
#define COL8_LITPUR     5
#define COL8_PURBLU     6
#define COL8_WHITE      7
#define COL8_LITGRY     8
#define COL8_DRKRED     9
#define COL8_DRKGRE     10
#define COL8_DRKYEL     11
#define COL8_DRKBLU     12
#define COL8_DRKPUR     13
#define COL8_DRKPUREBLUE     14
#define COL8_DRKGRY     15

//寻址部分
#define BOOTINFO_ADR 0X1701
#define GDT_ADR 0x270000
#define IDT_ADT 0X26F800
#define AR_INTGATE32 0x008e
#define LIMIT_GDT 0x0000ffff
#define LIMIT_IDT 0x000007ff
#define LIMIT_BOTPAK 0x0007ffff
#define AR_DATA32_RW 0x4092
#define AR_CODE32_ER 0x409e
#define ADR_BOTPAK 0x00280000

//描述符属性
#define AR_TSS 0x0089	//任务状态段描述符

//鼠标键盘定义部分
#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47
#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

struct BOOTINFO {
	/*howos.asm中开头保存的信息*/
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
};

struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;  // x, y, btn分别用与存放移动信息和鼠标按键状态
};

//naskfunc.nsk
extern void io_hlt(void);
extern void write_mem8(int addr,int data);
extern int io_load_eflags(void);
extern int io_in8(int);
extern void io_cli();
extern void io_sti();
extern void io_stihlt();
extern void io_store_eflags(int);
extern void io_out8(int,int);
extern void io_delay();
extern void asm_int21handle(void);
extern void asm_int2chandle(void);
extern void asm_int20handle(void);
extern void load_tr(int tr);
extern void farjmp(int offset,short selector);
extern void taskswitch6(void);


//dtsys.c
// IDT和GDT的初始化
struct SEGMENT_DESCRIPTOR { /* 存放GDT的8字节内容 */
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};

struct GATE_DESCRIPTOR {   /* 存放IDT的8字节内容 */
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};

void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
void load_gdt(int limit, int addr); /* 在func.asm中 */
void load_idt(int limit, int addr); /* 在func.asm中 */

/* int.c 中断处理*/
void init_pic(void);
void int21handle(int *esp);
void int27handle(int *esp);
void int2chandle(int *esp);
void int20handle(int *esp);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

struct FIFO {
	int *buf;
	int p, q, size, free, flags;
	struct TASK *task;
};
int fifo_status(struct FIFO* fifo);
int fifo_get(struct FIFO *fifo);
int fifo_put(struct FIFO *fifo, int data);
void fifo_init(struct FIFO *fifo, int size, int *buf, struct TASK *task);

//mouse.c
void enable_mouse(struct FIFO *fifo, int data, struct MOUSE_DEC* mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

//keyboard.c
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO *fifo, int data);

//memory.c 内存管理
#define MemChkBuf 0x1820
#define dwMCRNumber 0x1800
#define MEMMAN_FREES 4096
#define MEMMAN_ADDR 0x3c0000 //内存管理表起始地址

struct AddrRangeDesc{
	unsigned int  baseAddrLow ;  //内存基地址的低32位
    unsigned int  baseAddrHigh;  //内存基地址的高32位
    unsigned int  lengthLow;     //内存块长度的低32位
    unsigned int  lengthHigh;    //内存块长度的高32位
    unsigned int  type;          //描述内存块的类型
};

struct FREEINFO{
	unsigned int addr,size;
};
struct MEMMAN{//memory manager
	int frees,maxfrees,lostsize,losts;
	//losts记录丢弃的内存碎片数量
	struct FREEINFO free[1000]; 
};
void memtest(struct MEMMAN* man);
void memman_init(struct MEMMAN* man);
//返回内存可用量
unsigned int memman_total(struct MEMMAN* man);
//内存分配和释放
int memman_alloc(struct MEMMAN*man,unsigned size);
int memman_free(struct MEMMAN* man,unsigned int addr,unsigned int size);
int memman_alloc_4k(struct MEMMAN* man,unsigned int size);
int memman_free_4k(struct MEMMAN*man,unsigned int addr,unsigned int size);

//sheet.c
#define MAX_SHEETS		256
struct SHEET {//32byte
	unsigned char *buf;//图层缓冲
	int bxsize, bysize, vx0, vy0, col_inv, height, flags;
	//图层大小，坐标，透明色，高度，标志
	//struct SHTCTL *ctl;
};
struct SHTCTL {//9232byte
	unsigned char *vram,*map;
	int xsize, ysize, top;
	struct SHEET *sheets[MAX_SHEETS];//按高度排序
	struct SHEET sheets0[MAX_SHEETS];
};

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHTCTL *ctl,struct SHEET *sht, int height);
void sheet_refresh(struct SHTCTL *ctl,struct SHEET *sht, int bx0, int by0, int bx1, int by1);
//只刷新h0以上的图层
void sheet_slide(struct SHTCTL *ctl,struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHTCTL *ctl,struct SHEET *sht);
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1,int h0,int h1);
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1,int h0);
void putfonts_asc_sht(struct SHTCTL *shtctl, struct SHEET *sht, int x, int y, int color, int background, char *s, int len);

//timer.c
#define MAX_TIMER 500
#define TIMER_FLAG_ALLOC 1 //定时器已配置
#define TIMER_FLAG_USING 2 //定时器正在运行
struct TIMER{
	struct TIMER *next_timer;//下一个超时计时器的地址
	unsigned int timeout,flags;
	struct FIFO *fifo;
	int data;
};

struct TIMECTL{
	//中断计数器，下一个超时时刻，正在使用的计时器个数
	unsigned int count,next_time;
	//,used
	struct TIMER timer[MAX_TIMER];
	struct TIMER *t0;//下一个超时的计时器
};
struct TIMER* timer_alloc(void);
void timer_free(struct TIMER* timer);
void timer_init(struct TIMER* timer,struct FIFO* fifo, int data);
void timer_set(struct TIMER* timer,unsigned int timeout);
void init_pit(void);

//graphic.c
void init_palette(void);
void set_palette(int start,int end,unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen(char *vram, int x, int y);
// 显示字符
void put_font(char *vram, int xsize, int x, int y, char c, char *font);
// 显示字符串（ASCII）
void putfonts_asc(char *vram, int xsize, int x, int y, char c, unsigned char * s);
void printMem(char *vram, int start,int end);
// 显示鼠标指针
void init_mouse_cursor(char * mouse, char bc);
void put_block(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);
/*参数vram和vxsize是关于VRAM的信息； pxsize和pysize是显示的图形的大小； px0和py0指定图形在画面的显示位置；
buf指定图形的存放地址； pxsize指图形每一行所含的像素数。*/
void make_window8(unsigned char *buf,int xsize,int ysize,char *title, char act);
void make_textbox(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
//起始坐标，大小，背景色

/* mtask.c 进程切换*/
#define MAX_TASKS   	1000  // 最大任务数量
#define TASK_GDT0		5
#define MAX_TASKS_LEVEL 	100
#define MAX_TASKLEVELS	10

//任务切换
struct TSS32{
	int backlink,esp0,ss0,esp1,ss1,esp2,ss2,cr3;//不会被CPU自动写入的部分
	int eip,eflags,eax,ecx,edx,ebx,esp,ebp,esi,edi;
	int es,cs,ss,ds,fs,gs;
	int ldtr,iomap;//不会被CPU自动写入的部分
};

struct TASK{
	int sel, flags; /* sel表示GDT的编号 */
    int level, priority;
    struct FIFO fifo;//任务的缓冲区
    struct TSS32 tss;//任务段的内容
};

struct TASKLEVEL{
	int runnings; 	// 正在运行的任务数量 
	int now;		// 当前正在运行的任务
	struct TASK *tasks[MAX_TASKS_LEVEL];
};

struct TASKCTL{
	int now_level; // 当前正在活动的LEVEL
	char lv_change;  // 在下次任务切换时是否需要改变LEVEL
	struct TASKLEVEL level[MAX_TASKLEVELS];
	struct TASK task[MAX_TASKS];
};

void tasktm_init();
struct TASK * task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
struct TASK *task_now(void);
void task_add(struct TASK *task);
void task_remove(struct TASK *task);
void task_switchsub (void);