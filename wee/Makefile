all: stage2_size.h wee63.mbr

.SUFFIXES: .exec .S .c .o

wee63start: wee63start.o
	objcopy -O binary $< $@
pre_stage2: asm.o builtins.o disk_io.o fsys_ext2fs.o fsys_fat.o fsys_ntfs.o
	gcc -o $@ -nostdlib -Wl,-N -Wl,-Ttext -Wl,308200 -Wl,-N asm.o builtins.o disk_io.o fsys_ext2fs.o fsys_fat.o fsys_ntfs.o
	objcopy -O binary $@
	echo -n -e "\0260\002\032\0316" >> $@

asm.o: asm.S a20.inc shared.h

builtins.o: builtins.c shared.h

disk_io.o: disk_io.c shared.h

fsys_ext2fs.o: fsys_ext2fs.c shared.h

fsys_fat.o: fsys_fat.c shared.h

fsys_ntfs.o: fsys_ntfs.c shared.h

wee63start.o: wee63start.S stage2_size.h

.S.o:
	gcc -c $<

.c.o:
	gcc -Os -fno-stack-protector -fno-builtin -nostdinc -Wall -Wmissing-prototypes -Wunused -Wshadow -Wpointer-arith -Wundef -c $<

.PHONY: all

stage2_size.h: pre_stage2
	-rm -f stage2_size.h
	set dummy `ls -l pre_stage2`; \
	echo "#define STAGE2_SIZE $$6" > stage2_size.h

wee63.mbr: pre_stage2 wee63start
	-rm -f wee63.mbr
	echo -n -e "\000\000\000\000\000\000\000\000\000\000\000\000" | cat wee63start pre_stage2 - > wee63.mbr
	if [ -f ./preset_menu_used ]; then echo -n -e "\000" | cat ./preset_menu_used - >> wee63.mbr ;fi

