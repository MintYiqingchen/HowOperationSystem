#include "osmain.h"
#include <stdio.h>

#define SHEET_USE	1
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize)
{												//图层控制变量的初始化
	struct SHTCTL *ctl=NULL;	
	int i;
	//分配图层管理空间
	unsigned int addr = memman_alloc_4k(memman, sizeof (struct SHTCTL));
	if (addr == -1) {		//分配空间失败,返回空指针
		return ctl;
	}
	ctl=(struct SHTCTL*)addr;
	ctl->vram = vram;	//给控制变量赋值
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1; //表示没有图层 

	addr =  memman_alloc_4k(memman, xsize*ysize);
	if(addr==-1){
		memman_free_4k(memman , (int) ctl, sizeof(struct SHTCTL));//归还已分配内存
		return NULL;//分配失败返回空指针
	}
	ctl->map=(char*)addr;

	for (i = 0; i < MAX_SHEETS; i++) {
		ctl->sheets0[i].flags = 0; //标志为未使用
		//ctl->sheets0[i].ctl=ctl;
	}
	return ctl;//返回图层管理器
}

struct SHEET *sheet_alloc(struct SHTCTL *ctl)
{						//用于取得新生成的未使用的图层
	struct SHEET *sht;
	int i;
	for (i = 0; i < MAX_SHEETS; i++) {
		if (ctl->sheets0[i].flags == 0) {	//找到第一个未使用的图层
			sht = &(ctl->sheets0[i]);		//返回第一个未使用的图层的地址
			sht->flags = SHEET_USE; 	//要分配，标记为正在使用
			sht->height = -1; 			//隐藏，此时高度还没有设置
			return sht;					//分配成功
		}
	}
	return 0;	//没有空闲图层，分配失败
}
//设定图层缓冲区大小和透明色的函数
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
	sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	sht->col_inv = col_inv;
	return;
}
//设定底板高度的函数
void sheet_updown(struct SHTCTL *ctl,struct SHEET *sht, int height)
{
	int h, old = sht->height; //存储设置前的高度信息
	//struct SHTCTL *ctl=sht->ctl;
	//如果指定的高度过高或过低，则进行修正
	if (height > ctl->top + 1) {
		height = ctl->top + 1;
	}
	if (height < -1) {
		height = -1;
	}
	sht->height = height; //设定高度

	//有了新的高度，对sheet[]重新排序
	if (old > height) {	//比以前低
		if (height >= 0) {
			//把中间的往上提
			for (h = old; h > height; h--) {
				ctl->sheets[h] = ctl->sheets[h - 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
			sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 +sht->bxsize,sht->vy0+sht->bysize,height+1);
			sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height+1,old);//按新图层信息重新绘制画面
		} else {	//隐藏图层高度为-1
			if (ctl->top > old) {
				//把上面的降下来
				for (h = old; h < ctl->top; h++) {
					ctl->sheets[h] = ctl->sheets[h + 1];
					ctl->sheets[h]->height = h;
				}
			}
			ctl->top--; //现实中的图层少了一个，最上面的图层高度下降
			sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 +sht->bxsize,sht->vy0+sht->bysize,0);
			sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0,old-1);//按新图层信息重新绘制画面
		}
	} else if (old < height) {	//图层更新后比以前高
		if (old >= 0) {
			//把中间的拉下去
			for (h = old; h < height; h++) {
				ctl->sheets[h] = ctl->sheets[h + 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		} else {	//由隐藏状态转为显示状态
			//将已经在上面的提上来
			for (h = ctl->top; h >= height; h--) {
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1]->height = h + 1;
			}
			ctl->sheets[height] = sht;
			ctl->top++; //由于已显示的图层增加了一个，所以最上面的图层高度增加
		}
		sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 +sht->bxsize,sht->vy0+sht->bysize,height);
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height,height);//按新图层信息重新绘制画面
	}
	return;
}

//图层刷新函数。
//刷新原理：对于已经设定的高度的所有图层，从下往上，将透明以外的所有像素都复制到VRAM中，由于是从下往上复制，所以最后最上面的内容就留在了画面上。
void sheet_refresh(struct SHTCTL *ctl,struct SHEET *sht, int bx0, int by0, int bx1, int by1)
{
    if (sht->height >= 0) { /* 如果正在显示，则按图层信息刷新 */
        sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height,sht->height);
    }
    return;
}

//功能：不改变图层的高度，只上下左右移动图层（滑动）
void sheet_slide(struct SHTCTL *ctl,struct SHEET *sht, int vx0, int vy0)
{
	int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0) {         //如果正在显示，则按新图层的信息刷新画面
    	sheet_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
        sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0,sht->height-1);
        //露出下方图层
        sheet_refreshmap(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
		sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height,sht->height);
		//显示当前图层
    }
    return;
}

//释放已经使用图层的内存的函数
void sheet_free(struct SHTCTL *ctl,struct SHEET *sht)
{
    if (sht->height >= 0) {
        sheet_updown(ctl,sht, -1);/* 如果该图层正在显示，设置height=-1;隐藏 */
    }
    sht->flags = 0;             /* FLAG=0表示该图层未使用 */
    return;
}

void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1,int h0,int h1){
    int h, bx, by, vx, vy, bx0, by0, bx1, by1,sid;
    unsigned char *buf, c, *vram = ctl->vram,*map=ctl->map;
    struct SHEET *sht;
    //超出范围则修正
    if(vx0<0){ vx0=0; }
    if(vy0<0){ vy0=0; }
    if(vx1 > ctl->xsize ){ vx1=ctl->xsize; }
    if(vy1 > ctl->ysize ){ vy1=ctl->ysize; }

    for (h = h0; h <= h1; h++) {
        sht = ctl->sheets[h];
        buf = sht->buf;
        sid = sht - ctl->sheets0;
        /* 使用vx0--vy1,对bx0--by1进行倒推得到图层中需要刷新的范围
            vx = sht->vx0 ; --> bx = vx - sht->vx0;  */
        bx0 = vx0 - sht->vx0;//倒推得到需要刷新的范围（bx0,by0）(bx1,by1)
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        if (bx0 < 0) { bx0 = 0; } //第一种情况：刷新范围的一部分被其他图层遮盖；用于处理刷新范围在图层外侧的情况
        if (by0 < 0) { by0 = 0; }
        if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }//第二种情况：需要刷新的坐标超出了图层的范围
        if (by1 > sht->bysize) { by1 = sht->bysize; }
         //改良后，这段循环不再对整个图层进行刷新，只刷新需要的部分
        for (by = by0; by < by1; by++) {
            vy = sht->vy0 + by;
            for (bx = bx0; bx < bx1; bx++) {
                vx = sht->vx0 + bx;
                c = buf[by * sht->bxsize + bx];  //获得该像素出缓冲区的内容
                if (map[vy *ctl->xsize+vx] == sid) {
                    vram[vy * ctl->xsize + vx] = c; //将该像素出缓冲区的内容给VRAM
                }
            }
        }
    }
    return;
}

void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0){
	int h,bx,by,vx,vy,bx0,by0,bx1,by1;
	unsigned char *buf,sid,*map=ctl->map;
	struct SHEET *sht;

	if(vx0<0){ vx0=0; }
	if(vy0<0){ vy0=0; }
	if(vx1>ctl->xsize){ vx1=ctl->xsize; }
	if(vy1>ctl->ysize){ vy1=ctl->ysize; }

	for(h=h0;h<=ctl->top;h++){
		sht = ctl->sheets[h];
		sid = sht - ctl->sheets0;//图层号码,代表一个屏幕里应该显示哪一个图层
		buf = sht->buf;
		//计算该图层更新范围
		bx0 = vx0 -sht->vx0;
		by0 = vy0 -sht->vy0;
		bx1 =vx1 -sht->vx0;
		by1=vy1-sht->vy0;
		if(bx0<0){ bx0=0; }
		if(by0<0){ by0=0; }
		if(bx1>sht->bxsize){ bx1=sht->bxsize; }
		if(by1>sht->bysize){ by1=sht->bysize; }

		for(by=by0;by<by1;by++){
			vy=sht->vy0+by;
			for(bx=bx0;bx<bx1;++bx){
				vx=sht->vx0+bx;
				if(buf[by* sht->bxsize +bx]!= sht->col_inv)
					map[vy* ctl->xsize +vx]=sid;
			}
		}
	}
	return ;
}