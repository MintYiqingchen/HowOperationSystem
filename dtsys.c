#include "osmain.h"
extern struct BOOTINFO *botinfo;
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar){
	if (limit > 0xfffff) {
		ar |= 0x8000; /* G_bit = 1 */
		limit /= 0x1000;
	}
	sd->limit_low    = limit & 0xffff;
	sd->base_low     = base & 0xffff;
	sd->base_mid     = (base >> 16) & 0xff;
	sd->access_right = ar & 0xff;
	sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
	sd->base_high    = (base >> 24) & 0xff;
	return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar){
	gd->offset_low   = offset & 0xffff;
	gd->selector     = selector;
	gd->dw_count     = (ar >> 8) & 0xff;
	gd->access_right = ar & 0xff;
	gd->offset_high  = (offset >> 16) & 0xffff;
	return;
}
extern struct TSS32 tss_a,tss_b;
void init_gdtidt(void){  /* 初始化IDT和GDT */
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) 0x00270000;
	struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR*) 0x0026f800;

	int i;

	/* GDT移植 */
	for (i = 0; i <= LIMIT_GDT / 8; i++) {
		set_segmdesc(gdt + i, 0, 0, 0);
	}
	struct SEGMENT_DESCRIPTOR *former = (struct SEGMENT_DESCRIPTOR *)0x90004;//指向以前设定好的描述符,千万不要搞错地址了！
	for(i=0;i<5;++i){
		gdt->limit_low=former->limit_low;
		gdt->base_low=former->base_low;
		gdt->base_mid=former->base_mid;
		gdt->access_right=former->access_right;
		gdt->base_high=former->base_high;
		gdt->limit_high=former->limit_high;
		former = former+1;
		gdt = gdt+1;
	}
	gdt = (struct SEGMENT_DESCRIPTOR *) 0x00270000;
	set_segmdesc(gdt + 4, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);//本代码段
	set_segmdesc(gdt + 2, 0xffff,(int)botinfo->vram,0xF2);//重新设置视频段
	// set_segmdesc(gdt + 5, 103, (int)&tss_a, AR_TSS );
	// set_segmdesc(gdt + 6, 103, (int)&tss_b, AR_TSS );
	load_gdt(LIMIT_GDT, GDT_ADR);
	/* IDT初始化 */
	for (i = 0; i < 256; i++) {
		set_gatedesc(idt + i, 0, 0, 0);
	}
	load_idt(LIMIT_IDT, 0x0026f800);
	set_gatedesc(idt+0x21,(int)asm_int21handle,4<<3,AR_INTGATE32);//处理函数在第4个段，即howos.sys所在段
	set_gatedesc(idt+0x2c,(int)asm_int2chandle,4<<3,AR_INTGATE32);
	set_gatedesc(idt+0x20,(int)asm_int20handle,4<<3,AR_INTGATE32);
	return;
}
