/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2004 Free Software Foundation, Inc.
 *  Copyright (C) 2010  Tinybit(tinybit@tom.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * Note: These functions defined in this file may be called from C.
 *       Be careful of that you must not modify some registers. Quote
 *       from gcc-2.95.2/gcc/config/i386/i386.h:

   1 for registers not available across function calls.
   These must include the FIXED_REGISTERS and also any
   registers that can be used without being saved.
   The latter must include the registers where values are returned
   and the register where structure-value addresses are passed.
   Aside from that, you can include as many other registers as you like.

  ax,dx,cx,bx,si,di,bp,sp,st,st1,st2,st3,st4,st5,st6,st7,arg
{  1, 1, 1, 0, 0, 0, 0, 1, 1,  1,  1,  1,  1,  1,  1,  1,  1 }
 */

#define ASM_FILE

#include "shared.h"

#define	ABS(x)	((x) - EXT_C(main) + 0x8200)
#define	SBA(x)	((x) + 0x300000)

	.file	"asm.S"

	.text

	/*
	 * In stage2, do not link start.S with the rest of the source
	 * files directly, so define the start symbols here just to
	 * force ld quiet. These are not referred anyway.
	 */
	.globl	start, _start
start:
_start:

ENTRY(main)

	.code16

	ljmp $0, $ABS(codestart)

	. = EXT_C(main) + 0x5

	/* control byte: pxe, DUCE, tune
	 * bit 0 = 1: disable pxe
	 * bit 1 = 1: disable keyboard intervention in boot process
	 * bit 2 = 1: disable the "unconditional command-line entrance" feature
	 * bit 3 = 1: disable geometry tune
	 */
	.byte	0

	. = EXT_C(main) + 0x6

	.byte	3, 2

	. = EXT_C(main) + 0x8

VARIABLE(install_partition)
	.long	0x00FFFFC3

VARIABLE(saved_entryno)
	/* Note: GRUB for DOS uses this for the commandline preset_menu.
	 * A preset_menu can be embedded in the commandline of GRUB.EXE.
	 * This new preset_menu overrides the built-in preset_menu.
	 * If the variable is not touched, and the first byte at
	 * config_file == 0, then the new menu at 0x0800 will work.
	 * If the variable here is cleared to 0, or the first byte at
	 * config_file != 0, then the built-in preset_menu will work.
	 *
	 * Do NOT change this variable to other value than 0.
	 */
	.long	0	// ABS(EXT_C(preset_menu))

VARIABLE(stage2_id)
	.byte	0

VARIABLE(force_lba)
	.byte	0

VARIABLE(version_string)
	.string "0.97"

VARIABLE(config_file)
	.string "/menu.lst"

	/* Utilities may use this to locate the bss starting address,
	 * where the "bootlace" signature is placed and preset-menu
	 * follows.
	 */

	. = EXT_C(main) + 0x6C  #; bss starting address

#if defined(HAVE_USCORE_USCORE_BSS_START_SYMBOL)
	.long	ABS(__bss_start)
#elif defined(HAVE_USCORE_EDATA_SYMBOL)
	.long	ABS(_edata)
#elif defined(HAVE_EDATA_SYMBOL)
	.long	ABS(edata)
#else
#error no bss starting address
#endif

	. = EXT_C(main) + 0x70

/* the real mode code continues... */
codestart:
	jmp	real_codestart

	/* internal variables follow */

	. = EXT_C(main) + 0x80

VARIABLE(boot_drive)
	.long	0

VARIABLE(pxe_yip)
	.long	0
VARIABLE(pxe_sip)
	.long	0
VARIABLE(pxe_gip)
	.long	0

	. = EXT_C(main) + 0x90

VARIABLE(filesize)
	.long	0, 0
VARIABLE(saved_mem_upper)
	/* maximum contiguous memory in KB starting at 1M and below 4G */
	.long	0
VARIABLE(saved_partition)
	.long	0

	. = EXT_C(main) + 0xA0

VARIABLE(saved_drive)
	.long	0
VARIABLE(no_decompression)
	.long	0
VARIABLE(part_start)
	.long	0, 0

	. = EXT_C(main) + 0xB0

VARIABLE(part_length)
	.long	0, 0

VARIABLE(fb_status)
	.long	0

VARIABLE(is64bit)
	.long	0

	. = EXT_C(main) + 0xC0

VARIABLE(saved_mem_higher)
	/* maximum contiguous memory in KB starting at 4G */
	.long	0, 0

VARIABLE(cdrom_drive)
	.long	0xFFFFFFFF	/* default is the invalid drive number */

VARIABLE(ram_drive)
	.long	0x7F	/* default is a floppy */

	. = EXT_C(main) + 0xD0

VARIABLE(rd_base)
	.long	0, 0

VARIABLE(rd_size)
	.long	0, 1	/* default is 4G */

	. = EXT_C(main) + 0xE0

	/* mem alloc array structure

		long	// mem block starting address(16 byte align)
			// bit0=0 for free mem, 1 for used mem
			// bit 2:1 =0 for program code
			//         =1 for environment
			//         =2 for extra data
			//         =3 reserved
			// bit 3=reserved
		long	// process id.
			// 0 for kernel, others for user program
	 */
VARIABLE(mem_alloc_array_start)
	.long	0x800000	/* at 8M */

VARIABLE(mem_alloc_array_end)
	.long	0xA00000	/* at 10M */

VARIABLE(page_map_start)
	.long	0xA00000	/* at 10M */

VARIABLE(page_map_end)
	.long	0xE00000	/* at 14M */


	. = EXT_C(main) + 0xF0

VARIABLE(free_mem_start)	/* unused mem starting address. */
	.long	0x1000000	/* initially at 16M, but may increase later. */

VARIABLE(free_mem_end)		/* unused mem ending address */
	.long	0		/* 0 for 4G(i.e., unlimited) */

VARIABLE(saved_mmap_addr)	/* system memory map starting addess */
	.long	ABS(end_of_low_16bit_code)

VARIABLE(saved_mmap_length)	/* system memory map length */
	.long	0

	. = EXT_C(main) + 0x100

VARIABLE(addr_system_functions)
	.long	EXT_C(system_functions)

VARIABLE(addr_system_variables)
	.long	EXT_C(system_variables)

	. = EXT_C(main) + 0x110

VARIABLE(addr_saved_dir)
	.long	EXT_C(saved_dir)

VARIABLE(errnum)
	.long	0

VARIABLE(current_drive)
	.long	0xFFFFFFFF

VARIABLE(current_partition)
	.long	0

	. = EXT_C(main) + 0x120

VARIABLE(filemax)
	.long	0, 0

VARIABLE(filepos)
	.long	0, 0

	. = EXT_C(main) + 0x130

VARIABLE(debug)
	.long	0

VARIABLE(current_slice)
	.long	0

	. = EXT_C(main) + 0x140

real_codestart:
	cli
	cld

	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movl	$0x7000, %esp

	movb	%dl, ABS(EXT_C(boot_drive))

//	call	empty_8042
	/* output a dummy command (USB keyboard hack) */
	movb	$0xFF, %al
	outb	%al, $0x64
//	call	empty_8042

	sti

	/* check keys 'C' and Insert. */

	xorw	%bx, %bx	/* BL=0, key C not pressed. */
				/* BH=0, Insert key not pressed. */

	movl	0x46C, %eax	/* initial tick */
	addl	$10, %eax	/* wait 0.54 seconds */
	pushl	%eax

2:
	/* checkkey 'c' */

	pushw	%bx
	movb	$0x01, %ah	/* checkkey */
	int	$0x16
	popw	%bx

	jz	1f		/* no keypress */

	/* getkey */

	pushw	%bx
	movb	$0x00, %ah	/* getkey */
	int	$0x16
	popw	%bx

	cmpw	$KEY_IC, %ax	/* the Insert char */
	jne	3f
	movb	$1, %bh		/* Insert key pressed */
3:
	orb	$0x20, %al
	cmpb	$0x63, %al	/* is "C"? */
	jne	2b		/* no, get next key */

	/* "C" is pressed. */

	movb	$1, %bl		/* key C pressed. */
	jmp	2b
1:
	popl	%eax
	pushl	%eax
	movl	0x46C, %ecx	/* current tick */
	cmpl	%eax, %ecx
	jnb	1f		/* time out, exit. */
	subl	$10, %eax	/* old tick */
	cmpl	%eax, %ecx
	jnb	2b		/* tick is for today */
	cmpl	$10, %ecx	/* tick is for tomorrow */
	jb	2b		/* wait 10 ticks for tomorrow */
1:
	/* time out */
	popl	%eax
	movw	%bx, ABS(EXT_C(startup_debug_keys))

	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es

	/* clear keyboard buffer */
2:
	movb	$0x01, %ah	/* checkkey */
	int	$0x16

	jz	1f		/* no keypress */

	movb	$0x00, %ah	/* getkey */
	int	$0x16
	jmp	2b
1:

	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es

#if 1
	/* Fast A20: enable A20 via System Control Port A */
	inb	$0x92, %al
	testb	$02, %al
	jnz	1f
	orb	$02, %al		# enable A20
	andb	$0xfe, %al		# avoid reseting the CPU by accident
        outb    %al, $0x92		# some chips have only this
1:
	cli
	call	a20_test
	sti
	jnz	1f			# A20 enabled
#else
	/* Oops! I forgot to enable A20. Thanks to zw2312914. */
	
	movw	$0x00ff, %cx		# try so many times on failure
	movw	$0x0101, %dx		# DL=1(enable A20), DH=1(debug on)

	cli	/* yes, keep interrupt off when controlling A20 */
	call	enable_disable_a20	# EAX, CX modified
	sti

	jz	1f		/* enabled A20 with success */
#endif
	/* A20 control failed. Notify the user. */
	pushw	$ABS(bootup_a20_failure)	# the format string
	call	realmode_printf
	//addw	$2, %sp	# adjust the stack pointer, but not required for now.

hard_stop:
2:
	sti
	hlt
	jmp	2b

1:

	/* check int15/E820 */

	xorl	%eax, %eax
	xorl	%ebx, %ebx
1:
	movw	$0xe820, %ax	/* high word already=0 */
	movl	$20, %ecx
	movl	$0x534d4150, %edx	# "SMAP"
	movw	$0x7000, %di
	int	$0x15
	jnc	2f
	xorl	%ebx, %ebx
2:
	xorl	$0x534d4150, %eax	# EAX=0
	jne	1f

	decl	16(%di)
	jnz	2f
	cmpl	%eax, 4(%di)
	jnz	2f		/* above 4G, skip */
	cmpl	$0x100000, (%di)
	jb	2f		/* below 1M, skip */
	movl	%eax, %edx		# EDX=0
	cmpl	%eax, 12(%di)
	jne	3f		/* length more than 4G, success */
	
	movl	8(%di), %ecx	/* length lo */
	addl	(%di), %ecx	/* end of block */
	jc	4f		/* exceeding 4G */
	movl	%ecx, %edx	/* within 4G */
4:
	//cmpl	$0x3C00000, 8(%di)	/* length < 60M ? */
	//jnb	3f			/* success */
	jmp	3f
2:
	testl	%ebx, %ebx
	jnz	1b
1:
	pushw	$ABS(E820_error_string)	# the format string
	call	realmode_printf
	//addw	$2, %sp	# adjust the stack pointer, but not required for now.
	jmp	hard_stop

bootup_a20_failure:
	.ascii	"\r\nA20 fail\0"

E820_error_string:
	.ascii	"\r\nMem fail\0"

3:
	movl	%edx, ABS(EXT_C(free_mem_end))
#if 0
	/* EDX=top end of the usable block */

	subl	$0x02000000, %edx	# EDX=start address, room 32M
	andl	$0xFFE00000, %edx	# align 2M

	/* save this in the handler space. */

	//movl	%edx, ABS(system32_base)

	movb	0x0410, %al
	movb	%al, ABS(EXT_C(floppies_orig))
	movb	0x0475, %al
	movb	%al, ABS(EXT_C(harddrives_orig))

	movl	0x54, %eax
	movl	%eax, ABS(EXT_C(ROM_int15))
	movl	0x4C, %eax
	movl	%eax, ABS(EXT_C(ROM_int13))
	movl	%eax, ABS(EXT_C(ROM_int13_dup))
	cmpl	$0xC0000000, %eax
	jnb	1f
	cmpl	$0x5A000000, %eax
	jb	1f
	andl	$0x3FFFFF, %eax
	cmpl	$0x100, %eax
	jnz	1f
	movw	0x413, %ax		/* Memory size in Kb */
	shlw	$6, %ax			/* Memory size in paragragh */
	cmpw	0x4E, %ax		/* 0000:004E=current int 13 segment */
	jne	1f			/* not hooked */

	movw	%ax, %ds		/* DS=current int13 code segment */

	/* check our int13 signature "$INT13SFGRUB4DOS" */
	cmpl	$0x544E4924, 0x103	/* $INT */
	jnz	2f
	cmpl	$0x46533331, 0x107	/* 13SF */
	jnz	2f
	cmpl	$0x42555247, 0x10B	/* GRUB */
	jnz	2f
	cmpl	$0x534F4434, 0x10F	/* 4DOS */
	jnz	2f

	movl	(EXT_C(ROM_int13) - int13_handler), %eax	# 0x1C=int 13
	cmpl	$0x5A000000, %eax
	jb	2f			/* not our handler */

	movl	(EXT_C(ROM_int15) - int13_handler), %eax	# 0x0C=int 15
	cmpl	$0x5A000000, %eax
	jb	2f			/* not our handler */

	/* restore old emu data, except the first byte of handler size. */
	movw	$(0x140 - 1), %cx
	movw	$1, %si			/* DS=current int13 code segment */
	movw	$ABS(int13_handler + 1), %di		# ES=0
	repz movsb

2:
	xorw	%ax, %ax
	movw	%ax, %ds				# DS=0
1:

	/* setup a map for system itself */

	movb	$0xFF, ABS(int13_handler + 0xE1)	# TO drive=(md)
	movl	%edx, ABS(int13_handler + 0xE4)		# system32_base
	shrl	$9, %edx				# start sector number
	movl	%edx, ABS(int13_handler + 0xE8)
	movl	$0x10000, ABS(int13_handler + 0xF0)	# 32M
#endif

	call real_to_prot

	.code32

#ifndef STAGE1_5

	/* before clearing the bss, we move preset_menu to 0x800 */

	movl	ABS(EXT_C(preset_menu)), %eax

#if defined(HAVE_USCORE_USCORE_BSS_START_SYMBOL)
	cmpl	$ABS(__bss_start), %eax
#elif defined(HAVE_USCORE_EDATA_SYMBOL)
	cmpl	$ABS(_edata), %eax
#elif defined(HAVE_EDATA_SYMBOL)
	cmpl	$ABS(edata), %eax
#else
#error no bss starting address
#endif
	jnz	4f		/* use old bootp for diskless */

	xorl	%eax, %eax
	cmpb	%al, ABS(EXT_C(config_file))	/* AL == 0 */
	jnz	2f
	movl	ABS(EXT_C(saved_entryno)), %ebx
	testl	%ebx, %ebx
	jnz	3f	/* use menu embedded in commnad-line of grub.exe */
2:
	/* use builtin preset_menu */

	/* set the starting address of the preset_menu */

#if defined(HAVE_USCORE_USCORE_BSS_START_SYMBOL)
	movl	$ABS(__bss_start), %esi
#elif defined(HAVE_USCORE_EDATA_SYMBOL)
	movl	$ABS(_edata), %esi
#elif defined(HAVE_EDATA_SYMBOL)
	movl	$ABS(edata), %esi
#else
#error no bss starting address
#endif

	cld
	addl	$16, %esi	/* skip 4 bytes of B0 02 1A CE */
				/* skip 4 bytes of reserved */
				/* skip 4 bytes of reserved */
				/* skip 4 bytes of zeroes */

	movl	$0x400, %ecx	/* move 4KB of the menu ... */
	movl	$0x800, %edi	/* ... to 0x800 */
	repz movsl

3:
	movl	$0x0800, EXT_C(preset_menu)	/* use new menu at 0x800 */
4:
#endif /* !STAGE1_5 */

	/* before clearing bss, move 32-bit code to above 3M. */
	/* This need A20 to be enabled. */

	movl	$ABS(end_of_low_16bit_code), %esi
	leal	SBA(0)(%esi), %edi
	//addl	ABS(int13_handler + 0xE4), %edi		# system32_base


#if defined(HAVE_USCORE_USCORE_BSS_START_SYMBOL)
	movl	$(__bss_start), %ecx
#elif defined(HAVE_USCORE_EDATA_SYMBOL)
	movl	$(_edata), %ecx
#elif defined(HAVE_EDATA_SYMBOL)
	movl	$(edata), %ecx
#else
#error no bss starting address
#endif
	subl	%edi, %ecx
	shrl	$2, %ecx

	cld
	repz movsl

	/* clean out the bss */

	/* set %ecx to the bss end */
#if defined(HAVE_END_SYMBOL)
	movl	$(end), %ecx
#elif defined(HAVE_USCORE_END_SYMBOL)
	movl	$(_end), %ecx
#else
#error no bss ending address
#endif

	/* compute the bss length */
	subl	%edi, %ecx
	shrl	$2, %ecx

	/* zero %eax */
	xorl	%eax, %eax

	/* set the direction */
	cld

	/* clean out */
	rep stosl

1:
	//pushl	$0
	//movl	$SBA(ABS(EXT_C(unset_int13_handler))), %eax
	//addl	ABS(int13_handler + 0xE4), %eax		# system32_base
	//call	%eax
	//popl	%eax

	//pushl	$ABS(EXT_C(hooked_drive_map))
	//movl	$SBA(ABS(EXT_C(set_int13_handler))), %eax
	//addl	ABS(int13_handler + 0xE4), %eax		# system32_base
	//call	%eax
	//popl	%eax

	/*
	 *  Call the start of main body of C code, which does some
	 *  of it's own initialization before transferring to "cmain".
	 */

	call	SBA(EXT_C(init_bios_info))

	//movl	$SBA(ABS(EXT_C(init_bios_info))), %eax
	//addl	ABS(int13_handler + 0xE4), %eax		# system32_base
	//call	%eax

1:
	call	SBA(EXT_C(enter_cmdline))
	//movl	$SBA(ABS(EXT_C(enter_cmdline))), %eax
	//addl	ABS(int13_handler + 0xE4), %eax		# system32_base
	//call	%eax

	jmp	1b


/* If preset_menu == __bss_start, use the new menu at end of pre_stage2. */

VARIABLE(preset_menu)
#if defined(HAVE_USCORE_USCORE_BSS_START_SYMBOL)
	.long	ABS(__bss_start)
#elif defined(HAVE_USCORE_EDATA_SYMBOL)
	.long	ABS(_edata)
#elif defined(HAVE_EDATA_SYMBOL)
	.long	ABS(edata)
#else /* ! HAVE_EDATA_SYMBOL */
#error no bss starting address
#endif /* ! HAVE_EDATA_SYMBOL */

VARIABLE(startup_debug_keys)
	.long	0	/* indicate whether or not C and Insert key pressed. */
VARIABLE(script_char_pointer)
	/* initially point to bootup script. 0 to invalidate the script. */
	.long	0x800
//VARIABLE(titles_count)
//	.long	0

	.code16

#include "a20.inc"

#;============================================================================

/* void realmode_printf(const char *format, ...)
 *
 * input:	format is offset in CS segment
 *
 * Usage example:
 *
 * 		pushw	IntegerN
 *		 ... ... ... ...
 * 		pushw	Integer2
 * 		pushw	Integer1
 * 		pushw	$format_string - int13_handler
 *		call	realmode_printf
 * 		addw	$(2*(N+1)), %sp
 *
 * where int13_handle should be the base of the CS segment,
 * and format_string like this:
 *
 * format_string:
 *		 .string "Int1=%x, Int2=%x, ..., IntN=%x\r\n"
 *
 * Currently only %d, %x and %X are implemented.
 */

realmode_printf:

	.code16

	pushaw
	movw	%sp, %bp
	# bp+18:	format
	# bp+20:	variables
	addw	$18, %bp
	movw	(%bp), %si		# points to format string
	addw	$2, %bp			# (%bp) is the first variable
1:
	cs lodsb
	testb	%al, %al
	jz	1f
	cmpb	$'%', %al
	jne	2f

	#; %d, %x, %X

	cs lodsb
	testb	%al, %al
	jz	1f
	cmpb	$'d', %al
	movw	$10, %bx		# base 10
	jz	4f
	cmpb	$'x', %al
	jz	3f
	cmpb	$'X', %al
	jne	1b			# unkown directive, continue
3:
	/* print hexa number */
	movw	$16, %bx		# base 16
4:
	/* print decimal or hexa number */
	pushl	%edi

	xorl	%edi, %edi
	xorw	%cx, %cx		# count the digits
	movw	(%bp), %ax
5:
	xorw	%dx, %dx
	divw	%bx			# AX=quo, DX=rem
	movw	%dx, %di
	rorl	$4, %edi
	incw	%cx
	testw	%ax, %ax		# end?
	jnz	5b

	/* print the digits in EDI */
	xorw	%bx, %bx	/* video page 0 */
5:
	roll	$4, %edi
	movw	%di, %ax		# get digit in AL
	andb	$0x0f, %al
	cmpb	$9, %al
	jbe	6f
	addb	$7, %al			# A, B, C, D, E, F
6:
	addb	$0x30, %al
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	loop	5b

	popl	%edi

	addw	$2, %bp			# (%bp) is the next variable
	jmp	1b			# continue
2:
	/* print char in AL */
	xorw	%bx, %bx	/* video page 0 */
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	jmp	1b			# continue
1:
	popaw
	ret

#;============================================================================

	.code32

#if 0
	.align	16

MyGDT:
/* 0*/	.word	0	/* NULL entry. Can be used at will. */
gdtdesc:		/* 6 bytes used for gdt descriptor. */
	.word	MyGDTEnd - MyGDT - 1	/* GDT limit */

	/* The gdt linear base address will be adjusted by int13_handler,
	 * but not touched outside int13_handler. */

	.long	ABS(MyGDT)		/* GDT linear address */

/* 8*/	.long	0x0000FFFF, 0x00009300	/* 16-bit data 64K limit */
/*16*/	.long	0x0000FFFF, 0x00CF9300	/* 32-bit data 4GB limit */

	/* The 16-bit code segment base will be adjusted by int13_handler,
	 * but not touched outside int13_handler. */
/*24*/	.long	0x0000FFFF, 0x00009B00	/* 16-bit code 64K limit */
/*32*/	.long	0x0000FFFF, 0x00AF9B00	/* 64-bit code 4GB limit */

OldGDT:
	/* This gdt entry is no use for int13_handler, but used outside. The
	 * int13_handler only uses it as a temp space for the OldGDT variable.
	 * Only the last 6 bytes are used by OldGDT, and the starting 2 bytes
	 * are not used by OldGDT. */
/*40*/	.long	0x0000FFFF, 0x00CF9B00	/* 32-bit code 4GB limit */

MyGDTEnd:

#else
	.align 16
MyGDT:
//gdt:
/* 0*/	.word	0	/* NULL entry. Can be used at will. */
gdtdesc:		/* 6 bytes used for gdt descriptor. */
	.word	MyGDTEnd - MyGDT - 1	/* GDT limit */
	/* The gdt linear base address will be adjusted by set_int13_handler,
	 * but not touched outside int13_handler. */
	.long	ABS(MyGDT)		/* GDT linear address */

/* 8*/	PM_DS16 = (. - MyGDT)	/* adjusted base = int13_handler */
	.long	0x0000FFFF, 0x00009300	/* 16-bit data 64K limit */
/*16*/	PM_DS32 = (. - MyGDT)
	.long	0x0000FFFF, 0x00CF9300	/* 32-bit data 4GB limit */

	/* The 16-bit code segment base will be adjusted by set_int13_handler,
	 * but not touched outside int13_handler. */
/*24*/	PM_CS16 = (. - MyGDT)	/* adjusted base = int13_handler */
	.long	0x0000FFFF, 0x00009B00	/* 16-bit code 64K limit */
/*32*/	LM_CS64 = (. - MyGDT)
	.long	0x0000FFFF, 0x00AF9B00	/* 64-bit code 4GB limit */

	/* This gdt entry is not used by int13_handler, but used outside. */
/*40*/	PM_CS32 = (. - MyGDT)
	.long	0x0000FFFF, 0x00CF9B00	/* 32-bit code 4GB limit */
MyGDTEnd:
#endif

#if 0

	.align	16

int13_handler:

	.code16

	/* memory size in K that int13 handler uses. */

	.byte	((int13_handler_end - int13_handler + 0x3ff) / 0x400)

	/* 1-byte space reserved. */

	. = int13_handler + 0x2

ENTRY(atapi_dev_count)	.byte	0
ENTRY(min_cdrom_id)	.byte	0xE0

	. = int13_handler + 0x4

	/* Signature */
	.ascii	"G4DS"		# Please don't use this signature any longer.
				# This field might be used for other purposes.
				# Use signature at offset 0x103 instead.

	. = int13_handler + 0x08

	/* Version number */
	.word	1

	. = int13_handler + 0x0A

VARIABLE(floppies_orig)
	.byte	0			/* original value at 0040:0010 */

	. = int13_handler + 0x0B

VARIABLE(harddrives_orig)
	.byte	0			/* original value at 0040:0075 */

	. = int13_handler + 0x0C

VARIABLE(ROM_int15)
	.long	0			/* original int15 vector */

	. = int13_handler + 0x10

	/* 12-byte space reserved. */

	. = int13_handler + 0x1C

VARIABLE(ROM_int13)
	.long	0		/* original int13 vector */

	. = int13_handler + 0x20	/* drive map table begins at 0x20 */

ENTRY(hooked_drive_map)
	.space	(DRIVE_MAP_SIZE + 1) * DRIVE_MAP_SLOT_SIZE

	/* 8-byte space reserved. */

	. = int13_handler + 0x100	/* real int13 handler entry at 0x100 */

int13_handler_entry:

	/* only handles
	 * 
	 * 1. memdrives or disk swap
	 * 2. memdirves can be of FD, HD and ISO.
	 * 3. disk swap can be of FD/HD.
	 *
	 */

	jmp	int13_handler_start

	. = int13_handler + 0x103

	/*******************************************************************/
	/* ----------------- SafeMBRHook structure begin ----------------- */

	.ascii	"$INT13SF"		/* Win9x use this! Don't touch! */

	.ascii	"GRUB4DOS"		/* 8-byte Vender ID */

	. = int13_handler + 0x113

VARIABLE(ROM_int13_dup)	/* Win9x Safe-MBR-Hook structure requires this! */

	.long	0

	. = int13_handler + 0x117

VARIABLE(safe_mbr_hook)

	.long	0x00000001		/* safe MBR hook flag */

	/* -----------------  SafeMBRHook structure end  ----------------- */
	/*******************************************************************/

	/* But Win9x may expect additional data after SafeMBRHook structure.
	 * This is undocumented, and mysterious. If this area is not what
	 * Win9x expected, Win9x could hang.
	 */

	. = int13_handler + 0x11B

	.long	0x00000001
	
	/* space reserved. */

	. = int13_handler + 0x120

VARIABLE(saved_pxe_ip)

	.long	0

	. = int13_handler + 0x124

VARIABLE(saved_pxe_mac)

	.byte	0, 0, 0, 0, 0, 0

	/* space reserved. */

	. = int13_handler + 0x140

	/* GDT used by int13_handler RAM disk (32-bit protected mode, PAE
	 * paging, and 64-bit long mode).
	 * Linear address will be set by set_int13_handler routine.
	 * int13_handler does not have to modify it.
	 */
	.align 16
MyGDT:
//gdt:
/* 0*/	.word	0	/* NULL entry. Can be used at will. */
gdtdesc:		/* 6 bytes used for gdt descriptor. */
	.word	MyGDTEnd - MyGDT - 1	/* GDT limit */
	/* The gdt linear base address will be adjusted by set_int13_handler,
	 * but not touched outside int13_handler. */
	.long	ABS(MyGDT)		/* GDT linear address */

/* 8*/	PM_DS16 = (. - MyGDT)	/* adjusted base = int13_handler */
	.long	0x0000FFFF, 0x00009300	/* 16-bit data 64K limit */
/*16*/	PM_DS32 = (. - MyGDT)
	.long	0x0000FFFF, 0x00CF9300	/* 32-bit data 4GB limit */

	/* The 16-bit code segment base will be adjusted by set_int13_handler,
	 * but not touched outside int13_handler. */
/*24*/	PM_CS16 = (. - MyGDT)	/* adjusted base = int13_handler */
	.long	0x0000FFFF, 0x00009B00	/* 16-bit code 64K limit */
/*32*/	LM_CS64 = (. - MyGDT)
	.long	0x0000FFFF, 0x00AF9B00	/* 64-bit code 4GB limit */

	/* This gdt entry is not used by int13_handler, but used outside. */
/*40*/	PM_CS32 = (. - MyGDT)
	.long	0x0000FFFF, 0x00CF9B00	/* 32-bit code 4GB limit */
MyGDTEnd:

OldGDT:
	/* The int13_handler only uses it as a temp space for the OldGDT
	 * variable. Only the last 6 bytes are used by OldGDT, and the
	 * starting 2 bytes are not used by OldGDT. */
	//.word	0
int13_is64bit:
	.byte	0	// bit0 1=support PAE. bit1 1=support AMD64 long mode.
int13_need_64_bit:
int13_prev_DL:		// used in 64-bit code
	.byte	0
OldGDTdesc:
	.word	0
	.long	0

	.align 4
int13_prev_CR0:
	.long	0
int13_prev_CR3:
old_cr3:
	.long	0
int13_prev_CR4:
old_cr4:
	.long	0
int13_prev_PML4T_entry:
	.long	0, 0
int13_prev_PDPT_entry:
	.long	0, 0
int13_prev_PDT_entry:
	.long	0, 0
	.long	0, 0
	.long	0, 0

data_addr_above_1M:
	.long	0
	.long	0
data_addr_below_1M:
	.long	0
//	.long	0

	.align	4

int13_old_cs_ip:	.long	0
int13_old_eax:		.long	0
int13_old_ebx:		.long	0
int13_old_ecx:		.long	0
int13_old_edx:		.long	0
int13_old_esi:		.long	0
int13_old_edi:		.long	0
int13_old_esp:		.long	0
int13_old_ebp:		.long	0
int13_old_ds:		.word	0
int13_old_es:		.word	0
int13_old_flags:	.word	0
int13_new_bp:		.word	0

int13_handler_start:

	.code16

	/* backup far return address */
	popl	%cs:(int13_old_cs_ip - int13_handler)

	/* backup old flags */
	popw	%cs:(int13_old_flags - int13_handler)

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	cld
	movl	%eax, %cs:(int13_old_eax - int13_handler)
	movl	%ebx, %cs:(int13_old_ebx - int13_handler)
	movl	%ecx, %cs:(int13_old_ecx - int13_handler)
	movl	%edx, %cs:(int13_old_edx - int13_handler)
	movl	%esi, %cs:(int13_old_esi - int13_handler)
	movl	%edi, %cs:(int13_old_edi - int13_handler)
	movl	%esp, %cs:(int13_old_esp - int13_handler)
	movl	%ebp, %cs:(int13_old_ebp - int13_handler)
	movw	%ds, %cs:(int13_old_ds - int13_handler)
	movw	%es, %cs:(int13_old_es - int13_handler)

/****************************************************************************/
	/* find the drive number from the drive map */
	movw	$(EXT_C(hooked_drive_map) - int13_handler - DRIVE_MAP_SLOT_SIZE), %bp
1:
	addw	$DRIVE_MAP_SLOT_SIZE, %bp
	movw	%bp, %cs:(int13_new_bp - int13_handler)
	cmpw	$(EXT_C(hooked_drive_map) - int13_handler + (DRIVE_MAP_SIZE * DRIVE_MAP_SLOT_SIZE)), %bp
	jnb	2f
	movl	%cs:(%bp), %eax		/* FROM, TO, Hmax, Smax */

	/* check if this is the end */
	testl	%eax, %eax
	jnz	3f			/* not end, continue */

	movl	%cs:12(%bp), %eax	/* StartLBA_Hi */
	testl	%eax, %eax
	jnz	3f			/* not end, continue */

	movl	%cs:8(%bp), %eax	/* StartLBA_Lo */
	testl	%eax, %eax
	jnz	3f			/* not end, continue */

	movl	%cs:20(%bp), %eax	/* S_count_Hi */
	testl	%eax, %eax
	jnz	3f			/* not end, continue */

	movl	%cs:16(%bp), %eax	/* S_count_Lo */
	testl	%eax, %eax
	jz	2f		/* map whole drive to itself signals the end */
3:
	/* Now this is a valid drive map slot */
	cmpb	%cs:(%bp), %dl	/* check if this matches the drive number */
	jne	1b		/* no, continue to check the next map */

	/* yes, found the map corresponding to drive DL */

	movw	%cs:2(%bp), %ax		/* AL=Hmax, AH=Smax */

	/* non-zero StartLBA signals emulation */

	cmpl	$0, %cs:12(%bp)		/* StartLBA_Hi */
	jnz	drive_emulation
	cmpl	$0, %cs:8(%bp)		/* StartLBA_Lo */
	jnz	drive_emulation

	/* StartLBA == 0 */

	/* S_count being not 1 signals emulation */

	movl	%cs:20(%bp), %eax	/* S_count_Hi */
	testl	%eax, %eax
	jnz	drive_emulation
	movl	%cs:16(%bp), %eax	/* S_count_Lo */
	shrl	$1, %eax
	jnz	drive_emulation

	/* now StartLBA=0 and sector count=1(for whole disk) */

	/* if sectors per track > 1, this is force geometry screw. */

	movw	%cs:2(%bp), %ax		/* AL=Hmax, AH=Smax */
	testb	$62, %ah	/* Sectors > 1 means force geom, this -- */
	jnz	drive_emulation	/* -- also leads to drive emulation */

	/* ignore geom and directly map a whole drive */

	/* map a whole drive, normal access */

	movb	%cs:1(%bp), %dl	/* Let DL access TO instead of FROM */

2:
	/* might map to itself, i.e., actually not mapped */

	movl	%cs:(int13_old_eax - int13_handler), %eax
	movl	%cs:(int13_old_ebp - int13_handler), %ebp  /* BP changed!! */

	call	backup_int13

	pushw	%cs:(int13_old_flags - int13_handler)
	popfw

	int	$0x13

	/* save the returned CF flag to int13_old_flags */

	jnc	1f
	orb	$1, %cs:(int13_old_flags - int13_handler)
	jmp	2f
1:
	andb	$0xFE, %cs:(int13_old_flags - int13_handler)
2:
	call	restore_int13

	/* restore BP!! */
	movw	%cs:(int13_new_bp - int13_handler), %bp
	xchgw	%ax, %cs:(int13_old_eax - int13_handler)
				/* old AX changed!! */

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	pushw	%cs:(int13_old_flags - int13_handler)
	popfw

	/* check if should restore(reversely map) the drive number */
	jc	1f		/* restore DL on error */

	cmpb	$0x08, %ah	/* int13 AH=08h, read drive parameters */
	jne	2f

	/* DL==number of drives, should not restore */
	xorw	%ax, %ax
	movw	%ax, %ds
	movb	0x475, %dl	/* DL=number of hard drives */
	testb	$0x80, %cs:(int13_old_edx - int13_handler) /* harddrive? */
	jnz	3f		/* yes, jump */
	//movw	$0x10, %bx	/* for floppy, refer to function 0x48 */
	movb	0x410, %al
	rorb	$1, %al
	cbw
	shlb	$1, %al
	shrb	$6, %al
	incw	%ax
	andb	%ah, %al
	movb	%al, %dl	/* DL=number of floppy drives */
	//lesw	0x0078, %di	/* point to int 1E floppy parameters */
	jmp	3f
2:
	cmpb	$0x15, %ah	/* read drive type */
	jne	2f
	testb	$0x80, %cs:(int13_old_edx - int13_handler) /* harddrive? */
	jnz	3f		/* yes, do not restore DL */
	/* restore DL for floppy int13 AH=15h call */
	jmp	1f
2:
	movw	%cs:(%bp), %ax	/* get the drive mapping */
	/* try to restore DL if possible */
	cmpb	%al, %ah	/* check if the mapping was performed */
	je	3f		/* not performed, so need not restore DL */
	cmpb	%dl, %ah
	jne	3f		/* DL changed by int13, so do not restore */
1:
	movb	%cs:(int13_old_edx - int13_handler), %dl	/* restore DL back to the FROM drive */
3:
	/* return */

	/* BX, CX, DX, ES, DI are output registers and should not touch. */

	pushw	%cs:(int13_old_flags - int13_handler)
	popfw

	movl	%cs:(int13_old_esi - int13_handler), %esi
	movw	%cs:(int13_old_ds - int13_handler), %ds
	movl	%cs:(int13_old_eax - int13_handler), %eax
	movl	%cs:(int13_old_ebp - int13_handler), %ebp
	movw	%cs:(int13_old_esp - int13_handler), %sp
	ljmp	*%cs:(int13_old_cs_ip - int13_handler)

/****************************************************************************/
drive_emulation:
	movw	%cs:(int13_old_eax - int13_handler), %ax

	testb	%ah, %ah	/* reset disk system, always succeed */
	jnz	1f
	/*clc*/			/* CF already cleared by TEST */
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x01, %ah	/* get status, always succeed */
	jnz	1f
	xorb	%ah, %ah
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x04, %ah	/* verify sectors, always succeed */
	jnz	1f
	xorb	%ah, %ah
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x09, %ah /* INITIALIZE CONTROLLER WITH DRIVE PARAMETERS */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */
	xorb	%ah, %ah
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x0c, %ah	/* SEEK TO CYLINDER */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */
	xorb	%ah, %ah
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x0d, %ah	/* reset hard disks */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */
	xorb	%ah, %ah
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x10, %ah	/* check if drive ready */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */
	xorb	%ah, %ah
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x11, %ah	/* recalibrate drive */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */
	xorb	%ah, %ah
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x14, %ah	/* CONTROLLER INTERNAL DIAGNOSTIC */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */
	xorb	%ah, %ah
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x05, %ah	/* format track */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */

	/* CH=cylinder number (bits 8,9 in high bits of CL)
	 * CL=high bits of cylinder number (bits 7,6)
	 * DH=head number
	 * DL=drive number
	 */

	xorb	%ah, %ah	/* do nothing but return success */
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x08, %ah	/* get drive parameters */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */
	pushw	%ds
	xorw	%ax, %ax
	movw	%ax, %ds
	testb	%dl, %dl	/* hard drive? */
	movb	0x475, %dl	/* DL=number of hard drives */
	js	2f		/* yes, jump */
	movw	$0x10, %bx	/* for floppy, refer to function 0x48 */
	movb	0x410, %al
	rorb	$1, %al
	cbw
	shlb	$1, %al
	shrb	$6, %al
	incw	%ax
	andb	%ah, %al
	xchgw	%ax, %dx	/* DL=number of floppy drives */
	lesw	0x0078, %di	/* point to int1E floppy parameters */
2:
	popw	%ds
	movw	%cs:2(%bp), %ax	/* AL=Hmax, AH=Smax */
	movb	%al, %dh	/* max head number */
	andb	$63, %ah
	movb	%ah, %cl	/* max sector number */

	/* max cylinder number */

	pushl	%edx
	pushl	%ecx
	movzbl	%dh, %eax
	movzbl	%cl, %ecx
	incl	%eax
	mull	%ecx		/* EDX=0, EAX=sectors per cylinder */
	xchgl	%eax, %ecx

	movl	%cs:16(%bp), %eax	/* S_count_Lo */
	cmpl	$4, %eax
	ja	2f
	cmpl	$0, %cs:8(%bp)		/* StartLBA_Lo */
	jnz	2f
	/* map whole drive, use TO_C instead. */
	movw	%cs:4(%bp), %ax		/* TO_C */
	jmp	3f
2:
	decl	%eax
	//xorl	%edx, %edx
	divl	%ecx		/* EAX=max cylinder number */
3:
	popl	%ecx
	popl	%edx

	movb	%al, %ch	/* low 8 bits of cylinder */
	shlb	$6, %ah		/* high 2 bits of cylinder */
	orb	%ah, %cl

	xorw	%ax, %ax
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x15, %ah	/* get disk type */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */
	testb	%dl, %dl	/* hard drive? */
	js	2f		/* yes, jump */
	movb	$0x02, %ah	/* removable drive with change-line support */
	clc
	jmp	int13_return
2:
	movb	$0x03, %ah	/* hard disk */

	/* CX:DX=total number of sectors */
	movw	%cs:16(%bp), %dx	/* lo word of S_count_Lo */
	movw	%cs:18(%bp), %cx	/* hi word of S_count_Lo */
	clc
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x16, %ah
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */
	xorb	%ah, %ah	/* AH=0 means disk not changed */
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x17, %ah	/* set floppy type for format */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */
	movb	$0x03, %al	/* 1.44M drive, 1.44M floppy */
	xorb	%ah, %ah
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x18, %ah	/* set media type for format */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */
	testb	%dl, %dl	/* hard drive? */
	js	error_01_invalid
	pushw	%ax
	xorw	%ax, %ax
	movw	%ax, %es
	movw	$0x0078, %di
	movl	%es:(%di), %eax
	movw	%ax, %di
	shrl	$0x10, %eax
	movw	%ax, %es
	popw	%ax
	xorb	%ah, %ah
	jmp	int13_return
/****************************************************************************/
1:
	/* Now AH is neither 0 nor 1 */

	testb	$0xfc, %ah	/* CHS read/write sectors */
	jnz	1f

	/* so AH is either 2(for read) or 3(for write) */

	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */

//	cmpw	$0x0301, %ax	/* is it write 1 sector? */
//	jne	2f
//	cmpw	$0x0001, %cx	/* write to cylinder 0, sector 1? */
//	jne	2f
//	cmpb	$0x01, %dh	/* write to head 0 or 1? */
//	ja	2f
//	/* cmpw	$0xaa55, %es:0x1fe(%bx)
//	je	2f */
//	testb	%dl, %dl
//	js	3f		/* protect hard disk head 0 and 1 */
//	testb	%dh, %dh	/* write to floppy head 0? */
//	jne	2f		/* no, write permitted */
//3:
//	testb	$0x40, %cs:7(%bp)	/* TO_S, bit 6=fake write(safeboot) */
//	jnz	readonly_fakewrite	/* fake the write */
//2:
	cmpb	$0x7F, %al	/* check if sectors exceed 127 */
	ja	error_01_invalid
	testb	%al, %al	/* read 0 sectors not allowed */
	jz	error_01_invalid
	testb	$63, %cl	/* beginning sector number 0 is invalid */
	jz	error_01_invalid
	movb	%cs:3(%bp), %ah	/* AH=Smax */
	andb	$63, %ah
	pushw	%cx
	andb	$63, %cl
	cmpb	%ah, %cl	/* CL should not > max sector number */
	popw	%cx
	ja	error_01_invalid
	movb	%cs:2(%bp), %ah	/* AH=Hmax */
	cmpb	%ah, %dh	/* DH should not > max head number */
	ja	error_01_invalid

	movw	%cs, %si	/* use SI as temp var, instead of AX */
	movw	%si, %ds	/* DS=CS */

	movw	$(EBIOS_disk_address_packet - int13_handler), %si
	movw	$0x0010, (%si)	/* Disk Address Packet length */
	movb	%al, 2(%si)	/* number of sectors to transfer */
	movw	%bx, 4(%si)	/* offset */
	movw	%es, 6(%si)	/* segment */
	xorl	%eax, %eax	/* EAX=0 */
	movb	%ah, 3(%si)	/* sectors_hi_reserved */
	movl	%eax, 12(%si)	/* zero out hi 32 bits */
	movb	%ch, %al	/* cylinder number lo 8 bits */
	movb	%cl, %ah	/* CL holds higher 2 bits */
	shrb	$6, %ah		/* AH lower holds the 2 bits */
				/* EAX=cylinder number, <1024 */
	pushl	%ebx		/* save EBX */

	xorl	%ebx, %ebx	/* EBX=0 */
	movb	%cs:2(%bp), %bl	/* BL=Hmax */
	incw	%bx		/* EBX=total heads, <=256 */

	pushl	%edx
	mull	%ebx		/* EDX=0, EAX=tracks before this cylinder */
	popl	%edx

	xorw	%bx, %bx	/* EBX=0 */
	movb	%dh, %bl	/* EBX=head number */
	addl	%ebx, %eax	/* EAX=tracks before this head */
	movb	%cs:3(%bp), %bl	/* Max sector number */
	andb	$63, %bl	/* EBX=sectors per track */

	pushl	%edx
	mull	%ebx		/* EDX=0, EAX=sectors before this head */
	popl	%edx

	movb	%cl, %bl	/* sector number */
	andb	$63, %bl
	decb	%bl
	addl	%ebx, %eax	/* EAX=lo 32 bits of logical sector number */
	movl	%eax, 8(%si)

	popl	%ebx		/* restore EBX */

disk_address_packet_ready:

	/* DS=CS */

	/* start-sector-number(LBA) in DAP is still for FROM_DRIVE */

/****************************************************************************/
	/*
	 * check if the request exceeds the boundary of the emulated disk.
	 *
	 * input:	DS:SI
	 * output:	CF=0, success, all sectors transferred
	 *		CF=1, failure, no sectors transferred
	 *		EAX changed
	 *
	 */
	cmpl	$0, %cs:20(%bp)		/* S_count_Hi */
	ja	2f
	cmpl	$1, %cs:16(%bp)		/* S_count_Lo */
	ja	2f
	cmpl	$0, %cs:12(%bp)		/* StartLBA_Hi */
	jne	2f
	cmpl	$0, %cs:8(%bp)		/* StartLBA_Lo */
	je	4f			/* map whole drive, no restrictions */
2:
	movl	%cs:20(%bp), %eax	/* S_count_Hi */
	cmpl	%eax, 12(%si)		/* requested StartLBA_Hi */
	ja	3f
	jb	2f
	movl	%cs:16(%bp), %eax	/* S_count_Lo */
	cmpl	%eax, 8(%si)		/* requested StartLBA_Lo */
	jnb	3f
2:
	pushl	%ebx
	subl	8(%si), %eax
	movl	%cs:20(%bp), %ebx	/* S_count_Hi */
	sbbl	12(%si), %ebx		/* EBX:EAX available sectors */
	testl	%ebx, %ebx		/* CF=0 */
	popl	%ebx
	jnz	4f
	pushl	%ebx
	movzwl	2(%si), %ebx		/* requested sectors */
	cmpl	%ebx, %eax
	popl	%ebx
//	jnb	4f
//3:
//	stc
4:
/****************************************************************************/

	jc	3f		/* no sectors to transfer, fail */

	/* adjust start-sector-number(LBA) to access TO_DRIVE */
	movl	%cs:8(%bp), %eax	/* StartLBA_Lo */
	addl	%eax, 8(%si)
	movl	%cs:12(%bp), %eax	/* StartLBA_Hi */
	adcl	%eax, 12(%si)

	/* set drive number(TO_DRIVE) and function number(EBIOS) */
	movb	%cs:1(%bp), %dl		/* DL=TO_DRIVE */
	movb	%cs:(int13_old_eax - int13_handler + 1), %ah
				/* 0x02=read, 0x03=write */
	orb	$0x40, %ah	/* 0x42=EXT_read, 0x43=EXT_write */

	call	real_int13_service

	///* restore original start-sector-number(in the DAP) */
	//pushfw
	//movl	%cs:8(%bp), %eax	/* StartLBA_Lo */
	//subl	%eax, 8(%si)
	//sbbl	$0, 12(%si)
	//popfw

	jc	3f			/* failure */

	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	4f			/* CDROM */
	movw	$(EBIOS_disk_address_packet - int13_handler), %si
	movw	%cs, %ax
	movw	%ax, %ds
	/* restore original start-sector-number(in the DAP) */
	movl	%cs:8(%bp), %eax	/* StartLBA_Lo */
	subl	%eax, 8(%si)
	movl	%cs:12(%bp), %eax	/* StartLBA_Hi */
	sbbl	%eax, 12(%si)
	//call	modify_boot_sectors
4:
	movb	%cs:(int13_old_eax - int13_handler), %al
	xorb	%ah, %ah
	jmp	4f			/* success */

3:
	movw	$0x400, %ax	/* no sectors transferred */
	/* write back sectors transferred to original DAP for LBA access */
	cmpb	$0x40, %cs:(int13_old_eax - int13_handler + 1)
	jb	4f		/* CF=1, function 02 or 03, not an LBA call */

	movw	%cs:(int13_old_esi - int13_handler), %si
	movw	%cs:(int13_old_ds - int13_handler), %ds

	/* DS:SI -> original DAP */
	movb	%al, 2(%si)
	stc				/* failure */
4:
	movl	%cs:(int13_old_edx - int13_handler), %edx
	movl	%cs:(int13_old_ebx - int13_handler), %ebx
	movl	%cs:(int13_old_ecx - int13_handler), %ecx
	movl	%cs:(int13_old_edi - int13_handler), %edi
	movw	%cs:(int13_old_es - int13_handler), %es
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x41, %ah	/* EBIOS installation check */
	jnz	1f
	cmpw	$0x55aa, %bx
	jnz	error_01_invalid
//	testb	%dl, %dl
//	jns	error_01_invalid
	movw	$0xaa55, %bx
	movb	$0x21, %ah	/* major version 2.1(EDD-1.1) */
	movw	$0x01, %cx	/* support functions 42h,43h,44h,47h,48h */
	clc
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x42, %ah	/* EBIOS read sectors */
	jz	2f
	cmpb	$0x43, %ah	/* EBIOS write sectors */
	jnz	1f

//	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
//	jz	2f
//
//	/* CDROM */
//	movb	$0x03, %ah	/* write protection */
//	stc			/* error */
//	jmp	int13_return
2:
	/* get old SI, disk address packet */
	movw	%cs:(int13_old_esi - int13_handler), %si
	movl	(%si), %eax	/* packet length, sectors, etc. */
	testb	%ah, %ah
	jnz	error_01_invalid
	testb	$0xf0, %al
	jz	error_01_invalid
	shrl	$16, %eax
	testw	$0xff80, %ax
	jnz	error_01_invalid
	testb	%al, %al
	jz	error_01_invalid

	/* copy disk address packet to EBIOS_disk_address_packet */

	pushw	%es
	pushw	%cx
	pushw	%di

	movw	%cs, %ax
	movw	%ax, %es	/* ES=CS */

	movw	$(EBIOS_disk_address_packet - int13_handler), %di
	movw	$8, %cx		/* will copy only 16 bytes! */
	cld
	repz movsw

	popw	%di
	popw	%cx
	popw	%es

	/* set DS:SI */

	movw	%cs, %ax
	movw	%ax, %ds	/* DS=CS */
	movw	$(EBIOS_disk_address_packet - int13_handler), %si

	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jz	2f
	shlb	$2, 2(%si)
	movl	8(%si), %eax
	shldl	$2, %eax, 12(%si)
	shll	$2, 8(%si)
2:
	//movw	4(%si), %bx
	//movw	6(%si), %es
	lesw	4(%si), %bx
	jmp	disk_address_packet_ready

/****************************************************************************/
1:
	cmpb	$0x44, %ah	/* EBIOS verify sectors */
	jnz	1f
	xorb	%ah, %ah
	clc
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x47, %ah	/* EBIOS seek */
	jnz	1f
	xorb	%ah, %ah
	clc
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x48, %ah	/* EBIOS GET DRIVE PARAMETERS */
	jnz	1f

	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jz	2f			/* normal disks */

	/* CDROM */
	/* get old SI, extended drive parameter table */
	movw	%cs:(int13_old_esi - int13_handler), %si
	cmpw	$26, (%si)
	jb	error_01_invalid

	movw	$26, (%si)	/* buffer length */
	movw	$0x00, 2(%si)		# no flags
	movw	$0x800, 24(%si)		# bytes per sect=2048
	xorl	%eax, %eax
	decw	%ax
	movl	%eax, 4(%si)		# cylinders=0xFFFF
	movb	$0, %ah
	movl	%eax, 8(%si)		# heads=0xFF
	movb	$15, %al
	movl	%eax, 12(%si)		# sectors per track=15
	movl	%eax, 20(%si)		# total sectors hi dword=0
	xorw	%ax, %ax		# CF cleared
	decl	%eax			# EAX=0xFFFFFFFF
	movl	%eax, 16(%si)		# total sectors lo dword
					# CF is cleared

	xorw	%ax, %ax	/* success, CF cleared */
	jmp	int13_return
2:
	/* get old SI, extended drive parameter table */
	movw	%cs:(int13_old_esi - int13_handler), %si
	movw	$26, (%si)	/* buffer length */
	movw	$2, 2(%si)	/* info */
	xorl	%eax, %eax
	movl	%eax, 8(%si)	/* total heads */
	movl	%eax, 12(%si)	/* sectors per track */
	movl	%eax, 16(%si)	/* total sectors */
	movl	%eax, 20(%si)	/* total sectors hi */
	pushl	%ebx
	xorl	%ebx, %ebx
	movw	%cs:2(%bp), %ax	/* AL=Hmax, AH=Smax */
	andb	$63, %ah
	movb	%ah, 12(%si)	/* sectors per track */
	movb	%ah, %bl
	xorb	%ah, %ah
	incw	%ax		/* total heads=Hmax+1 */
	movw	%ax, 8(%si)	/* total heads */
	pushl	%edx
	mulw	%bx		/* DX:AX=product, DX=0 */
	movw	%ax, %bx	/* BX=sectors per cylinder */
	movl	%cs:20(%bp), %edx	/* S_count_Hi */
	movl	%edx, 20(%si)	/* total sectors hi */
	movl	%cs:16(%bp), %eax	/* S_count_Lo */
	movl	%eax, 16(%si)	/* total sectors lo */
	/*xorl	%edx, %edx*/	/* EDX:EAX=64bit total sectors */
	testw	%bx, %bx
	jz	2f
	divl	%ebx		/* EAX=quotient, EDX=residue */
2:
	testl	%edx, %edx
	popl	%edx
	popl	%ebx
	jz	2f
	incl	%eax
2:
	movl	%eax, 4(%si)	/* total cylinders */
	movw	$512, 24(%si)	/* bytes per sector */
	xorb	%ah, %ah
	/*clc*/			/* signal success, CF already cleared by XOR */
	jmp	int13_return

/****************************************************************************/
1:
	cmpw	$0x4B01, %ax	/* CDROM GET DISK EMULATION STATUS */
	jnz	1f

	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jz	error_01_invalid	/* normal disks. no 0x4B01 function */

	/* CDROM */
	pushw	%es
	pushw	%di
	movw	%cs:(int13_old_esi - int13_handler), %di	/* old SI */
	pushw	%ds
	popw	%es
	movw	$0x0013, %ax	/* packet size=13h, boot type=0 (no-emu) */
	cld
	stosw
	movb	%dl, %al	/* drive=DL, controller=0 */
	stosw

	pushl	%cs:EXT_C(lba_cd_boot) - int13_handler
	popw	%ax
	stosw
	popw	%ax
	stosw			/* LBA for no-emu image */

	xorw	%ax, %ax
	stosw			/* device specification */
	stosw			/* user buffer segment */
	stosw			/* load segment */
	movb	$4, %al
	stosw			/* sector count=4 */
				/* CHS makes no sense for no-emu */
	popw	%di
	popw	%es
	xorb	%ah, %ah	/* success, CF cleared */
	jmp	int13_return

/****************************************************************************/
1:
/****************************************************************************/
error_01_invalid:
	movb	$0x01, %ah	/* unsupported function call */
	stc			/* signal error */

int13_return:
	movw	%ax, %cs:(int13_old_eax - int13_handler)	/* status */
	jnc	1f
	orb	$1, %cs:(int13_old_flags - int13_handler)
	jmp	2f
1:
	andb	$0xFE, %cs:(int13_old_flags - int13_handler)
2:
	pushw	%cs:(int13_old_flags - int13_handler)
	popfw

	movl	%cs:(int13_old_eax - int13_handler), %eax
	movl	%cs:(int13_old_esi - int13_handler), %esi
	movw	%cs:(int13_old_ds - int13_handler), %ds
	movl	%cs:(int13_old_ebp - int13_handler), %ebp
	movw	%cs:(int13_old_esp - int13_handler), %sp
	ljmp	*%cs:(int13_old_cs_ip - int13_handler)

/****************************************************************************/
real_int13_service:

	/* AH = 0x42 or 0x43 */

	/* save return address to memory */
	popw	%cs:(int13_ret_IP - int13_handler)
	/* save AX to memory */
	movw	%ax, %cs:(int13_reg_AX - int13_handler)

	cmpb	$0xff, %dl	/* mem drive */
	jne	normal_disk_drive

	testw	$0x4000, %cs:4(%bp)	/* TO_C bit 14=(TO is cdrom) */
	jnz	normal_disk_drive

	/* handle memdrive */

	sti		/* for hardware interrupt or watchdog */
	cmpb	$0x42, %ah
	je	3f
	cmpb	$0x43, %ah
	je	3f
normal_disk_drive:
	stc
	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret
	/* DS:SI points to disk address packet 	*/
	/* 	2(%si), byte, sectors(1-127) 	*/
	/* 	4(%si), word, offset 		*/
	/* 	6(%si), word, segment 		*/
	/* 	8(%si), qword, lba		*/
3:
	/* AH = 0x42 or 0x43 */
	movzwl	4(%si), %ebx	#;  BX=offset, EBX_high_word=0
	movzwl	6(%si), %edi	#;  DI=segment
	shll	$4, %edi	#; EDI=linear base address of segment
	addl	%ebx, %edi	#; EDI=linear address of BUFFER(below 1M)

	movl	12(%si), %ebx	#; EBX=LBA of MEMDRIVE SECTOR hi
	movl	8(%si), %ecx	#; ECX=LBA of MEMDRIVE SECTOR lo
	shldl	$9, %ecx, %ebx
	shll	$9, %ecx	#; EBX:ECX=linear address of SECTOR(above 1M)
	//xorl	%edx, %edx
	movl	%edi, %cs:(data_addr_below_1M - int13_handler)
	//movl	%edx, %cs:(data_addr_below_1M - int13_handler + 4)
	movl	%ecx, %cs:(data_addr_above_1M - int13_handler)
	movl	%ebx, %cs:(data_addr_above_1M - int13_handler + 4)
	testl	%ebx, %ebx
	setnz	%dl
	movzbl	2(%si), %ebx	# sectors to read/write
	shll	$9, %ebx	# bytes to read/write
	decl	%ebx
	addl	%ecx, %ebx
	setc	%dh
	orb	%dh, %dl
	movb	%dl, %cs:(int13_need_64_bit - int13_handler)

	/* raw mode as in memdisk, contributed by Bean */

	/* switch to protected mode myself */
	/* ebx destroy */

	/* EDI=linear address of BUFFER(below 1M) */
	/* ECX=linear address of SECTOR(above 1M), lower 32 bits */
	movzbl	2(%si), %ebx	# sectors to read/write
	shll	$9, %ebx	# bytes to read/write
	movl	%ebx, %edx	# bytes to read/write
	addl	%edi, %ebx	# point to the byte after the buffer
	decl	%ebx		# point to the last byte of the source
	orl	%edi, %ebx	# to see if the buffer is inside an even mega
	addl	%ecx, %edx	# point to the byte after the sectors
	decl	%edx		# point to the last byte of the sectors
	orl	%ecx, %edx	# to see if the sectors is inside an even mega
	orl	%ebx, %edx	# to see if both are in even megas
	andl	$0x100000, %edx	# zero means both are in even megas, DX=0
	setz	%dl		# we think of it as if A20 were on
	jz	3f		# no need to enable A20. DX=1.

	/* DX=0 */
	movw	$0x00ff, %cx	# try so many times on failure
	incw	%dx		# DL=1(enable A20), DH=0(debug off)

#if 1
	/* Turn on A20: only via System Control Port A (Fast A20) */
	inb	$0x92, %al
	testb	$02, %al
	jnz	1f
	orb	$02, %al		# enable A20
	andb	$0xfe, %al		# avoid reseting the CPU by accident
	outb    %al, $0x92		# some chips have only this
1:
	/* output a dummy command (USB keyboard hack) */
//	movb	$0xFF, %al
//	outb	%al, $0x64
#else
	cli	/* yes, keep interrupt off when controlling A20 */
	call	enable_disable_a20	# EAX, CX modified
	sti

	setc	%dl		# CF=1 means A20 was originally enabled.
	jz	3f		/* enabled A20 with success */

	/* A20 control failed. Notify the user. */
	movzbw	%cs:(%bp), %ax		/* FROM_DRIVE */
	pushw	%ax
	pushw	$a20_failure - int13_handler	# the format string
	call	realmode_printf
	addw	$4, %sp			# adjust the stack pointer
	stc
	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret

a20_failure:
	.ascii	"\r\ngrub4dos: A20 failure on memdrive=0x%X.\r\n\0"
#endif

3:
	/* EDI=linear address of BUFFER(below 1M) */
	xorl	%ebx, %ebx
	movw	$(EBIOS_disk_address_packet - int13_handler), %si
	movw	%cs, %bx
	movw	%bx, %ds

	// GDT is initialized by set_int13_handler
	//shll	$4, %ebx
	//addl	$(MyGDT - int13_handler), %ebx
	//movl	%ebx, %cs:MyGDT - int13_handler + 4

	movzbl	2(%si), %ecx	#; ECX=number of sectors to transfer
	shll	$7, %ecx	/* 128 dwords per sector */
	cld

	sgdtl	%cs:(OldGDTdesc - int13_handler)
	lgdt	%cs:(gdtdesc - int13_handler)
//	testb	$0xFF, %cs:(int13_need_64_bit - int13_handler)	# need 64 bit?
//	jnz	move_block_using_64_bit			# yes
	//jmp	move_block_using_64_bit		/* only test 64-bit code */

	/*********************************/
	/* normal 32-bit(4GB) block move */
	/*********************************/

	/* EDI=linear address of BUFFER(below 1M) */

	//movl	%cs:(data_addr_below_1M - int13_handler), %edi
	movl	%cs:(data_addr_above_1M - int13_handler), %esi
	testb	$1, %cs:(int13_reg_AX - int13_handler + 1)	# AH
	jz	3f						# read
	xchgl	%esi, %edi
3:
				#; ESI changed!!
	movl	%cr0, %eax
	//andl	$0x7FFFFFFF, %eax
	orb	$1, %al		// set CR0.PE(bit0)

	cli
	movl	%eax, %cr0	/* Switch to protected mode */

	movw	$(PM_DS32), %bx	/* Switch to 4G data segment */
	movw	%bx, %ds
	movw	%bx, %es

	addr32	rep movsl	/* ESI, EDI changed! */

	/* Don't switch back to 64K limit for DS and ES. Keeping 4G limit
	 * would hurt nothing(in principle). There are softwares which run
	 * initially with DS and ES able to access 4G memory. They call
	 * int13 and expect on return the DS and ES could still access 4G
	 * memory. So we should not forcibly set the limit to a lower value
	 * of 64K. -- tinybit
	 */
#if 0
	movw	$(PM_DS16), %bx	// 64KB data segment limit
	movw	%bx,  %ds
	movw	%bx,  %es
#endif

	//movl	%cr0, %eax	/* EAX not touched */
	andb	$0xFE, %al	// reset CR0.PE(bit0)
	movl	%eax, %cr0	// back to real mode

	/* no problem if we skip this and save some bytes of code. */
#if 0
	movw	%cs,  %bx
	movw	%bx,  %ds
	movw	%bx,  %es
#endif

move_block_finished:
	lgdtl	%cs:(OldGDTdesc - int13_handler)

	sti

	clc
	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret

/****************************************************************************/
	.align	2
int13_ret_IP:
	.word	0
int13_reg_AX:
	.word	0
int13_chs_cx:
	.word	0
int13_chs_dx:
	.word	0

	.align	4
int13_chs_edi:
	.long	0

/****************************************************************************/
backup_int13:

	/* backup the current int 13 to tmp_int13, then install ROM_int13 */
	cli
	pushw	%ds
	pushl	%eax
	xorw	%ax, %ax
	movw	%ax, %ds
	movl	0x4C, %eax	/* current int 13 vector */
	movl	%eax, %cs:(tmp_int13 - int13_handler)
	movl	%cs:(EXT_C(ROM_int13) - int13_handler), %eax
	movl	%eax, 0x4C
	popl	%eax
	popw	%ds
	ret

restore_int13:

	/* restore the current int 13 from tmp_int13 */
	cli
	pushw	%ds
	pushl	%eax
	xorw	%ax, %ax
	movw	%ax, %ds
	movl	%cs:(tmp_int13 - int13_handler), %eax
	movl	%eax, 0x4C
	popl	%eax
	popw	%ds
	ret

//force_int13:
//	.byte	0

	.align	2
int13_retry_IP:
	.word	0
int13_retry_ds:
	.word	0
int13_retry_es:
	.word	0

	.align	4
int13_retry_eax:
	.long	0
int13_retry_ebx:
	.long	0
int13_retry_ecx:
	.long	0
int13_retry_edx:
	.long	0
int13_retry_esi:
	.long	0
int13_retry_edi:
	.long	0
int13_retry_ebp:
	.long	0
tmp_int13:
	.long	0
//sector_last_dword:
//	.long	0

/****************************************************************************/

	.align	4
int13_bak_cs_ip:	.long	0
int13_bak_eax:		.long	0
int13_bak_ebx:		.long	0
int13_bak_ecx:		.long	0
int13_bak_edx:		.long	0
int13_bak_esi:		.long	0
int13_bak_edi:		.long	0
int13_bak_ebp:		.long	0
int13_bak_ds:		.word	0
int13_bak_es:		.word	0
int13_bak_flags:	.word	0

/****************************************************************************/
	.align	4

	/* LBA of no-emulation boot image, in 2048-byte sectors */
ENTRY(lba_cd_boot)	.long	0

minimum_mem_lo_in_map:
	.long	0	/* min ram drive base below 16M */
minimum_mem_hi_in_map:
	.long	0	/* min ram drive base above 16M */

system_active:
	.byte	0

/****************************************************************************/
int15_e820_handler:

	/* Comments are mostly gotten from Ralf Brown's Interrupt List. */

	cmpb	$0x85, %ah
	jne	1f
	cmpb	$0, %al
	je	2f		/* ignore the key pressed */

	/* SysRq key released */

	cmpb	$0, %cs:(system_active - int13_handler)
	je	enter_system
2:
	xorb	%ah, %ah
	//clc
	lret	$2
1:
	cmpw	$0xe820, %ax		//cmpl	$0x0000e820, %eax
	jne	1f

	/* Newer BIOSes - GET SYSTEM MEMORY MAP */

	//AX = E820h
	//EAX = 0000E820h
	//EDX = 534D4150h ('SMAP')
	//EBX = continuation value or 00000000h to start at beginning of map
	//ECX = size of buffer for result, in bytes (should be >= 20 bytes)
	//ES:DI -> buffer for result

	//Return:
	//CF clear if successful
	//EAX = 534D4150h ('SMAP')
	//ES:DI buffer filled
	//EBX = next offset from which to copy or 00000000h if all done
	//ECX = actual length returned in bytes
	//CF set on error
	//AH = error code (86h)

	/* Notes: Originally introduced with the Phoenix BIOS v4.0, this
	 * function is now supported by most newer BIOSes, since various
	 * versions of Windows call it to find out about the system memory.
	 * A maximum of 20 bytes will be transferred at one time, even if ECX
	 * is higher; some BIOSes (e.g. Award Modular BIOS v4.50PG) ignore the
	 * value of ECX on entry, and always copy 20 bytes. Some BIOSes expect
	 * the high word of EAX to be clear on entry, i.e. EAX=0000E820h. If
	 * this function is not supported, an application should fall back to
	 * AX=E802h, AX=E801h, and then AH=88h. The BIOS is permitted to return
	 * a nonzero continuation value in EBX and indicate that the end of the
	 * list has already been reached by returning with CF set on the next
	 * iteration. This function will return base memory and ISA/PCI memory
	 * contiguous with base memory as normal memory ranges; it will
	 * indicate chipset-defined address holes which are not in use and
	 * motherboard memory-mapped devices, and all occurrences of the system
	 * BIOS as reserved; standard PC address ranges will not be reported
	 */

//Format of Phoenix BIOS system memory map address range descriptor:
//
//Offset Size	Description
//------ -----	----------------------
//  00h  QWORD	base address
//  08h  QWORD	length in bytes
//  10h  DWORD	type of address range

//Values for System Memory Map address type:
//01h memory, available to OS
//02h reserved, not available (e.g. system ROM, memory-mapped device)
//03h ACPI Reclaim Memory (usable by OS after reading ACPI tables)
//04h ACPI NVS Memory (OS is required to save this memory between NVS sessions)
//other not defined yet -- treat as Reserved

	cmpl	$0x534D4150, %edx	/* "SMAP" */
	jne	1f
	cmpl	$20, %ecx
	jb	2f

	pushfw
	lcall	%cs:*(EXT_C(ROM_int15) - int13_handler)

	jc	3f
	cmpl	$0x534D4150, %eax	/* "SMAP" */
	jne	2f
	pushal
	movl	%es:16(%di), %eax	/* Type */
	decl	%eax /* 1=usable memory, available to the operating system */
	jnz	4f
	/* set %si to the drive map */
	movw	$(EXT_C(hooked_drive_map) - int13_handler), %si
	movw	$(DRIVE_MAP_SIZE + 1), %cx
6:
	/* the code should not change SI, DI and CX! */
	cmpb	$0xff, %cs:1(%si)
	jne	5f			/* not memdrive */
	testb	$0x40, %cs:5(%si)	/* TO_C bit 14=(TO is cdrom) */
	jnz	5f			/* not memdrive */
	movl	%cs:12(%si), %edx	/* start_sector hi */
	movl	%cs:8(%si), %eax	/* start_sector lo */
	shldl	$9, %eax, %edx
	shll	$9, %eax		/* EDX:EAX=start_address */
	subl	%es:(%di), %eax
	sbbl	%es:4(%di), %edx	/* EDX:EAX=new length */
	jb	5f			/* start_address is below the base */
	subl	%es:8(%di), %eax
	sbbl	%es:12(%di), %edx	/* EDX:EAX=difference */
	jnb	5f			/* new length is too big */
	addl	%eax, %es:8(%di)
	adcl	%edx, %es:12(%di)	/* apply new length */

5:
	/* try next slot */
	addw	$DRIVE_MAP_SLOT_SIZE, %si
	loop	6b
4:
	popal
	clc
	lret	$2

2:
	movb	$0x86, %ah	/* function not supported */
3:
	stc
	lret	$2
#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
1:
	ljmp	%cs:*(EXT_C(ROM_int15) - int13_handler)

leave_system:

	/* clear system active flag */
	decb	%cs:(system_active - int13_handler)
	xorb	%ah, %ah

enter_system:
	/* set system active flag */
	incb	%cs:(system_active - int13_handler)
	/* save all */


	
/* EBIOS_disk_address_packet should be at the end of the handler because some
 * buggy BIOSes could destroy the memory that immediately follows.
 */

EBIOS_disk_address_packet:
	.byte	0x10	/* packet size, 16 or more */
	.byte	0	/* reserved, must be 0 */
	.byte	0	/* number of sectors, must be from 1 to 127 */
	.byte	0	/* reserved, must be 0 */
	.word	0	/* displacement of memory address */
	.word	0	/* segment of memory address */
	.long	0	/* 64bit, start logical sector number */
	.long	0

/* Don't insert code or data here! Buggy BIOSes could overwrite this area! */

//	.align	16

//	.space	0x1000	/* 4KB stack */

int13_handler_end:

	/* Total size of int13_handler should better not exceed 12KB. */

	. = . - (int13_handler_end - int13_handler)/0x3001

#endif

/****************************************************************************/

/*
 * long chain_stage1(long segment_offset)
 *
 *  This starts another stage1 loader, at segment_offset.
 */

ENTRY(chain_stage1)

	.code32

	pushl	%ebp
	movl	%esp, %ebp
	pushal
	pushal
	pushal
	pushal
	movl	%esp, ABS(chain_stage1_esp)

	/* set up to pass boot drive */
	movb	ABS(EXT_C(current_drive)), %dl
	movb	ABS(EXT_C(current_partition))+2, %dh

	call	prot_to_real

	.code16

	sti

	lcall	*8(%bp)
	//lcall	$0x2000, $0

	call	real_to_prot

	.code32

	movl	ABS(chain_stage1_esp), %esp
	popal
	popal
	popal
	popal
	popl	%ebp
	ret

chain_stage1_esp:
	.long	0

/****************************************************************************/

/*
 * long realmode_run(long regs_ptr)
 *
 *  This starts another stage1 loader, at segment_offset.
 */

ENTRY(realmode_run)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	movl	8(%ebp), %ebx		/* EBX=regs_ptr */

	/* check sanity */

	xorl	%eax, %eax		/* EAX=0 for failure */

	movl	0x20(%ebx), %ecx	/* GS */
	incl	%ecx
	jz	1f
	decl	%ecx
	shrl	$16, %ecx
	jnz	2f			/* failure */
1:

	movl	0x24(%ebx), %ecx	/* FS */
	incl	%ecx
	jz	1f
	decl	%ecx
	shrl	$16, %ecx
	jnz	2f			/* failure */
1:

	movl	0x28(%ebx), %ecx	/* ES */
	incl	%ecx
	jz	1f
	decl	%ecx
	shrl	$16, %ecx
	jnz	2f			/* failure */
1:

	movl	0x2C(%ebx), %ecx	/* DS */
	incl	%ecx
	jz	1f
	decl	%ecx
	shrl	$16, %ecx
	jnz	2f			/* failure */
1:

	movl	0x30(%ebx), %ecx	/* SS */
	movl	0x0C(%ebx), %edx	/* ESP */
	incl	%ecx
	jz	1f
	decl	%ecx
	orl	%edx, %ecx
	shrl	$16, %ecx
	jnz	2f			/* failure */
	jmp	3f
1:
	incl	%edx
	jnz	2f			/* failure */
3:

	movl	0x38(%ebx), %ecx	/* CS */
	movl	0x34(%ebx), %edx	/* EIP */
	incl	%ecx
	jz	1f
	decl	%ecx
	orl	%edx, %ecx
	shrl	$16, %ecx
	jnz	2f			/* failure */
	jmp	3f
1:
	cmpb	$0xCD, %dl
	jnz	2f			/* failure */
	shrl	$16, %edx
	incw	%dx
	jnz	2f			/* failure */
3:
	/* sanity check ok, run it! */

	pushal
	pushal
	pushal
	pushal
	movl	%esp, ABS(chain_stage1_esp)

	/* mov the struct to stack */
	subl	$64, %esp
	movl	%esp, %edi
	movl	%ebx, %esi
	movl	$16, %ecx
	cld
	repz movsl

	movl	%esp, %ebp	/* EBP point to struct on stack */

	call	prot_to_real

	.code16

	sti

	movl	%ebp, %esp

	/* check CS:EIP */

	movl	0x38(%bp), %ecx		/* CS */
	movl	0x34(%bp), %edx		/* EIP */
	incl	%ecx			/* CS = -1? */
	jz	3f			/* yes, jump to INTxx code */

	/* run user defined code */
	decl	%ecx			/* CS */
	/* 0xEA might have been changed to 0xCD previously, so we must restore
	 * it back to 0xEA. 
	 */
	movb	$0xEA, ABS(1f-1)	/* opcode for "far jump" */
	movw	%dx, ABS(1f)		/* IP in the far jump instruction */
	movw	%cx, ABS(1f + 2)	/* CS in the far jump instruction */

	/* set return address for user code to return */
	movl	$ABS(4f), %ecx
	movl	%ecx, 0x8201
	jmp	6f
3:
	/* run int xx */
	movw	%dx, ABS(1f - 1)	/* int XX */
	movb	$0xE9, ABS(1f + 1)	/* jmp */
	movw	$(4f - 1f - 4), ABS(1f + 2)
6:
	movw	%sp, ABS(8f)		/* initial SP */
	movw	%ss, ABS(7f)		/* initial SS */

	/* set DS, ES, FS, GS */
	movl	0x20(%bp), %ecx		/* GS */
	incl	%ecx
	jz	5f
	decl	%ecx
	movw	%cx, %gs
5:

	movl	0x24(%bp), %ecx		/* FS */
	incl	%ecx
	jz	5f
	decl	%ecx
	movw	%cx, %fs
5:

	movl	0x28(%bp), %ecx		/* ES */
	incl	%ecx
	jz	5f
	decl	%ecx
	movw	%cx, %es
5:

	movl	0x2C(%bp), %ecx		/* DS */
	incl	%ecx
	jz	5f
	decl	%ecx
	movw	%cx, %ds
5:

	movl	0x30(%bp), %ecx		/* SS */
	incl	%ecx
	jz	5f
	decl	%ecx
	movw	%cx, ABS(7f)
	movw	0x0C(%bp), %dx		/* SP */
	movw	%dx, ABS(8f)
5:
	movl	0x3C(%bp), %ecx		/* EFLAGS */
	incl	%ecx
	jz	5f
	decl	%ecx
	pushl	%ecx
	popfl
5:
	popal				/* setup general registers */

	/* move SS and SP */
	.byte	0xBC			/* MOV SP */
7:
	.word	0			/* SS */
	movw	%sp, %ss
	.byte	0xBC			/* MOV SP */
8:
	.word	0			/* SP */

	/* far jump to user code */
	.byte	0xEA			/* ljmp */
1:
	.word	0			/* IP */
	.word	0			/* CS */

4:	/* return from user code */

	/* CS:IP need not care */

	/* use the stack already built by the user */

	pushal
	pushfl
	pushl	%cs
	pushl	$0
	pushl	%ss
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	movl	%ss, %ebp
	shll	$4, %ebp
	xorl	%eax, %eax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%sp, %ax
	addl	%eax, %ebp		/* physical address of stack */

	call	real_to_prot

	.code32

	cld
	movl	%ebp, %esi

	movl	ABS(chain_stage1_esp), %esp
	movl	16(%esp), %ebx		/* EBX=regs_ptr */

	leal	32(%ebx), %edi
	movl	$8, %ecx
	repz movsl

	leal	32(%ebp), %esi
	movl	%ebx, %edi
	movl	$8, %ecx
	repz movsl

	/* the stack should not change! */

	/* fix the pushed ESPs before checking */
	movl	$32, %ecx
	subl	%ecx, (32 + 12)(%esp)
	movl	$64, %ecx
	subl	%ecx, (64 + 12)(%esp)
	movl	$96, %ecx
	subl	%ecx, (96 + 12)(%esp)

	movl	%esp, %esi
	leal	32(%esp), %edi
	movl	$8, %ecx
	repz cmpsl
	jnz	1f
	movl	%esp, %esi
	leal	64(%esp), %edi
	movl	$16, %ecx
	repz cmpsl
	jnz	1f
	popal
	popal
	popal
	popal
	movl	$1, %eax		/* success */
	popl	%ebx
	popl	%ebp
	ret
1:
	popal
	popal
	popal
	popal
	xorl	%eax, %eax		/* failure */
2:
	popl	%ebx
	popl	%ebp
	ret

/****************************************************************************/

/*
 * linux_boot()
 *
 * Does some funky things (including on the stack!), then jumps to the
 * entry point of the Linux setup code.
 */

ENTRY(linux_boot)

	.code32

	/* don't worry about saving anything, we're committed at this point */

	movl	EXT_C(linux_data_real_addr), %ebx

	/* copy kernel text */
	movl	EXT_C(chain_load_from), %esi
	movl	EXT_C(chain_load_to), %edi
	movl	EXT_C(chain_load_dword_len), %ecx
	cld
	repz movsl

	/* change %ebx to the segment address */
	shrl	$4, %ebx	#; CS of LINUX SETUP, high word = 0
	movl	%ebx, %eax
	addl	$0x20, %eax	#; one sector
	movw	%ax, ABS(1f)	// linux_setup_seg

	/* final setup for linux boot */

	call	prot_to_real

	.code16

	/* final setup for linux boot */
	cli
	movw	%bx, %ss
	movw	$0x9000, %sp	# LINUX_SETUP_STACK

	movw	%bx, %ds
	movw	%bx, %es
	movw	%bx, %fs
	movw	%bx, %gs

	/* jump to start of setup */

	.byte	0xea		# ljmp
	.word	0
1:				//linux_setup_seg:
	.word	0

real_to_prot:

	.code16

	cli

	/* load the GDT register */
	xorw	%ax, %ax
	movw	%ax, %ds
	lgdt	ABS(gdtdesc)

	/* turn on protected mode */
	movl	%cr0, %eax
	orb	$1, %al
	movl	%eax, %cr0

	/* jump to relocation, flush prefetch queue, and reload %cs */
	ljmp	$PROT_MODE_CSEG, $ABS(protcseg)

	/*
	 *  The ".code32" directive only works in GAS, the GNU assembler!
	 *  This gets out of "16-bit" mode.
	 */

	.code32

protcseg:
	/* reload other segment registers */
	movw	$PROT_MODE_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* zero %eax */
	xorl	%eax, %eax

	/* return on the old (or initialized) stack! */
	.byte	0x66	# data32	/* YES!! only a 16-bit RET!! */
	ret

prot_to_real:

	.code32

	/* just in case, set GDT */
	lgdt	ABS(gdtdesc)

	/* set up segment limits */
	movw	$PSEUDO_RM_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* this might be an extra step */
	ljmp	$PSEUDO_RM_CSEG, $ABS(tmpcseg)	/* jump to a 16-bit segment */

tmpcseg:
	.code16

	/* clear the PE bit of CR0 */
	movl	%cr0, %eax
	andb 	$0xFE, %al
	movl	%eax, %cr0

	/* flush prefetch queue, reload %cs */
	ljmp	$0, $ABS(realcseg)

realcseg:
	/* we are in real mode now
	 * set up the real mode segment registers : DS, SS, ES
	 */
	/* zero %eax */
	xorl	%eax, %eax

	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* restore interrupts */
	/* oh, don't enable interrupt when we are controlling gateA20 */
	//sti

	/* return on new stack! */
	DATA32	ret		/* 32-bit RET!! */


/*
 *   int biosdisk_int13_extensions (int ax, int drive, void *dap)
 *
 *   Call IBM/MS INT13 Extensions (int 13 %ax=AX) for DRIVE. DAP
 *   is passed for disk address packet. If an error occurs, return
 *   non-zero, otherwise zero.
 */

ENTRY(biosdisk_int13_extensions)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	#; +16	dap
	#; +12	drive
	#;  +8	 ax
	#;  +4	EIP
	#; ebp	EBP
	#;  -4	ESI
	#;  -8	EBX

	pushl	%esi
	pushl	%ebx
	pushl	%ecx
	pushl	%edx

	/* compute the address of disk_address_packet */
	movl	0x10(%ebp), %eax	#; linear address of dap

	/* if DS can be 0x40, we can avoid AWARD BIOS bug of int13/AX=4B01 */
	subl	$0x400, %eax

	shll	$1, %eax
	movw	%ax, %si
	shrw	$1, %si			#; low 15-bit for offset

	xorw	%ax, %ax
	shrl	$5, %eax		#; segment value in AX
	addw	$0x40, %ax
	movw	%ax, %cx	/* save the segment to cx */

	/* drive */
	movb	0xc(%ebp), %dl
	/* ax */
	movw	0x8(%ebp), %bx
	/* enter real mode */
	call	prot_to_real

	.code16

	sti	#; cli should also work for biosdisk_int13_extensions

	movw	%bx, %ax
	movw	%cx, %ds

	/* set additional registers to serve buggy BIOSes. */
	pushw	%di
	pushw	%bx
	movw	%cx, %es
	movw	%si, %di
	movw	%si, %bx

	int	$0x13
	popw	%bx
	popw	%di

	/* some buggy USB BIOSes fail to clear AH on success */
	setc	%dl

	/* clear the data segment */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es
#ifndef STAGE1_5

	/* if it is not read/write operation, we can skip the A20 code. */

	andb	$0xFE, %bh
	cmpb	$0x42, %bh
	jne	1f

	/* ensure A20 is on when we come back to protected mode. */

	pushal

	movw	$0x00ff, %cx		# try so many times on failure
	movw	$0x0001, %dx		# DL=enable A20, DH=debug off

	cli	/* yes, keep interrupt off when controlling A20 */
	call	enable_disable_a20
	sti

	//sete	%dl			# DL=1 means success

	popal
1:
#endif
	call	real_to_prot
	.code32

	movzbl	%dl, %eax	/* return value in %eax */

	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%esi
	popl	%ebp

	ret

/*
 * void console_putchar (int c)
 *
 * Put the character C on the console. Because GRUB wants to write a
 * character with an attribute, this implementation is a bit tricky.
 * If C is a control character (CR, LF, BEL, BS), use INT 10, AH = 0Eh
 * (TELETYPE OUTPUT). Otherwise, save the original position, put a space,
 * save the current position, restore the original position, write the
 * character and the attribute, and restore the current position.
 *
 * The reason why this is so complicated is that there is no easy way to
 * get the height of the screen, and the TELETYPE OUPUT BIOS call doesn't
 * support setting a background attribute.
 */
ENTRY(console_putchar)

	.code32

	movl	0x4(%esp), %edx
	pushal

	call	prot_to_real
	.code16

	sti

	movb	%dl, %al
	movw	$0x0007, %bx

	/* use teletype output if control character */
	cmpb	$0x07, %al
	je	1f
	cmpb	$0x0a, %al
	je	9f
	cmpb	$0x0d, %al
	je	1f

	cmpb	$0x08, %al	# BackSpace?
	jne	2f		# no, continue

	movb	$0x03, %ah	# yes. get cursor position
	int	$0x10

	testw	%dx, %dx	# at line 0 and column 0?
	jz	3f		# yes, do nothing

	decb	%dl
	jns	4f
	decb	%dh		# DL=-1, so goto last line ...
	movb	$79, %dl	# ... and goto end of line
4:
	pushw	%dx
	movb	$0x02, %ah	# set cursor position
	movb	$0x00, %bh
	int	$0x10

	movw	$0x0e20, %ax	# print a Space to wipe out the char
	int	$0x10

	popw	%dx
	movb	$0x02, %ah	# again set cursor position
	movb	$0x00, %bh
	int	$0x10
	jmp	3f		# done
2:
	cmpb	$0x09, %al	# Tab?
	jne	1f		# no, continue

	movb	$0x03, %ah	# yes. get cursor position
	int	$0x10		# DH=line, DL=column

	cmpb	$72, %dl
	jnb	9f		# simply print a CR and LF

	addb	$8, %dl		# step to next range
	andb	$0xF8, %dl	# align 8

	movb	$0x02, %ah	# set cursor position
	movb	$0x00, %bh
	int	$0x10

	jmp	3f		# done
1:
	movb	$0x0e, %ah	# teletype print the char
	int	$0x10
	jmp	3f		# done
//8:
//	movb	$0x0e, %ah
//	int	$0x10
//	movw	$0x0e20, %ax
//	int	$0x10
//	movw	$0x0e08, %ax
//	int	$0x10
//	jmp	3f		# done
9:
	/* insert a CR before LF */
	movw	$0x0e0d, %ax
	int	$0x10
	movw	$0x0e0a, %ax
	int	$0x10
3:
	call	real_to_prot
	.code32

	popal
	ret


#ifndef STAGE1_5

/*
 * int console_getkey (void)
 * BIOS call "INT 16H Function 00H" to read character from keyboard
 *	Call with	%ah = 0x0
 *	Return:		%ah = keyboard scan code
 *			%al = ASCII character
 */

ENTRY(console_getkey)

	.code32

	push	%ebp

	call	prot_to_real

	.code16

1:
	sti		/* getkey needs interrupt on */
	hlt

	#; work around for Apple BIOS getkey bug
	#; check the keyboard buffer, until there is a keypress.
	movb	$0x01, %ah		#; check key
	int	$0x16
	jz	1b			#; no keypress

	xorw	%ax, %ax
	int	$0x16

	movw	%ax, %dx

	call	real_to_prot
	.code32

	movw	%dx, %ax

	pop	%ebp
	ret


/*
 * int console_checkkey (void)
 *	if there is a character pending, return it; otherwise return -1
 * BIOS call "INT 16H Function 01H" to check whether a character is pending
 *	Call with	%ah = 0x1
 *	Return:
 *		If key waiting to be input:
 *			%ah = keyboard scan code
 *			%al = ASCII character
 *			Zero flag = clear
 *		else
 *			Zero flag = set
 */
ENTRY(console_checkkey)

	.code32

	push	%ebp

	call	prot_to_real

	.code16

	movl	$500, %ecx
1:
	movl	0x46c, %eax	/* get old timer counter */

	sti		/* checkkey needs interrupt on */
	hlt

	cmpl	0x46c, %eax	/* timer counter changed by int 8? */
	loopz	1b		/* no change, wait a timer interrupt */

	/* counter changed or timed out. */

	movb	$0x1, %ah
	int	$0x16

	movl	$0xFFFFFFFF, %edx
	jz	1f		# no key
	movzwl	%ax, %edx	# save the key to DX
1:
	call	real_to_prot
	.code32

	mov	%edx, %eax

	pop	%ebp
	ret


#endif /* ! STAGE1_5 */

/*
 *  This is the area for all of the special variables.
 */

	.align	4


	/* an address can only be long-jumped to if it is in memory, this
	   is used by multiple routines */
offset:
	.long	0x8000
segment:
	.word	0


end_of_low_16bit_code:

	/* ensure this resides in the first 64KB */
	. = . - (ABS(.) / 0x10001)


/****************************************************************************/
/************************* 32-bit functions follow **************************/
/****************************************************************************/


#ifndef STAGE1_5

	//.space	0x300000	/* !!!! insert 3M !!!! */

	.align	16

ENTRY(HMA_start)

	.code32

	/* the first entry of GDT, i.e., the default null entry,
	 * can be any value. it never get used. So we use these
	 * 8 bytes for our jmp and GDT descriptor.
	 */

	. = EXT_C(HMA_start) + 0	/* GDT entry: default null */

	jmp	1f		/* two-byte short jmp */

	. = EXT_C(HMA_start) + 2

	/* 6-byte GDT descriptor */
gdtdescHMA:
	.word	0x17			/* limit */
	.long	EXT_C(HMA_start)	/* linear base address */

/* 8*/	.long	0x0000FFFF, 0x00009300	/* 16-bit data 64K limit */

	/* 16-bit code 64K limit */
/*16*/	.word	0xFFFF
	.long	EXT_C(HMA_start) + 0x9B000000
	.word	0

1:

	/* boot drive and partition number, though partition number
	 * is not needed for DOSes.
	 */
	movl	EXT_C(chain_edx), %edx
	movl	EXT_C(chain_ebx), %ebx

	/* move new loader from extended memory to conventional memory.
	 * this will overwrite our GRUB code, data and stack, so we should not
	 * use instuctions like push/pop/call/ret, and we should not use
	 * functions like gateA20().
	 */

	movl	EXT_C(chain_load_from), %esi
	movl	EXT_C(chain_load_to), %edi
	movl	EXT_C(chain_load_dword_len), %ecx
	cld
	repz movsl

	/* switch to real mode */

	/* set new GDT */
	lgdt	gdtdescHMA

	/* set up segment limits */
	movw	$8, %ax		/* 16-bit data 64K base 0 */
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* jump to a 16 bit segment, this might be an extra step:
	 * set up CS limit, also clear high word of EIP
	 */
	ljmp	$16, $(1f - EXT_C(HMA_start))
1:
	.code16

	/* clear the PE bit of CR0 */
	movl	%cr0, %eax
	andb 	$0xFE, %al
	movl	%eax, %cr0

	/* setup DS, ES, SS, FS and GS before loading CS */
	xorl	%eax, %eax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movl	$0x400, %esp
	movw	%ax, %fs
	movw	%ax, %gs

	/* flush prefetch queue, reload %cs */

	.byte	0xEA		/* ljmp 0000:7c00 */
VARIABLE(chain_boot_IP)
	.word	0x7c00		/* offset */
VARIABLE(chain_boot_CS)
	.word	0x0000		/* segment */

	.align	4

VARIABLE(chain_load_from)
	.long	0
VARIABLE(chain_load_to)
	.long	0
VARIABLE(chain_load_dword_len)
	.long	0x80
VARIABLE(linux_data_real_addr)
	.long	0
VARIABLE(chain_edx)
	.long	0
VARIABLE(chain_ebx)
	.long	0

	/* max length of code is 1 sector */
	. = . - (. - EXT_C(HMA_start))/0x201

/*
 * void init_bios_info (void)
 *
 */

ENTRY(init_bios_info)

	.code32

	pushl	%edi

	/* initialize mem alloc array */
	movl	ABS(EXT_C(free_mem_start)), %eax
	movl	ABS(EXT_C(mem_alloc_array_start)), %edi
	cld
	stosl
	xorl	%eax, %eax
	stosl
	stosl		/* end the array */
	stosl

	/* invalidate the drive part table buffer */
	decl	%eax	/* EAX = -1 */
	movl	%eax, PART_TABLE_BUF

	/* Set root drive and partition.  */
	movl	ABS(EXT_C(boot_drive)), %eax
	movl	%eax, ABS(EXT_C(saved_drive))
	movb	ABS(EXT_C(install_partition))+2, %ah
	movb	%ah, ABS(EXT_C(saved_partition))+2

	popl	%edi
	ret

/*
 * void enter_cmdline (void)
 *
 * CMDLINE_BUF is used for the command-line buffer.
 *
 */

ENTRY(enter_cmdline)

	.code32

	movl	$-1, ABS(EXT_C(current_drive))
	movb	$'\n', %al
	pushl	%eax		# use only AL
	call	ABS(EXT_C(console_putchar))
	popl	%eax
	pushl	%esi
	pushl	%ebx
	movl	$(CMDLINE_BUF), %ebx
	movb	$0, (%ebx)
	//movl	$0x800, ABS(EXT_C(script_char_pointer))
	cmpb	$0, ABS(EXT_C(startup_debug_keys))	# Key C pressed?
	je	1f					# No. Run script.
	movl	$0, ABS(EXT_C(script_char_pointer))	# Yes. Skip the script.
	movw	$0, ABS(EXT_C(startup_debug_keys))	# Clear key C & Insert.
1:
	//call	EXT_C(add_history)

	movl	$(CMDLINE_BUF), %ebx
	movb	$0, (%ebx)
	pushl	ABS(EXT_C(script_char_pointer))		# non-zero for script.
	call	EXT_C(get_cmdline)
	testl	%eax, %eax				# EAX=0 for success.
	popl	%eax					# non-zero for script.
	jnz	1f					# failure, return.

	xorl	%esi, %esi	# indicator: last command in last menu item
	testl	%eax, %eax				# non-zero for script.
	jz	4f					# from keyboard.
	//cmpl	$0x800, %eax
	//jb	4f
	//cmpl	$0x1800, %eax
	//jnb	4f

	/* if it is the last command in the script, and titles exist, then
	 * set ESI=1.
	 */
	cmpl	$0, ABS(EXT_C(script_char_pointer))
	jne	3f
	cmpl	$0, EXT_C(titles_count)
	je	3f
	incl	%esi		# indicator: last commad in last menu item
3:

	/* this is a command in the bootup script. */
	cmpb	$0, ABS(EXT_C(startup_debug_keys)) + 1	# Insert key pressed?
	je	4f					# No. Continue.
	/* Yes. Insert key pressed. */
	/* if it is the last command, clear the Insert key. */
	cmpl	$0, ABS(EXT_C(script_char_pointer))
	jne	3f
	movb	$0, ABS(EXT_C(startup_debug_keys)) + 1	# Clear Insert key.
3:
	pushl	$(99f)					# Print [Yes/No?]
	call	EXT_C(grub_putstr)
	popl	%eax
3:
	/* getkey */
	call	ABS(EXT_C(console_getkey))
	orb	$0x20, %al
	cmpb	$'y', %al
	je	4f
	cmpb	$'n', %al
	je	1b
	jmp	3b
4:

	//call	EXT_C(add_history)

	pushl	%ebx
	call	EXT_C(find_command)
	popl	%ebx

	movl	$-1, EXT_C(buf_drive)

	testl	%eax, %eax
	je	2f

	/* if it is command "exit", then return. */
	cmpl	$EXT_C(builtin_exit), %eax
	je	1f

	/* if it is command "title", then display menu. */
	cmpl	$EXT_C(builtin_title), %eax
	je	30f

	pushl	%eax
	pushl	%ebx
	call	EXT_C(skip_to)
	popl	%edx
	popl	%ebx

	pushl	%eax
	call	*4(%ebx)
	popl	%ebx
	jmp	3f
2:
	pushl	%ebx
	call	EXT_C(command_func)
	popl	%ebx
3:
	call	EXT_C(add_history)
	decl	%esi		# last command in last menu item?
	jnz	1b		# no, get next line.

	# yes. print menu.

	//call	EXT_C(add_history)
30:
	/* check titles */
	//movb	$0, ABS(EXT_C(startup_debug_keys)) + 1	# Clear Insert key.
	pushal	#-----------------------------------------------------------
	/* find all titles in script */
	movl	$1, EXT_C(is_real_title)	# initialize to true
	movl	$0, EXT_C(real_titles_count)
	movl	$0, EXT_C(titles_count)
	movl	$0x800, %edi
10:
	movb	$0x20, %al
	movl	$0x1800, %ecx
	subl	%edi, %ecx
	jbe	19f
	cld
	repz scasb
	decl	%edi
	//cmpb	$0, (%edi)
	//je	19f
	xorl	%ebx, %ebx		/* indicator: title not found */
	cmpl	$0x6C746974, (%edi)	/* "titl" */
	jne	11f			/* not title, try next */
	cmpb	$0x65, 4(%edi)		/* "e"   */
	jne	11f			/* not title, try next */
	cmpb	$0x20, 5(%edi)		/* space */
	ja	11f			/* not title, try next */
	/* "title" found */
	cmpl	$20, EXT_C(titles_count)
	jnb	19f		# too many titles, ignored
	call	94f	/* check if the last title is real */
	cmpl	$0, EXT_C(titles_count)
	jne	12f
	/* for the first title, clear screen */
#if 0
	//////////////////////////////////////////////////////////
	/* print 40 new lines to simulate clearing screen */	//
	movl	$40, %ecx					//
	movb	$'\n', %al					//
50:								//
	pushl	%eax		# use only AL			//
	call	ABS(EXT_C(console_putchar))			//
	popl	%eax						//
	loop	50b						//
	/* cursor is now at the bottom left of the screen */	//
	//////////////////////////////////////////////////////////
#else
	//////////////////////////////////////////////////////////
	/* place cursor at bottom left corner */		//
	movw	$0x1800, 0x450		# BDA 0040:0050		//
	/* clear screen by writing VGA text buffer */		//
	movl	$0x07200720, %eax	# white Spaces		//
	movl	%edi, %ebx		# save EDI into EBX	//
	movl	$0xB8000, %edi		# EDI=Video buffer	//
	movl	$(80*25/2), %ecx				//
	cld							//
	repz stosl						//
	movl	%ebx, %edi		# restore EDI from EBX	//
	//////////////////////////////////////////////////////////
#endif
12:
	/* print at most 78 bytes after the title */
	//////////////////////////////////////////////////////////////////
	/* now the titles_count is counted from 0, we will print */	//
	/* the titles from the top line -- line 0 of the screen. */	//
	movl	$160, %eax	# 80 chars, each has an attribute byte	//
	mull	EXT_C(titles_count)	# line number			//
        /* EDX:EAX=offset of the line in VGA text buffer, EDX=0  */	//
	movl	%edi, %ebx	# save EDI into EBX			//
	leal	5(%edi), %esi	# points to space after "title"		//
	leal	0xB8002(%eax), %edi	# EDI=offset			//
	movl	$78, %ecx						//
	cld								//
	movb	$0x07, %ah	# attribute "white"			//
50:									//
	lodsb								//
	cmpb	$0x20, %al						//
	jb	50f		# end at non-printable chars		//
	stosw								//
	loop	50b							//
50:									//
	movl	%ebx, %edi	# restore EDI from EBX			//
	//////////////////////////////////////////////////////////////////
	incl	EXT_C(titles_count)
	incl	EXT_C(real_titles_count)
	movl	$0, EXT_C(is_real_title)	# reset to false
	jmp	13f
11:
	cmpb	$0x20, (%edi)
	jbe	13f
	movl	$1, EXT_C(is_real_title)	# set to true
13:
	/* locate the beginning of next line */
	cmpb	$0x20, (%edi)
	incl	%edi		# INC will not touch CF
	jnb	13b
	cmpb	$0, -1(%edi)	# end of script?
	je	14f		# yes, exit
	call	95f
	jmp	10b		# no, check next line
14:
	decl	%edi
	call	95f
	call	94f	/* check if the last title is real */
19:
	popal	#-----------------------------------------------------------

	/* all titles printed. if none, skip to 1f */

	cmpl	$0, EXT_C(titles_count)
	je	1f

	cmpl	$0, EXT_C(real_titles_count)
	je	49f

	/* if default_entry is too big, set it to 0, and set timeout=-1 */
	movl	EXT_C(default_entry), %eax
	cmpl	EXT_C(real_titles_count), %eax
	jb	50f
	movl	$0, EXT_C(default_entry)
	movl	$-1, EXT_C(grub_timeout)
50:

	/* highlight the default item */
	call	96f

	/* get user's key press */
	//movl	EXT_C(grub_timeout), %ecx
49:
	xorl	%ecx, %ecx
	movl	$91, %eax	# (91/18.2)=5 seconds
	mull	EXT_C(grub_timeout)
	jc	51f
	movb	$5, %cl
	divl	%ecx
	xchgl	%eax, %ecx	# ECX=timer ticks
50:
	incl	%ecx
	jz	51f
	decl	%ecx
	jz	55f
#if 0	/* QEMU cannot update VGA on buffer change:( */
	//////////////////////////////////////////////////////////////////
	/* print the time out, write VGA text buffer */			//
	pushal								//
	movl	$5, %eax						//
	mull	%ecx							//
	movl	$91, %ebx						//
	divl	%ebx							//
	cmpl	$5, %edx	# the remainder should be less than 5	//
	jnb	48f							//
	/* EAX is time out value in seconds */				//
	pushal			# 32-byte buffer for int_to_ascii	//
	movl	%esp, %esi						//
	pushl	$0							//
	pushl	%eax							//
	pushl	$'d'		# decimal				//
	pushl	%esi		# the buffer				//
	call	EXT_C(int_to_ascii)					//
	movl	%eax, %esi	# the string				//
	movl	$(0xB8000+(160*24)), %edi	# VGA text buffer	//
	cld								//
	movb	$0x07, %ah						//
46:									//
	lodsb								//
	testb	%al, %al						//
	jz	47f							//
	stosw								//
	jmp	46b							//
47:									//
	movb	$0x20, %al						//
	stosw								//
	stosw								//
	addl	$48, %esp						//
48:									//
	popal								//
	//////////////////////////////////////////////////////////////////
#else
	//////////////////////////////////////////////////////////////////
	pushal								//
	movl	$5, %eax						//
	mull	%ecx							//
	movl	$91, %ebx						//
	divl	%ebx							//
	cmpl	$5, %edx	# the remainder should be less than 5	//
	jnb	48f							//
	/* EAX is time out value in seconds */				//
	pushl	%ebx		# highest byte=0 to end ASCII string	//
	pushal			# 32-byte buffer for int_to_ascii	//
	movl	%esp, %esi						//
	pushl	$0							//
	pushl	%eax							//
	pushl	$'d'		# decimal				//
	pushl	%esi		# the buffer				//
	call	EXT_C(int_to_ascii)					//
	movl	$0x0D202020, 31(%esi)	# 3 Spaces, CR			//
	subl	$4, %eax						//
	movl	$0x2020200D, (%eax)					//
	pushl	%eax		# the string				//
	call	EXT_C(grub_putstr)					//
	addl	$56, %esp						//
48:									//
	popal								//
	//////////////////////////////////////////////////////////////////
#endif
51:
	pushl	%ecx
	call	ABS(EXT_C(console_checkkey))
	popl	%ecx
	incl	%eax
	loopz	50b	# no key-press
	jz	55f	# time out, use default entry number
	
	/* key pressed, set timeout=-1 */
	movl	$-1, EXT_C(grub_timeout)
	call	ABS(EXT_C(console_getkey))
	cmpb	$'c', %al
	jz	1f
	cmpb	$'C', %al
	jz	1f
	cmpb	$'q', %al
	jz	1f
	cmpb	$'Q', %al
	jz	1f
	cmpb	$0x1B, %al	# -- Esc --, cannot use $'\e' here
	jz	1f
	/* if no real titles, ignore other keys */
	cmpl	$0, EXT_C(real_titles_count)
	je	49b
	cmpb	$'b', %al
	jz	55f
	cmpb	$'B', %al
	jz	55f
	cmpb	$0x0D, %al	# -- CR --
	jz	55f
	cmpb	$0x0A, %al	# -- LF --
	jz	55f
	cmpw	$0x4800, %ax	# KEY_UP
	jnz	52f
	/* cancel old highlighted entry */
	call	96f
	decl	EXT_C(default_entry)
	jmp	53f
52:
	cmpw	$0x5000, %ax	# KEY_DOWN
	jnz	54f
	/* cancel old highlighted entry */
	call	96f
	incl	EXT_C(default_entry)
53:
	/* get the selected item */
	movl	EXT_C(default_entry), %eax
	addl	EXT_C(real_titles_count), %eax
	xorl	%edx, %edx
	divl	EXT_C(real_titles_count)	# EDX=new entry number
	movl	%edx, EXT_C(default_entry)	# set new entry number
	/* highlight it */
	call	96f
54:
	jmp	49b		# get next key

55:
	/* run script of selected item */
	/* if no real titles, exit to command line */
	cmpl	$0, EXT_C(real_titles_count)
	je	1f
	/* get the script pointer */
	movl	EXT_C(default_entry), %esi
	shll	$2, %esi
	movl	$EXT_C(title_scripts), %ebx
	movl	(%ebx, %esi), %eax
	movl	%eax, ABS(EXT_C(script_char_pointer))
	//movb	$0, ABS(EXT_C(startup_debug_keys)) + 1	# Clear Insert key.
	jmp	1b		# run the script!!
1:
	movl	$0, ABS(EXT_C(script_char_pointer))	# Skip the script.
	movb	$0, ABS(EXT_C(startup_debug_keys)) + 1	# Clear Insert key.
	popl	%ebx
	popl	%esi
	ret

94://check_real_title
	cmpl	$0, EXT_C(is_real_title)	# last title is real?
	jnz	52f				# yes, accept it
	decl	EXT_C(real_titles_count)	# no, cancel it
52:
	ret

95://record the starting pointer of the script for this title
	testl	%ebx, %ebx	# is it a title line?
	jz	51f		# no, skip
	pushal
	movl	EXT_C(real_titles_count), %esi
	decl	%esi
	movl	EXT_C(titles_count), %eax
	decl	%eax
	movl	$EXT_C(real_title_numbers), %ebx
	movb	%al, (%ebx, %esi)
	shll	$2, %esi
	movl	$EXT_C(title_scripts), %ebx
	movl	%edi, (%ebx, %esi)
	popal
51:
	ret
96://toggle highlight
	//pushal
	//////////////////////////////////////////////////////////////////
	movl	EXT_C(default_entry), %eax				//
	movl	$EXT_C(real_title_numbers), %ecx			//
	movzbl	(%eax, %ecx), %ecx					//
	movl	$160, %eax	# 80 chars, each has an attribute byte	//
	mull	%ecx		# line number				//
        /* EDX:EAX=offset of the line in VGA text buffer, EDX=0  */	//
	leal	0xB8003(%eax), %edx	# EDX=offset			//
	movl	$78, %ecx						//
	cld								//
50:									//
	xorb	$0x77, (%edx)	# reverse attribute			//
	incl	%edx							//
	incl	%edx							//
	loop	50b							//
	//////////////////////////////////////////////////////////////////
	//popal
	ret
99:
	.string	" [Yes/No?]"

/*
 * struct builtin *find_command (char *command)
 *
 */

#if 1
ENTRY(find_command)

	.code32

	pushl	%esi

	xorl	%eax, %eax
	movl	8(%esp), %esi	/* command */
	pushl	%edi
	cld
1:
	lodsb
	cmpb	$0x20, %al
	je	1b
	testb	%al, %al
	jz	1f
	/* (ESI - 1) points to command */
	decl	%esi
	movl	%esi, %edi
2:
	lodsb
	testb	%al, %al
	jz	2f
	cmpb	$0x20, %al
	jnz	2b

2:
	decl	%esi
	movb	%ah, (%esi)	/* AH=0 */
	xchgl	%eax, %edx	/* save AL to DL */

	movl	$EXT_C(builtin_table), %eax
3:
	movl	(%eax), %ecx
	jecxz	2f

	pushl	%eax
	pushl	%ecx
	pushl	%edx
	pushl	(%ecx)
	pushl	%edi
	call	EXT_C(grub_strcmp)
	testl	%eax, %eax
	popl	%edx
	popl	%edx
	popl	%edx
	popl	%ecx
	popl	%eax
	js	2f
	jne	4f
	xchgl	%eax, %ecx	/* return ECX */
	jmp	3f
4:
	addl	$0x04, %eax
	jmp	3b
2:
	xorl	%eax, %eax
3:
	movb	%dl, (%esi)	/* restore the char */
1:
	popl	%edi
	popl	%esi
	ret
#endif

/*
 * int grub_tolower (int c)
 *
 */

ENTRY(grub_tolower)

	.code32
#if 0
	movl	4(%esp), %eax
	leal	-0x41(%eax), %edx
	cmpl	$25, %edx
	ja	1f
	addl	$0x20, %eax
1:
	ret
	. = . - (. - grub_tolower)/17
#else
	movl	4(%esp), %eax
	cmpb	$'A', %al
	jb	1f
	cmpb	$'Z', %al
	ja	1f
	addb	$('a' - 'A'), %al
1:
	ret
	. = . - (. - grub_tolower)/16
#endif

/*
 * int grub_memcmp (const char *s1, const char *s2, int n)
 *
 */

ENTRY(grub_memcmp)

	.code32

	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi

	cld
	movl	8(%ebp), %esi
	movl	12(%ebp), %edi
	movl	16(%ebp), %ecx

	xorl	%eax, %eax
	repz cmpsb
	jz	1f
	jl	2f
	incl	%eax
	jmp	1f
2:
	decl	%eax
1:
	popl	%edi
	popl	%esi
	leave
	ret

/*
 * int grub_strcmp (const char *s1, const char *s2)
 *
 */

ENTRY(grub_strcmp)

	.code32

	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi

	cld
	movl	8(%ebp), %esi
	movl	12(%ebp), %edi

	xorl	%eax, %eax
1:
	cmpb	$0, (%esi)
	jne	3f
	cmpb	$0, (%edi)
	je	1f
3:
	cmpsb
	jz	1b

	jl	2f
	incl	%eax
	jmp	1f
2:
	decl	%eax
1:
	popl	%edi
	popl	%esi
	leave
	ret

/*
 * int grub_strlen (const char *str)
 *
 */

ENTRY(grub_strlen)

	.code32
#if 0
	pushl	%edi

	cld
	movl	8(%esp), %edi

	xorl	%eax, %eax
	xorl	%ecx, %ecx
	decl	%ecx		# ECX=0xFFFFFFFF
	cld
	repnz scasb
	jnz	1f		# failure
	decl	%edi
	subl	8(%esp), %edi
	xchgl	%eax, %edi
	jmp	2f
1:
	decl	%eax		# EAX=0xFFFFFFFF
2:
	popl	%edi
	ret
#else
	//pushl	%ebp
	//movl	%esp, %ebp
	xorl	%eax, %eax
	movl	0x4(%esp), %edx
	jmp	2f
1:
	incl	%eax
2:
	cmpb	$0x0, (%edx, %eax)
	jne	1b
	//popl	%ebp
	ret
#endif


/*
 * void *grub_memset (void *start, int c, int len)
 *
 */

ENTRY(grub_memset)

	.code32

	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi

	cld
	movl	8(%ebp), %edi
	movl	12(%ebp), %eax
	movl	16(%ebp), %ecx

	repz stosb
	xchgl	%eax, %edi	# return the pointer to end

	popl	%edi
	leave
	ret

/*
 * char *grub_strcpy (char *dest, const char *src)
 *
 */

ENTRY(grub_strcpy)

	.code32

	movl	8(%esp), %eax		# src
	pushl	%eax
	call	EXT_C(grub_strlen)
	popl	%edx			# src
	incl	%eax			# len++

	movl	4(%esp), %ecx		# dest
	pushl	%eax			# len
	pushl	%edx			# src
	pushl	%ecx			# dest
	call	EXT_C(grub_memmove)
	popl	%eax			# dest
	popl	%edx			# src
	popl	%ecx			# len

	ret

/*
 * void *grub_memmove (void *to, const void *from, int len)
 *
 */

ENTRY(grub_memmove)

	.code32

	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi

	movl	8(%ebp), %edi		# to
	movl	%edi, %eax
	movl	12(%ebp), %esi		# from
	movl	16(%ebp), %ecx		# len
	cmpl	%esi, %edi
	jae	1f

	cld
	repz movsb
	jmp	2f
1:
	addl	%ecx, %esi
	decl	%esi
	addl	%ecx, %edi
	decl	%edi
	std
	repz movsb
	cld
2:
	popl	%esi
	popl	%edi
	popl	%ebp
	ret

/*
 * int substring (const char *s1, const char *s2, int case_insensitive)
 *
 */

ENTRY(substring)

	.code32

	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi

	cld
	movl	8(%ebp), %esi
	movl	12(%ebp), %edi
	movl	16(%ebp), %ecx		# case_insensitive

	xorl	%eax, %eax
3:
	lodsb
	movb	(%edi), %dl
	incl	%edi

	jecxz	1f			# case sensitive
	/* tolower */
	cmpb	$'A', %al
	jb	2f
	cmpb	$'Z', %al
	ja	2f
	orb	$0x20, %al
2:
	cmpb	$'A', %dl
	jb	1f
	cmpb	$'Z', %dl
	ja	1f
	orb	$0x20, %dl
1:
	testb	%al, %al
	jnz	1f
	testb	%dl, %dl
	setnz	%al
	negl	%eax
	jmp	3f
1:
	cmpb	%dl, %al
	je	3b

	movb	$1, %al
3:
	popl	%edi
	popl	%esi
	leave
	ret

/*
 * int nul_terminate (char *str)
 *
 */

ENTRY(nul_terminate)

	.code32

	pushl	%esi

	movl	8(%esp), %esi	/* str */
	cld
1:
	lodsb
	testb	%al, %al	/* Null */
	je	9f		/* end */
	cmpb	$0x20, %al	/* Space */
	je	1f
//	cmpb	$0x09, %al	/* Tab */
//	je	1f
	cmpb	$'\\', %al	/* backslash */
	jne	1b
	lodsb
	testb	%al, %al	/* Null */
	je	9f		/* end */
	jmp	1b
1:
	/* AL= Space or Tab */
//	lodsb
//	cmpb	$0x20, %al	/* Space */
//	je	1b
//	cmpb	$0x09, %al	/* Tab */
//	je	1b
9:
	decl	%esi
	movb	$0, (%esi)
//	xchgl	%eax, %esi	/* return the position */

	popl	%esi
	ret

/*
 * char *skip_to (char *cmdline)
 *
 */

#if 1
ENTRY(skip_to)

	.code32

	pushl	%esi

	movl	8(%esp), %esi	/* cmdline */
	cld
1:
	lodsb
	testb	%al, %al	/* Null */
	je	9f		/* end */
	cmpb	$0x20, %al	/* Space */
	je	1f
//	cmpb	$0x09, %al	/* Tab */
//	je	1f
	cmpb	$'\\', %al	/* backslash */
	jne	1b
	lodsb
	testb	%al, %al	/* Null */
	je	9f		/* end */
	jmp	1b
1:
	/* AL= Space or Tab */
	lodsb
	cmpb	$0x20, %al	/* Space */
	je	1b
//	cmpb	$0x09, %al	/* Tab */
//	je	1b
9:
	decl	%esi
	xchgl	%eax, %esi	/* return the cmdline */

	popl	%esi
	ret
	. = . - (. - skip_to)/36
#else
ENTRY(skip_to)

	.code32

	//.byte	0x8B, 0x44, 0x24, 0x04
	movl	4(%esp), %eax	/* cmdline */
1:
	movb	(%eax), %dl
	testb	%dl, %dl
	jz	9f
	cmpb	$0x20, %dl	/* Space */
	je	1f
	cmpb	$0x09, %dl	/* Tab */
	je	1f

	incl	%eax
	cmpb	$0x5C, %dl
	jne	1b
	cmpb	$0, (%eax)
	je	9f
	jmp	1b
1:
	incl	%eax
	movb	(%eax), %dl
	cmpb	$0x20, %dl	/* Space */
	je	1b
	cmpb	$0x09, %dl	/* Tab */
	je	1b
9:
	ret
	. = . - (. - skip_to)/48
#endif

#if 1
/*
 * int rawread (long drive, long long sector, long byte_offset,
 *		long long byte_len, long long buf, long write)
 *
 *  Return 0 on failure, non-zero on success.
 */

ENTRY(rawread)

	.code32

	enter	$60, $0
	//pushl	%ebp
	//movl	%esp, %ebp
	//subl	$60, %esp

#define write		40(%ebp)
#define buf_hi		36(%ebp)
#define buf		32(%ebp)
#define byte_len_hi	28(%ebp)	/* currently not used */
#define byte_len	24(%ebp)
#define byte_offset	20(%ebp)
#define sector_hi	16(%ebp)
#define sector		12(%ebp)
#define drive		 8(%ebp)

	# EBP+ 4	--> return address
	# EBP		--> OLD EBP

#define slen		 -8(%ebp)
#define soff		-16(%ebp)
#define num_sect	-24(%ebp)
#define track_hi	-28(%ebp)
#define track		-32(%ebp)
#define size		-40(%ebp)
#define bufaddr		-44(%ebp)
#define bufseg		-48(%ebp)

	cmpl	$0x900ddeed, write	# write?
	jne	1f			# no
	cmpl	$0, buf			# write to address of 0?
	jne	2f			# no, continue
	movl	$1, %eax		# yes, return with success
	jmp	9f			# return
1:
	cmpl	$0xedde0d90, write	# read?
	je	2f			# yes, continue
	movl	$71, ABS(EXT_C(errnum))	# ERR_FUNC_CALL
	xorl	%eax, %eax		# no, return with failure
	jmp	9f			# return
2:
	pushl	%esi
	pushl	%edi
	pushl	%ebx

	# EBP-64	--> old ESI
	# EBP-68	--> old EDI
	# EBP-72	--> old EBX

1:
	cmpl	$0, byte_len
	jz	1f

	movl	drive, %eax
	cmpl	%eax, EXT_C(buf_drive)
	je	2f
	movl	%eax, EXT_C(buf_drive)
	movl	$-1, EXT_C(buf_track)
2:
	/* Sectors that need to read. */
	movl	byte_offset, %eax
	addl	byte_len, %eax
	addl	$(SECTOR_SIZE - 1), %eax
	shrl	$(SECTOR_BITS), %eax
	movl	%eax, slen

	movl	sector, %eax
	movl	%eax, %edx
	andl	$63, %eax
	movl	%eax, soff
	andb	$0xC0, %dl
	movl	%edx, track
	movl	sector_hi, %edx
	movl	%edx, track_hi
	movl	$64, %edx
	subl	%eax, %edx
	movl	%edx, num_sect
	shll	$9, %eax
	addl	byte_offset, %eax
	addl	$(BUFFERADDR), %eax
	movl	%eax, bufaddr
	movl	$(BUFFERSEG), %esi	# bufseg

	movl	track, %eax
	movl	track_hi, %edx
	cmpl	%edx, EXT_C(buf_track)+4
	jne	2f
	cmpl	%eax, EXT_C(buf_track)
	je	3f
2:
	movl	%edx, EXT_C(buf_track)+4
	movl	%eax, EXT_C(buf_track)

	movl	$64, %ebx		# read_len

	movl	num_sect, %ecx
	cmpl	%ecx, slen
	jbe	4f
	movl	$-1, EXT_C(buf_track)
	movl	sector_hi, %edx
	movl	sector, %eax
	movl	%ecx, %ebx		# read_len
	movl	soff, %esi
	shll	$(SECTOR_BITS-4), %esi
	addl	$(BUFFERSEG), %esi
4:
	pushl	%esi			# bufseg
	pushl	%ebx			# read_len
	pushl	%edx			# read_start_hi
	pushl	%eax			# read_start
	pushl	drive
	pushl	$(BIOSDISK_READ)
	call	EXT_C(biosdisk)
	addl	$24, %esp
	testl	%eax, %eax
	jz	3f			# read succeeded
	movl	$-1, EXT_C(buf_track)
	/* On error try again to load only the required sectors. */
	movl	slen, %edi
	cmpl	num_sect, %edi
	ja	7f			# ERR_READ
	cmpl	%ebx, %edi		# EBX=read_len
	je	7f			# ERR_READ
	movl	soff, %esi
	shll	$(SECTOR_BITS-4), %esi
	addl	$(BUFFERSEG), %esi
	pushl	%esi			# bufseg
	pushl	%edi			# slen
	pushl	sector_hi
	pushl	sector
	pushl	drive
	pushl	$(BIOSDISK_READ)
	call	EXT_C(biosdisk)
	addl	$24, %esp
	testl	%eax, %eax
	jnz	7f			# ERR_READ
	movl	%edi, num_sect
3:
	/* num_sect=sectors read at BUFADDR and will be used. */

	movl	byte_len, %eax
	movl	%eax, size
	movl	num_sect, %eax
	shll	$9, %eax
	subl	byte_offset, %eax
	cmpl	%eax, size
	jbe	2f
	movl	%eax, size
2:
	cmpl	$0x900ddeed, write
	jne	2f
	pushl	size
	pushl	bufaddr
	pushl	buf
	call	EXT_C(grub_memcmp)
	addl	$12, %esp
	testl	%eax, %eax
	jz	6f		/* no need to write */
	movl	$-1, EXT_C(buf_track)
	/* update data at bufaddr */
	pushl	size
	pushl	buf
	pushl	bufaddr
	call	EXT_C(grub_memmove)
	addl	$12, %esp
	/* write it to disk! */
	movl	soff, %esi
	shll	$(SECTOR_BITS-4), %esi
	addl	$(BUFFERSEG), %esi
	pushl	%esi			# bufseg
	pushl	num_sect
	pushl	sector_hi
	pushl	sector
	pushl	drive
	pushl	$(BIOSDISK_WRITE)
	call	EXT_C(biosdisk)
	addl	$24, %esp
	testl	%eax, %eax
	jnz	8f			# ERR_WRITE
	jmp	6f			# continue
2:
	/* Use this interface to tell which sectors were read and used. */
	cmpl	$0, EXT_C(disk_read_func)
	jz	2f
	movl	sector_hi, %edx
	movl	sector, %eax
	movl	$(SECTOR_SIZE), %edi
	subl	byte_offset, %edi	# length
	cmpl	size, %edi
	jbe	3f
	movl	size, %edi
3:
	pushl	%edi
	pushl	byte_offset
	pushl	%edx
	pushl	%eax
	call	*EXT_C(disk_read_func)
	popl	%eax
	popl	%edx
	addl	$8, %esp
	incl	%eax
	jnz	3f
	incl	%edx
3:
	movl	size, %ecx
	subl	%edi, %ecx		# ECX=length
	jecxz	2f
3:
	cmpl	$(SECTOR_SIZE), %ecx
	jbe	3f
	pushl	%ecx
	pushl	$(SECTOR_SIZE)
	pushl	$0
	pushl	%edx
	pushl	%eax
	call	*EXT_C(disk_read_func)
	popl	%eax
	popl	%edx
	popl	%ecx
	popl	%ecx
	popl	%ecx
	incl	%eax
	jnz	4f
	incl	%edx
4:
	subl	$(SECTOR_SIZE), %ecx
	jmp	3b
3:
	pushl	%ecx			# ECX=length
	pushl	$0
	pushl	%edx
	pushl	%eax
	call	*EXT_C(disk_read_func)
	addl	$16, %esp
2:
	cmpl	$0, buf
	jz	2f
	pushl	size
	pushl	bufaddr
	pushl	buf
	call	EXT_C(grub_memmove)
	addl	$12, %esp
6:
	movl	size, %eax
	addl	%eax, buf
2:
	movl	size, %eax
	subl	%eax, byte_len
	movl	num_sect, %eax
	addl	%eax, sector
	adcl	$0, sector_hi
	movl	$0, byte_offset
	jmp	1b
8:
	movl	$(29/*ERR_WRITE*/), ABS(EXT_C(errnum))
	jmp	8f
7:
	movl	$(25/*ERR_READ*/), ABS(EXT_C(errnum))
8:
	xorl	%eax, %eax		# error return
	jmp	8f
1:
	movl	$1, %eax		# success
8:
	popl	%ebx
	popl	%edi
	popl	%esi
9:
	leave
	//movl	%ebp, %esp
	//popl	%ebp
	ret

#undef write
#undef buf_hi
#undef buf
#undef byte_len_hi
#undef byte_len
#undef byte_offset
#undef sector_hi
#undef sector
#undef drive

#undef slen
#undef soff
#undef num_sect
#undef track_hi
#undef track
#undef size
#undef bufaddr
#undef bufseg


/*
 * int devread (long long sector, long byte_offset,
 *		long long byte_len, long long buf, long write)
 *
 *  Return 0 on failure, non-zero on success.
 */

ENTRY(devread)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

#define write		36(%ebp)
#define buf_hi		32(%ebp)
#define buf		28(%ebp)
#define byte_len_hi	24(%ebp)
#define byte_len	20(%ebp)
#define byte_offset	16(%ebp)
#define sector_hi	12(%ebp)
#define sector		 8(%ebp)

	movl	ABS(EXT_C(part_start)), %eax
	orl	ABS(EXT_C(part_start))+4, %eax
	jz	1f
	movl	sector_hi, %edx
	movl	sector, %eax
	movl	byte_offset, %ecx
	addl	byte_len, %ecx
	decl	%ecx
	shrl	$9, %ecx
	addl	%ecx, %eax
	adcl	$0, %edx
	cmpl	ABS(EXT_C(part_length))+4, %edx
	jb	1f
	ja	2f
	cmpl	ABS(EXT_C(part_length)), %eax
	jb	1f
2:
	movl	$(24/*ERR_OUTSIDE_PART*/), ABS(EXT_C(errnum))
	xorl	%eax, %eax		# error return
	jmp	9f			# return
1:
	movl	sector_hi, %edx
	movl	sector, %eax
	movl	byte_offset, %ecx
	pushl	%ecx
	shrl	$9, %ecx
	addl	%ecx, %eax
	adcl	$0, %edx
	addl	ABS(EXT_C(part_start)), %eax
	adcl	ABS(EXT_C(part_start))+4, %edx
	popl	%ecx
	andl	$(SECTOR_SIZE - 1), %ecx
	
	pushl	write
	pushl	buf_hi
	pushl	buf
	pushl	$0
	pushl	byte_len
	pushl	%ecx			# byte_offset
	pushl	%edx			# sector_hi
	pushl	%eax			# sector
	pushl	ABS(EXT_C(current_drive))
	call	EXT_C(rawread)
	//addl	$36, %esp		# can omit

9:
	leave
	//movl	%ebp, %esp
	//popl	%ebp
	ret

#undef write
#undef buf_hi
#undef buf
#undef byte_len_hi
#undef byte_len
#undef byte_offset
#undef sector_hi
#undef sector

#endif

/*
 * int list_partitions (void)
 *
 *  Return 0 on failure, non-zero on success.
 */

ENTRY(list_partitions)

	.code32

	/* check whether the drive part table is already buffered */
	movl	PART_TABLE_BUF, %eax	# the buffered drive
	cmpl	%eax, ABS(EXT_C(current_drive))
	jne	1f
	ret
1:
	/* initialize buffer end pointer */
	movl	$(PART_TABLE_BUF+16), PART_TABLE_BUF+4

	pushl	%ebp
	movl	%esp, %ebp

	pushal				# 32-byte room

//#define current_partnum		(-4)(%ebp)

	pushl	%edi
	pushl	%esi
	pushl	%ebx

	cld

	/* build the first entry for "whole drive" */
	movl	$(PART_TABLE_BUF+16), %edi
	xorl	%eax, %eax
	stosl
	stosl
	stosb				# boot indicator=0
	movb	$0x0C, %al
	stosb				# part type=0x0C
	movb	$0xFF, %al
	movl	%eax, -4(%ebp)		# current_partnum = 0xFF
	stosw
	xorl	%eax, %eax
	stosl
	/* EDI = end of current entry */

	movl	%edi, %ebx
9:

	/* Read the MBR, or the boot sector of the extended partition.  */
	pushl	$0xedde0d90		# read
	pushl	$0			# buf_hi
	pushl	$(PART_TABLE_TMPBUF)
	pushl	$0			# byte_len_hi
	pushl	$(SECTOR_SIZE)
	pushl	$0				# byte_offset
	pushl	$0				# sector_hi
	/* calculate the start sector number for read */
	movl	-16(%ebx), %eax
	addl	-4(%ebx), %eax
	pushl	%eax				# sector
	pushl	ABS(EXT_C(current_drive))
	call	EXT_C(rawread)
	addl	$36, %esp
	testl	%eax, %eax
	jz	9f			# return with failure

	/* Check if it is valid.  */
	movl	$(PART_TABLE_TMPBUF+446), %esi
	cmpw	$0xAA55, 64(%esi)
	jne	9f
	pushl	%esi
	call	98f
	popl	%esi
	jns	9f

8:

	/* copy entries to PART_TABLE_BUF */
1:
	movl	8(%esi), %eax		# start sector
	pushl	%edi
	movl	%esi, %edi
	movl	$16, %ecx
	repz scasb
	popl	%edi
	je	2f			# empty entry, ignored
	stosl				# relative start sector
	movl	12(%esi), %eax		# part length
	stosl
	movb	(%esi), %al		# boot indicator
	stosb
	movb	4(%esi), %al		# part type
	stosb
	call	99f			# Is AL extended?
	movl	-4(%ebx), %eax		# current offset
	jne	3f			# no, normal entry
	testl	%eax, %eax		# yes, extended
	jnz	4f
	cmpl	$0, -16(%ebx)
	jz	3f
	/* low byte no change, increment the high byte */
	incb	-3(%ebp)		# current_partnum_hi
	jmp	5f
//	/* low byte incremented, clear the high byte */
//	incb	-4(%ebp)		# current_partnum
//	jo	9f			# done
//	movb	$0, -3(%ebp)		# current_partnum_hi
//	jmp	5f
4:
	/* low byte no change, increment the high byte */
	incb	-3(%ebp)		# current_partnum_hi
	jmp	4f
3:
	/* low byte incremented, clear the high byte */
	incb	-4(%ebp)		# current_partnum
	jo	9f			# done
	movb	$0, -3(%ebp)		# current_partnum_hi
5:
	addl	-16(%ebx), %eax		# current start relative
4:
	pushl	%eax
//	cmpb	$3, -4(%ebp)		# Is this table primary?
//	jbe	3f
//	/* low byte no change, increment the high byte */
//	incb	-3(%ebp)		# current_partnum_hi
//3:
//	/* low byte incremented, clear the high byte */
//	incb	-4(%ebp)		# current_partnum
//	jo	9f			# done
//	movb	$0, -3(%ebp)		# current_partnum_hi
	movl	-4(%ebp), %eax		# current_partnum
	stosw
	popl	%eax
	stosl				# new offset

	cmpl	$(PART_TABLE_BUF+PART_TABLE_BUFLEN-16), %edi
	jnb	9f
	jmp	3f
2:
	/* for primary part table, the empty entry is counted */
	cmpb	$3, -4(%ebp)		# current_partnum
	ja	3f
	incb	-4(%ebp)		# current_partnum
3:
	addl	$16, %esi
	cmpl	$(PART_TABLE_TMPBUF+510), %esi
	jb	1b
	/* copy entries to PART_TABLE_BUF done */

8:
	addl	$16, %ebx
	cmpl	%edi, %ebx
	ja	9f

	/* check if this entry is extended */

	movb	-7(%ebx), %al
	call	99f
	jne	8b

	//movl	%ebx, entry_expanding
	jmp	9b			# do next
9:
	andl	$0xFFFFFFF0, %edi

	/* end the entries */
	movw	$0xFFFF, 10(%edi)

	movl	ABS(EXT_C(current_drive)), %eax
	movl	%eax, PART_TABLE_BUF	# the buffered drive

	popl	%ebx
	popl	%esi
	popl	%edi
	//popl	%ebp
	leave
	ret
98:
	/* check if the part table at ESI is valid */
	xorl	%edx, %edx
1:
	movb	(%esi), %al
	pushl	%edi
	movl	%esi, %edi
	movl	$16, %ecx
	repz scasb
	popl	%edi
	je	2f			# empty entry, ignored
	testb	$0x7F, %al		# boot indicator
	jnz	3f			# invalid part table
	testb	$0x3F, 2(%esi)
	jz	3f			# CHS start sector must not be 0
	testb	$0x3F, 6(%esi)
	jz	3f			# CHS end sector must not be 0
	cmpl	$0, 8(%esi)
	jz	3f			# LBA start sector must not be 0
	cmpl	$0, 12(%esi)
	jz	3f			# LBA length must not be 0
	decl	%edx			# count one valid entry
2:
	addl	$16, %esi
	cmpl	$(PART_TABLE_TMPBUF+510), %esi
	jb	1b
	testl	%edx, %edx
3:
	/* SF == 1 for success, 0 for failure */
	ret

99:
	/* check if AL is extended */
	/* mask off hidden flag */
	andb	$0xEF, %al
	cmpb	$5, %al
	je	2f
	cmpb	$0xF, %al
	je	2f
	cmpb	$0x85, %al
2:
	ret

//96:
//	.string	"\n%08x, %08x: %08X, %08X:"
//#undef current_partnum

/*
 * int open_partition (void)
 *
 *  Return 0 on failure, non-zero on success.
 */

ENTRY(open_partition)

	.code32

	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx

	call	EXT_C(list_partitions)

	xorl	%eax, %eax
	movl	%eax, ABS(EXT_C(errnum))	# errnum=0
	movl	$(PART_TABLE_BUF), %edi
1:
	addl	$16, %edi
	cmpw	$0xFFFF, 10(%edi)
	jz	1f

	//testl	%eax, %eax
	//jz	1f

	cmpb	$0, 9(%edi)				# part type
	jz	1b

	//movl	10(%edi), %eax				# part num
	//movw	%ax, ABS(EXT_C(current_partition))+2
	movl	8(%edi), %eax				# bootind, type, part num
	movl	%eax, ABS(EXT_C(current_partition))

	movl	(%edi), %ebx				# part start relative
	addl	12(%edi), %ebx				# part start offset
	movl	%ebx, ABS(EXT_C(part_start))
	movl	$0, ABS(EXT_C(part_start))+4
	movl	4(%edi), %eax				# part length
	movl	%eax, ABS(EXT_C(part_length))
	movl	$0, ABS(EXT_C(part_length))+4

	cmpl	$0x00FEFFFF, EXT_C(dest_partition)
	jne	2f

	movw	10(%edi), %ax				# part num
	testb	%ah, %ah
	jnz	5f
	movsbl	%al, %eax
	jmp	4f
5:
	movzwl	%ax, %eax
4:
	cdq
	cmpl	$0, EXT_C(do_completion)
	jne	3f

	movzbl	9(%edi), %ecx				# part type
	pushl	%ecx
	movzbl	8(%edi), %ecx				# boot indicator
	pushl	%ecx
	pushl	$0
	pushl	4(%edi)					# part length
	pushl	$0

	pushl	%ebx
	pushl	%edx
	pushl	%eax
	pushl	$99f			# format string
	pushl	$0			# console
	call	EXT_C(grub_sprintf)
	addl	$40, %esp
	call	EXT_C(attempt_mount)
	movl	EXT_C(fsys_type), %esi
	imul	$0x10, %esi, %esi
	pushl	EXT_C(fsys_table)(%esi)
	pushl	$98f			# format string
	pushl	$0			# console
	call	EXT_C(grub_sprintf)
	addl	$12, %esp
	jmp	1b
3:
	pushal				# str[32] is here
	movl	%esp, %esi
	pushl	%edx
	pushl	%eax
	pushl	$97f			# format string
	pushl	%esi
	call	EXT_C(grub_sprintf)
	call	EXT_C(print_a_completion)
	addl	$48, %esp
	jmp	1b
2:
	movw	ABS(EXT_C(current_partition))+2, %ax
	cmpw	%ax, EXT_C(dest_partition)+2
	jne	1b
	cmpl	$0, EXT_C(request_fstype)
	je	9f			# return success
	call	EXT_C(attempt_mount)
	jmp	9f			# return
1:
	/* EAX=0 is ready for failure */
	xorl	%eax, %eax
9:
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
97:
	.string	"%ld)"
98:
	.string	"%s\n"
99:
	.string	"%3ld:%08lx,%08lx:%02X,%02X:"


/*
 * int attempt_mount (void)
 */

ENTRY(attempt_mount)

	.code32

	xorl	%eax, %eax
1:
	cmpl	$(NUM_FSYS), %eax
	jnb	1f

	pushl	%eax
	imul	$0x10, %eax, %eax
	call	*(EXT_C(fsys_table) + 4)(%eax)
	testl	%eax, %eax
	popl	%eax
	jnz	2f

	incl	%eax
	jmp	1b
2:
	/* success */
	movl	%eax, EXT_C(fsys_type)
	movl	$0, ABS(EXT_C(errnum))	# errnum=0
	movl	$1, %eax
	ret

1:
	/* failure */
	movl	%eax, EXT_C(fsys_type)
	movl	$(17/*ERR_FSYS_MOUNT*/), ABS(EXT_C(errnum))
	movl	$0, %eax
	ret

/*
 * int biosdisk (int read, int drive, long long sector, int nsec, int segment)
 *
 *  Return 0 on success, non-zero on failure.
 */

ENTRY(biosdisk)

	.code32

#define read	8(%ebp)
#define drive	12(%ebp)
#define sector	16(%ebp)
#define sectorh	20(%ebp)
#define nsec	24(%ebp)
#define segment	28(%ebp)

	pushl	%ebp
	movl	%esp, %ebp

	movl	$-1, %eax
	cmpl	$0xFFFF, drive
	je	1f
	movl	ABS(EXT_C(ram_drive)), %edx
	cmpl	%edx, drive
	jne	2f
	cmpl	%eax, ABS(EXT_C(rd_base))
	jne	1f
	cmpl	%eax, ABS(EXT_C(rd_base)) + 4
	je	2f
1:
	/* (md) or (rd) */
	cmpb	$0x80, nsec
	jnb	3f		/* return failure */

	cmpl	$0, sectorh
	jne	3f		/* return failure */

	movl	2 + sector, %eax
	testl	$0xF800, %eax
	jne	3f		/* return failure */

	movzbl	nsec, %edx
	shll	$9, %edx

	movl	sector, %eax
	shll	$9, %eax
	cmpl	$0xFFFF, drive
	je	1f
	addl	ABS(EXT_C(rd_base)), %eax
	jc	3f		/* return failure */
1:
	/* EAX points to disk sector */
	movl	%eax, %ecx
	addl	%edx, %ecx
	jc	3f		/* return failure */

	pushl	%edx		/* bytes to move for grub_memmove */

	movl	segment, %edx
	shll	$4, %edx

	/* EDX points to buffer */

	testb	$1, read	/* zero for read */
	jz	1f
	xchgl	%eax, %edx	/* write */
1:
	pushl	%eax
	pushl	%edx
	call	EXT_C(grub_memmove)
	//addl	$12, %esp	/* ignored since we will leave immediately */
	xorl	%eax, %eax	/* return success */
	leave
	ret
2:
	/* real disk */

	movl	$0x580, %eax
	pushl	%eax		/* dap */
	pushl	%edi
	xchgl	%eax, %edi	/* EDI = 0x580 */
	movl	-2 + nsec, %eax
	movw	$0x10, %ax
	cld
	stosl
	movl	-2 + segment, %eax
	stosl
	movl	sector, %eax
	stosl
	movl	sectorh, %eax
	stosl
	popl	%edi

	movzbl	drive, %eax
	pushl	%eax		/* drive */
	movb	read, %ah
	addb	$0x42, %ah
	pushl	%eax		/* EBIOS read/write */
	call	ABS(EXT_C(biosdisk_int13_extensions))
	popl	%edi
	//addl	$12, %esp	/* ignored since we will leave immediately */
	leave
	ret
3:
	movl	$1, %eax
	leave
	ret

#undef read
#undef drive
#undef sector
#undef sectorh
#undef nsec
#undef segment

/*
 * int safe_parse_maxint (char **str_ptr, unsigned long long *myint_ptr)
 *
 *  On call:	myint_ptr is a pointer to unsigned long long.
 *		str_ptr is a pointer to the string to parse.
 *  On return:	the result written to the unsigned long long integer.
 *		the char pointer (*str_ptr) updated to point to the char
 *			immediately following the parsed char.
 */

ENTRY(safe_parse_maxint)

	.code32

	pushl	%ebp
	movl	%esp, %ebp
	subl	$32, %esp

	# EBP+12	--> myint_ptr
	# EBP+ 8	--> str_ptr
	# EBP+ 4	--> return address
	# EBP		--> OLD EBP

	# EBP- 4	--> Carry
	# EBP- 8	--> myint_hi
	# EBP-12	--> myint
	# EBP-16	--> found
	# EBP-20	--> negtive
	# EBP-24	--> not used yet
	# EBP-28	--> mult_hi
	# EBP-32	--> mult

	pushl	%esi
	pushl	%edi
	pushl	%ebx

	# EBP-36	--> old ESI
	# EBP-40	--> old EDI
	# EBP-44	--> old EBX

	movl	8(%ebp), %esi		# ESI=str_ptr
	movl	(%esi), %esi		# ESI=ptr
	cld
	xorl	%eax, %eax
	movl	%eax, -8(%ebp)		# myint_hi
	movl	%eax, -12(%ebp)		# myint
	movl	%eax, -16(%ebp)		# found
	movl	%eax, -28(%ebp)		# mult_hi
	movb	$10, %al
	movl	%eax, -32(%ebp)		# mult

	lodsb
	cmpb	$'-', %al
	sete	-20(%ebp)		# negative
	je	1f
	decl	%esi
1:
	/* is hex? */
	movl	(%esi), %eax
	orb	$0x20, %ah
	cmpw	$0x7830, %ax		# '0x'
	jne	1f
	incl	%esi
	incl	%esi
	movb	$16, -32(%ebp)		# mult
1:
	lodsb
	orb	$0x20, %al
	subb	$'0', %al
	cmpb	$9, %al
	jbe	2f
	subb	$('a' - '0'), %al
	cmpb	$10, -32(%ebp)
	je	1f
	cmpb	$5, %al
	ja	1f
	addb	$10, %al
2:
	movb	$1, -16(%ebp)		# found
	cmpb	$10, -32(%ebp)		# mult
	jne	2f
	/* decimal */
	movb	$0, -4(%ebp)		# Carry
	movl	-12(%ebp), %ecx		# myint
	movl	-8(%ebp), %edx		# myint_hi
	shldl	$1, %ecx, %edx
	setc	%ah
	orb	%ah, -4(%ebp)		# Carry
	shll	$1, %ecx
	movl	%ecx, %ebx
	movl	%edx, %edi
	shldl	$1, %ecx, %edx
	setc	%ah
	orb	%ah, -4(%ebp)		# Carry
	shll	$1, %ecx
	shldl	$1, %ecx, %edx
	setc	%ah
	orb	%ah, -4(%ebp)		# Carry
	shll	$1, %ecx
	addl	%ebx, %ecx
	adcl	%edi, %edx
	setc	%ah
	orb	%ah, -4(%ebp)		# Carry
	movzbl	%al, %eax
	addl	%eax, %ecx
	adcl	$0, %edx
	setc	%ah
	orb	%ah, -4(%ebp)		# Carry
	movl	%ecx, -12(%ebp)		# myint
	movl	%edx, -8(%ebp)		# myint_hi
	/* do not check overflow for negative number */
	cmpb	$0, -20(%ebp)		# negative?
	jne	1b			# yes.
	cmpb	$0, -4(%ebp)		# Carry?
	je	1b
	movl	$(35/*ERR_NUMBER_OVERFLOW*/), ABS(EXT_C(errnum))
	xorl	%eax, %eax		# 0 for failure
	jmp	9f

2:
	/* hex */
	movl	-12(%ebp), %edx		# myint
	shldl	$4, %edx, -8(%ebp)	# myint_hi
	shll	$4, -12(%ebp)		# myint
	orb	%al, -12(%ebp)		# myint

	jmp	1b
1:
	cmpb	$0, -16(%ebp)		# found?
	jne	1f			# yes
	movl	$(23/*ERR_NUMBER_PARSING*/), ABS(EXT_C(errnum))
	xorl	%eax, %eax		# 0 for failure
	jmp	9f
1:
	decl	%esi
	movl	8(%ebp), %edi		# EDI=str_ptr
	movl	%esi, (%edi)
	movl	-12(%ebp), %eax
	movl	-8(%ebp), %edx
	cmpb	$0, -20(%ebp)		# negative?
	je	1f			# no.
	notl	%edx
	negl	%eax
	jc	1f
	incl	%edx
1:
	movl	12(%ebp), %edi
	stosl
	xchgl	%eax, %edx
	stosl
	movl	$1, %eax		# success
9:
	popl	%ebx
	popl	%edi
	popl	%esi

	leave
	ret

/*
 * char * int_to_ascii (char *buf, int c, long long i)
 *
 *  On Call: buf must have at least 32-byte room.
 *  Return the start address of the result string. The ending null of
 *  the string is at the end of the buffer, that is, buf[31].
 */

ENTRY(int_to_ascii)

	.code32

	enter	$32, $0
	//pushl	%ebp
	//movl	%esp, %ebp
	//subl	$32, %esp

	# EBP+16	--> i
	# EBP+12	--> c
	# EBP+ 8	--> buf
	# EBP+ 4	--> return address
	# EBP		--> OLD EBP

	# EBP- 4	--> L0
	# EBP- 8	--> L1
	# EBP-12	--> H0
	# EBP-16	--> H1
	# EBP-20	--> negtive
	# EBP-32	--> not used yet

	pushl	%esi
	pushl	%edi
	pushl	%ebx

	# EBP-36	--> old ESI
	# EBP-40	--> old EDI
	# EBP-44	--> old EBX

	leal	16(%ebp), %esi		# ESI=dataptr
	movl	8(%ebp), %edi		# EDI=buf
	addl	$31, %edi		# EDI=end of buf
	xorl	%eax, %eax
	std				# backward
	stosb				# first, place the ending null

	movb	12(%ebp), %dl		# c
	cmpb	$'x', %dl
	je	5f
	cmpb	$'X', %dl
	jne	3f
5:
	/* hex is easy job. */

	movl	$8, %ecx		# load the 8 long long chars
6:
	cld
	lodsb
	/* print this byte */

	/* turn the byte into two bytes, using aam... */
	.byte	0xD4, 0x10

	/* now AH=hi, AL=lo */
	/* print AL, then AH */
	call	99f
	movb	%ah, %al
	call	99f
	loop	6b

	/* all digits printed */

	cld
	incl	%edi			# points to start byte
	movb	$'0', %al
	movl	$16, %ecx
	repz scasb
	decl	%edi
	xchgl	%eax, %edi		# return the start address
	jmp	1f
3:
	/* decimal convertion */

	/* check if the number is negative */
	cld
	lodsl
	xchgl	%eax, %ecx		# ECX=lo
	lodsl				# EAX=hi

	movb	$0, -20(%ebp)		# clear negative indicator
	cmpb	$'u', %dl
	je	5f
	testl	%eax, %eax
	jns	5f
	/* it is negative */
	incb	-20(%ebp)		# set negative indicator
	notl	%eax
	negl	%ecx
	jc	5f
	incl	%eax
5:
	/* EAX:ECX is positive number */

	std
#if 0

/* C code for reference. */

do {
	unsigned long H0, H1, L0, L1;

/* 0x100000000 == 4294967296 */
/* num.ll == (H1 * 10 + H0) * 0x100000000 + L1 * 10 + L0 */

	H0 = num.hi % 10;
	H1 = num.hi / 10;
	L0 = num.lo % 10;
	L1 = num.lo / 10;

/* num.ll == H1 * 10 * 0x100000000 + H0 * 0x100000000 + L1 * 10 + L0 */
/* num.ll == H1 * 10 * 0x100000000 + H0 * 4294967290 + H0 * 6 + L1 * 10 + L0 */

	L0 += H0 * 6;
	L1 += L0 / 10;
	L0 %= 10;

/* num.ll == H1 * 10 * 0x100000000 + H0 * 4294967290 + L1 * 10 + L0 */
/* num.ll == (H1 * 0x100000000 + H0 * 429496729 + L1) * 10 + L0 */
/* quo = (H1 * 0x100000000 + H0 * 429496729 + L1) */
/* rem = L0 */

	num.hi = H1;
	num.lo = H0;
	num.lo *= 429496729UL;
	num.ll += L1;

	*(ptr++) = '0' + L0;

} while (num.ll);
#endif
	//movl	$6, -24(%ebp)
5:
	//decb	-24(%ebp)
	//jz	5f
	movl	$10, %ebx
	xorl	%edx, %edx
	divl	%ebx			# EDX=rem, EAX=quo
	movl	%edx, -12(%ebp)		# H0
	movl	%eax, -16(%ebp)		# H1
	xchgl	%eax, %ecx
	xorl	%edx, %edx
	divl	%ebx			# EDX=rem, EAX=quo
	movl	%edx, -4(%ebp)		# L0
	movl	%eax, -8(%ebp)		# L1
	movl	$6, %eax
	mull	-12(%ebp)		# H0
					# EDX=0, EAX <= 6*9
	addl	-4(%ebp), %eax		# L0
	//xorl	%edx, %edx		// no need
	divl	%ebx			# EDX=rem, EAX=quo
	addl	%eax, -8(%ebp)		# L1
	movl	%edx, -4(%ebp)		# L0
	
	movl	-12(%ebp), %ecx		# H0
	movl	$429496729, %eax
	mull	%ecx			# EDX=0
	movl	%eax, %ecx		# save to low
	movl	-16(%ebp), %eax		# H1, save to high
	addl	-8(%ebp), %ecx		# L1
	adcl	$0, %eax

	pushl	%eax
	orl	%ecx, %eax
	lahf				# save ZF in AH
	movb	-4(%ebp), %al		# L0
	addb	$'0', %al
	stosb
	sahf				# restore ZF from AH
	popl	%eax
	jnz	5b
5:
	/* all digits printed */
	/* if negative, still need to print '-'. */
	cmpb	$0, -20(%ebp)		# negative indicator
	je	5f
	movb	$'-', %al
	stosb
5:
	incl	%edi
	xchgl	%eax, %edi		# return the start address
1:

	popl	%ebx
	popl	%edi
	popl	%esi

	cld
	leave
	//movl	%ebp, %esp
	//popl	%ebp
	ret

99:
	/* Print AL. On call, DL='x' or 'X' */
	std
	cmpb	$9, %al
	jbe	1f
	addb	%dl, %al		# DL='x' or 'X'
	subb	$33, %al
	stosb
	ret
1:
	addb	$'0', %al
	stosb
	ret

/*
 * int grub_sprintf (char *buffer, const char *format, ...)
 *
 *  Return the chars printed.  BUFFER==0 for printf.
 */

ENTRY(grub_sprintf)

	.code32

	enter	$60, $0
	//pushl	%ebp
	//movl	%esp, %ebp
	//subl	$60, %esp

	# EBP+16	--> args list
	# EBP+12	--> format
	# EBP+ 8	--> buffer
	# EBP+ 4	--> return address
	# EBP		--> OLD EBP

	# EBP- 4	--> dataptr
	# EBP- 8	--> ptr
	# EBP-40	--> str[32]
	# EBP-44	--> bp
	# EBP-48	--> width
	# EBP-52	--> length
	# EBP-56	--> pad
	# EBP-60	--> c

	pushl	%esi
	pushl	%edi
	pushl	%ebx

	# EBP-64	--> old ESI
	# EBP-68	--> old EDI
	# EBP-72	--> old EBX

	cld
	leal	16(%ebp), %ebx		# EBX=dataptr
	movl	8(%ebp), %edi		# EDI=buffer
	movl	12(%ebp), %esi		# ESI=format
1:
	lodsb
	testb	%al, %al		# End of string?
	jz	1f			# yes.

	cmpb	$'%', %al
	je	2f

	cmpl	$0, 8(%ebp)
	je	3f
	stosb
	jmp	1b
3:
	pushl	%eax
	call	ABS(EXT_C(console_putchar))
	popl	%eax
	incl	%edi
	jmp	1b
2:
	xorl	%eax, %eax
	movb	$0x20, -56(%ebp)	# pad = ' '
	movl	%eax, -48(%ebp)		# width = 0
	movl	%eax, -52(%ebp)		# length = 0

//get_next_c:
2:
	lodsb
//find_specifier:
4:
	cmpb	$'d', %al
	je	5f
	cmpb	$'x', %al
	je	5f
	cmpb	$'X', %al
	je	5f
	cmpb	$'u', %al
	jne	3f
5:
	xorl	%edx, %edx
	cmpl	%edx, -52(%ebp)		# long?
	je	5f			# not long
	pushl	4(%ebx)
	pushl	(%ebx)
	addl	$8, %ebx
	jmp	7f
5:
	pushl	%edx			# 0
	pushl	(%ebx)
	addl	$4, %ebx
7:
	pushl	%eax	//pushl	$'x'
	leal	-40(%ebp), %eax		# str[32]
	pushl	%eax
	call	EXT_C(int_to_ascii)
	/* EAX=starting address of the result. */
	popl	%edx
	addl	$31, %edx		# ending null
	addl	$12, %esp

	pushl	%esi
	pushl	%eax			# start of string
	subl	%eax, %edx		# EDX = length of str
	pushl	%edx			# length of str
	movl	-48(%ebp), %ecx		# ECX = width
	subl	%edx, %ecx		# ECX = width - strlen(str)
	jbe	5f
	movb	-56(%ebp), %al		# pad
	cmpl	$0, 8(%ebp)
	jz	7f
	repz stosb
	jmp	5f
7:
	pushl	%eax
	call	ABS(EXT_C(console_putchar))
	popl	%eax
	incl	%edi
	loop	7b
    ##############################################
5:
	popl	%ecx			# length of str
	popl	%esi			# start of string
	jecxz	5f
	cmpl	$0, 8(%ebp)
	jz	7f
	repz movsb
	jmp	5f
7:
	lodsb
	pushl	%eax
	call	ABS(EXT_C(console_putchar))
	popl	%eax
	incl	%edi
	loop	7b
5:
	popl	%esi
	jmp	1b
//////////////////////////////////////////////////////////////////////////////
3:
	cmpb	$'c', %al
	jne	3f

	cmpb	$0, -52(%ebp)		# long?
	jne	1b			# yes, invalid

	movl	-48(%ebp), %ecx		# width
	testl	%ecx, %ecx
	js	5f
	jz	5f
	decl	%ecx
	jz	5f
	movb	-56(%ebp), %al		# pad
	cmpl	$0, 8(%ebp)
	jz	7f
	repz stosb
	jmp	5f
7:
	pushl	%eax
	call	ABS(EXT_C(console_putchar))
	popl	%eax
	incl	%edi
	loop	7b
    ##############################################
5:
	movb	(%ebx), %al
	addl	$4, %ebx
	cmpl	$0, 8(%ebp)
	jz	7f
	stosb
	jmp	5f
7:
	pushl	%eax
	call	ABS(EXT_C(console_putchar))
	popl	%eax
	incl	%edi
5:
	jmp	1b
//////////////////////////////////////////////////////////////////////////////
3:
	cmpb	$'s', %al
	jne	3f

	cmpb	$0, -52(%ebp)		# long?
	jne	1b			# yes, invalid

	pushl	%esi

	pushl	(%ebx)
	addl	$4, %ebx
	call	EXT_C(grub_strlen)	# EAX=strlen
	popl	%esi

	movl	-48(%ebp), %ecx		# ECX = width
	subl	%eax, %ecx		# ECX = width - strlen
	movl	%eax, -48(%ebp)		# width = strlen
	jbe	5f
	movb	-56(%ebp), %al		# pad
	cmpl	$0, 8(%ebp)
	jz	7f
	repz stosb
	jmp	5f
7:
	pushl	%eax
	call	ABS(EXT_C(console_putchar))
	popl	%eax
	incl	%edi
	loop	7b
    ##############################################
5:
	/* ESI = string */
	movl	-48(%ebp), %ecx		# ECX=strlen
	jecxz	5f
	cmpl	$0, 8(%ebp)
	jz	7f
	repz movsb
	jmp	5f
7:
	lodsb
	pushl	%eax
	call	ABS(EXT_C(console_putchar))
	popl	%eax
	incl	%edi
	loop	7b
5:
	popl	%esi
	jmp	1b
//////////////////////////////////////////////////////////////////////////////
3:
	cmpb	$'l', %al
	jne	3f

	cmpb	$0, -52(%ebp)		# long?
	jne	1b			# yes, invalid

	incl	-52(%ebp)
	jmp	2b			# get_next_c
//////////////////////////////////////////////////////////////////////////////
3:
	cmpb	$'0', %al
	jne	3f

	cmpb	$0, -52(%ebp)		# long?
	jne	1b			# yes, invalid

	movb	$'0', -56(%ebp)		# pad = '0'
	jmp	5f
//////////////////////////////////////////////////////////////////////////////
3:
	cmpb	$'1', %al
	jb	1b
	cmpb	$'9', %al
	ja	1b

	cmpb	$0, -52(%ebp)		# long?
	jne	1b			# yes, invalid
5:
	subb	$'0', %al
	movzbl	%al, %ecx
	movl	%ecx, -48(%ebp)		# width
5:
	lodsb
	cmpb	$'0', %al
	jb	4b			# goto find_specifier
	cmpb	$'9', %al
	ja	4b			# goto find_specifier

	subb	$'0', %al
	movzbl	%al, %ecx
	
	movl	$10, %eax
	mull	-48(%ebp)
	addl	%ecx, %eax
	movl	%eax, -48(%ebp)		# update width

	jmp	5b			# get next digit
//////////////////////////////////////////////////////////////////////////////
1:
	xorl	%eax, %eax
	cmpl	%eax, 8(%ebp)
	jz	1f
	stosb
	decl	%edi
1:
	movl	%edi, %eax
	subl	8(%ebp), %eax		# return value

	popl	%ebx
	popl	%edi
	popl	%esi

	leave
	//movl	%ebp, %esp
	//popl	%ebp
	ret

/*
 * int grub_putstr (const char *str)
 *
 *  Print ASCIZ string STR. Return the chars printed.
 */

ENTRY(grub_putstr)

	.code32

	pushl	%esi
	movl	0x8(%esp), %esi
1:
	lodsb
	testb	%al, %al
	jz	1f
	pushl	%eax
	call	ABS(EXT_C(console_putchar))
	popl	%eax
	jmp	1b
1:
	xchgl	%eax, %esi
	subl	0x8(%esp), %eax
	decl	%eax
	popl	%esi
	ret


/*
 * int get_cmdline (void)
 *
 * Get input from the keyboard and place the string at CMDLINE_BUF.
 *
 */

ENTRY(get_cmdline)

	.code32

	//pushl	%esi
	pushl	%edi
2:
	movl	$-1, EXT_C(history)
	movl	$(CMDLINE_BUF), %edi
	movb	$0, (%edi)

	cld

	/* print "grub> " */
	call	99f
1:
	/* get a key */

	/* first, try preset_menu script commands */
	/* the preset menu starts at 0x800 */

	cmpl	$0, ABS(EXT_C(script_char_pointer))
	je	4f
	//cmpl	$0x800, ABS(EXT_C(script_char_pointer))
	//jb	4f
	//cmpl	$0x1800, ABS(EXT_C(script_char_pointer))
	//jnb	4f

	movl	ABS(EXT_C(script_char_pointer)), %eax
	movb	(%eax), %al
	incl	ABS(EXT_C(script_char_pointer))
	testb	%al, %al		# End of script?
	jnz	3f			# No. Continue.

	movl	$0, ABS(EXT_C(script_char_pointer))  # Invalidate the script.

	jmp	1f		# end the line
4:
	call	ABS(EXT_C(console_getkey))
3:
	cmpb	$0x0D, %al	# -- CR --
	je	1f		# end the line
	cmpb	$0x0A, %al	# -- LF --
	je	1f		# end the line
	cmpb	$0x1B, %al	# -- ESC --
	je	2b		# cancel all input, redo from start
//////////////////////////////////////////////////////////////////////////////
	cmpb	$0x08, %al	# -- BackSpace --
	jne	2f
	cmpl	$(CMDLINE_BUF), %edi
	jbe	1b		# ignore the BackSpace
	decl	%edi
	movb	$0, (%edi)
4://print the char and continue
	pushl	%eax		# use only AL
	call	ABS(EXT_C(console_putchar))
	popl	%eax
	jmp	1b
//////////////////////////////////////////////////////////////////////////////
2:
	cmpb	$0x09, %al	# -- Tab --
	jne	2f

	cmpl	$((CMDLINE_BUF)+(MAX_CMDLINE)-10), %edi
	jnb	1b		# line too long, ignored

	/* TAB lists completions */
	pushl	%edi
	movl	$(CMDLINE_BUF), %edi

	/* find a non-space char */
	movl	$(MAX_CMDLINE), %ecx
	movb	$0x20, %al
	repz scasb
	jz	3f		# all are spaces, ignored.

	/* EDI points to the char immediately after the non-space char. */

	pushl	%esi
	//decl	%edi
	movl	%edi, %esi	# immediately after the non-space.
	/* find a space char */
6:
	lodsb
	cmpb	$0, %al
	jz	5f
	cmpb	$0x20, %al
	jz	5f
	loop	6b
	jmp	7f
5:
	decl	%esi		# the last space
	movl	%edi, %ecx
	popl	%eax		# old ESI
	popl	%edi		# old EDI
	pushl	%edi		# old EDI
	pushl	%eax		# old ESI
	cmpl	%esi, %edi
	seta	EXT_C(is_filename)
	movl	%ecx, %esi
	cmpb	$'(', -1(%esi)
	je	6f
	cmpb	$'/', -1(%esi)
	jne	5f
6:
	orb	$1, EXT_C(is_filename)
5:

//////////////////////////////////////////////////////////////////////////////
	/* Find the position of the first character in this word.  */
	movl	%edi, %esi
9:
	cmpl	$(CMDLINE_BUF), %esi
	jbe	9f			# hit beginning of cmdline, done.

	cmpb	$0x20, -1(%esi)		# Space?
	jnz	11f			# No, find next Space

	/* this is a Space, but may be quoted... */

	/* find backslashes immediately before the space */
	pushl	%edi
	leal	-2(%esi), %edi
13:
	cmpl	$(CMDLINE_BUF), %edi	# exceeding the cmdline buffer?
	jb	13f			# yes, exit.

	cmpb	$0x5C, (%edi)		# backslash '\\'?
	jnz	13f			# no, exit.

	decl	%edi
	jmp	13b			# find next backslash
13:
	/* EDI points to non-backslash char, (ESI - 1) points to the Space. */
	/* So if (ESI - EDI) is odd, then the Space is quoted. Otherwise if */
	/* (ESI - EDI) is even, then the Space is not quoted. */
	subl	%esi, %edi
	testl	$1, %edi		# odd or even?
	popl	%edi

	jz	9f			# even, unquoted Space, done.
11:
	decl	%esi
	jmp	9b			# try next char
9:
	/* ESI = first char in this word */
//////////////////////////////////////////////////////////////////////////////

	// /* add a ')' if needed. */

	cmpb	$0, EXT_C(is_filename)
	je	9f
	cmpb	$0x20, -1(%esi)
	jne	9f
	cmpb	$0x28, (%esi)		# '('
	jne	11f
	cmpb	$0x20, 1(%esi)
	ja	10f
	//movb	$0x29, %al		# ')'
	//stosb
	//movb	$0, (%edi)
	//pushl	%eax		# use only AL
	//call	ABS(EXT_C(console_putchar))
	//popl	%eax
	jmp	9f
11:
	/* add a '(' if needed. */

	cmpb	$0x20, (%esi)
	ja	9f
	movb	$0x28, %al		# '('
	stosb
	movb	$0, (%edi)
	pushl	%eax		# use only AL
	call	ABS(EXT_C(console_putchar))
	popl	%eax
	jmp	9f
10:
	/* add a '1' if needed. */
	cmpw	$(',' | ('-' << 8)), -2(%edi)
	jne	9f
	movb	$'1', %al		# '1'
	stosb
	movb	$0, (%edi)
	pushl	%eax		# use only AL
	call	ABS(EXT_C(console_putchar))
	popl	%eax
9:
	/* Invalidate the cache, since the user may change removable disks */
	movl	$0xFFFFFFFF, EXT_C(buf_drive)

	/* Copy this word to COMPLETION_BUFFER */
	subl	%esi, %edi
	pushl	%edi
	pushl	%esi
	pushl	$(COMPLETION_BUF)
	call	EXT_C(grub_memmove)
	popl	%eax
	popl	%esi
	popl	%edi

	/* do the completion */
	movb	$0, (COMPLETION_BUF)(%edi)
	movb	$1, EXT_C(do_completion)
	call	EXT_C(print_completions)

	testl	%eax, %eax
	js	9f
	//js	7f

	pushl	%eax
11:
	movb	(COMPLETION_BUF)(%edi), %al
	testb	%al, %al
	jz	11f
	movb	%al, (%esi, %edi)
	incl	%edi

	pushl	%eax		# use only AL
	call	ABS(EXT_C(console_putchar))
	popl	%eax

	leal	(%esi, %edi), %eax
	cmpl	$((CMDLINE_BUF)+(MAX_CMDLINE)), %eax
	jb	11b
11:
	addl	%esi, %edi
	movb	$0, (%edi)
	popl	%eax
	pushl	%eax
	testl	%eax, %eax
	jz	11f

	/* more than one candidates, so print the list */
	pushl	$0x0A
	call	ABS(EXT_C(console_putchar))
	popl	%eax
	movb	$0, EXT_C(do_completion)
	call	EXT_C(print_completions)
11:
	popl	%eax
9:
	testl	%eax, %eax
	jz	11f

	call	99f

	pushl	$(CMDLINE_BUF)
	call	EXT_C(grub_putstr)
	popl	%edi
	addl	%eax, %edi	# update line position
	//jmp	5f
11:
//	pushl	$0x32768
//	pushl	$(COMPLETION_BUF)
//	pushl	$(CMDLINE_BUF)
//	call	EXT_C(grub_strncat)
//	popl	%eax
//	popl	%eax
//	popl	%eax
//5:
	popl	%esi
	popl	%eax		# discard EDI
	jmp	1b
	
7:
	popl	%esi
3:
	popl	%edi
	jmp	1b
//////////////////////////////////////////////////////////////////////////////
2:
//	cmpw	$0x4D00, %ax	# RIGHT
//	jne	2f
//	movb	$0x09, %al
//	jmp	3f
//2:
	cmpw	$0x4800, %ax	# KEY_UP
	jne	2f
	incl	EXT_C(history)
	call	EXT_C(get_history)
	testl	%eax, %eax
	jz	5f
8:
	pushl	%eax		# point to the history string
	/* wipe out current cmdline */
	movb	$0x08, %al	# using BackSpace
7:
	cmpl	$(CMDLINE_BUF), %edi
	jbe	6f		# ignore the BackSpace
	decl	%edi
	movb	$0, (%edi)

	pushl	%eax		# use only AL
	call	ABS(EXT_C(console_putchar))
	popl	%eax
	jmp	7b
6:
	/* copy history to cmdline buffer */
	//pushl	%eax		# history string already pushed!!
	pushl	$(CMDLINE_BUF)
	call	EXT_C(grub_strcpy)
	popl	%eax
	popl	%eax

	/* print the new cmdline */
	pushl	$(CMDLINE_BUF)
	call	EXT_C(grub_putstr)
	popl	%edi
	addl	%eax, %edi

	jmp	1b
5:
	decl	EXT_C(history)
	jmp	1b
2:
	cmpw	$0x5000, %ax	# KEY_DOWN
	jne	2f
	cmpl	$0, EXT_C(history)
	jle	1b		# ignored
	decl	EXT_C(history)
	call	EXT_C(get_history)
	testl	%eax, %eax
	jnz	8b
	incl	EXT_C(history)
	jmp	1b
2:
	cmpb	$0, %al		# ctrl keys
	jz	1b		# ignored

	/* normal keys */
	cmpl	$((CMDLINE_BUF)+(MAX_CMDLINE)), %edi
	jnb	1b		# line too long, ignored
3:
	stosb
	movb	$0, (%edi)
	jmp	4b		# print the char and continue
//////////////////////////////////////////////////////////////////////////////
1:
	/* get to end of line, print CRLF */
	movb	$'\n', %al
	pushl	%eax		# use only AL
	call	ABS(EXT_C(console_putchar))
	popl	%eax

	xorl	%eax, %eax	# return 0 for success
	stosb			# end the command line with 0

	popl	%edi
	//popl	%esi
	ret
99:
	pushl	ABS(EXT_C(errnum))	# print error number to prompt user
	# check current cursor position. if already at left-most,
	# we need not print CRLF.
	movzbl	0x450, %eax
	testb	%al, %al		# Zero mean at left-most (column 0)
	setz	%al			# skip '\n' if already left-most
	addl	$98f, %eax
	pushl	%eax
	pushl	$0
	call	EXT_C(grub_sprintf)
	popl	%eax
	popl	%eax
	popl	%eax
	ret

98:
	.string	"\nwee %d> "

	.align	4

VARIABLE(is_filename)
	.long	0

#if 0
/*
 * set_int13_handler(map)
 *
 * Copy MAP to the drive map and set up int13_handler.
 */
ENTRY(set_int13_handler)

	.code32

	cld
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%edi
	pushl	%esi

	/* copy MAP to the drive map */
	movl	$(DRIVE_MAP_SIZE * DRIVE_MAP_SLOT_SIZE / 4), %ecx
	movl	$ABS(EXT_C(hooked_drive_map)), %edi
	movl	8(%ebp), %esi
	cmpl	%esi, %edi
	je	1f
	repz movsl
1:
	/* other variable(s) */
	movb	ABS(EXT_C(is64bit)), %dl
	movb	%dl, ABS(int13_is64bit)

//	Now initialized early at the beginning of this file
//
//	/* save the original int13 handler */
//	movl	$0x4c, %edi
//	movl	(%edi), %eax
//	movl	%eax, ABS(EXT_C(ROM_int13))

	/* decrease the lower memory size and set it to the BIOS memory */

	/* read available lower memory size */
	movzwl  0x413, %eax
	
	/* KBytes that int13 handler occupies */
	movzbl	ABS(int13_handler), %edx # EDX=KBytes used by new int13_handler
	subl	%edx, %eax		 # EAX=new available low memory(KBytes)
	/* save remaining lower memory size in BIOS data area */
	movw	%ax, 0x413
	
	/* copy int13_handler to the reserved area */
	shll    $10, %eax	
	movl    %eax, %edi	# EDI=EAX=destination int13_handler address
	movl	$ABS(int13_handler), %esi
	movl	$((int13_handler_end - int13_handler + 3) / 4), %ecx
	// cld
	repz	movsl
	
	movl	%eax, %edi	# EDI=EAX=linear address of int13_handler

	///* save size (KBytes) of int13_handler in its header */
	//movb	%dl, (%edi)

	/* linear address of int13_handler (below 0x100000). yes, use 'orl'
	 * because we only write the lower 24 bits of the linear addess.
	 * The higher byte(8 bits) is on the highest byte of the gdt entry. 
	 */
	orl	%eax, (PM_DS16 + 2 + MyGDT - int13_handler)(%edi)
	orl	%eax, (PM_CS16 + 2 + MyGDT - int13_handler)(%edi)
	//orl	%eax, (PM_CS32 + 2 + MyGDT - int13_handler)(%edi)

	/* calculate linear address of MyGDT and save in gdtdesc+2 */
	addl	$(MyGDT - int13_handler), %eax	# EAX=linear address of MyGDT
	movl	%eax, (gdtdesc - int13_handler + 2)(%edi)

	/* calculate linear address of int13_lm64move_lm64_start */
//	addl	$(int13_lm64move_lm64_start - MyGDT), %eax
//	movl	%eax, (int13_lm64move_lm64_start_addr - int13_handler)(%edi)

	movl	%edi, %eax	# EDI=EAX=linear address of int13_handler

	/* real mode segment of int13_handler */
//	shrl	$4, %eax
//	movw	%ax, (int13_pgmove_to_rm_cs - int13_handler)(%edi)

//	shll	$16, %eax	# move segment to high word
	shll	$12, %eax	# move segment to high word
	movw	$(int13_handler_entry - int13_handler), %ax	# AX=0x100
	movl	%eax, 0x004C	# save the new int13 handler
	
	/* set ESI to the drive map */
	movl	$ABS(EXT_C(hooked_drive_map)), %esi
	cmpl	8(%ebp), %esi
	je	3f
	movl	$(DRIVE_MAP_SIZE), %ecx
1:
	cmpb	$0xFF, 1(%esi)	# It is memdrive?
	jne	2f		# No. Try next slot.
	testb	$0x40, 5(%esi)	# To_DRIVE is CDROM?
	jz	3f		# No. Memdrive indeed. Hook int15.
2:
	/* try next slot */
	addl	$DRIVE_MAP_SLOT_SIZE, %esi
	loop	1b
	jmp	2f			/* no memdrives, don't hook int15 */
3:
	/* save the new int15 handler. segment still in high word of EAX. */
	movw	$(int15_e820_handler - int13_handler), %ax	# offset
	movl	%eax, 0x54
2:
	popl	%esi
	popl	%edi
	popl	%ebp
	ret

/* int
 * unset_int13_handler(check_status_only)
 *
 * Restore the original int13 handler
 *
 * Return 0 for success and non-zero for failure.
 */
ENTRY(unset_int13_handler)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%edi

	/* check if int13_handler is set */
	movl	$0x413, %edi
	movw	(%edi), %ax
	cmpw	$640, %ax
	jae	1f		#; needn't unset
//	cmpw	$632, %ax
//	jb	1f
	shll	$(16 + 6), %eax

	/* the offset(low word) should be 0x100 */
	movw	$0x100, %ax

	cmpl	%eax, 0x4c
	jne	1f		#; not hooked, unset failure
	movl	%eax, %edi
	shrl	$12, %edi	/* segment base address */

	/* check our int13 signature "$INT13SFGRUB4DOS" */
	cmpl	$0x544E4924, 0x103(%edi)	/* $INT */
	jnz	1f
	cmpl	$0x46533331, 0x107(%edi)	/* 13SF */
	jnz	1f
	cmpl	$0x42555247, 0x10B(%edi)	/* GRUB */
	jnz	1f
	cmpl	$0x534F4434, 0x10F(%edi)	/* 4DOS */
	jnz	1f

	//cmpl	$0x9A000000, 0x1C(%edi)		/* old int 13 */
	cmpl	$0x5A000000, 0x1C(%edi)		/* old int 13 */
	jb	1f

	//cmpl	$0x9A000000, 0x0C(%edi)		/* old int 15 */
	cmpl	$0x5A000000, 0x0C(%edi)		/* old int 15 */
	jb	1f

	movl	ABS(EXT_C(ROM_int13)), %eax
	cmpl	0x1C(%edi), %eax
	jnz	1f

	movl	ABS(EXT_C(ROM_int15)), %eax
	cmpl	0x0C(%edi), %eax
	jnz	1f

	xorl	%eax, %eax

	cmpl	%eax, 8(%ebp)
	jnz	1f

	/* increase the lower memory size */

	movzbw	(%edi), %ax
	addw	%ax, 0x413

	/* restore the original int15 handler */
	movl	ABS(EXT_C(ROM_int15)), %eax
	movl	%eax, 0x54

	/* restore the original int13 handler */
	movl	ABS(EXT_C(ROM_int13)), %eax
	movl	%eax, 0x4c

	xorl	%eax, %eax			/* success */
1:
	/* return non-zero for failure */
	popl	%edi
	popl	%ebp
	ret
#endif

/* get_code_end() :  return the address of the end of the code
 * This is here so that it can be replaced by asmstub.c.
 */

ENTRY(get_code_end)

	.code32

	/* will be the end of the bss */
# if defined(HAVE_END_SYMBOL)
	movl	$end, %eax
# elif defined(HAVE_USCORE_END_SYMBOL)
	movl	$_end, %eax
# endif
	shrl	$2, %eax		/* Round up to the next word. */
	incl	%eax
	shll	$2, %eax
	ret


#endif /* ! STAGE1_5 */

#ifndef STAGE1_5

/*
 * The C code for a grub4dos executable may have defines as follows:
 *

#define sprintf ((int (*)(char *, const char *, ...))((*(int **)0x8300)[0]))
#define printf(...) sprintf(NULL, __VA_ARGS__)
#define putstr ((void (*)(const char *))((*(int **)0x8300)[1]))
#define putchar ((void (*)(int))((*(int **)0x8300)[2]))
#define get_cmdline ((int (*)(char *))((*(int **)0x8300)[3]))
#define getxy ((int (*)(void))((*(int **)0x8300)[4]))
#define gotoxy ((void (*)(int, int))((*(int **)0x8300)[5]))
#define cls ((void (*)(void))((*(int **)0x8300)[6]))
#define setcursor ((int (*)(int))((*(int **)0x8300)[7]))
#define nul_terminate ((int (*)(char *))((*(int **)0x8300)[8]))
#define safe_parse_maxint ((int (*)(char **, unsigned long long *))((*(int **)0x8300)[9]))
#define substring ((int (*)(const char *, const char *, int))((*(int **)0x8300)[10]))
#define strstr ((char *(*)(const char *, const char *))((*(int **)0x8300)[11]))
#define strlen ((int (*)(const char *))((*(int **)0x8300)[12]))
#define strtok ((char *(*)(char *, const char *))((*(int **)0x8300)[13]))
#define strncat ((int (*)(char *, const char *, int))((*(int **)0x8300)[14]))
#define strcmp ((int (*)(const char *, const char *))((*(int **)0x8300)[15]))
#define strcpy ((char *(*)(char *, const char *))((*(int **)0x8300)[16]))
#define tolower ((int (*)(int))((*(int **)0x8300)[17]))
#define isspace ((int (*)(int))((*(int **)0x8300)[18]))
#define getkey ((int (*)(void))((*(int **)0x8300)[19]))
#define checkkey ((int (*)(void))((*(int **)0x8300)[20]))
#define sleep ((unsigned int (*)(unsigned int))((*(int **)0x8300)[21]))
#define memcmp ((int (*)(const char *, const char *, int))((*(int **)0x8300)[22]))
#define memmove ((void *(*)(void *, const void *, int))((*(int **)0x8300)[23]))
#define memset ((void *(*)(void *, int, int))((*(int **)0x8300)[24]))
#define mem64 ((int (*)(int, unsigned long long, unsigned long long, unsigned long long))((*(int **)0x8300)[25]))
#define open ((int (*)(char *))((*(int **)0x8300)[26]))
#define read ((unsigned long (*)(unsigned long long, unsigned long long, unsigned long))((*(int **)0x8300)[27]))
#define close ((void (*)(void))((*(int **)0x8300)[28]))
#define unicode_to_utf8 ((void (*)(unsigned short *, unsigned char *, unsigned long))((*(int **)0x8300)[29]))
#define rawread ((int (*)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long long, unsigned long))((*(int **)0x8300)[30]))
#define rawwrite ((int (*)(unsigned long, unsigned long, char *))((*(int **)0x8300)[31]))
#define devread ((int (*)(unsigned long, unsigned long, unsigned long, unsigned long long, unsigned long))((*(int **)0x8300)[32]))
#define devwrite ((int (*)(unsigned long, unsigned long, char *))((*(int **)0x8300)[33]))
#define next_partition ((int (*)(void))((*(int **)0x8300)[34]))
#define open_device ((int (*)(void))((*(int **)0x8300)[35]))
#define real_open_partition ((int (*)(int))((*(int **)0x8300)[36]))
#define set_device ((char *(*)(char *))((*(int **)0x8300)[37]))
#define dir ((int (*)(char *))((*(int **)0x8300)[38]))
#define print_a_completion ((void (*)(char *))((*(int **)0x8300)[39]))
#define print_completions ((int (*)(int, int))((*(int **)0x8300)[40]))
#define parse_string ((int (*)(char *))((*(int **)0x8300)[41]))
#define hexdump ((void (*)(unsigned long, char *, int))((*(int **)0x8300)[42]))

 *
 */

VARIABLE(system_functions)
	.long	EXT_C(grub_sprintf)
	.long	EXT_C(grub_putstr)
	.long	ABS(EXT_C(console_putchar))
	.long	EXT_C(get_cmdline)
	.long	ABS(EXT_C(install_partition))	// getxy
	.long	ABS(EXT_C(install_partition))	// gotoxy
	.long	ABS(EXT_C(install_partition))	// cls
	.long	ABS(EXT_C(install_partition))	// setcursor, obsolete
	.long	EXT_C(nul_terminate)
	.long	EXT_C(safe_parse_maxint) // safe_parse_maxint_with_suffix!
	.long	EXT_C(substring)
	.long	ABS(EXT_C(install_partition))	// grub_strstr
	.long	EXT_C(grub_strlen)
	.long	ABS(EXT_C(install_partition))	// grub_strtok
	.long	ABS(EXT_C(install_partition))	// grub_strncat
	.long	EXT_C(grub_strcmp)
	.long	EXT_C(grub_strcpy)
	.long	EXT_C(grub_tolower)		// grub_tolower, obsolete
	.long	ABS(EXT_C(install_partition))	// grub_isspace, obsolete
	.long	ABS(EXT_C(console_getkey))
	.long	ABS(EXT_C(console_checkkey))
	.long	ABS(EXT_C(install_partition))	// grub_sleep, obsolete
	.long	EXT_C(grub_memcmp)
	.long	EXT_C(grub_memmove)
	.long	EXT_C(grub_memset)
	.long	ABS(EXT_C(install_partition))	// mem64, obsolete
	.long	EXT_C(grub_open)
	.long	EXT_C(grub_read)
	.long	ABS(EXT_C(install_partition))	// grub_close
	.long	EXT_C(unicode_to_utf8)		// unicode_to_utf8, obsolete
	.long	EXT_C(rawread)			// rawread, obsolete
	.long	ABS(EXT_C(install_partition))	// rawwrite, obsolete
	.long	EXT_C(devread)
	.long	ABS(EXT_C(install_partition))	// devwrite
	.long	ABS(EXT_C(install_partition))	// next_partition
	.long	EXT_C(open_device)
	.long	EXT_C(open_partition)		// real_open_partition!
	.long	EXT_C(set_device)
	.long	EXT_C(dir)			// dir, obsolete
	.long	EXT_C(print_a_completion)	// print_a_completion, obsolete
	.long	EXT_C(print_completions)	// print_completions, obsolete
	.long	ABS(EXT_C(install_partition))	// parse_string
	.long	ABS(EXT_C(install_partition))	// hexdump
	.long	ABS(EXT_C(install_partition))	// skip_to
	.long	ABS(EXT_C(install_partition))	// builtin_cmd
	.long	ABS(EXT_C(install_partition))	// get_datetime
	.long	ABS(EXT_C(install_partition))	// lba_to_chs, obsolete
	.long	ABS(EXT_C(install_partition))	// probe_bpb, obsolete
	.long	ABS(EXT_C(install_partition))	// probe_mbr, obsolete
	.long	ABS(EXT_C(install_partition))	// get_mmap_entry
	.long	ABS(EXT_C(install_partition))	// grub_malloc
	.long	ABS(EXT_C(install_partition))	// grub_free
	.long	EXT_C(list_partitions)
	.long	ABS(EXT_C(realmode_run))
	.long	0				// reserved 1
	.long	0				// reserved 2
	.long	0				// reserved 3
	.long	0				// reserved 4
	.long	0				// reserved 5
	.long	0				// reserved 6
	.long	0				// reserved 7
	.long	EXT_C(dir)			// moved 01
	.long	EXT_C(print_a_completion)	// moved 02
	.long	EXT_C(print_completions)	// moved 03
	.long	ABS(EXT_C(install_partition))	// moved 04, lba_to_chs
	.long	ABS(EXT_C(install_partition))	// moved 05, probe_bpb
	.long	ABS(EXT_C(install_partition))	// moved 06, probe_mbr
	.long	EXT_C(unicode_to_utf8)		// moved 07
	.long	EXT_C(rawread)			// moved 08
	.long	ABS(EXT_C(install_partition))	// moved 09, rawwrite
	.long	ABS(EXT_C(install_partition))	// moved 10, setcursor
	.long	EXT_C(grub_tolower)		// moved 11
	.long	ABS(EXT_C(install_partition))	// moved 12, grub_isspace
	.long	ABS(EXT_C(install_partition))	// moved 13, grub_sleep
	.long	ABS(EXT_C(install_partition))	// moved 14, mem64


/*
 * The C code for a grub4dos executable may have defines as follows:
 *

#define next_partition_drive		((*(unsigned long **)0x8304)[0])
#define next_partition_partnum		((*(unsigned char ***)0x8304)[1])
#define next_partition_type		((*(unsigned long ***)0x8304)[2])
#define next_partition_start		((*(unsigned long ***)0x8304)[3])
#define next_partition_len		((*(unsigned long ***)0x8304)[4])
#define next_partition_offset		((*(unsigned long ***)0x8304)[5])
#define next_partition_entry		((*(unsigned long ***)0x8304)[6])
#define next_partition_ext_offset	((*(unsigned long ***)0x8304)[7])
#define next_partition_buf		((*(char ***)0x8304)[8])

 *
 */

VARIABLE(system_variables)

#if 1	/* this group of variables are not used by wee */
VARIABLE(next_partition_drive)
	.long	0
VARIABLE(next_partition_dest)
	.long	0
VARIABLE(next_partition_partition)
	.long	0
VARIABLE(next_partition_type)
	.long	0
VARIABLE(next_partition_start)
	.long	0
VARIABLE(next_partition_len)
	.long	0
VARIABLE(next_partition_offset)
	.long	0
VARIABLE(next_partition_entry)
	.long	0
VARIABLE(next_partition_ext_offset)
	.long	0
VARIABLE(next_partition_buf)
	.long	0
#endif

VARIABLE(quit_print)
	.long	0
//VARIABLE(buf_drive)
	.long	-1	// obsolete
//VARIABLE(buf_track)
	.long	-1	// obsolete
VARIABLE(filesystem_type)
	.long	0
//VARIABLE(query_block_entries)
	.long	0	// obsolete

#if 1	/* this group of variables are not used by wee */
//VARIABLE(map_start_sector)
	.long	0	// obsolete
	.long	0	// ABS(EXT_C(buf_geom))
	.long	0	// ABS(EXT_C(tmp_geom))
	.long	0	// ABS(EXT_C(term_table))
VARIABLE(current_term)
	.long	0	// ABS(EXT_C(term_table))
	.long	0	// ABS(EXT_C(fsys_table))
#endif

//VARIABLE(fsys_type)
	.long	NUM_FSYS	// obsolete
	.long	NUM_FSYS	// obsolete

#if 1	/* this group of variables are not used by wee */
VARIABLE(graphics_inited)
	.long	0
	.long	0	// ABS(EXT_C(VARIABLE_GRAPHICS))
VARIABLE(font8x16)
	.long	0
VARIABLE(fontx)
	.long	0
VARIABLE(fonty)
	.long	0
VARIABLE(graphics_CURSOR)
	.long	0
	.long	0	// (EXT_C(menu_broder))
VARIABLE(gzip_filemax)
	.long	0
	.long	0
#endif
	.long	0	// reserved 1
	.long	0	// reserved 2
	.long	0	// reserved 3
	.long	0	// reserved 4
	.long	0	// reserved 5
	.long	0	// reserved 6
	.long	0	// reserved 7
	.long	0	// reserved 8
	.long	0	// reserved 9
	.long	0	// reserved 10

#endif /* ! STAGE1_5 */


