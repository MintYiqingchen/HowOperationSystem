TOOLPATH = ../z_tools/
INCPATH = ../z_tools/haribote/

MAKE = $(TOOLPATH)make.exe -r
NASK = $(TOOLPATH)nask.exe
CC1 = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM = $(TOOLPATH)obj2bim.exe
BIM2HRB = $(TOOLPATH)bim2hrb.exe
RULEFILE = $(TOOLPATH)haribote/haribote.rul
EDIMG = $(TOOLPATH)edimg.exe
DEL = del
SHORTCUT = "D:\Program Files\Oracle\VirtualBox\VirtualBox.exe" --comment "OS1" --startvm "a5c4b0e6-e142-4720-98ee-056911204b29"

OBJS = bootpack.obj naskfunc.obj font.obj graphic.obj dtsys.obj int.obj \
	fifo.obj mouse.obj keyboard.obj memory.obj sheet.obj timer.obj mtask.obj

howos.sys :asmhead.bin bootpack.hrb Makefile
	copy /B asmhead.bin+bootpack.hrb howos.sys
	
font.obj: font.bin Makefile
	bin2obj.exe font.bin font.obj _font
	
bootpack.gas : osmain.c Makefile
	$(CC1) -o bootpack.gas osmain.c

bootpack.nas : bootpack.gas Makefile
	$(GAS2NASK) bootpack.gas bootpack.nas

bootpack.obj : bootpack.nas Makefile
	$(NASK) bootpack.nas bootpack.obj bootpack.lst

naskfunc.obj : naskfunc.nas Makefile
	$(NASK) naskfunc.nas naskfunc.obj naskfucn.lst

bootpack.bim : $(OBJS) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		$(OBJS)
#echo error

# 3MB+64KB=3136KB


bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

%.gas : %.c Makefile
	$(CC1) -o $*.gas $*.c
%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas
%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst

clean :
	-$(DEL) *.lst
	-$(DEL) *.gas
	-$(DEL) *.obj
	-$(DEL) bootpack.nas
	-$(DEL) bootpack.map
	-$(DEL) bootpack.bim
	-$(DEL) bootpack.hrb
	-$(DEL) *.*~
	-$(DEL) *~
	echo Finished.