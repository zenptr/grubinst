/* builtins.c - the GRUB builtin commands */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004  Free Software Foundation, Inc.
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

/* Include stdio.h before shared.h, because we can't define
   WITHOUT_LIBC_STUBS here.  */

#include "shared.h"

extern unsigned long chain_load_from;
extern unsigned long chain_load_to;
extern unsigned long chain_load_dword_len;
extern unsigned long chain_boot_IP;//0x7c00;
extern unsigned long chain_edx;
extern unsigned long chain_ebx;
void HMA_start (void) __attribute__ ((noreturn));

void linux_boot (void) __attribute__ ((noreturn));
#define linux_boot ((void (*)(void))((char *)(&linux_boot) - 0x300000))

extern char *linux_data_real_addr;

/* For the Linux/i386 boot protocol version 2.03.  */
struct linux_kernel_header
{
  char code1[0x0020];
  unsigned short cl_magic;		/* Magic number 0xA33F */
  unsigned short cl_offset;		/* The offset of command line */
  char code2[0x01F1 - 0x0020 - 2 - 2];
  unsigned char setup_sects;		/* The size of the setup in sectors */
  unsigned short root_flags;		/* If the root is mounted readonly */
  unsigned short syssize;		/* obsolete */
  unsigned short swap_dev;		/* obsolete */
  unsigned short ram_size;		/* obsolete */
  unsigned short vid_mode;		/* Video mode control */
  unsigned short root_dev;		/* Default root device number */
  unsigned short boot_flag;		/* 0xAA55 magic number */
  unsigned short jump;			/* Jump instruction */
  unsigned long header;			/* Magic signature "HdrS" */
  unsigned short version;		/* Boot protocol version supported */
  unsigned long realmode_swtch;		/* Boot loader hook */
  unsigned long start_sys;		/* Points to kernel version string */
  unsigned char type_of_loader;		/* Boot loader identifier */
  unsigned char loadflags;		/* Boot protocol option flags */
  unsigned short setup_move_size;	/* Move to high memory size */
  unsigned long code32_start;		/* Boot loader hook */
  unsigned long ramdisk_image;		/* initrd load address */
  unsigned long ramdisk_size;		/* initrd size */
  unsigned long bootsect_kludge;	/* obsolete */
  unsigned short heap_end_ptr;		/* Free memory after setup end */
  unsigned short pad1;			/* Unused */
  char *cmd_line_ptr;			/* Points to the kernel command line */
  unsigned long initrd_addr_max;	/* The highest address of initrd */
} __attribute__ ((packed));

int command_func (char *arg/*, int flags*/);

/* The number of the history entries.  */
int num_history = 0;
int history;

int titles_count = 0;
int real_titles_count = 0;
int title_scripts[40];
char real_title_numbers[40];
int is_real_title;
int default_entry = 0;
int grub_timeout = -1;

char * get_history (void);
void add_history (void);

/* Get the NOth history. If NO is less than zero or greater than or
   equal to NUM_HISTORY, return NULL. Otherwise return a valid string.  */
char *
get_history (void)
{
  int j;
  char *p = (char *) HISTORY_BUF;
  if (history < 0 || history >= num_history)
	return 0;
  /* get history */
  for (j = 0; j < history; j++)
  {
	p += *(unsigned short *)p;
	if (p > (char *) HISTORY_BUF + HISTORY_BUFLEN)
	{
		num_history = j;
		return 0;
	}
  }

  return p + 2;
}

/* Add CMDLINE to the history buffer.  */
void
add_history (void)
{
  int j, len;
  char *p = (char *) HISTORY_BUF;
  char *cmdline = (char *) CMDLINE_BUF;

  if ((unsigned long)num_history >= 0x7FFFFFFFUL)
	return;
  if (*cmdline == 0 || *cmdline == 0x20)
	return;
  /* if string already in history, skip */

  for (j = 0; j < num_history; j++)
  {
	if (! grub_strcmp (p + 2, cmdline))
		return;
	p += *(unsigned short *)p;
	if (p > (char *) HISTORY_BUF + HISTORY_BUFLEN)
	{
		num_history = j;
		break;
	}
  }

  /* get cmdline length */
  len = grub_strlen (cmdline) + 3;
  p = (char *) HISTORY_BUF;
  if (((char *) HISTORY_BUF + HISTORY_BUFLEN) > (p + len))
	grub_memmove (p + len, p, ((char *) HISTORY_BUF + HISTORY_BUFLEN) - (p + len));
  *(unsigned short *)p = len;
  grub_strcpy (p + 2, cmdline);
  num_history++;
}

/* command */
int
command_func (char *arg/*, int flags*/)
{
  while (*arg == ' ' || *arg == '\t') arg++;
  if ((unsigned char)*arg < 0x20)
	return 0;

  /* open the command file. */
  {
	char *filename;
	char command_filename[256];

	grub_memmove (command_filename + 1, arg, sizeof(command_filename) - 2);
	command_filename[0] = '/';
	command_filename[255] = 0;
	nul_terminate (command_filename + 1);
	filename = command_filename + 1;

	if ((*arg >= 'a' && *arg <= 'z') || (*arg >= 'A' && *arg <= 'Z'))
		filename--;
	if (! grub_open (filename))
		return 0;
	if (((unsigned long *)(&filemax))[1] || (unsigned long)filemax < 9)
	{
		errnum = ERR_EXEC_FORMAT;
		goto fail;
	}
  }

  /* check if we have enough memory. */
  {
	unsigned long long memory_needed;

	arg = skip_to (arg);		/* get argument of command */
	memory_needed = filemax + ((grub_strlen (arg) + 16) & ~0xF) + 16 + 16;

	/* The amount of free memory is (free_mem_end - free_mem_start) */

	if (memory_needed > (free_mem_end - free_mem_start))
	{
		errnum = ERR_WONT_FIT;
		goto fail;
	}
  }

  /* Where is the free memory? build PSP, load the executable image. */
  {
	unsigned long pid = 255;
	unsigned long j;
	unsigned long psp_len;
	unsigned long prog_start;
	unsigned long long end_signature;

	for (j = 1;
		(unsigned long)&mem_alloc_array_start[j] < mem_alloc_array_end
		&& mem_alloc_array_start[j].addr; j++)
	{
	    if (pid < mem_alloc_array_start[j].pid)
		pid = mem_alloc_array_start[j].pid;
	}
	pid++;	/* new pid. */

	/* j - 1 is possibly for free memory start */
	--j;

	if ((mem_alloc_array_start[j].addr & 0x01))	/* no free memory. */
	{
		errnum = ERR_WONT_FIT;
		goto fail;
	}

	/* check sanity */
	if ((mem_alloc_array_start[j].addr & 0xFFFFFFF0) != free_mem_start)
	{
		errnum = ERR_INTERNAL_CHECK;
		goto fail;
	}

	psp_len = ((grub_strlen(arg) + 16) & ~0xF) + 16 + 16;
	prog_start = free_mem_start + psp_len;

	((unsigned long *)free_mem_start)[0] = psp_len;
	/* copy args into somewhere in PSP. */
	grub_memmove ((char *)(free_mem_start+16), arg, grub_strlen(arg)+1);
	/* PSP length in bytes. it is in both the starting dword and the
	 * ending dword of the PSP. */
	*(unsigned long *)(prog_start - 4) = psp_len;
	*(unsigned long *)(prog_start - 8) = psp_len - 16;	/* args */
	/* (prog_start - 16) is reserved for full pathname of the program. */
	if (grub_read (prog_start, -1ULL, 0xedde0d90) != filemax)
	{
		goto fail;
	}

	/* check exec signature. */
	end_signature = *(unsigned long long *)(prog_start + (unsigned long)filemax - 8);
	if (end_signature != 0xBCBAA7BA03051805ULL)
	{
		unsigned long boot_entry;
		unsigned long load_addr;
		unsigned long load_length;

		load_length = (unsigned long)filemax;
		/* check Linux */
		if (load_length < 5120 || load_length > 0x4000000)
			goto non_linux;
		{
		  //char *linux_data_real_addr;
		  char *linux_data_tmp_addr;
		  unsigned long cur_addr;
		  struct linux_kernel_header *lh = (struct linux_kernel_header *) prog_start;
		  int big_linux = 0;
		  int setup_sects = lh->setup_sects;
		  unsigned long data_len = 0;
		  unsigned long text_len = 0;
		  char *linux_bzimage_tmp_addr;

		  if (lh->boot_flag != 0xAA55 || setup_sects >= 0x40)
			goto non_linux;
		  if (lh->header == 0x53726448/* "HdrS" */ && lh->version >= 0x0200)
		  {
			big_linux = (lh->loadflags & 1/*LINUX_FLAG_BIG_KERNEL*/);
			lh->type_of_loader = 0x71/*LINUX_BOOT_LOADER_TYPE*/;

			/* Put the real mode part at as a high location as possible.  */
			linux_data_real_addr = (char *) (((*(unsigned short *)0x413) << 10) - 0x9400);
			/* But it must not exceed the traditional area.  */
			if (linux_data_real_addr > (char *) 0x90000)
			    linux_data_real_addr = (char *) 0x90000;

			if (lh->version >= 0x0201)
			{
			  lh->heap_end_ptr = (0x9000 - 0x200)/*LINUX_HEAP_END_OFFSET*/;
			  lh->loadflags |= 0x80/*LINUX_FLAG_CAN_USE_HEAP*/;
			}

			if (lh->version >= 0x0202)
			  lh->cmd_line_ptr = linux_data_real_addr + 0x9000/*LINUX_CL_OFFSET*/;
			else
			{
			  lh->cl_magic = 0xA33F/*LINUX_CL_MAGIC*/;
			  lh->cl_offset = 0x9000/*LINUX_CL_OFFSET*/;
			  lh->setup_move_size = 0x9400;
			}
		  } else {
			/* Your kernel is quite old...  */
			lh->cl_magic = 0xA33F/*LINUX_CL_MAGIC*/;
			lh->cl_offset = 0x9000/*LINUX_CL_OFFSET*/;
			if (setup_sects != 4)
				goto non_linux;
			linux_data_real_addr = (char *) 0x90000;
		  }

		  if (! setup_sects)
			setup_sects = 4;

		  data_len = setup_sects << 9;
		  text_len = load_length - data_len - 0x200;

		  //
		  // initrd
		  //
		  // ---------- new cur_addr ------------------------
		  //
		  // .........
		  //
		  // ---------- cur_addr ----------------------------
		  //
		  // a copy of bootsect+setup of vmlinuz
		  //
		  // ---------- linux_data_tmp_addr -----------------
		  //
		  // a copy of pmode code of vmlinuz, the "text"
		  //
		  // ---------- linux_bzimage_tmp_addr --------------
		  //
		  // vmlinuz
		  //
		  // ---------- prog_start --------------------------

		  linux_bzimage_tmp_addr = (char *)((prog_start + load_length + 15) & 0xFFFFFFF0);
		  linux_data_tmp_addr = (char *)(((unsigned long)(linux_bzimage_tmp_addr + text_len + 15)) & 0xFFFFFFF0);

		  if (! big_linux && text_len > linux_data_real_addr - (char *)0x10000)
		  {
			return !(errnum = ERR_WONT_FIT);
		  }
		  if (linux_data_real_addr + 0x9400 > ((char *) ((*(unsigned short *)0x413) << 10)))
		  {
			return !(errnum = ERR_WONT_FIT);
		  }

		  grub_printf ("   [Linux-%s, setup=0x%x, size=0x%x]\n", (big_linux ? "bzImage" : "zImage"), data_len, text_len);

		  grub_memmove (linux_data_tmp_addr, (char *)prog_start, data_len + 0x200);
		  if (lh->header != 0x53726448/* "HdrS" */ || lh->version < 0x0200)
			/* Clear the heap space.  */
			grub_memset (linux_data_tmp_addr + ((setup_sects + 1) << 9), 0, (64 - setup_sects - 1) << 9);

		  cur_addr = (unsigned long)(linux_data_tmp_addr + 0x9400 + 0xfff);
		  cur_addr &= 0xfffff000;

		/******************* begin loading INITRD *******************/
		{
		  unsigned long len;
		  //unsigned long long moveto;
		  //unsigned long long tmp;
		  //unsigned long long top_addr;

		  lh = (struct linux_kernel_header *) linux_data_tmp_addr;//(cur_addr - 0x9400);

		  len = 0;

		  /* now arg points to arguments of Linux kernel, i.e., initrd, etc. */

		  if (*arg != '(' && *arg != '/')
		    goto done_initrd;

		  if (! grub_open (arg))
			goto fail;

		  if ( (filemax >> 32)	// ((unsigned long *)(&filemax))[1]
			|| (unsigned long)filemax < 512
			|| (unsigned long)filemax > ((0xfffff000 & free_mem_end) - cur_addr)/*0x40000000UL*/)
		  {
			errnum = ERR_WONT_FIT;
			goto fail;
		  }

		  cur_addr = free_mem_end - (unsigned long)filemax; /* load to top */
		  cur_addr &= 0xfffff000;

		  if ((len = grub_read ((unsigned long long)cur_addr, -1ULL, 0xedde0d90)) != filemax)
			goto fail;

		  grub_printf ("   [Linux-initrd @ 0x%X, 0x%X bytes]\n", cur_addr, (unsigned long)len);

		  /* FIXME: Should check if the kernel supports INITRD.  */
		  lh->ramdisk_image = cur_addr;
		  lh->ramdisk_size = len;

		  arg = skip_to (arg);	/* points to the rest of command-line arguments */
		}
		/******************* end loading INITRD *******************/
done_initrd:

		  /* Copy command-line */
		  {
		    //char *src = arg;
		    char *dest = linux_data_tmp_addr + 0x9000;

		    while (dest < linux_data_tmp_addr + 0x93FF && *arg)
			*(dest++) = *(arg++);
		    *dest = 0;
		  }

		  grub_memmove (linux_bzimage_tmp_addr, (char *)(prog_start + data_len + 0x200), text_len);

		  if (errnum)
			return 0;

		  {
			unsigned int p;
			/* check grub.exe */
			for (p = (unsigned int)linux_bzimage_tmp_addr; p < (unsigned int)linux_bzimage_tmp_addr + 0x8000; p += 0x200)
			{
				if (((*(long long *)(void *)p & 0xFFFF00FFFFFFFFFFLL) == 0x02030000008270EALL) && ((*(long long *)(void *)(p + 0x12) & 0xFFFFFFFFFFLL) == 0x0037392E30LL))
				{
					if (*(long *)(void *)(p + 0x80) == 0xFFFFFFFF)//boot_drive
					{
						*(long *)(void *)(p + 0x80) = saved_drive;
						*(long *)(void *)(p + 0x08) = saved_partition;
					}
					break;
				}
			}
		  }
		  ///* Ugly hack.  */
		  //linux_text_len = text_len;

		  //type = (big_linux ? KERNEL_TYPE_BIG_LINUX : KERNEL_TYPE_LINUX);
		  //goto success; 
		  //grub_printf ("   linux_data_real_addr=%X, linux_data_tmp_addr=%X\n", linux_data_real_addr, linux_data_tmp_addr);
		  /* real mode part is in low mem, and above 0x10000, no conflict loading it now. */
		  //return 1;
		  grub_memmove (linux_data_real_addr, linux_data_tmp_addr, 0x9400);
		  /* loading the kernel code */
		  //grub_memmove ((char *)(big_linux ? 0x100000 : 0x10000), linux_bzimage_tmp_addr, text_len);
		  chain_load_to = (big_linux ? 0x100000 : 0x10000);
		  chain_load_from = (unsigned long)linux_bzimage_tmp_addr;
		  chain_load_dword_len = ((text_len + 3) >> 2);
		  //return 1;
		  linux_boot();	/* no return */
		}
non_linux:
		/* check pure 16-bit real-mode program.
		 * The size should be in the range 512 to 512K
		 */
		if (load_length < 512 || load_length > 0x80000)
			return ! (errnum = ERR_EXEC_FORMAT);
		if (end_signature == 0x85848F8D0C010512ULL)	/* realmode */
		{
			unsigned long ret;
			struct realmode_regs {
				unsigned long edi; // input and output
				unsigned long esi; // input and output
				unsigned long ebp; // input and output
				unsigned long esp; //stack pointer, input
				unsigned long ebx; // input and output
				unsigned long edx; // input and output
				unsigned long ecx; // input and output
				unsigned long eax;// input and output
				unsigned long gs; // input and output
				unsigned long fs; // input and output
				unsigned long es; // input and output
				unsigned long ds; // input and output
				unsigned long ss; //stack segment, input
				unsigned long eip; //instruction pointer, input
				unsigned long cs; //code segment, input
				unsigned long eflags; // input and output
			};

			struct realmode_regs regs;

			if (load_length + 0x10100 > ((*(unsigned short *)0x413) << 10))
				return ! (errnum = ERR_WONT_FIT);

			ret = grub_strlen (arg); /* command-tail count */
#define INSERT_LEADING_SPACE 0
#if INSERT_LEADING_SPACE
			if (ret > 125)
#else
			if (ret > 126)
#endif
				return ! (errnum = ERR_BAD_ARGUMENT);

			/* first, backup low 640K memory to address 2M */
			grub_memmove ((char *)0x200000, 0, 0xA0000);
			
			/* copy command-tail */
			if (ret)
#if INSERT_LEADING_SPACE
				grub_memmove ((char *)0x10082, arg, ret++);
#else
				grub_memmove ((char *)0x10081, arg, ret);
#endif

			/* setup offset 0x80 for command-tail count */
#if INSERT_LEADING_SPACE
			*(short *)0x10080 = (ret | 0x2000);
#else
			*(char *)0x10080 = ret;
#endif

			/* end the command-tail with CR */
			*(char *)(0x10081 + ret) = 0x0D;

			/* clear the beginning word of DOS PSP. the program
			 * check it and see it is running under grub4dos.
			 * a normal DOS PSP should begin with "CD 20".
			 */
			*(short *)0x10000 = 0;

			/* copy program to 1000:0100 */
			grub_memmove ((char *)0x10100, (char *)prog_start, load_length);

			/* setup DS, ES, CS:IP */
			regs.cs = regs.ds = regs.es = 0x1000;
			regs.eip = 0x100;

			/* setup FS, GS, EFLAGS and stack */
			regs.ss = regs.esp = regs.fs = regs.gs = regs.eflags = -1;

			/* for 64K .com style command, setup stack */
			if (load_length < 0xFF00)
			{
				regs.ss = 0x1000;
				regs.esp = 0xFFFE;
			}

			ret = realmode_run ((unsigned long)&regs);
			/* restore memory 0x10000 - 0xA0000 */
			grub_memmove ((char *)0x10000, (char *)0x210000, ((*(unsigned short *)0x413) << 10) - 0x10000);
			return ret;
		}
		if ((unsigned long)filemax <= (unsigned long)1024)
		{
			/* check 55 AA */
			if (*(unsigned short *)(prog_start + 510) != 0xAA55)
				return ! (errnum = ERR_EXEC_FORMAT);
			boot_entry = 0x00007C00; load_addr = boot_entry;
		} else if (*(unsigned char *)prog_start == (unsigned char)0xE9
			&& *(unsigned char *)(prog_start + 2) == 0x01
			&& (unsigned long)filemax > 0x30000)	/* NTLDR */
		{
			{
				char tmp_str[64];

				grub_sprintf (tmp_str, "(%d,%d)+1",
					(unsigned char)current_drive,
					*(((unsigned char *)
						&current_partition)+2));
				if (! grub_open (tmp_str))
					return 0;//! (errnum = ERR_EXEC_FORMAT);
				if (grub_read (0x7C00, 512, 0xedde0d90) != 512)
					return ! (errnum = ERR_EXEC_FORMAT);
			}
			if (*((unsigned long *) (0x7C00 + 0x1C)))
			    *((unsigned long *) (0x7C00 + 0x1C)) = part_start;

			boot_entry = 0x20000000; load_addr = 0x20000;
		} else if (*(short *)prog_start == 0x3EEB
			&& *(short *)(prog_start + 0x1FF8) >= 0x40
			&& *(short *)(prog_start + 0x1FF8) <= 0x1B8
			&& *(long *)(prog_start + 0x2000) == 0x008270EA
			&& (*(long *)(prog_start
				+ *(short *)(prog_start + 0x1FF8)))
					== (*(long *)(prog_start + 0x1FFC))
			&& filemax >= 0x4000)	/* GRLDR */
		{
			boot_entry = 0x10000000; load_addr = 0x10000;
		} else if ((*(long long *)prog_start | 0xFF00LL) == 0x4749464E4F43FFEBLL && filemax > 0x4000)	/* FreeDOS */
		{
			//boot_drive = current_drive;
			//install_partition = current_partition;
			chain_boot_IP = 0x00600000;
			chain_load_to = 0x0600;
drdos:
			chain_load_from = prog_start;
			chain_load_dword_len = ((load_length + 3) >> 2);
			chain_edx = current_drive;
			((char *)(&chain_edx))[1] = ((char *)(&current_partition))[2];
			chain_ebx = chain_edx;
			HMA_start();   /* no return */
		} else if ((*(short *)prog_start) == 0x5A4D && filemax > 0x10000
			&& (*(long *)(prog_start + 0x1FE)) == 0x4A420000
			&& (*(short *)(prog_start + 0x7FE)) == 0x534D
			&& (*(short *)(prog_start + 0x800)) != 0x4D43	)	/* MS-DOS 7.x, not WinMe */
		{
			//boot_drive = current_drive;
			//install_partition = current_partition;

			/* to prevent MS from wiping int vectors 32-3F. */
			/* search these 12 bytes ... */
			/* 83 C7 08	add di,08	*/
			/* B9 0E 00	mov cx,0E	*/
			/* AB		stosw		*/
			/* 83 C7 02	add di,02	*/
			/* E2 FA	loop (-6)	*/
			/* ... replace with NOPs */
			unsigned long p = prog_start + 0x800;
			unsigned long q = prog_start + load_length - 16;
			while (p < q)
			{
				if ((*(long *)p == 0xB908C783) &&
				    (*(long long*)(p+4)==0xFAE202C783AB000ELL))
				{
					((long *)p)[2]=((long *)p)[1]=
						*(long *)p = 0x90909090;
					p += 12;
					continue; /* would occur 3 times */
				}
				p++;
			}

			chain_boot_IP = 0x00700000;
			chain_load_to = 0x0700;
			chain_load_from = prog_start + 0x800;
			chain_load_dword_len = ((load_length - 0x800 + 3) >> 2);
			chain_edx = current_drive;
			((char *)(&chain_edx))[1] = ((char *)(&current_partition))[2];
			chain_ebx = 0;
			HMA_start();   /* no return */
		} else if ((*(long long *)prog_start == 0x501E0100122E802ELL) /* packed with pack101 */ || ((*(long long *)prog_start | 0xFFFF02LL) == 0x4F43000000FFFFEBLL && (*(((long long *)prog_start)+1) == 0x706D6F435141504DLL)))   /* DR-DOS */
		{
			chain_boot_IP = 0x00700000;
			chain_load_to = 0x0700;
			goto drdos;
		} else if ((*(long long *)(prog_start + 0x200)) == 0xCB5052C03342CA8CLL && (*(long *)(prog_start + 0x208) == 0x5441464B))   /* ROM-DOS */
		{
			chain_boot_IP = 0x10000000;
			chain_load_to = 0x10000;
			chain_load_from = prog_start + 0x200;
			chain_load_dword_len = ((load_length - 0x200 + 3) >> 2);
			*(unsigned long *)0x84 = current_drive | 0xFFFF0000;
			HMA_start();   /* no return */
		} else
			return ! (errnum = ERR_EXEC_FORMAT);

		//boot_drive = current_drive;
		//install_partition = current_partition;
		grub_memmove ((char *)load_addr, (char *)prog_start, load_length);
		return chain_stage1 (boot_entry);
	}

	/* allocate all memory to the program. */
	mem_alloc_array_start[j].addr |= 0x01;	/* memory is now in use. */
	mem_alloc_array_start[j].pid = pid;	/* with this pid. */

	//psp_len += free_mem_start;
	free_mem_start = free_mem_end;	/* no free mem for other programs. */

	/* call the new program. */
	pid = ((int (*)(void))prog_start)();	/* pid holds return value. */

	/* on exit, release the memory. */
	for (j = 1;
		(unsigned long)&mem_alloc_array_start[j] < mem_alloc_array_end
		&& mem_alloc_array_start[j].addr; j++)
			;
	--j;
	mem_alloc_array_start[j].pid = 0;
	free_mem_start = (mem_alloc_array_start[j].addr &= 0xFFFFFFF0);
	return pid;
  }


fail:

	if (! errnum)
		errnum = ERR_EXEC_FORMAT;
	return 0;
}

static struct builtin builtin_command =
{
  "command",
  command_func,
};

//-----------------------------------------------
#if 0
int
parse_string (char *arg)
{
  int len;
  char *p;
  char ch;
  int quote;

  //nul_terminate (arg);

  for (quote = len = 0, p = arg; (ch = *p); p++)
  {
	if (ch == '\\')
	{
		if (quote)
		{
			*arg++ = ch;
			len++;
			quote = 0;
			continue;
		}
		quote = 1;
		continue;
	}
	if (quote)
	{
		if (ch == 't')
		{
			*arg++ = '\t';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'r')
		{
			*arg++ = '\r';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'n')
		{
			*arg++ = '\n';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'a')
		{
			*arg++ = '\a';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'b')
		{
			*arg++ = '\b';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'f')
		{
			*arg++ = '\f';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'v')
		{
			*arg++ = '\v';
			len++;
			quote = 0;
			continue;
		}
		if (ch >= '0' && ch <= '7')
		{
			/* octal */
			int val = ch - '0';

			if (p[1] >= '0' && p[1] <= '7')
			{
				val *= 8;
				p++;
				val += *p -'0';
				if (p[1] >= '0' && p[1] <= '7')
				{
					val *= 8;
					p++;
					val += *p -'0';
				}
			}
			*arg++ = val;
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'x')
		{
			/* hex */
			int val;

			p++;
			ch = *p;
			if (ch >= '0' && ch <= '9')
				val = ch - '0';
			else if (ch >= 'A' && ch <= 'F')
				val = ch - 'A' + 10;
			else if (ch >= 'a' && ch <= 'f')
				val = ch - 'a' + 10;
			else
				return len;	/* error encountered */
			p++;
			ch = *p;
			if (ch >= '0' && ch <= '9')
				val = val * 16 + ch - '0';
			else if (ch >= 'A' && ch <= 'F')
				val = val * 16 + ch - 'A' + 10;
			else if (ch >= 'a' && ch <= 'f')
				val = val * 16 + ch - 'A' + 10;
			else
				p--;
			*arg++ = val;
			len++;
			quote = 0;
			continue;
		}
		if (ch)
		{
			*arg++ = ch;
			len++;
			quote = 0;
			continue;
		}
		return len;
	}
	*arg++ = ch;
	len++;
	quote = 0;
  }
  return len;
}


void hexdump(unsigned long ofs,char* buf,int len);

void hexdump(unsigned long ofs,char* buf,int len)
{
  while (len>0)
    {
      int cnt,k;

      grub_printf ("\n%08X: ", ofs);
      cnt=16;
      if (cnt>len)
        cnt=len;

      for (k=0;k<cnt;k++)
        {	  
          printf("%02X ", (unsigned long)(unsigned char)(buf[k]));
          if ((k!=15) && ((k & 3)==3))
            printf(" ");
        }

      for (;k<16;k++)
        {
          printf("   ");
          if ((k!=15) && ((k & 3)==3))
            printf(" ");
        }

      printf("; ");

      for (k=0;k<cnt;k++)
        printf("%c",(unsigned long)((((unsigned char)buf[k]>=32) && ((unsigned char)buf[k]!=127))?buf[k]:'.'));

      //printf("\n");

      ofs+=16;
      len-=cnt;
    }
}


/* cat */
static int
cat_func (char *arg/*, int flags*/)
{
  unsigned char c;
  unsigned char s[128];
  unsigned char r[128];
  unsigned long long Hex = 0;
  unsigned long long len, i, j;
  char *p;
  unsigned long long skip = 0;
  unsigned long long length = 0xffffffffffffffffULL;
  char *locate = 0;
  char *replace = 0;
  unsigned long long locate_align = 1;
  unsigned long long len_s;
  unsigned long long len_r = 0;
  unsigned long long ret = 0;

  for (;;)
  {
	if (grub_memcmp (arg, "--hex=", 6) == 0)
	{
		p = arg + 6;
		if (! safe_parse_maxint (&p, &Hex))
			return 0;
	}
    else if (grub_memcmp (arg, "--hex", 5) == 0)
      {
	Hex = 1;
      }
    else if (grub_memcmp (arg, "--skip=", 7) == 0)
      {
	p = arg + 7;
	if (! safe_parse_maxint (&p, &skip))
		return 0;
      }
    else if (grub_memcmp (arg, "--length=", 9) == 0)
      {
	p = arg + 9;
	if (! safe_parse_maxint (&p, &length))
		return 0;
      }
    else if (grub_memcmp (arg, "--locate=", 9) == 0)
      {
	p = locate = arg + 9;
	if (*p == '\"')
	{
	  while (*(++p) != '\"');
	  arg = ++p; // or: arg = p;
	}
      }
    else if (grub_memcmp (arg, "--replace=", 10) == 0)
      {
	p = replace = arg + 10;
	if (*replace == '*')
	{
        replace++;
        if (! safe_parse_maxint (&replace, &len_r))
			replace = p;
	} 
	if (*p == '\"')
	{
	  while (*(++p) != '\"');
	  arg = ++p;
	}
      }
    else if (grub_memcmp (arg, "--locate-align=", 15) == 0)
      {
	p = arg + 15;
	if (! safe_parse_maxint (&p, &locate_align))
		return 0;
	if ((unsigned long)locate_align == 0)
		return ! (errnum = ERR_BAD_ARGUMENT);
      }
    else
	break;
    arg = skip_to (arg);
  }
  if (! length)
  {
	if (grub_memcmp (arg,"()-1\0",5) == 0 )
	{
        if (! grub_open ("()+1"))
            return 0;
        filesize = filemax*(unsigned long long)part_start;
	} 
	else if (grub_memcmp (arg,"()\0",3) == 0 )
     {
        if (! grub_open ("()+1"))
            return 0;
        filesize = filemax*(unsigned long long)part_length;
     }
    else 
    {
       if (! grub_open (arg))
            return 0;
       filesize = filemax;
     }
		grub_printf ("\nFilesize is 0x%lX", (unsigned long long)filesize);
	ret = filemax;
	return ret;
  }
  if (! grub_open (arg))
    return 0; 
  if (length > filemax)
      length = filemax;
  filepos = skip;
  if (replace)
  {
	if (! len_r)
	{
		Hex = 0;
        if (*replace == '\"')
        {
        for (i = 0; i < 128 && (r[i] = *(++replace)) != '\"'; i++);
        }else{
        for (i = 0; i < 128 && (r[i] = *(replace++)) != ' ' && r[i] != '\t'; i++);
        }
        r[i] = 0;
        len_r = parse_string ((char *)r);
    }
    else
      {
		if (! Hex)
			Hex = -1ULL;
      }
  }
  if (locate)
  {
    if (*locate == '\"')
    {
      for (i = 0; i < 128 && (s[i] = *(++locate)) != '\"'; i++);
    }else{
      for (i = 0; i < 128 && (s[i] = *(locate++)) != ' ' && s[i] != '\t'; i++);
    }
    s[i] = 0;
    len_s = parse_string ((char *)s);
    if (len_s == 0 || len_s > 16)
	return ! (errnum = ERR_BAD_ARGUMENT);
    //j = skip;
    grub_memset ((char *)(SCRATCHADDR), 0, 32);
    for (j = skip; ; j += 16)
    {
	len = 0;
	if (j - skip < length)
	    len = grub_read ((unsigned long long)(SCRATCHADDR + 16), 16, 0xedde0d90);
	if (len < 16)
	    grub_memset ((char *)(SCRATCHADDR + 16 + (unsigned long)len), 0, 16 - len);

	if (j != skip)
	{
	    for (i = 0; i < 16; i++)
	    {
		unsigned long long k = j - 16 + i;
		if (locate_align == 1 || ! ((unsigned long)k % (unsigned long)locate_align))
		    if (! grub_memcmp ((char *)&s, (char *)(SCRATCHADDR + (unsigned long)i), len_s))
		    {
			/* print the address */
				grub_printf (" %lX", (unsigned long long)k);
			/* replace strings */
			if ((replace) && len_r)
			{
				unsigned long long filepos_bak;

				filepos_bak = filepos;
				filepos = k;
				if (Hex)
				  {
                    grub_read (len_r,(Hex == -1ULL)?8:Hex, 0x900ddeed);
					i = ((Hex == -1ULL)?8:Hex)+i;
                  }
				else
				 {
	 				/* write len_r bytes at string r to file!! */
                    grub_read ((unsigned long long)(unsigned int)(char *)&r, len_r, 0x900ddeed);
                    i = i+len_r;
                  }
				i--;
				filepos = filepos_bak;
			}
			ret++;
		    }
	    }
	}
	if (len == 0)
	    break;
	grub_memmove ((char *)SCRATCHADDR, (char *)(SCRATCHADDR + 16), 16);
    }
  }else if (Hex == (++ret))	/* a trick for (ret = 1, Hex == 1) */
  {
    for (j = skip; j - skip < length && (len = grub_read ((unsigned long long)(unsigned long)&s, 16, 0xedde0d90)); j += 16)
    {
		hexdump(j,(char*)&s,(len>length+skip-j)?(length+skip-j):len);
    }
  }else
    for (j = 0; j < length && grub_read ((unsigned long long)(unsigned long)&c, 1, 0xedde0d90); j++)
    {
#if 1
		console_putchar (c);
#else
	/* Because running "cat" with a binary file can confuse the terminal,
	   print only some characters as they are.  */
	if (grub_isspace (c) || (c >= ' ' && c <= '~'))
		grub_putchar (c);
	else
		grub_putchar ('?');
#endif
    }
  
  return ret;
}

static struct builtin builtin_cat =
{
  "cat",
  cat_func,
};
#endif

#if 0

struct drive_map_slot
{
	/* Remember to update DRIVE_MAP_SLOT_SIZE once this is modified.
	 * The struct size must be a multiple of 4.
	 */

	  /* X=max_sector bit 7: read only or fake write */
	  /* Y=to_sector  bit 6: safe boot or fake write */
	  /* ------------------------------------------- */
	  /* X Y: meaning of restrictions imposed on map */
	  /* ------------------------------------------- */
	  /* 1 1: read only=0, fake write=1, safe boot=0 */
	  /* 1 0: read only=1, fake write=0, safe boot=0 */
	  /* 0 1: read only=0, fake write=0, safe boot=1 */
	  /* 0 0: read only=0, fake write=0, safe boot=0 */

	unsigned char from_drive;
	unsigned char to_drive;		/* 0xFF indicates a memdrive */
	unsigned char max_head;
	unsigned char max_sector;	/* bit 7: read only */
					/* bit 6: disable lba */

	unsigned short to_cylinder;	/* max cylinder of the TO drive */
					/* bit 15:  TO  drive support LBA */
					/* bit 14:  TO  drive is CDROM(with big 2048-byte sector) */
					/* bit 13: FROM drive is CDROM(with big 2048-byte sector) */

	unsigned char to_head;		/* max head of the TO drive */
	unsigned char to_sector;	/* max sector of the TO drive */
					/* bit 7: in-situ */
					/* bit 6: fake-write or safe-boot */

	unsigned long long start_sector;
	//unsigned long start_sector_hi;	/* hi dword of the 64-bit value */
	unsigned long long sector_count;
	//unsigned long sector_count_hi;	/* hi dword of the 64-bit value */
};

struct drive_map_slot bios_drive_map[DRIVE_MAP_SIZE + 1];
extern struct drive_map_slot hooked_drive_map[DRIVE_MAP_SIZE + 1];
void set_int13_handler (struct drive_map_slot *map);
int unset_int13_handler (int check_status_only);

int map_func (char *);

static int
drive_map_slot_empty (struct drive_map_slot item)
{
	unsigned long *array = (unsigned long *)&item;
	
	unsigned long n = sizeof (struct drive_map_slot) / sizeof (unsigned long);
	
	while (n)
	{
		if (*array)
			return 0;
		array++;
		n--;
	}

	return 1;
	//if (*(unsigned long *)(&(item.from_drive))) return 0;
	//if (item.start_sector) return 0;
	//if (item.sector_count) return 0;
	//return 1;
}

static int
drive_map_slot_equal (struct drive_map_slot a, struct drive_map_slot b)
{
	//return ! grub_memcmp ((void *)&(a.from_drive), (void *)&(b.from_drive), sizeof(struct drive_map_slot));
	return ! grub_memcmp ((void *)&a, (void *)&b, sizeof(struct drive_map_slot));
	//if (*(unsigned long *)(&(a.from_drive)) != *(unsigned long *)(&(b.from_drive))) return 0;
	//if (a.start_sector != b.start_sector) return 0;
	//if (a.sector_count != b.sector_count) return 0;
	//return 1;
}

/* map */
/* Map FROM_DRIVE to TO_DRIVE.  */
int
map_func (char *arg/*, int flags*/)
{
  char *to_drive;
  char *from_drive;
  unsigned long to, from;
  int i, j;
  
  for (;;)
  {
    if (grub_memcmp (arg, "--status", 8) == 0)
      {
	if (rd_base != -1ULL)
	    grub_printf ("\nram_drive=0x%X, rd_base=0x%lX, rd_size=0x%lX\n", ram_drive, rd_base, rd_size); 
	if (unset_int13_handler (1) && drive_map_slot_empty (bios_drive_map[0]))
	  {
	    grub_printf ("\nThe int13 hook is off. The drive map table is currently empty.\n");
	    return 1;
	  }

	  grub_printf ("\nFr To Hm Sm To_C _H _S   Start_Sector     Sector_Count   DHR"
		       "\n-- -- -- -- ---- -- -- ---------------- ---------------- ---\n");
	if (! unset_int13_handler (1))
	  for (i = 0; i < DRIVE_MAP_SIZE; i++)
	    {
		if (drive_map_slot_empty (hooked_drive_map[i]))
			break;
		for (j = 0; j < DRIVE_MAP_SIZE; j++)
		  {
		    if (drive_map_slot_equal(bios_drive_map[j], hooked_drive_map[i]))
			break;
		  }
		  grub_printf ("%02X %02X %02X %02X %04X %02X %02X %016lX %016lX %c%c%c\n", hooked_drive_map[i].from_drive, hooked_drive_map[i].to_drive, hooked_drive_map[i].max_head, hooked_drive_map[i].max_sector, hooked_drive_map[i].to_cylinder, hooked_drive_map[i].to_head, hooked_drive_map[i].to_sector, (unsigned long long)hooked_drive_map[i].start_sector, (unsigned long long)hooked_drive_map[i].sector_count, ((hooked_drive_map[i].to_cylinder & 0x4000) ? 'C' : hooked_drive_map[i].to_drive < 0x80 ? 'F' : hooked_drive_map[i].to_drive == 0xFF ? 'M' : 'H'), ((j < DRIVE_MAP_SIZE) ? '=' : '>'), ((hooked_drive_map[i].max_sector & 0x80) ? ((hooked_drive_map[i].to_sector & 0x40) ? 'F' : 'R') :((hooked_drive_map[i].to_sector & 0x40) ? 'S' : 'U')));
	    }
	for (i = 0; i < DRIVE_MAP_SIZE; i++)
	  {
	    if (drive_map_slot_empty (bios_drive_map[i]))
		break;
	    if (! unset_int13_handler (1))
	      {
		for (j = 0; j < DRIVE_MAP_SIZE; j++)
		  {
		    if (drive_map_slot_equal(hooked_drive_map[j], bios_drive_map[i]))
			break;
		  }
		if (j < DRIVE_MAP_SIZE)
			continue;
	      }
		  grub_printf ("%02X %02X %02X %02X %04X %02X %02X %016lX %016lX %c<%c\n", bios_drive_map[i].from_drive, bios_drive_map[i].to_drive, bios_drive_map[i].max_head, bios_drive_map[i].max_sector, bios_drive_map[i].to_cylinder, bios_drive_map[i].to_head, bios_drive_map[i].to_sector, (unsigned long long)bios_drive_map[i].start_sector, (unsigned long long)bios_drive_map[i].sector_count, ((bios_drive_map[i].to_cylinder & 0x4000) ? 'C' : bios_drive_map[i].to_drive < 0x80 ? 'F' : bios_drive_map[i].to_drive == 0xFF ? 'M' : 'H'), ((bios_drive_map[i].max_sector & 0x80) ? ((bios_drive_map[i].to_sector & 0x40) ? 'F' : 'R') :((bios_drive_map[i].to_sector & 0x40) ? 'S' : 'U')));
	  }
	return 1;
      }
    else if (grub_memcmp (arg, "--hook", 6) == 0)
      {
	char *p;

	p = arg + 6;
	unset_int13_handler (0);
	if (drive_map_slot_empty (bios_drive_map[0]))
		return ! (errnum = ERR_NO_DRIVE_MAPPED);
	set_int13_handler (bios_drive_map);
	buf_drive = -1;
	buf_track = -1;
	return 1;
      }
    else if (grub_memcmp (arg, "--unhook", 8) == 0)
      {
	unset_int13_handler (0);
	buf_drive = -1;
	buf_track = -1;
	return 1;
      }
    else
	break;
    arg = skip_to (arg);
  }
  
  to_drive = arg;
  from_drive = skip_to (arg);

  /* Get the drive number for TO_DRIVE.  */
  set_device (to_drive);
  if (errnum)
    return 1;
  to = current_drive;

  /* Get the drive number for FROM_DRIVE.  */
  set_device (from_drive);
  if (errnum)
    return 1;
  from = current_drive;

  /* Search for an empty slot in BIOS_DRIVE_MAP.  */
  for (i = 0; i < DRIVE_MAP_SIZE; i++)
    {
      /* Perhaps the user wants to override the map.  */
      if (((*(char *)(&(bios_drive_map[i]))) /*& 0xff*/) == from)
	break;
      
      if (! (*(unsigned short *)(void *)(&(bios_drive_map[i]))))
	break;
    }

  if (i == DRIVE_MAP_SIZE)
    {
      errnum = ERR_WONT_FIT;
      return 1;
    }

  if (to == from)
    /* If TO is equal to FROM, delete the entry.  */
    grub_memmove ((char *) &bios_drive_map[i], (char *) &bios_drive_map[i + 1],
		  sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE - i));
  else
    (*(unsigned short *)(void *)(&(bios_drive_map[i]))) = from | (to << 8);
  
  return 0;
}

static struct builtin builtin_map =
{
  "map",
  map_func,
};
#endif

#if 1

extern int attempt_mount (void);
extern unsigned long request_fstype;
extern unsigned long dest_partition;
extern unsigned long entry;

static int
real_root_func (char *arg, int attempt_mnt)
{
  char *next;
  unsigned long i, tmp_drive = 0;
  unsigned long tmp_partition = 0;
  char ch;

  /* Get the drive and the partition.  */
  if (! *arg || *arg == ' ' || *arg == '\t')
    {
	current_drive = saved_drive;
	current_partition = saved_partition;
	next = 0; /* If ARG is empty, just print the current root device.  */
    }
  else
    {
	/* Call set_device to get the drive and the partition in ARG.  */
	if (! (next = set_device (arg)))
	    return 0;
    }

  if (next)
  {
	/* check the length of the root prefix, i.e., NEXT */
	for (i = 0; i < sizeof (saved_dir); i++)
	{
		ch = next[i];
		if (ch == 0 || ch == 0x20 || ch == '\t')
			break;
		if (ch == '\\')
		{
			//saved_dir[i] = ch;
			i++;
			ch = next[i];
			if (! ch || i >= sizeof (saved_dir))
			{
				i--;
				//saved_dir[i] = 0;
				break;
			}
		}
	}

	if (i >= sizeof (saved_dir))
	{
		errnum = ERR_WONT_FIT;
		return 0;
	}

	tmp_partition = current_partition;
	tmp_drive = current_drive;
  }

  errnum = ERR_NONE;

  /* Ignore ERR_FSYS_MOUNT.  */
  if (attempt_mnt)
    {
      if (! ((request_fstype = 1), (dest_partition = current_partition), open_partition ()) && errnum != ERR_FSYS_MOUNT)
	return 0;

      //if (fsys_type != NUM_FSYS || ! next)
      {
	grub_printf ("(%X,%d):%lx,%lx:%02X,%02X:%s\n"
		, (long)current_drive
		, (long)(((unsigned char *)(&current_partition))[2])
		, (unsigned long long)part_start
		, (unsigned long long)part_length
		, (long)(((unsigned char *)(&current_partition))[0])
		, (long)(((unsigned char *)(&current_partition))[1])
		, fsys_table[fsys_type].name
	    );
      }
      //else
	//return ! (errnum = ERR_FSYS_MOUNT);
    }
  
  if (next)
  {
	saved_partition = tmp_partition;
	saved_drive = tmp_drive;

	/* copy root prefix to saved_dir */
	for (i = 0; i < sizeof (saved_dir); i++)
	{
		ch = next[i];
		if (ch == 0 || ch == 0x20 || ch == '\t')
			break;
		if (ch == '\\')
		{
			saved_dir[i] = ch;
			i++;
			ch = next[i];
			if (! ch || i >= sizeof (saved_dir))
			{
				i--;
				saved_dir[i] = 0;
				break;
			}
		}
		saved_dir[i] = ch;
	}

	if (saved_dir[i-1] == '/')
	{
		saved_dir[i-1] = 0;
	} else
		saved_dir[i] = 0;
  }

  if (*saved_dir)
	grub_printf ("The current working directory(i.e., the relative path) is %s\n", saved_dir);

  /* Clear ERRNUM.  */
  errnum = 0;
  /* If ARG is empty, then return TRUE for harddrive, and FALSE for floppy */
  return next ? 1 : (saved_drive & 0x80);
}

static int
root_func (char *arg/*, int flags*/)
{
  return real_root_func (arg, 1);
}

static struct builtin builtin_root =
{
  "root",
  root_func,
};

static int
rootnoverify_func (char *arg/*, int flags*/)
{
  return real_root_func (arg, 0);
}

static struct builtin builtin_rootnoverify =
{
  "rootnoverify",
  rootnoverify_func,
};

#endif




#if 1

//static int real_root_func (char *arg1, int attempt_mnt);
/* find */
/* Search for the filename ARG in all of partitions and optionally make that
 * partition root("--set-root", Thanks to Chris Semler <csemler@mail.com>).
 */
static int
find_func (char *arg/*, int flags*/)
{
  struct builtin *builtin1 = 0;
  int ret;
  char *filename;
  unsigned long drive;
  unsigned long tmp_drive = saved_drive;
  unsigned long tmp_partition = saved_partition;
  unsigned long got_file = 0;
  char *set_root = 0;
//  unsigned long ignore_cd = 0;
  unsigned long ignore_floppies = 0;
  unsigned long ignore_oem = 0;
  //char *in_drives = NULL;	/* search in drive list */
  char root_found[16];
  
  for (;;)
  {
    if (grub_memcmp (arg, "--set-root=", 11) == 0)
      {
	set_root = arg + 11;
	if (*set_root && *set_root != ' ' && *set_root != '\t' && *set_root != '/')
		return ! (errnum = ERR_FILENAME_FORMAT);
      }
    else if (grub_memcmp (arg, "--set-root", 10) == 0)
      {
	set_root = "";
      }
//    else if (grub_memcmp (arg, "--ignore-cd", 11) == 0)
//      {
//	ignore_cd = 1;
//      }
    else if (grub_memcmp (arg, "--ignore-floppies", 17) == 0)
      {
	ignore_floppies = 1;
      }
	else if (grub_memcmp (arg, "--ignore-oem", 12) == 0)
      {
	ignore_oem = 1;
      }
    else
	break;
    arg = skip_to (arg);
  }
  
  /* The filename should NOT have a DEVICE part. */
  filename = set_device (arg);
  if (filename)
	return ! (errnum = ERR_FILENAME_FORMAT);

  if (*arg == '/' || *arg == '+' || (*arg >= '0' && *arg <= '9'))
  {
    filename = arg;
    arg = skip_to (arg);
  } else {
    filename = 0;
  }

  /* arg points to command. */

  if (*arg)
  {
    builtin1 = find_command (arg);
//    if ((int)builtin1 != -1)
//    if (! builtin1 /*|| ! (builtin1->flags & flags)*/)
//    {
//	errnum = ERR_UNRECOGNIZED;
//	return 0;
//    }
    if ((int)builtin1/* != -1*/)
	arg = skip_to (arg);	/* get argument of command */
    else
	builtin1 = &builtin_command;
  }

  errnum = 0;

  /* Hard disks. Search in hard disks first, since floppies are slow */
#define FIND_DRIVES (*((char *)0x475))
  for (drive = 0x80; /*drive < 0x80 + FIND_DRIVES*/; drive++)
    {
      //unsigned long part = 0xFFFFFF;
      //unsigned long start, len, offset, ext_offset1;
      //unsigned long type, entry1;
      unsigned long type;
      unsigned long i;

      struct part_info *pi;

      if (drive >= 0x80 + FIND_DRIVES)
#undef FIND_DRIVES
      {
	if (ignore_floppies)
		break;
	drive = 0;	/* begin floppy */
      }
#define FIND_DRIVES (((*(char*)0x410) & 1)?((*(char*)0x410) >> 6) + 1 : 0)
      if (drive < 0x80)
      {
	if (drive >= FIND_DRIVES)
		break;	/* end floppy */
      }
#undef FIND_DRIVES

      pi = (struct part_info *)(PART_TABLE_BUF);

      current_drive = drive;
      list_partitions ();
      for (i = 1; pi[i].part_num != 0xFFFF; i++)
      /* while ((	next_partition_drive		= drive,
		next_partition_dest		= 0xFFFFFF,
		next_partition_partition	= &part,
		next_partition_type		= &type,
		next_partition_start		= &start,
		next_partition_len		= &len,
		next_partition_offset		= &offset,
		next_partition_entry		= &entry1,
		next_partition_ext_offset	= &ext_offset1,
		next_partition_buf		= mbr,
		next_partition ())) */
	{
	  type = pi[i].part_type;
	  if (type != PC_SLICE_TYPE_NONE
		  && ! (ignore_oem == 1 && (type & ~PC_SLICE_TYPE_HIDDEN_FLAG) == 0x02) 
	      //&& ! IS_PC_SLICE_TYPE_BSD (type)
	      && ! IS_PC_SLICE_TYPE_EXTENDED (type))
	    {
	      ((unsigned short *)(&current_partition))[1] = (pi[i].part_num);
	      if (filename == 0)
		{
		  saved_drive = current_drive;
		  saved_partition = current_partition;

		  ret = builtin1 ? (builtin1->func) (arg/*, flags*/) : 1;
		  if (ret)
		  {
			grub_sprintf (root_found, "(0x%X,%d)", drive, pi[i].part_num);
		        grub_printf ("%s ", root_found);
			got_file = 1;
		        if (set_root && (drive < 0x80 || pi[i].part_num != 0xFF))
				goto found;
		  }
		}
	      else if (open_device ())
		{
		  saved_drive = current_drive;
		  saved_partition = current_partition;
		  if (grub_open (filename))
		    {
		      ret = builtin1 ? (builtin1->func) (arg/*, flags*/) : 1;
		      if (ret)
		      {
			grub_sprintf (root_found, "(0x%X,%d)", drive, pi[i].part_num);
			grub_printf ("%s ", root_found);
		        got_file = 1;
		        if (set_root && (drive < 0x80 || pi[i].part_num != 0xFF))
				goto found;
		      }
		    }
		}
	    }

	  /* We want to ignore any error here.  */
	  errnum = ERR_NONE;
	}

	if (got_file && set_root)
		goto found;

      /* next_partition always sets ERRNUM in the last call, so clear
	 it.  */
      errnum = ERR_NONE;
    }

#if 0
  /* CD-ROM.  */
  if (cdrom_drive != GRUB_INVALID_DRIVE && ! ignore_cd)
    {
      current_drive = cdrom_drive;
      current_partition = 0xFFFFFF;
      
      if (filename == 0)
	{
	  saved_drive = current_drive;
	  saved_partition = current_partition;
	  ret = builtin1 ? (builtin1->func) (arg/*, flags*/) : 1;
	  if (ret)
	  {
	    grub_sprintf (root_found, "(cd)");
		grub_printf (" %s\n", root_found);
	    got_file = 1;
	    if (set_root)
		goto found;
	  }
	}
      else if (open_device ())
	{
	  saved_drive = current_drive;
	  saved_partition = current_partition;
	  if (grub_open (filename))
	    {
	      ret = builtin1 ? (builtin1->func) (arg/*, flags*/) : 1;
	      if (ret)
	      {
	        grub_sprintf (root_found, "(cd)");
		  grub_printf (" %s\n", root_found);
	        got_file = 1;
	        if (set_root)
		  goto found;
	      }
	    }
	}

      errnum = ERR_NONE;
    }

  /* Floppies.  */
#define FIND_DRIVES (((*(char*)0x410) & 1)?((*(char*)0x410) >> 6) + 1 : 0)
  if (! ignore_floppies)
  {
    for (drive = 0; drive < 0 + FIND_DRIVES; drive++)
#undef FIND_DRIVES
    {
      current_drive = drive;
      current_partition = 0xFFFFFF;
      
      if (filename == 0)
	{
	  saved_drive = current_drive;
	  saved_partition = current_partition;
	  ret = builtin1 ? (builtin1->func) (arg/*, flags*/) : 1;
	  if (ret)
	  {
	    grub_sprintf (root_found, "(fd%d)", drive);
		grub_printf ("\n%s", root_found);
	    got_file = 1;
	    if (set_root)
		goto found;
	  }
	}
      else if (((request_fstype = 1), (dest_partition = current_partition), open_partition ()))
	{
	  saved_drive = current_drive;
	  saved_partition = current_partition;
	  if (grub_open (filename))
	    {
	      ret = builtin1 ? (builtin1->func) (arg/*, flags*/) : 1;
	      if (ret)
	      {
	        grub_sprintf (root_found, "(fd%d)", drive);
		  grub_printf ("\n%s", root_found);
	        got_file = 1;
	        if (set_root)
		  goto found;
	      }
	    }
	}

      errnum = ERR_NONE;
    }
  }
#endif

found:
  saved_drive = tmp_drive;
  saved_partition = tmp_partition;

  if (got_file)
    {
	errnum = ERR_NONE;
	if (set_root)
	{
		int j;

		//return real_root_func (root_found, 1);
		saved_drive = current_drive;
		saved_partition = current_partition;
		/* copy root prefix to saved_dir */
		for (j = 0; j < sizeof (saved_dir); j++)
		{
		    char ch;

		    ch = set_root[j];
		    if (ch == 0 || ch == 0x20 || ch == '\t')
			break;
		    if (ch == '\\')
		    {
			saved_dir[j] = ch;
			j++;
			ch = set_root[j];
			if (! ch || j >= sizeof (saved_dir))
			{
				j--;
				saved_dir[j] = 0;
				break;
			}
		    }
		    saved_dir[j] = ch;
		}

		if (saved_dir[j-1] == '/')
		{
		    saved_dir[j-1] = 0;
		} else
		    saved_dir[j] = 0;
	}
	return 1;
    }

  errnum = ERR_FILE_NOT_FOUND;
  return 0;
}

static struct builtin builtin_find =
{
  "find",
  find_func,
};

#endif

struct builtin builtin_exit =
{
  "exit",
  0/* we have no exit_func!! */,
};

struct builtin builtin_title =
{
  "title",
  0/* we have no title_func!! */,
};

static int
default_func (char *arg/*, int flags*/)
{
  unsigned long long ull;
  if (! safe_parse_maxint (&arg, &ull))
    return 0;

  default_entry = ull;
  return 1;
}

static struct builtin builtin_default =
{
  "default",
  default_func,
};

static int
timeout_func (char *arg/*, int flags*/)
{
  unsigned long long ull;
  if (! safe_parse_maxint (&arg, &ull))
    return 0;

  grub_timeout = ull;
  return 1;
}

static struct builtin builtin_timeout =
{
  "timeout",
  timeout_func,
};

static int
pause_func (char *arg/*, int flags*/)
{
  char *p;
  unsigned long long wait = -1;
  unsigned long i;

  for (;;)
  {
    if (grub_memcmp (arg, "--wait=", 7) == 0)
      {
        p = arg + 7;
        if (! safe_parse_maxint (&p, &wait))
                return 0;
      }
    else
        break;
    arg = skip_to (/*0, */arg);
  }

  printf("%s\n", arg);

  if ((unsigned long)wait != -1)
  {
    /* calculate ticks */
    i = ((unsigned long)wait) * 91;
    wait = i / 5;
  }
  for (i = 0; i <= (unsigned long)wait; i++)
  {
    /* Check if there is a key-press.  */
    if (console_checkkey () != -1)
	return console_getkey ();
  }

  return 1;
}

static struct builtin builtin_pause =
{
  "pause",
  pause_func,
};

/* The table of builtin commands. Sorted in dictionary order.  */
struct builtin *builtin_table[] =
{
  //&builtin_cat,
  &builtin_command,
  &builtin_default,
  &builtin_exit,
  &builtin_find,
  //&builtin_map,
  &builtin_pause,
  &builtin_root,
  &builtin_rootnoverify,
  &builtin_timeout,
  &builtin_title,
  0
};



