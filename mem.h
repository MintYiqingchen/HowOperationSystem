;mem.h
;定义内存相关的常量

;内存地址分配定义
FAT_BASE EQU 0x50		;0x500 紧接bios数据区,直到0x1700，常驻内存
CYLS EQU 0x1701			;引导扇区
LEDS EQU 0x1702			;键盘状态
VMODE EQU 0x1703		;显示模式
RESERVE EQU 0X1704		;保留扇区
SCRNX EQU 0x1705		;分辨率x
SCRNY EQU 0x1707
VRAM EQU 0x1709			;显存地址
GDT_PHY_ADR EQU 0x270000	;全局描述符表
IDT_PHY_ADR EQU 0x26F800	;中断描述符表
BUFFER_BASE EQU 0x7e0	;扇区缓冲区基址0x7e00,紧接引导
BUFFER_OFFSET EQU 0		;缓冲区偏移量
;VIDEO_BASE EQU 0xA0000	;和VRAM中存的一样
VIDEO_BASE EQU 0xe0000000	;和VRAM中存的一样

SYS_INIT_BASE EQU 0x9000	;系统init程序初始地址
BOTPAK  EQU 0x00280000 ;c语言程序放置物理地址
DSKCAC EQU  0x00100000  ;引导程序物理地址

MemChkBuf equ 0x1820	;内存检查信息
dwMCRNumber equ 0x1800	;内存检查块数

;磁盘参数定义
RootDirSectors	equ	14		; 224*32/512=14个扇区
SectorNoOfRootDirectory	equ	19	; 根目录区的首扇区号