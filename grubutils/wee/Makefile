all: wee63.mbr wee127.mbr

.SUFFIXES: .exec .S .c .o

wee63start: wee63start.o
	objcopy -O binary $< $@
pre_stage2: asm.o builtins.o disk_io.o fsys_ext2fs.o fsys_fat.o fsys_ntfs.o
	gcc -o $@ -nostdlib -Wl,-N -Wl,-Ttext -Wl,308200 -Wl,-N -Wl,--build-id=none asm.o builtins.o disk_io.o fsys_ext2fs.o fsys_fat.o fsys_ntfs.o
	objcopy -O binary $@
	echo -n -e "\0260\002\032\0316" >> $@

asm.o: asm.S a20.inc shared.h extra.h

builtins.o: builtins.c shared.h extra.h

disk_io.o: disk_io.c shared.h extra.h

fsys_ext2fs.o: fsys_ext2fs.c shared.h extra.h

fsys_fat.o: fsys_fat.c shared.h extra.h

fsys_ntfs.o: fsys_ntfs.c shared.h extra.h

wee63start.o: wee63start.S stage2_size.h

.S.o:
	gcc -c $<

.c.o:
	gcc -Os -fno-stack-protector -fno-builtin -mpreferred-stack-boundary=2 -momit-leaf-frame-pointer -nostdinc -Wall -Wmissing-prototypes -Wunused -Wshadow -Wpointer-arith -Wundef -c $<

clean:
	rm -f *.o wee63start pre_stage2 wee127/*.o wee127/pre_stage2

distclean:
	rm -rf *~ *.o wee63start pre_stage2 extra.h wee127

wee127.mbr: wee127/wee63.mbr
	-rm -f $@
	cp wee127/wee63.mbr $@

wee127/pre_stage2:
wee127/wee63start:
wee127/preset_menu_used:
wee127/wee63.mbr: wee63.mbr wee127/pre_stage2 wee127/wee63start wee127/preset_menu_used
	if [ -d wee127 ]; then cd wee127; make wee63.mbr; cd ..; else mkdir wee127; cd wee127; ln -s ../a20.inc a20.inc; ln -s ../asm.S asm.S; ln -s ../builtins.c builtins.c; ln -s ../disk_io.c disk_io.c; ln -s ../fsys_ext2fs.c fsys_ext2fs.c; ln -s ../fsys_fat.c fsys_fat.c; ln -s ../fsys_ntfs.c fsys_ntfs.c; ln -s ../Makefile Makefile; ln -s ../preset_menu_used preset_menu_used; ln -s ../shared.h shared.h; ln -s ../wee63start.S wee63start.S; echo "#define MBRSECTORS127" > extra.h; make wee63.mbr; cd ..; fi

.PHONY: all clean distclean

stage2_size.h: pre_stage2
	-rm -f stage2_size.h
	set dummy `ls -l pre_stage2`; \
	echo "#define STAGE2_SIZE $$6" > stage2_size.h

wee63.mbr: pre_stage2 wee63start preset_menu_used
	-rm -f wee63.mbr
	echo -n -e "\000\000\000\000\000\000\000\000\000\000\000\000" | cat wee63start pre_stage2 - > wee63.mbr
	if [ -f ./preset_menu_used ]; then echo -n -e "\000" | cat ./preset_menu_used - >> wee63.mbr ;fi

preset_menu_used:
	echo File not found: preset_menu_used
	exit 1

extra.h:
	touch $@

