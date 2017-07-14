#include "osmain.h"

#define FLAGS_OVERRUN		0x0001

void fifo_init(struct FIFO *fifo, int size, int *buf){// FIFO缓冲区的初始化{
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size; // 空
	fifo->flags = 0;
	fifo->p = 0; // 写入位置
	fifo->q = 0; // 读取位置
	return;
}

/*像FIFO传送数据并保存*/
int fifo_put(struct FIFO *fifo, int data){
	if (fifo->free == 0) {
		/*无空余则溢出*/
		fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	fifo->buf[fifo->p] = data;
	fifo->p++;
	if (fifo->p == fifo->size) {
		fifo->p = 0;
	}
	fifo->free--;
	return 0;
}

int fifo_status(struct FIFO *fifo){
	return fifo->size - fifo->free;
}

/*从FIFO取得一个数据*/
int fifo_get(struct FIFO *fifo){
	int data;
	if (fifo->free == fifo->size) {
		/*如果缓冲区为空，则返回 -1*/
		return -1;
	}
	data = fifo->buf[fifo->q];
	fifo->q++;
	if (fifo->q == fifo->size) {
		fifo->q = 0;
	}
	fifo->free++;
	return data;
}

