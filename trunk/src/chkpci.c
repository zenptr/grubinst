/*
 *	GRUB	--	GRand Unified Bootloader
 *	Copyright (C) 1999,2000,2001,2002,2003,2004	 Free Software Foundation, Inc.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * compile:

gcc -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE chkpci.c

 * disassemble:			objdump -d a.out 
 * confirm no relocation:	readelf -r a.out
 * generate executable:		objcopy -O binary a.out chkpci
 *
 */
/*
 * to get this source code & binary: http://grub4dos-chenall.google.com
 * For more information.Please visit web-site at http://chenall.net
 更新信息(changelog):
	2010-09-04
		1.尝试添加新的PCI信息显示格式（类似CHKPCI).
		文件内容格式:
			第一行固定PCI$
			可选固定输出内容。
			...
			$PCI设备信息
			匹配后要显示的内容
			...
			$pci设备信息2
			匹配后要显示的内容
			...
		一个例子:
			===========CHKPCI.PCI=============
			PCI$
			$PCI\VEN_8086&DEV_7113
			Intel
			test
			$PCI\VEN_8086&DEV_7000&CC_020000&REV_00
			fat copy /IASTOR.SYS (fd0)/
			fat copy /iastor.inf (fd0)/
			fat copy /txtsetup.oem (fd0)/
			===========CHKPCI.PCI=============
	2010-08-28
		1.添加帮助信息 -h 参数.
		2.添加参数 -cc:CC,用于显示指定设备.
	2010-08-27
		修正,在实机使用时会造成卡机的问题.
	2010-08-26
		添加读取PCIDEVS.TXT按格式显示设备信息的功能.
	2010-08-25
		第一版,只能显示PCI信息.
 */

#include "grub4dos.h"

union bios32 {
	struct {
		unsigned long signature;	/* _32_ */
		unsigned long entry;		/* 32 bit physical address */
		unsigned char revision;		/* Revision level, 0 */
		unsigned char length;		/* Length in paragraphs should be 01 */
		unsigned char checksum;		/* All bytes must add up to zero */
		unsigned char reserved[5];	/* Must be zero */
	} fields;
	char chars[16];
};

struct pci_dev {
	unsigned short	venID;
	unsigned short	devID;
	unsigned char revID;
	unsigned char prog;
	unsigned short class;

	unsigned long subsys;
} __attribute__ ((packed));

struct vdata
{
	unsigned long addr;
	unsigned long dev;
} __attribute__ ((packed));
//#define DEBUG
#define VDATA	 ((struct vdata *)(0x600000+0x4000))
#define FILE_BUF ((char *)0x684000)
#define PCI ((struct pci_dev *)0x600000)

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
 
int chkpci_func (char *,int);

int main (void)
{
	void *p = &main;
	char *arg = p - (*(int *)(p - 8));
	int flags = (*(int *)(p - 12));
	return chkpci_func (arg , flags);
}

int inl(int port)
{
		int ret_val;
		__asm__ volatile ("inl %%dx,%%eax" : "=a" (ret_val) : "d"(port));
		return ret_val;
}

void outb(int port, char val) {
		__asm__ volatile ("out %%al,%%dx"::"a" (val), "d"(port));
}

void outl(int port, int val)
{
		__asm__ volatile ("outl %%eax,%%dx" : : "a"(val), "d"(port));
}

/*检测是否存在PCI BIOS*/
unsigned long pcibios_init(int flags)
{
	unsigned long tmp;
	outb(0xCFB,1);
	tmp = inl(0xCF8);
	outl(0xCF8,0x80000000);
	if (inl(0xcf8) == 0x80000000)
	{
		outl(0xCF8,tmp);
		if (debug>1)
		printf("pcibios_init: Using configuration type 1\n");
		return 1;
	}
	#if 0
	outl(0xcf8,tmp);
	outl(0xcfb,0);
	outl(0xcf8,0);
	outl(0xcfa,0);
	if (inl(0xcf8) == 0 && inl(0xcfb) == 0)
	{
		if (debug>1)
		printf("pcibios_init: Using configuration type 2\n");
		return 2;
	}
	printf("pcibios_init: Not supported chipset for direct PCI access !\n");
 #endif
	return 0;
}

int help(void)
{
return printf("\n CHKPCI For GRUB4DOS,Compiled time: %s %s\n\
\n CHKPCI is a utility that can be used to scan PCI buses and report information about the PCI device.\n\
\n CHKPCI [-h] [-cc:CC] [FILE]\n\
\n Options:\n\
\n -h      show this info.\n\
\n -cc:CC  scan Class Codes with CC only.\n\
\n FILE    PCI device database file.\n\
\n For more information.Please visit web-site at http://chenall.net/tag/grub4dos\n\
 to get this source and bin: http://grubutils.googlecode.com\n",__DATE__,__TIME__);
}

/*字符串转换成HEX数据如AB12转成0xAB12*/
unsigned long asctohex(char *asc)
{
	unsigned long t = 0L;
	unsigned char a;
	while(a=*asc++)
	{
		if ((a -= '0') > 9) /*字符减去'0',如果是'0'-'9'得到0-9数字*/
		{
			a = (a | 32) - 49;/*其它的字母转换成大写再减去49应该在0-5之间否则都是非法.*/

			if (a > 5)
				break;
			a += 10;
		}
		t <<= 4;
		t |= a;
	}
	return t;
}

/*查找以ch开头的行,碰到以ch1字符开头就结束.
	如果ch是0,则查询任意非以ch1字符开头的行.
	返回值该行在内存中的地址.否则返回0;
*/
unsigned char *find_line(char *P,char ch,char ch1)
{
	while(*P)
	{
		if (*P++ == 0xa)
		{
			if (*P == ch1)
				return NULL;
			if ( ch == 0 || *P == ch)
				return P;
		}
	}
	return NULL;
}

/*读取PCIDEVS.TXT文件到内容中,并作简单索引*/
int read_cfg(long len)
{
	unsigned char *P=FILE_BUF;
	char *P1;
	unsigned long t;
	memset((char *)VDATA,0,0x70000);
	while( P = find_line(P, 'V', 0))
	{
		P += 2;
		t = asctohex(P);
		P = skip_to (0, P);
		VDATA[t].addr = (unsigned int)P;
		while (*P != 0x0D && *++P);
		
		if (*P == 0x0D)
		{
			*P++ = 0;
			VDATA[t].dev = (unsigned long)find_line(P,'D',0 )-1;
		}
	}
	return 0;
}

/*按PCIDEVS.TXT格式显示对应的硬件信息,具体格式请查看相关文档*/
int show_dev(struct pci_dev *pci)
{
	char *P,*P1;
	unsigned long t;
	P = (char *)VDATA[pci->venID].addr;
	printf("VEN_%04X:\t%s\nDEV_%04X:\t",pci->venID,P,pci->devID);
	P = (char *)VDATA[pci->venID].dev;
	while (P = find_line (P, 'D' , 'V'))
	{
		P += 2;
		t = asctohex(P);
		if (t == pci->devID)
		{
			P1 = P;
			while (P1 = find_line (P1,'R','D'))
			{
				P1 += 2;
				t = asctohex(P1);
				if (t == pci->revID)
				{
					P = P1;
					printf("[R=%02X]",pci->revID);
					break;
				}
			}
			P = skip_to (0, P);				
			while (*P && *P != 0xD)
				putchar(*P++);
			putchar('\n');
			if (pci->subsys)
			{
			}
			return 1;
		}
	}
	printf("Unknown\n");
	return 0;
}
#define VEN 0x5F4E4556
#define DEV 0X5F564544
#define SUB 0x53425553
int chkpci(struct pci_dev *end)
{
	struct pci_dev *pci;
	char *p1,*P = find_line (FILE_BUF, 0 , 0);//跳过第一行。!
	unsigned long t;
	struct pci_dev dev;
	if (*P != '$')/*输出文件头部份*/
	{
		while (*P && P[1] != '$') putchar(*P++);
		putchar('\n');
	}
	while (P = find_line (P, '$' , 0))
	{
		if (*(unsigned long *)(P+1) & 0XFFDFDFDF != 0x5C494350) //PCI\
			continue;
		if (*(unsigned long *)(P+5) & 0xFFDFDFDF != VEN)//check VEN_
			continue;
		if (*(unsigned long *)(P+14) & 0xFFDFDFDF != DEV)// check DEV_
			continue;
		memset (&dev,0,sizeof(struct pci_dev));
		P = P + 9;
		t = asctohex(P);
		dev.venID = t;
		P += 9;
		t = asctohex(P);
		dev.devID = t;
		P += 5;

		if (*(unsigned long *)P == SUB)//check SUBSYS
		{
			P += 7;
			t = asctohex(P);
			P += 9;
			dev.subsys = t;
		}
		
		if (*(unsigned short *)P == 0x4343) //CC_
		{
			P += 3;
			t = asctohex(P);
			t = t < 0xffff?t:t>>8;
			dev.class = (unsigned short)t;
			P += 4;
			while (*P != 0xD && *P++ != '&');
		}
		
		if (*(unsigned short *)P == 0x5F564552) //REV_
		{
			P += 4;
			t = asctohex(P);
			dev.revID = (char)t;
		}

		for (pci=PCI;pci < end; pci++)
		{
			if (pci->venID == dev.venID && pci->devID == dev.devID)
			{
				if (dev.subsys && dev.subsys != pci->subsys) continue;
				if (dev.class && dev.class != pci->class) continue;
				if (dev.revID && dev.revID != pci->revID) continue;
				pci->venID = 0;
				for (P = find_line(P, 0, 0);*P == '$';P = find_line(P, 0, 0));
				//0x240a \n$ 也就是以$开头的行。
				while (*P && *(unsigned short *)P != 0x240a) putchar(*P++);//输出内容直到文件尾或下一个以$开始的行。
				putchar('\n');
				break;
			}
		}
	}
	return 1;
}

int chkpci_func(char *arg,int flags)
{
	unsigned long regVal,ret;
#ifdef DEBUG
	unsigned long bus,dev,func;
#endif
	unsigned long long cd = 0;
	struct pci_dev *devs = PCI;
	*(unsigned long *)FILE_BUF = 0;
	if (! pcibios_init(0)) return 0;
	if (*arg)
	{
		if (memcmp(arg,"-cc:",4) == 0 )
		{
			arg += 4;
			if (! safe_parse_maxint(&arg,&cd))
				return 0;
			cd &= 0xff;
			arg = skip_to(0,arg);
		}
		else if (memcmp(arg, "-h",2) == 0)
		{
			return help();
		}
		
		if (*arg)
		{
			if (! open(arg))
				return 0;
			if (! read ((unsigned long long)(int)FILE_BUF,0x100000,GRUB_READ))
				return 0;
			FILE_BUF[filemax] = 0;
			if (*(unsigned long *)FILE_BUF != 0X24494350) 
			{
				read_cfg(filemax);
			}
		}
	}

/* 循环查找所有设备.
	for (bus=0;bus<5;bus++)
	  for (dev=0;dev<32;dev++)
	    for (func=0;func<8;func++)
		    regVal = 0x80000000 | bus << 16 | dev << 11 | func << 8;
*/
	for (regVal = 0x80000000;regVal < 0x8005FF00;regVal += 0x100)
	{
		outl(0xCF8,regVal);
		ret = inl(0xCFC);

#ifdef DEBUG
		if (debug > 1)
		{
			bus = regVal >> 16 & 0xFF;
			dev = regVal >> 11 & 0x1f;
			func = regVal >> 8 & 0x7;
			printf("Check:%08X B:%02x E:%02X F:%02X\n",regVal,bus,dev,func);
		}
#endif
		if (ret == -1L) //0xFFFFFF无效设备
		{
			if ((regVal & 0x700) == 0) regVal += 0x700;//功能号为0,跳过该设备.
			continue;
		}

		*(unsigned long *)&devs->venID = ret;/*ret返回值低16位是PCI_VENDOR_ID,高16位是PCI_DEVICE_ID*/
		
		/*获取PCI_CLASS_REVISION,高24位是CLASS信息(CC_XXXXXX),低8位是版本信息(REG_XX)*/
		outl(0xCF8,regVal | 8);
		*(unsigned long *)&devs->revID = inl(0xCFC); /* High 24 bits are class, low 8 revision */

		if ((char)cd == 0 || (char)(devs->class >> 8) == (char)cd) //如果指定了CD参数,CD值不符合时不显示.
		{
			/*获取PCI_HEADER_TYPE类形,目前只用于后面检测是否单功能设备*/
			outl(0xCF8,regVal | 0xC);
			ret = inl(0xCFC);
			ret >>= 16;
			{//读取SUBSYS信息,低16位是PCI_SUBSYSTEM_ID,高16位是PCI_SUBSYSTEM_VENDOR_ID(也叫OEM信息)
				outl(0xcf8,regVal | 0x2c);
				devs->subsys = inl(0xcfc);
			}
			if (*(unsigned long *)FILE_BUF != 0X24494350)
			{
				printf("PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&CC_%04X%02X&REV_%02X\n",
						devs->venID,devs->devID,devs->subsys,devs->class,devs->prog,devs->revID);
				if (*arg) show_dev(devs);
			}
			devs++;
		}
		devs->venID = 0;
		/*如果是单功能设备,检测下一设备*/
		if ( (regVal & 0x700) == 0 && (ret & 0x80) == 0) regVal += 0x700;
	}
	if (*(unsigned long *)FILE_BUF == 0X24494350) return chkpci(devs);
	return 1;
}
