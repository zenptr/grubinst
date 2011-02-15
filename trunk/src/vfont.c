#include "grub4dos.h"
#define CHAR_BUF1 ((char *)0x7C30)
#define CHAR_BUF2 ((char *)0x7E00)
struct realmode_regs {
	unsigned long edi; // as input and output
	unsigned long esi; // as input and output
	unsigned long ebp; // as input and output
	unsigned long esp; // stack pointer, as input
	unsigned long ebx; // as input and output
	unsigned long edx; // as input and output
	unsigned long ecx; // as input and output
	unsigned long eax;// as input and output
	unsigned long gs; // as input and output
	unsigned long fs; // as input and output
	unsigned long es; // as input and output
	unsigned long ds; // as input and output
	unsigned long ss; // stack segment, as input
	unsigned long eip; // instruction pointer, as input, 
	unsigned long cs; // code segment, as input
	unsigned long eflags; // as input and output
};
int GRUB = 0x42555247;/* this is needed, see the following comment. */
/* gcc treat the following as data only if a global initialization like the
 * above line occurs.
 */
asm(".long 0x534F4434");
/* a valid executable file for grub4dos must end with these 8 bytes */
asm(".long 0x03051805");
asm(".long 0xBCBAA7BA");

/* thank goodness gcc will place the above 8 bytes at the end of the b.out
 * file. Do not insert any other asm lines here.
 */

int
main ()
{
	void *p = &main;
	char *arg = p - (*(int *)(p - 8));
	int flags = (*(int *)(p - 12));
	/*
	通过调用BIOS   10h替换系统字模来显示汉字   
	入口:	ax=1100h
		bh=字模的高度(有效值：0～20h，默认值：10h)
		bl=被替换的字模集代号(有效值：0～7)
		cx=要替换的字模数
		dx=被替换的第一个字模所对应的字符的ASCII
		es:bp=新字模起始地址
		int   10h
	恢复系统原字符集：
		ax=1104h
		bl=字模集代号(有效值：0～7)   
		int 10h
	*/
	unsigned char prog[40] = {
				0xBA, 0xC0, 0x00, 0xBD, 0x30, 0x7C, 0xB8, 0x00,
				0x11, 0xBB, 0x00, 0x10, 0xCD, 0x10, 0xBA, 0x80,
				0x00, 0xBD, 0x00, 0x7E, 0xCD, 0x10, 0xEB, 0x07,
				/*
				以下程序用于恢复字模，汇编源码:
				B8 04 11	mov ax,1104
				B3 00		mov bl,0
				CD 10		int 10
				*/
				0xB8,0x04,0x11,0xb3,0x00,0xcd,0x10,
				/*ljmp 0000:8200*/
				0xEA, 0x00, 0x82, 0x00,0x00};
	struct realmode_regs int_regs = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
	memmove((char*)0x7c00,prog,40);
	if (*arg == 0)//如果没有任何参数，则恢复字模
	{
		int_regs.eip = 0x7C18;
		int_regs.cs = 0;
		return realmode_run((long)&int_regs);
	}

	if (!open(arg))
	{
		return !printf("Error: open_file\n");
	}

	unsigned int size = 0;
	filepos = 0x1ce;
	if (! read((unsigned long long)(unsigned int)&size,2,GRUB_READ))
	{
		close();
		return !printf("Error:read1\n");
	}
	if (size > 20)
		size = 20;
	int i;
	for (i=0;i<size;i++)
	{
		read((unsigned long long)(unsigned int)(CHAR_BUF1 + (i << 4)),16,GRUB_READ);
		read((unsigned long long)(unsigned int)(CHAR_BUF2 + (i << 4)),16,GRUB_READ);
	}
	close();
	int_regs.eip = 0x7C00;
	int_regs.cs = 0;
	int_regs.esp = -1;
	int_regs.ss = -1;
	int_regs.ecx= size;
	int_regs.es = 0;
	return printf("%x",realmode_run((long)&int_regs));
}