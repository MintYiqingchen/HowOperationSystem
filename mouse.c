#include "osmain.h"

struct FIFO *mousefifo;
int mousedata;

void enable_mouse(struct FIFO *fifo, int data, struct MOUSE_DEC* mdec)
{
	mousefifo = fifo;
	mousedata = data;
	/*激活鼠标*/
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);/*键盘控制其会返回ACK*/
	mdec->phase = 0; // 等待0xfa的阶段
	return; 
}

#define PORT_KEYDAT 0x60
void int2chandle(int *esp)
/* 来自PS/2鼠标的中断 */
{
    int data;
    io_out8(PIC1_OCW2, 0x64);   /*通知PIC1 IRQ-12的受理已经完成*/
    io_out8(PIC0_OCW2, 0x62);   /*通知PIC0 IRQ-12的受理已经完成*/
    data = io_in8(PORT_KEYDAT);
    fifo_put(mousefifo, data + mousedata);
    return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat){
	if (mdec->phase == 0) {
	/*等待鼠标的0xfa的状态*/
		if (dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	} 
	if (mdec->phase == 1) {
	/*等待鼠标的第一字节*/
		if ((dat & 0xc8) == 0x08) {
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if (mdec->phase == 2) {
	/*等待鼠标的第二字节*/
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3) {
	/*等待鼠标的第三字节*/
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;   // 与二进制0000 0111相与取出低3位
		mdec->x = mdec->buf[1];
		
		mdec->y = mdec->buf[2];
		if((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}
		mdec->y = -mdec->y; // 鼠标的y方向与画面符号相反
		return 1;
	}
	return -1;
}