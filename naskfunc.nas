; naskfunc
; TAB=4

[FORMAT "WCOFF"]				; 制作目标文件的模式	
[INSTRSET "i486p"]				; 使用到486为止的指令
[BITS 32]						; 3制作32位模式用的机器语言
[FILE "naskfunc.nas"]			; 文件名

		GLOBAL	_io_hlt, _io_cli, _io_sti, _io_stihlt
		GLOBAL	_io_in8,  _io_in16,  _io_in32
		GLOBAL	_io_out8, _io_out16, _io_out32
		GLOBAL	_io_load_eflags, _io_store_eflags
		global _load_gdt,_load_idt,_load_tr
		GLOBAL _asm_int21handle,_asm_int2chandle,_asm_int20handle
		global _io_delay
		global _farjmp,_taskswitch6

		extern _int21handle,_int2chandle,_int20handle
		
[SECTION .text]
_taskswitch6:
	jmp 6<<3:0
	ret

_farjmp:
	; farjmp(int offset,short selector)
	jmp far [esp+4] ;先从指定内存读取4个字节到eip，再读取两个字节装到cs
	ret
	
_load_tr:
	ltr [esp+4]
	ret
	
_asm_int20handle:
	push es
	push ds
	pushad
	mov eax,esp
	push eax
	mov ax,ss
	mov ds,ax
	mov es,ax
	call _int20handle
	pop eax
	popad
	pop ds
	pop es
	iretd

_asm_int21handle:
	push es
	push ds
	pushad
	mov eax,esp
	push eax
	mov ax,ss
	mov ds,ax
	mov es,ax
	call _int21handle
	pop eax
	popad
	pop ds
	pop es
	iretd
	
_asm_int2chandle:
	push es
	push ds
	pushad
	mov eax,esp
	push eax
	mov ax,ss
	mov ds,ax
	mov es,ax
	call _int2chandle
	pop eax
	popad
	pop ds
	pop es
	iretd
	
_io_delay:
	nop
	nop
	nop
	nop
	ret
	
_load_gdt:
	mov ax,[esp+4] ;limit
	mov [esp+6],ax
	lgdt [esp+6]
	ret
	
_load_idt:
	mov ax,[esp+4] ;limit
	mov [esp+6],ax
	lidt [esp+6]
	ret
	
_io_hlt:	; void io_hlt(void);
		HLT
		RET

_io_cli:	; void io_cli(void);
		CLI
		RET

_io_sti:	; void io_sti(void);
		STI
		RET

_io_stihlt:	; void io_stihlt(void);
		STI
		HLT
		RET

_io_in8:	; int io_in8(int port);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AL,DX
		RET

_io_in16:	; int io_in16(int port);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AX,DX
		RET

_io_in32:	; int io_in32(int port);
		MOV		EDX,[ESP+4]		; port
		IN		EAX,DX
		RET

_io_out8:	; void io_out8(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		AL,[ESP+8]		; data
		OUT		DX,AL
		RET

_io_out16:	; void io_out16(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,AX
		RET

_io_out32:	; void io_out32(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,EAX
		RET

_io_load_eflags:	; int io_load_eflags(void);
		PUSHFD		; PUSH EFLAGS 
		POP		EAX
		RET

_io_store_eflags:	; void io_store_eflags(int eflags);
		MOV		EAX,[ESP+4]
		PUSH	EAX
		POPFD		; POP EFLAGS 
		RET