;HOWOS.SYS 
;真正的操作系统程序

;定义boot信息保存的地址
; CYLS EQU 0x1701			;引导扇区
; LEDS EQU 0x1702			;键盘状态
; VMODE EQU 0x1703		;显示模式
; RESERVE EQU 0X1704		;保留扇区
; SCRNX EQU 0x1705		;分辨率x
; SCRNY EQU 0x1707
; VRAM EQU 0x1709			;显存地址
; MemChkBuf equ 0x1820	;内存检查信息
; dwMCRNumber equ 0x1800	;内存检查块数
%include "mem.h"
%include "pm.inc"
VBEMODE EQU 0X101

jmp LABEL_START ;3字节

[section .gdt]				;段基址		段界限		段属性
LABEL_GDT:	Descriptor		  0,		0,			0
LABEL_DESC_FLAT: Descriptor 0,		0fffffh,		DA_32+DA_CR+DA_LIMIT_4K
LABEL_DESC_VIDEO:  Descriptor 0xe0000000,	0ffffh,		DA_DRW+DA_DPL3
LABEL_DESC_DATA:   Descriptor 0,		0fffffh,	DA_DRW+DA_LIMIT_4K+DA_32
LABEL_DESC_GEN: Descriptor 0x280000,	0fffffh,	DA_32+DA_CR+DA_LIMIT_4K

LEN_GDT EQU $-LABEL_GDT
GdtPtr dw LEN_GDT-1
	   dd SYS_INIT_BASE*16+LABEL_GDT;gdt的基地址
	   
;定义段选择符
SelectorFlat equ LABEL_DESC_FLAT-LABEL_GDT
SelectorVideo  equ LABEL_DESC_VIDEO-LABEL_GDT+SA_RPL3
SelectorData equ	LABEL_DESC_DATA	- LABEL_GDT
SelectorGen equ LABEL_DESC_GEN-LABEL_GDT

SYS_INIT_PHY_ADDR equ SYS_INIT_BASE*16
;end of [section .gdt]

[section .code16]
LABEL_START:
	mov ax,cs
	mov ss,ax
	mov ds,ax
	xor ax,ax
	mov es,ax
	
	;在设置画面模式之前，先看看bios是否支持VBE协议
	mov ax,0x2000
	mov es,ax
	mov di,0
	mov ax,0x4f00
	int 10h
	cmp ax,0x004f
	jne scrn320
	;VBE版本是否2.0以上
	mov ax,[es:di+4]
	cmp ax,0x200
	jb scrn320
	;取得画面模式信息
	mov cx,VBEMODE
	mov ax,0x4f01
	int 10h
	cmp ax,0x004f
	jne scrn320
	cmp byte [es:di+0x19],8;颜色数是否为8
	jne scrn320
	cmp byte [es:di+0x1b],4;是否为调色板模式
	jne scrn320
	mov ax,[es:di+0x00];画面模式号码
	and ax,0x0080
	jz scrn320
	
	mov   bx, 0x4101
    mov   ax, 0x4f02
    int   0x10
	xor ax,ax
	mov fs,ax
	mov byte [fs:VMODE],8;保存画面模式
	mov ax,[es:di+0x12]
	mov word [fs:SCRNX],ax
	mov ax,[es:di+0x14]
	mov word [fs:SCRNY],ax
	mov eax,[es:di+0x28]
	mov dword [fs:VRAM],eax
	jmp keyinfo
	
	scrn320:
	mov al,0x13
	mov ah,0
	int 10h
	mov byte [fs:VMODE],8
	mov word [fs:SCRNX],320
	mov word [fs:SCRNY],200
	mov dword [fs:VRAM],0xa0000
	;相应的GDT的部分也要修改，不过这部分留到C语言部分进行
	
	keyinfo:
	;获得键盘状态
	mov ah,0x02
	int 16h
	mov [fs:LEDS],al
	
	;检查可用内存
	call memoryCheck
	
	;加载gdtr
	lgdt [GdtPtr]
	
	;屏蔽中断
	mov al,0xff
	out 0x21,al
	nop
	out 0xa1,al
	cli
	
	;打开地址线
	in	al, 92h
	or	al, 00000010b
	out	92h, al
	
	; 置PE位，准备切换到保护模式
	mov	eax, cr0
	and eax,0x7fffffff	;禁止分页
	or	eax, 1
	mov	cr0, eax
	
	jmp dword SelectorFlat:LABEL_CODE32 + SYS_INIT_PHY_ADDR

memoryCheck:
	xor ax,ax
	mov es,ax
	mov ebx,0
	mov di,MemChkBuf
	.loop:
	mov eax,0e820h
	mov ecx,20
	mov edx,0534D4150h
	int 15h
	jc LABEL_MEM_CHK_FAIL
	add di,20
	inc dword [es:dwMCRNumber]
	cmp ebx,0
	jne .loop
	jmp LABEL_MEM_CHK_OK
	
	LABEL_MEM_CHK_FAIL:
	mov dword [es:dwMCRNumber],0
	
	LABEL_MEM_CHK_OK:
	ret
	
[section .code32]
align 32
[bits 32]
LABEL_CODE32:
	mov ax,SelectorData
	mov ds,ax
	mov es,ax
	mov ss,ax
	mov fs,ax
	mov ax,ss
	mov ax,SelectorVideo
	mov gs,ax
	
	;复制c语言程序
	mov esi,bootpack+SYS_INIT_PHY_ADDR
	mov edi,BOTPAK
	mov ecx,512*1024/4
	call memcpy
	
	;复制引导程序
	mov esi,0x7c00
	mov edi,DSKCAC
	mov ecx,512/4
	call memcpy
	
	; 完成其余的bootpack任务

	; bootpack启动

	MOV		EBX,BOTPAK
	MOV		ECX,[EBX+16]
	ADD		ECX,3			; ECX += 3;
	SHR		ECX,2			; ECX /= 4;
	JZ		skip			; 传输完成
	MOV		ESI,[EBX+20]	; 源
	ADD		ESI,EBX
	MOV		EDI,[EBX+12]	; 目标
	CALL	memcpy
skip:
	MOV		ESP,[EBX+12]	; 堆栈的初始化
	JMP		DWORD SelectorGen:0x0000001b
	
memcpy:
	;输入esi指向的地址,复制到edi指向地址
	;ecx存储字节数
	cld 
	repz movsd
	ret
	
bootpack:
	