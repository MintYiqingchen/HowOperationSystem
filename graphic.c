#include "osmain.h"
void set_palette(int start,int end,unsigned char *rgb){
	int i,eflags;
	eflags = io_load_eflags();  /* 记录中断许可标志的值 */
    io_cli();                   /* 将中断许可标志置为0，禁止中断 */
    io_out8(0x03c8, start);
    for (i = start; i <= end; i++) {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags);    /* 复原中断许可标志 */
    return;
}

void init_palette(void){
	static unsigned char table_rgb[16*3]={
		0x00, 0x00, 0x00,   /*  0:黑 */
        0xff, 0x00, 0x00,   /*  1:亮红 */
        0x00, 0xff, 0x00,   /*  2:亮绿 */
        0xff, 0xff, 0x00,   /*  3:亮黄 */
        0x00, 0x00, 0xff,   /*  4:亮蓝 */
        0xff, 0x00, 0xff,   /*  5:亮紫 */
        0x00, 0xff, 0xff,   /*  6:浅亮蓝 */
        0xff, 0xff, 0xff,   /*  7:白 */
        0xc6, 0xc6, 0xc6,   /*  8:亮灰 */
        0x84, 0x00, 0x00,   /*  9:暗红 */
        0x00, 0x84, 0x00,   /* 10:暗绿 */
        0x84, 0x84, 0x00,   /* 11:暗黄 */
        0x00, 0x00, 0x84,   /* 12:暗蓝 */
        0x84, 0x00, 0x84,   /* 13:暗紫 */
        0x00, 0x84, 0x84,   /* 14:浅暗蓝 */
        0x84, 0x84, 0x84    /* 15:暗灰 */
	};
	set_palette(0,15,table_rgb);
	return ;
}
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1){
	int x, y;
	for (y = y0; y <= y1; ++y) {
		for (x = x0; x <= x1; ++x)
			vram[y * xsize + x] = c;
	}
	return;
}
void init_screen(char *vram, int x, int y){ /* 显示画面背景 */
	boxfill8(vram, x, COL8_DRKPUREBLUE,0,0,x-1,y-29);
	boxfill8(vram, x, COL8_LITGRY,  0, y - 28, x -  1, y - 28);
	boxfill8(vram, x, COL8_WHITE,  0,     y - 27, x -  1, y - 27);
	boxfill8(vram, x, COL8_LITGRY,  0,     y - 26, x -  1, y -  1);

	boxfill8(vram, x, COL8_WHITE,  3,     y - 24, 59,     y - 24);
	boxfill8(vram, x, COL8_WHITE,  2,     y - 24,  2,     y -  4);
	boxfill8(vram, x, COL8_DRKGRY,  3,     y -  4, 59,     y -  4);
	boxfill8(vram, x, COL8_DRKGRY, 59,     y - 23, 59,     y -  5);
	boxfill8(vram, x, COL8_BLACK,  2,     y -  3, 59,     y -  3);
	boxfill8(vram, x, COL8_BLACK, 60,     y - 24, 60,     y -  3);

	boxfill8(vram, x, COL8_DRKGRY, x - 47, y - 24, x -  4, y - 24);
	boxfill8(vram, x, COL8_DRKGRY, x - 47, y - 23, x - 47, y -  4);
	boxfill8(vram, x, COL8_WHITE, x - 47, y -  3, x -  4, y -  3);
	boxfill8(vram, x, COL8_WHITE, x -  3, y - 24, x -  3, y -  3);
	return;
}
void put_font(char *vram, int xsize, int x, int y, char c, char *font){
	int i;
	char *p, d ;
	for (i = 0; i < 16; ++i) {
		p = vram + (y + i) * xsize + x;
		d = font[i];
		if ((d & 0x80) != 0) { p[0] = c; }
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}
	return;
}
void putfonts_asc(char *vram, int xsize, int x, int y, char c, unsigned char * s) {
	extern const char font[4096]; //导入外部字体
	for (; *s != 0x00; ++s) {
		put_font(vram, xsize, x, y, c, font+(*s)*16);
		x += 8;
	}
	return;
}
void init_mouse_cursor(char * mouse, char bc) {
	/* bc为背景色 */
	static char cursor[16][16] = {
		"*...............",
		"**..............",
		"*O*.............",
		"*OO*............",
		"*OOO*...........",
		"*OOOO*..........",
		"*OOOOO*.........",
		"*OOOOOO*........",
		"*OOOOOOO*.......",
		"*OOOO*****......",
		"*OO*O*..........",
		"*O*.*O*.........",
		"**..*O*.........",
		"*....*O*........",
		".....*O*........",
		"......*........."
	};
	int x, y;

	for (y = 0; y < 16; y++) {
		for (x = 0; x < 16; x++) {
			if (cursor[y][x] == '*') {
				mouse[y * 16 + x] = COL8_BLACK;
			}
			if (cursor[y][x] == 'O') {
				mouse[y * 16 + x] = COL8_WHITE;
			}
			if (cursor[y][x] == '.') {
				mouse[y * 16 + x] = bc;
			}
		}
	}
	return;
}
void put_block(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize){
	/*
	参数vram和vxsize是关于VRAM的信息； pxsize和pysize是显示的图形的大小； px0和py0指定图形在画面的显示位置；
	buf指定图形的存放地址； pxsize指图形每一行所含的像素数。
	*/
	int x, y;
	for (y = 0; y < pysize; y++) {
		for (x = 0; x < pxsize; x++) {
			vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
		}
	}
	return;
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act)
{   //这一部分是描绘窗口输入文本框的代码
    //第一步：将设置的文本框的信息写到缓存中
    boxfill8(buf, xsize, COL8_LITGRY, 0,         0,         xsize - 1, 0        );
    boxfill8(buf, xsize, COL8_WHITE, 1,         1,         xsize - 2, 1        );
    boxfill8(buf, xsize, COL8_LITGRY, 0,         0,         0,         ysize - 1);
    boxfill8(buf, xsize, COL8_WHITE, 1,         1,         1,         ysize - 2);
    boxfill8(buf, xsize, COL8_DRKGRY, xsize - 2, 1,         xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_BLACK, xsize - 1, 0,         xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_LITGRY, 2,         2,         xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_DRKGRY, 1,         ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_BLACK, 0,         ysize - 1, xsize - 1, ysize - 1);
    //第二步：调用这个函数，将缓存中的内容写到VRAM中
    make_wtitle8(buf, xsize, title, act);
    return;
}
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act){   //这里是描绘窗口标题栏的代码，这里没有什么可讲的。
    //就是把两部分代码区分开，便于设置标题的颜色（用ACT参数）
    static char closebtn[14][16] = {
        "OOOOOOOOOOOOOOO@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQQQ@@QQQQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"
    };
    int x, y;
    char c, tc, tbc;
    if (act != 0) {
        tc = COL8_WHITE;
        tbc = COL8_DRKBLU;
    } else {
        tc = COL8_LITGRY;
        tbc = COL8_DRKGRY;
    }
    boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
    putfonts_asc(buf, xsize, 24, 4, tc, title);
    for (y = 0; y < 14; y++) {
        for (x = 0; x < 16; x++) {
            c = closebtn[y][x];
            if (c == '@') {
                c = COL8_BLACK;
            } else if (c == '$') {
                c = COL8_DRKGRY;
            } else if (c == 'Q') {
                c = COL8_LITGRY;
            } else {
                c = COL8_WHITE;
            }
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
    return;
}

void putfonts_asc_sht(struct SHTCTL *shtctl, struct SHEET *sht, int x, int y, int c, int b, char *s, int l){
	//b为背景颜色，l为字符串长度
	boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8, y + 15);
	putfonts_asc(sht->buf, sht->bxsize, x, y, c, s);
	sheet_refresh(shtctl, sht, x, y, x + l * 8, y + 16);
	return;
}

void make_textbox(struct SHEET *sht, int x0, int y0, int sx, int sy, int c) {
    int x1 = x0 + sx, y1 = y0 + sy;
    boxfill8(sht->buf, sht->bxsize, COL8_DRKGRY, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
    boxfill8(sht->buf, sht->bxsize, COL8_DRKGRY, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_BLACK, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_BLACK, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_WHITE, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
    boxfill8(sht->buf, sht->bxsize, COL8_WHITE, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
    boxfill8(sht->buf, sht->bxsize, COL8_LITGRY, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_LITGRY, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, c, x0 - 1, y0 - 1, x1 + 0, y1 + 0); 
}