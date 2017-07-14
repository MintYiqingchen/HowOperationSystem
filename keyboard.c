#include "osmain.h"

struct FIFO *keyfifo;
int keydata;
//按键码表
/*static char keytable[0x54] = {
        0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.'
};*/
void wait_KBC_sendready(void)
{
	/*等待键盘控制电路准备完毕*/
	for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}
void init_keyboard(struct FIFO *fifo, int data)
{
	// 将FIFO缓冲区的信息保存到全局变量里
	keyfifo = fifo;
	keydata = data;
	/*初始化键盘控制电路*/
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

#define PORT_KEYDAT 0x60
void int21handle(int *esp)
/* 来自PS/2键盘的中断 */
{   
    int data;
    io_out8(PIC0_OCW2,0x61);
    data=io_in8(PORT_KEYDAT);//读取端口输入的按键编码
    fifo_put(keyfifo, data + keydata);
    return;
}
