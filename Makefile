all:
	make build

helloos.img: ipl.bin
	wine ./tolset/z_tools/edimg.exe imgin:./tolset/z_tools/fdimg0at.tek wbinimg src:ipl.bin len:512 from:0 to:0 imgout:helloos.img

ipl.bin: ipl.nas
	wine ./tolset/z_tools/nask.exe ipl.nas ipl.bin ipl.lst

build:
	make helloos.img

run:
	make build
	qemu-system-i386 -fda helloos.img

clean:
	rm ipl.bin ipl.lst helloos.img
