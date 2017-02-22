OBJS_BOOTPATH = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj int.obj fifo8.obj keyboard.obj mouse.obj

TOOLPATH = wine ./tolset/z_tools/
INCPATH  = ./tolset/z_tools/haribote/

NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet -std=c99
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = ./tolset/z_tools/haribote/haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe

all:
	make build

helloos.img: ipl.bin llfos.sys
	$(EDIMG) \
		imgin:./tolset/z_tools/fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0 \
		copy from:llfos.sys to:@: \
		imgout:helloos.img

ipl.bin: ipl.nas
	$(NASK) ipl.nas ipl.bin ipl.lst

asmhead.bin: asmhead.nas
	$(NASK) asmhead.nas asmhead.bin asmhead.lst

build:
	make helloos.img

hankaku.bin : hankaku.txt
	$(MAKEFONT) hankaku.txt hankaku.bin

hankaku.obj : hankaku.bin
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

bootpack.obj: bootpack.nas
	$(NASK) bootpack.nas bootpack.obj bootpack.lst

naskfunc.obj: naskfunc.nas
	$(NASK) naskfunc.nas naskfunc.obj naskfunc.lst

bootpack.bim: $(OBJS_BOOTPATH)
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map $(OBJS_BOOTPATH)
# 3MB+64KB=3136KB

bootpack.hrb: bootpack.bim
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

llfos.sys: asmhead.bin bootpack.hrb
	cat asmhead.bin bootpack.hrb > llfos.sys

%.gas: %.c
	$(CC1) -o $*.gas $*.c

%.nas: %.gas
	$(GAS2NASK) $*.gas $*.nas

%.obj: %.nas
	$(NASK) $*.nas $*.obj $*.lst

run:
	make build
	qemu-system-i386 -fda helloos.img -m 1G

clean:
	rm *.bin *.lst *.obj
	rm llfos.sys bootpack.map bootpack.nas bootpack.bim bootpack.hrb helloos.img
