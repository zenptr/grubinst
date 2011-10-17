/*
 *  memcheck  --  Check implementation of int15/E820.
 *  Copyright (C) 2011  tinybit(tinybit@tom.com)
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
 * compile:

gcc -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE demo1.c -o demo1.o

 * disassemble:			objdump -d demo1.o
 * confirm no relocation:	readelf -r demo1.o
 * generate executable:		objcopy -O binary demo1.o demo1
 *
 */

#include "grub4dos.h"
#define memcpy memmove
#define uint32_t unsigned long
#define uint16_t unsigned short
#define uint8_t unsigned char
//#include "vbe.h"
  struct _VBEModeInfo_t
  {
    uint16_t ModeAttributes;
    uint8_t  WinAAttributes;
    uint8_t  WinBAttributes;
    uint16_t WinGranularity;
    uint16_t WinSize;
    uint16_t WinASegment;
    uint16_t WinBSegment;
    uint32_t WinFuncPtr;
    uint16_t BytesPerScanline;

    // VBE 1.2+
    uint16_t XResolution;
    uint16_t YResolution;
    uint8_t  XCharSize;
    uint8_t  YCharSize;
    uint8_t  NumberOfPlanes;
    uint8_t  BitsPerPixel;
    uint8_t  NumberOfBanks;
    uint8_t  MemoryModel;
    uint8_t  BankSize;
    uint8_t  NumberOfImagePages;
    uint8_t  Reserved1;

    // VBE 1.2+ Direct Color fields
    uint8_t  RedMaskSize;
    uint8_t  RedFieldPosition;
    uint8_t  GreenMaskSize;
    uint8_t  GreenFieldPosition;
    uint8_t  BlueMaskSize;
    uint8_t  BlueFieldPosition;
    uint8_t  RsvdMaskSize;
    uint8_t  RsvdFieldPosition;
    uint8_t  DirectColorModeInfo;

    // VBE 2.0+
    uint32_t PhysBasePtr;
    uint32_t Reserved2; // OffScreenMemOffset;
    uint16_t Reserved3; // OffScreenMemSize;

    // VBE 3.0+
    uint16_t LinBytesPerScanline;
    uint8_t  BnkNumberOfImagePages;
    uint8_t  LinNumberOfImagePages;
    uint8_t  LinRedMaskSize;
    uint8_t  LinRedFieldPosition;
    uint8_t  LinGreenMaskSize;
    uint8_t  LinGreenFieldPosition;
    uint8_t  LinBlueMaskSize;
    uint8_t  LinBlueFieldPosition;
    uint8_t  LinRsvdMaskSize;
    uint8_t  LinRsvdFieldPosition;
    uint32_t MaxPixelClock;

    uint8_t  Reserved4[189];
  } __attribute__ ((packed));
  typedef struct _VBEModeInfo_t VBEModeInfo_t;
  struct _VBEDriverInfo_t
  {
    uint8_t  VBESignature[4];
    uint16_t VBEVersion;
    uint32_t OEMStringPtr;
    uint32_t Capabilities;
    uint32_t VideoModePtr;
    uint16_t TotalMemory;

    // VBE 2.0 extensions
    uint16_t OemSoftwareRev;
    uint32_t OemVendorNamePtr;
    uint32_t OemProductNamePtr;
    uint32_t OemProductRevPtr;
    uint8_t  Reserved[222];
    uint8_t  OemDATA[256];
  } __attribute__ ((packed));
  typedef struct _VBEDriverInfo_t VBEDriverInfo_t;
  #define VBE_CAPS_DAC8      (((uint32_t)1) << 0)
  #define VBE_CAPS_NOTVGA    (((uint32_t)1) << 1)
  #define VBE_CAPS_BLANKFN9  (((uint32_t)1) << 2)
  #define VBE_MA_MODEHW (((uint32_t)1) << 0)
  #define VBE_MA_HASLFB (((uint32_t)1) << 7)
int GetModeInfo (uint16_t mode);
int SetMode (uint16_t mode);
void CloseMode ();
uint32_t EncodePixel (uint8_t r, uint8_t g, uint8_t b);

void gfx_Clear8  (uint32_t color);
void gfx_Clear16 (uint32_t color);
void gfx_Clear24 (uint32_t color);
void gfx_Clear32 (uint32_t color);
void SetPixel (long x, long y, uint32_t color);
void gfx_FillRect  (long x1, long y1, long x2, long y2, uint32_t color);
void gfx_FillRect8  (long x1, long y1, long x2, long y2, uint32_t color);
void gfx_FillRect16 (long x1, long y1, long x2, long y2, uint32_t color);
void gfx_FillRect24 (long x1, long y1, long x2, long y2, uint32_t color);
void gfx_FillRect32 (long x1, long y1, long x2, long y2, uint32_t color);

/****************************************/
/* All global variables must be static! */
/****************************************/

static VBEDriverInfo_t drvInfo;
static VBEModeInfo_t modeInfo;
static uint32_t l_pixel;

static int get_font(int flags);
static int _INT10_(uint32_t eax,uint32_t ebx,uint32_t ecx,uint32_t edx,uint32_t es,uint32_t edi);
static int vbe_init(int mode);
static void vbe_end(void);
static void vbe_cls(void);
static void vbe_gotoxy(int x, int y);
static int vbe_getxy(void);
static void vbe_scroll(void);
static void vbe_putchar (unsigned int c);
static int vbe_setcursor (int on);
static int vbe_current_color = -1;
static int vbe_standard_color = -1;
static int vbe_normal_color = 0x55ffff;
static int vbe_highlight_color = 0xff5555;
static int vbe_helptext_color = 0xff55ff;
static int vbe_heading_color = 0xffff55;
static int vbe_cursor_color = 0xff00ff;
static color_state vbe_color_state = COLOR_STATE_STANDARD;
static void vbe_setcolorstate (color_state state);
static void vbe_setcolor (int normal_color, int highlight_color, int helptext_color, int heading_color);
static struct 
{
	unsigned short mode;
	unsigned short cursor;
	char *img;
} vbe_flags = {0,1,NULL};

static int vbe_vfont(char *file);
static unsigned vfont_size=0;
static uint8_t pre_ch;
static char vesa_name[]="VESA_TEST";
static struct term_entry vesa;
static char mask[8]={0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
static char vfont[2048];
static int open_bmp(char *file);

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
unsigned long long GRUB = 0x534f443442555247LL;/* this is needed, see the following comment. */
/* gcc treat the following as data only if a global initialization like the
 * above line occurs.
 */

//asm(".long 0x534F4434");

/* a valid executable file for grub4dos must end with these 8 bytes */
asm(ASM_BUILD_DATE);
asm(".long 0x03051805");
asm(".long 0xBCBAA7BA");

/* thank goodness gcc will place the above 8 bytes at the end of the b.out
 * file. Do not insert any other asm lines here.
 */

void demo1_Run();

/*******************************************************************/
/* main() must be the first function. Don't insert functions here! */
/*******************************************************************/

int main(char *arg,int flags)//( int argc, char* argv[] )
{
  uint16_t*	modeList;
  uint16_t	mode = 0;

	if ((int)&main != 0x50000)//init move program to 0x50000
	{
		if (strcmp(current_term->name,vesa_name) != 0)
		{
			int prog_len=(*(int *)(&main - 20));
			memmove((void*)0x50000,&main,prog_len);
		}
		return ((int (*)(char *,int))0x50000)(arg,flags);
	}

  // Display driver information
  if (! GetDriverInfo ())
  {
    printf( "ERROR: VBE driver not supported.\n" );
    return 0;
  }

	struct term_entry *pre_term = current_term;;
	if (strcmp(current_term->name,vesa_name) != 0)
	{
		vesa.name = vesa_name;
		vesa.flags = 0;
		vesa.PUTCHAR = vbe_putchar;
		vesa.CHECKKEY = current_term->CHECKKEY;
		vesa.GETKEY = current_term->GETKEY;
		vesa.GETXY = vbe_getxy;
		vesa.GOTOXY = vbe_gotoxy;
		vesa.CLS = vbe_cls;
		vesa.SETCOLORSTATE = vbe_setcolorstate;
		vesa.SETCOLOR = vbe_setcolor;
		vesa.SETCURSOR = vbe_setcursor;
		vesa.STARTUP = vbe_init;
		vesa.SHUTDOWN = vbe_end;
		font8x16 = (char*)get_font(0x600);
	}

	if (memcmp(arg,"demo",4) != 0)
	{
		while (*arg)
		{
			if (memcmp(arg,"vfont=",6) == 0)
			{
				arg = skip_to(1,arg);
				vbe_vfont(arg);
			}
			if (memcmp(arg,"mode=",5) == 0)
			{
				arg+=5;
				unsigned long long ll;
				if (!(safe_parse_maxint(&arg,&ll)))
					return 0;
				mode = (uint16_t)ll;
			}
			if (memcmp(arg,"bmp",3)== 0)
			{
				open_bmp(skip_to(1,arg));
			}
			arg = skip_to(0,arg);
		}
		if (!mode && !vbe_flags.mode)
		{
			if (GetModeInfo (0x115))
				mode = 0x115;
			else if (GetModeInfo (0x114))
				mode = 0x114;
		}
		if (!vbe_init (mode))
		{
			printf( "VBE mode %4x not supported\n", mode);
			getkey();
			return 0;
		}
		current_term = &vesa;
		vesa.chars_per_line = modeInfo.XResolution>>3;
		vesa.max_lines = modeInfo.YResolution>>4;
		if (vbe_flags.img)
			memmove((char*)modeInfo.PhysBasePtr,vbe_flags.img,modeInfo.YResolution*modeInfo.BytesPerScanline);
		printf("%dx%dx%d,%dx%d\n",modeInfo.XResolution,modeInfo.YResolution,modeInfo.BitsPerPixel,vesa.chars_per_line,vesa.max_lines);
		getkey();
		vbe_cls();
//		memmove((unsigned char*)modeInfo.PhysBasePtr,vbe_flags.img,204800);
		return 1;
	}

  printf( "Signature: '%c%c%c%c'\n",
    drvInfo.VBESignature[0], drvInfo.VBESignature[1],
    drvInfo.VBESignature[2], drvInfo.VBESignature[3] );

  printf( "Version: %X.%X\n",
    (drvInfo.VBEVersion >> 8), (drvInfo.VBEVersion & 0xFF) );

  printf( "OEM String: '%s'\n", (char*)drvInfo.OEMStringPtr );

  printf( "Capabilities: %08X\n", drvInfo.Capabilities );

  if( drvInfo.Capabilities & VBE_CAPS_DAC8 )
    printf( "  * DAC registers switchable to 8-Bits\n" );
  else
    printf( "  * DAC registers fixed at 6-Bits\n" );

  if( drvInfo.Capabilities & VBE_CAPS_NOTVGA )
    printf( "  * Video card is not VGA compatible\n" );

  if( drvInfo.Capabilities & VBE_CAPS_BLANKFN9 )
    printf( "  * Use Blank Bit in Function 09h\n" );

  printf( "Video Memory: %u\n", drvInfo.TotalMemory * 64 );

  if( drvInfo.VBEVersion >= 0x0200 )
  {
    printf( "OEM Soft Rev: %04X\n", drvInfo.OemSoftwareRev );

    if( drvInfo.OemVendorNamePtr )
      printf( "  OEM Vendor: '%s'\n", (char*)drvInfo.OemVendorNamePtr );

    if( drvInfo.OemProductNamePtr )
      printf( " OEM Product: '%s'\n", (char*)drvInfo.OemProductNamePtr );

    if( drvInfo.OemProductRevPtr )
      printf( "OEM Revision: '%s'\n", (char*)drvInfo.OemProductRevPtr );
  }

  // Display available 8/15/16/24/32-BPP modes
  printf( "\nAvailable Modes:" );

#if 0
  modeList = (uint16_t*)drvInfo.VideoModePtr;

  while ((mode = *modeList++) != 0xFFFF)
  {
    if (! GetModeInfo (mode))
	continue;
    switch (modeInfo.BitsPerPixel)
    {
    case 8:
    case 15:
    case 16:
    case 24:
    case 32:
          printf (" %X", mode);
    }
  }
#else
  builtin_cmd("vbeprobe","",0xff);
#endif
  printf ("\nPress any key to test each mode...\r");
  getkey();
  modeList = (uint16_t*)drvInfo.VideoModePtr;
	current_term = &vesa;

  while ((mode = *modeList++) != 0xFFFF)
  {
    if (! GetModeInfo (mode))
	continue;
    switch (modeInfo.BitsPerPixel)
    {
    case 8:
    case 15:
    case 16:
    case 24:
    case 32:
	if (! SetMode (mode))
	{
	    printf( "VBE mode %4x not supported\n", mode);
	    getkey();
	    break;
	}
	fontx=0,fonty=0;
	vesa.chars_per_line = modeInfo.XResolution>>3;
	vesa.max_lines = modeInfo.YResolution>>4;
	demo1_Run();
	printf ("mode=%X. %dx%dx%d, %sBase=%X\n"
		"ModeAttributes=%X\n"
		"MemoryModel=%X\n"
		"MaskSize=%d:%d:%d:%d\n"
		"FieldPos=%d:%d:%d:%d\n"
		"LinMaskSize=%d:%d:%d:%d\n"
		"LinFieldPos=%d:%d:%d:%d\n"
		"BytesPerScanline=%d\n"
		"Press any key to continue...\n",
		mode,
		modeInfo.XResolution,
		modeInfo.YResolution,
		modeInfo.BitsPerPixel,
		(modeInfo.ModeAttributes & 0x10)? "":"Text",
		modeInfo.PhysBasePtr,
		modeInfo.ModeAttributes,
		modeInfo.MemoryModel,
		modeInfo.RedMaskSize,
		modeInfo.GreenMaskSize,
		modeInfo.BlueMaskSize,
		modeInfo.RsvdMaskSize,
		modeInfo.RedFieldPosition,
		modeInfo.GreenFieldPosition,
		modeInfo.BlueFieldPosition,
		modeInfo.RsvdFieldPosition,
		modeInfo.LinRedMaskSize,
		modeInfo.LinGreenMaskSize,
		modeInfo.LinBlueMaskSize,
		modeInfo.LinRsvdMaskSize,
		modeInfo.LinRedFieldPosition,
		modeInfo.LinGreenFieldPosition,
		modeInfo.LinBlueFieldPosition,
		modeInfo.LinRsvdFieldPosition,
		modeInfo.BytesPerScanline
	);
	getkey();
	CloseMode ();
	break;
    default:
	printf ("mode=%X, not 8/15/16/24/32-BPP\nPress any key to continue...\n", mode);
	getkey();
	break;
    }
  }
	if (pre_term->STARTUP)
		pre_term->STARTUP();
	current_term = pre_term;
	return 0;
}

static int vbe_vfont(char *file)
{
	unsigned int size;
	if (!open(file))
	{
		return !printf("Error: open_file\n");
	}
	filepos = 0x59;
	if (! read((unsigned long long)(unsigned int)&size,4,GRUB_READ) || size != 0x52515350)
	{
		close();
		return !printf("Err: fil");
	}
	filepos = 0x1ce;
	if (! read((unsigned long long)(unsigned int)&vfont_size,2,GRUB_READ))
	{
		close();
		return !printf("Error:read1\n");
	}
	vfont_size &= 0xFF;
	if (vfont_size > 64)
		vfont_size = 64;
	read((unsigned long long)(unsigned int)vfont,vfont_size*32,GRUB_READ);
	close();
	return 1;
}

static int _INT10_(uint32_t eax,uint32_t ebx,uint32_t ecx,uint32_t edx,uint32_t es,uint32_t edi)
{
	struct realmode_regs int_regs = {edi,0,0,-1,ebx,edx,ecx,eax,-1,-1,es,-1,-1,0xFFFF10CD,-1,-1};
	realmode_run((long)&int_regs);
	return int_regs.eax;
}

static int vbe_setcursor (int on)
{
	vbe_flags.cursor = on;
	return 0;
}

static int vbe_init(int mode)
{
	if (!mode)
	{
		mode = vbe_flags.mode;
	}
	if (!GetModeInfo(mode) || !SetMode (mode))
		return 0;
	vbe_flags.mode = mode;
	fontx=0,fonty=0;
	vesa.chars_per_line = modeInfo.XResolution>>3;
	vesa.max_lines = modeInfo.YResolution>>4;
	return 1;
}

static void vbe_end(void)
{
	if (vbe_flags.img)
		free(vbe_flags.img);
	CloseMode ();
}

static void vbe_cls(void)
{
	uint8_t* l_lfb = (uint8_t*)modeInfo.PhysBasePtr;
	uint32_t size = modeInfo.YResolution * modeInfo.BytesPerScanline;
	while(size--)
	{
		*l_lfb++=0;
	}
	fontx = 0;
	fonty = 0;
}

static void vbe_gotoxy(int x, int y)
{
	int i;
	if (x > vesa.chars_per_line)
		x = vesa.chars_per_line;
	if (y > vesa.max_lines)
		y = vesa.max_lines;
	if (vbe_flags.cursor)
	{
		gfx_FillRect (fontx<<3,(fonty<<4)+15,(fontx+1)<<3,(fonty+1)<<4, 0);
		gfx_FillRect (x<<3,(y<<4)+15,(x+1)<<3,(fonty+1)<<4, vbe_cursor_color);
	}
	fontx = x;
	fonty = y;
	return;
}

static char inb(unsigned short port)
{
	char ret_val;
	__asm__ volatile ("inb %%dx,%%al" : "=a" (ret_val) : "d"(port));
	return ret_val;
}

static void vbe_scroll(void)
{
	uint8_t* l_lfb = (uint8_t*)modeInfo.PhysBasePtr;

	if(l_lfb)
	{
		uint32_t i,j;
		j = modeInfo.BytesPerScanline<<4;
		for (i=0;i<fonty;++i)
		{
			//while (!(inb(0x3da) & 8))
			//	;
			memmove(l_lfb,l_lfb+j,j);
			l_lfb+=j;
		}
	}
	return;
}

static int vbe_getxy(void)
{
	return (fontx << 8) | fonty;
}

static int get_font(int flags)
{
	struct realmode_regs int_regs = {0,0,0,-1,flags,0,0,0x1130,-1,-1,-1,-1,-1,0xFFFF10CD,-1,-1};
	realmode_run((long)&int_regs);
	return (int_regs.es<<4)+int_regs.ebp;
}

static void vbe_putchar (unsigned int c)
{
	if (fontx >= vesa.chars_per_line)
		fontx = 0,++fonty;
	if (fonty >= vesa.max_lines)
	{
		vbe_scroll();
		--fonty;
	}

	if ((char)c == '\n')
	{
		return vbe_gotoxy(fontx, fonty + 1);
	}
	else if ((char)c == '\r') {
		return vbe_gotoxy(0, fonty);
	}

	char *cha = font8x16 + ((unsigned char)c<<4);
	int i,j;

	if (vfont_size)
	{
		if ((unsigned char)c >= '\x80' && (pre_ch - (unsigned char)c) == '\x40' && (unsigned char)c < 0x80+vfont_size)
		{
			uint8_t ch = pre_ch;
			if (!fontx && fonty)
			{
				vbe_gotoxy(vesa.chars_per_line - 1 ,fonty-1);
				vbe_putchar(' ');
			}
			else if (fontx)
				--fontx;

			cha = vfont + (ch - 0xC0)*32;

			for (j=0;j<16;++j)
			{
				for (i=0;i<8;++i)
				{
					if (cha[j] & mask[i])
						SetPixel (fontx*8+i,fonty*16+j,vbe_current_color);
					else
						SetPixel (fontx*8+i,fonty*16+j,0);
				}
			}
			++fontx;
			cha += 16;
		}
		pre_ch = (uint8_t)c;
	}
	for (j=0;j<16;++j)
	{
		for (i=0;i<8;++i)
		{
			if (cha[j] & mask[i])
				SetPixel (fontx*8+i,fonty*16+j,vbe_current_color);
			else
				SetPixel (fontx*8+i,fonty*16+j,0);
		}
	}

	if (fontx + 1 >= vesa.chars_per_line)
	{
		++fonty;
		if (fonty >= vesa.max_lines)
		{
			vbe_scroll();
			--fonty;
		}
		vbe_gotoxy(0, fonty);
	}
	else
		vbe_gotoxy(fontx + 1, fonty);
	return;
}

//  /*
//   *  void gfx_SetPixel( *p_ctx, p_x, p_y, p_pixel )
//   *
//   *  Purpose:
//   *    Draws an 8/16/24-BPP pixel at the specified point
//   *    if within the boundaries of p_ctx->clip.
//   *
//   *  Returns:
//   *    Nothing
//   */
//void gfx_SetPixel (vbectx_t* p_ctx, long p_x, long p_y, uint32_t p_pixel)
//{
//	uint8_t* l_lfb;

//	if (p_ctx)
//	{
//		if( p_ctx->lfb )
//		{
//			if ( (p_x < p_ctx->clip.minx) || (p_y < p_ctx->clip.miny)
//				|| (p_x > p_ctx->clip.maxx) || (p_y > p_ctx->clip.maxy) )
//			{
//				return;
//			}
//			uint8_t PixelPerChar = p_ctx->bpp + 7 >> 3;
//			l_lfb = (uint8_t*)(p_ctx->lfb + (p_y * p_ctx->linesize) + p_x*PixelPerChar);
//			while(PixelPerChar--)
//			{
//				*l_lfb++ = (uint8_t)(p_pixel);
//				p_pixel >>= 8;
//			}
//		}
//	}
//}


void
gfx_FillRect (long p_x1, long p_y1, long p_x2, long p_y2, uint32_t p_pixel)
{
	switch(modeInfo.BitsPerPixel)
	{
		case 8:
			return gfx_FillRect8 (p_x1, p_y1, p_x2, p_y2, p_pixel);
		case 15:
		case 16:
			return gfx_FillRect16 (p_x1, p_y1, p_x2, p_y2, p_pixel);
		case 24:
			return gfx_FillRect24 (p_x1, p_y1, p_x2, p_y2, p_pixel);
		case 32:
			return gfx_FillRect32 (p_x1, p_y1, p_x2, p_y2, p_pixel);
	};
}


static void vbe_setcolorstate (color_state state)
{
	switch (state)
	{
		case COLOR_STATE_STANDARD:
			vbe_current_color = vbe_standard_color;
			break;
		case COLOR_STATE_NORMAL:
			vbe_current_color = vbe_normal_color;
			break;
		case COLOR_STATE_HIGHLIGHT:
			vbe_current_color = vbe_highlight_color;
			break;
		case COLOR_STATE_HELPTEXT:
			vbe_current_color = vbe_helptext_color;
			break;
		case COLOR_STATE_HEADING:
			vbe_current_color = vbe_heading_color;
			break;
		default:
			vbe_current_color = vbe_standard_color;
			break;
	}
	vbe_current_color = EncodePixel(vbe_current_color>>16,vbe_current_color>>8,vbe_current_color);
	vbe_color_state = state;
}


static void vbe_setcolor (int normal_color, int highlight_color, int helptext_color, int heading_color)
{
	if (normal_color == -1)
	{
		_INT10_(0x1010,0,highlight_color,highlight_color>>8,-1,0);
		vbe_cursor_color = EncodePixel(helptext_color>>16,helptext_color>>8,helptext_color);
		return;
	}
	vbe_normal_color = normal_color;
	vbe_highlight_color = highlight_color;
	vbe_helptext_color = helptext_color;
	vbe_heading_color = heading_color;
	vbe_setcolorstate (vbe_color_state);
}

void demo1_Run()
{
    l_pixel = EncodePixel (64, 128, 255);

    if (modeInfo.BitsPerPixel == 8)
    {
        gfx_Clear8 (l_pixel);

        l_pixel = EncodePixel (16, 32, 64);
        gfx_FillRect8 (modeInfo.XResolution / 2 - 100, modeInfo.YResolution / 2 - 100, modeInfo.XResolution / 2 + 100, modeInfo.YResolution / 2 + 100, l_pixel);
    } else if (modeInfo.BitsPerPixel == 15 || modeInfo.BitsPerPixel == 16)
    {
        gfx_Clear16 (l_pixel);

        l_pixel = EncodePixel (16, 32, 64);
        gfx_FillRect16 (modeInfo.XResolution / 2 - 100, modeInfo.YResolution / 2 - 100, modeInfo.XResolution / 2 + 100, modeInfo.YResolution / 2 + 100, l_pixel);
    } else if (modeInfo.BitsPerPixel == 24)
    {
        gfx_Clear24 (l_pixel);

        l_pixel = EncodePixel (16, 32, 64);
        gfx_FillRect24 (modeInfo.XResolution / 2 - 100, modeInfo.YResolution / 2 - 100, modeInfo.XResolution / 2 + 100, modeInfo.YResolution / 2 + 100, l_pixel);
    } else {
        gfx_Clear32 (l_pixel);

        l_pixel = EncodePixel (16, 32, 64);
        gfx_FillRect32 (modeInfo.XResolution / 2 - 100, modeInfo.YResolution / 2 - 100, modeInfo.XResolution / 2 + 100, modeInfo.YResolution / 2 + 100, l_pixel);
    }
    l_pixel = EncodePixel (128, 64, 32);
    SetPixel (modeInfo.XResolution / 2 - 2, modeInfo.YResolution / 2 + 0, l_pixel);
    SetPixel (modeInfo.XResolution / 2 - 1, modeInfo.YResolution / 2 + 0, l_pixel);
    SetPixel (modeInfo.XResolution / 2 + 0, modeInfo.YResolution / 2 + 0, l_pixel);
    SetPixel (modeInfo.XResolution / 2 + 1, modeInfo.YResolution / 2 + 0, l_pixel);
    SetPixel (modeInfo.XResolution / 2 + 2, modeInfo.YResolution / 2 + 0, l_pixel);
    SetPixel (modeInfo.XResolution / 2 + 0, modeInfo.YResolution / 2 - 2, l_pixel);
    SetPixel (modeInfo.XResolution / 2 + 0, modeInfo.YResolution / 2 - 1, l_pixel);
    SetPixel (modeInfo.XResolution / 2 + 0, modeInfo.YResolution / 2 + 0, l_pixel);
    SetPixel (modeInfo.XResolution / 2 + 0, modeInfo.YResolution / 2 + 1, l_pixel);
    SetPixel (modeInfo.XResolution / 2 + 0, modeInfo.YResolution / 2 + 2, l_pixel);
}

static uint32_t sys_RM16ToFlat32( uint32_t p_RMSegOfs )
{
	uint32_t hi, lo;
	hi = (p_RMSegOfs >> 16);
	lo = (uint16_t)p_RMSegOfs;
	return (hi << 4) + lo;
}

int GetDriverInfo ()
{
    char             l_vbe2Sig[4] = "VBE2";
    VBEDriverInfo_t* di;
    di = (VBEDriverInfo_t*)0x20000;
    memset (di, 0, 1024);
    memcpy (di, l_vbe2Sig, 4);
#if 0
    struct realmode_regs Regs;




      Regs.eax = 0x4F00;
      Regs.es  = 0x2000;
      Regs.edi = 0;
        Regs.ebx = 0;
        Regs.ecx = 0;
        Regs.edx = 0;
	Regs.esi = 0;
	Regs.ebp = 0;
	Regs.esp = -1;
	Regs.ss = -1;
	Regs.ds = -1;
	Regs.fs = -1;
	Regs.gs = -1;
	Regs.eflags = -1;
	Regs.cs = -1;
	Regs.eip = 0xFFFF10CD;

        realmode_run((long)&Regs);

      if( (((uint16_t)Regs.eax) == 0x004F)
#else
	   if ((_INT10_(0x4F00,0,0,0,0x2000,0) == 0x4f)
#endif
       &&
        (memcmp/*strncmp*/(di->VBESignature, "VESA", 4) == 0) &&
        (di->VBEVersion >= 0x200) )
      {
        di->OEMStringPtr = sys_RM16ToFlat32(di->OEMStringPtr);
        di->VideoModePtr = sys_RM16ToFlat32(di->VideoModePtr);

        di->OemVendorNamePtr =  sys_RM16ToFlat32(di->OemVendorNamePtr);
        di->OemProductNamePtr = sys_RM16ToFlat32(di->OemProductNamePtr);
        di->OemProductRevPtr =  sys_RM16ToFlat32(di->OemProductRevPtr);

        memcpy (&drvInfo, di, sizeof(VBEDriverInfo_t));

        return 1;
      }

    return 0;
}

int GetModeInfo (uint16_t mode)
{

    VBEModeInfo_t* mi;

    if (!mode || mode == 0xFFFF)
      return 0;

    mi = (VBEModeInfo_t*)(0x20000 + 1024);
    memset (mi, 0, 1024);
#if 0
   struct realmode_regs Regs;
    Regs.eax = 0x4F01;
    Regs.ecx = mode;
    Regs.es  = 0x2000;
    Regs.edi = 1024;
        Regs.ebx = 0;
        Regs.edx = 0;
	Regs.esi = 0;
	Regs.ebp = 0;
	Regs.esp = -1;
	Regs.ss = -1;
	Regs.ds = -1;
	Regs.fs = -1;
	Regs.gs = -1;
	Regs.eflags = -1;
	Regs.cs = -1;
	Regs.eip = 0xFFFF10CD;

        realmode_run((long)&Regs);

    if (((uint16_t)Regs.eax) != 0x004F)
#else
	if(_INT10_(0x4F01,0,mode,0,0x2000,1024) != 0x004F)
#endif
      return 0;

    // Mode must be supported
    if ((mi->ModeAttributes & VBE_MA_MODEHW) == 0)
      return 0;

    // LFB must be present
    if ((mi->ModeAttributes & VBE_MA_HASLFB) == 0)
      return 0;

    if (mi->PhysBasePtr == 0)
      return 0;

    if (mi->MemoryModel == 4 && mi->BitsPerPixel == 8)
    {
	/* simulate Direct Color of 3:3:2 */
	mi->RedMaskSize = 3;
	mi->GreenMaskSize = 3;
	mi->BlueMaskSize = 2;
	mi->RsvdMaskSize = 0;

	mi->RedFieldPosition = 5;
	mi->GreenFieldPosition = 2;
	mi->BlueFieldPosition = 0;
	mi->RsvdFieldPosition = 0;
    }
    memcpy (&modeInfo, mi, sizeof(VBEModeInfo_t));
    return 1;
}

  /*
   *  void SetPal (*pal);
   *
   *  Purpose:
   *    Sets internal palette from an array of R,G,B,A values.
   *
   *  Returns:
   *    Nothing
   */

static void SetPal (char* pal)
{
  asm ("  pushl %esi");
  asm ("  cld");
  asm ("  movl    %0, %%esi" : :"m"(pal));
  asm ("  testl   %esi, %esi");
  asm ("  jz      SetPal_Error");
  
  asm ("  movw    $0x3C8, %dx");
  asm ("  xorb    %al, %al");
  asm ("  movl    $256, %ecx");
  asm ("  outb    %al, %dx");
  
  asm ("  movw    $0x3C9, %dx");
  
  asm ("SetPal_Loop:");
  asm ("  lodsb");
  asm ("  shrb    $2, %al");
  asm ("  outb    %al, %dx");
  
  asm ("  lodsb");
  asm ("  shrb    $2, %al");
  asm ("  outb    %al, %dx");
  
  asm ("  lodsb");
  asm ("  shrb    $2, %al");
  asm ("  outb    %al, %dx");
  
  asm ("  incl    %esi");
  
  asm ("  loop    SetPal_Loop");
  
  asm ("SetPal_Error:");
  asm ("  popl %esi");
}

  /*
   *    Set VBE LFB display mode. Ignores banked display modes.
   */

int SetMode (uint16_t mode)
{
	//struct realmode_regs Regs;

	// Already validates Mode and LFB support

	//Regs.eax = 0x4F02;
	//Regs.ebx = mode | 0x4000; // 0x4000 = LFB flag
	//Regs.ecx = 0;
	//Regs.edx = 0;
	//Regs.esi = 0;
	//Regs.edi = 0;
	//Regs.ebp = 0;
	//Regs.esp = -1;
	//Regs.ss = -1;
	//Regs.ds = -1;
	//Regs.es = -1;
	//Regs.fs = -1;
	//Regs.gs = -1;
	//Regs.eflags = -1;
	//Regs.cs = -1;
	//Regs.eip = 0xFFFF10CD;

	//realmode_run((long)&Regs);

	//if (((uint16_t)Regs.eax) != 0x004F)
	if (_INT10_(0x4F02,0x4000|mode,0,0,-1,0) != 0x004F)
		return 0;

	if (modeInfo.MemoryModel == 4 && modeInfo.BitsPerPixel == 8)
	{
		/* Using BIOS to set palette to simulate Direct Color of 3:3:2 */
		struct PaletteEntry
		{
			uint8_t Blue;	// Blue channel value (6 or 8 bits)
			uint8_t Green;	// Green channel value (6 or 8 bits)
			uint8_t Red;	// Red channel value(6 or 8 bits)
			uint8_t Alignment;	// DWORD alignment byte (unused)
		} __attribute__ ((packed));

		int i;
		struct PaletteEntry palette[256];
		// Note that the palette array is passed on the stack. So it is
		// accessible in realmode.

		for (i = 0; i < 256; i++)
		{
			int r, g, b;
			r = (i >> 5)*37;
			g = ((i >> 2) & 7)*37;
			b = (i & 3)*85;
			palette[i].Blue = b;
			palette[i].Green = g>255?255:g;
			palette[i].Red = r>255?255:r;
		}
		//Regs.eax = 0x4F09;
		//Regs.ebx = 0;	// BL=0 Set Palette Data
		//Regs.ecx = 256;	// Number of palette registers to update
		//Regs.edx = 0;	// First of the palette registers to update (start)
		//Regs.esi = 0;
		//Regs.edi = (uint32_t)palette;	// Table of palette values
		//Regs.ebp = 0;
		//Regs.esp = -1;
		//Regs.ss = -1;
		//Regs.ds = -1;
		//Regs.es = 0;	// ES:DI points to palette entries.
		//Regs.fs = -1;
		//Regs.gs = -1;
		//Regs.eflags = -1;
		//Regs.cs = -1;
		//Regs.eip = 0xFFFF10CD;

		//	  realmode_run((long)&Regs);

		//if (((uint16_t)Regs.eax) != 0x004F)
		if (_INT10_(0x4F09,0,256,0,0,(uint32_t)palette) != 0x004F)
		{
			struct
			{
			 uint8_t r;
			 uint8_t g;
			 uint8_t b;
			 uint8_t a;
			} __attribute__ ((packed)) pal[256];

			for (i = 0; i < 256; i++)
			{
				int r, g, b;
				r = (i >> 5)*37;
				g = ((i >> 2) & 7)*37;
				b = (i & 3)*85;
				pal[i].r = r>255?255:r;
				pal[i].g = g>255?255:g;
				pal[i].b = b;
			}
			pal[0].r = 0;
			pal[0].g = 33;
			pal[0].b = 99;
			SetPal ((char*)pal);
			return 2;
		}
	}
	return 1;
}

void CloseMode ()
{
	_INT10_(3,0,0,0,-1,0);
}

  /*
   *  uint32_t EncodePixel (r, g, b)
   *
   *  Purpose:
   *    Encodes R, G, B triplet into generic 32-bit pixel format.
   *    Intended for 15/16/24/32-BPP, 8-BPP needs color matching.
   *
   *  Returns:
   *    Encoded pixel on success
   */
uint32_t EncodePixel (uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t R, G, B;

    R = ((r >> (8 - modeInfo.RedMaskSize  )) & ((1UL<<modeInfo.RedMaskSize)-1  )) << modeInfo.RedFieldPosition;
    G = ((g >> (8 - modeInfo.GreenMaskSize)) & ((1UL<<modeInfo.GreenMaskSize)-1)) << modeInfo.GreenFieldPosition;
    B = ((b >> (8 - modeInfo.BlueMaskSize )) & ((1UL<<modeInfo.BlueMaskSize)-1 )) << modeInfo.BlueFieldPosition;

    return (R | G | B);
}

  /*
   *    Clears the entire LFB
   */
void gfx_Clear8 (uint32_t color)
{
    uint32_t lfbsize = modeInfo.YResolution * modeInfo.BytesPerScanline;
  asm ("  pushl %edi");
  asm ("  cld");
  asm ("  movl    %0, %%edi" : :"m"(modeInfo.PhysBasePtr));
  asm ("  movl    %0, %%ecx" : :"m"(lfbsize));
  asm ("  movl    %0, %%eax" : :"m"(color));
  asm ("  testl   %edi, %edi");
  asm ("  jz      Clear8_Error");
  asm ("  rep stosb");
  asm ("Clear8_Error:");
  asm ("  popl %edi");
}

void gfx_Clear16 (uint32_t color)
{
    uint32_t lfbsize = modeInfo.YResolution * modeInfo.BytesPerScanline;
  asm ("  pushl %edi");
  asm ("  cld");
  asm ("  movl    %0, %%edi" : :"m"(modeInfo.PhysBasePtr));
  asm ("  movl    %0, %%ecx" : :"m"(lfbsize));
  asm ("  movl    %0, %%eax" : :"m"(color));
  asm ("  testl   %edi, %edi");
  asm ("  jz      Clear16_Error");
  asm ("  shrl    $1, %ecx");
  asm ("  rep stosw");
  asm ("Clear16_Error:");
  asm ("  popl %edi");
}

void gfx_Clear24 (uint32_t color)
{
    uint32_t lfbsize = modeInfo.YResolution * modeInfo.BytesPerScanline;
  asm ("  pushl %edi");
  asm ("  cld");
  asm ("  movl    %0, %%edi" : :"m"(modeInfo.PhysBasePtr));
  asm ("  movl    %0, %%ecx" : :"m"(lfbsize));
  asm ("  movl    %0, %%eax" : :"m"(color));
  asm ("  testl   %edi, %edi");
  asm ("  jz      Clear24_Error");
  asm ("  cmpl    $3, %ecx");
  asm ("  jb      Clear24_Error");

  asm ("  movl    %eax, %edx"); /* Prepare last pixel*/
  asm ("  shll    $8, %edx");
  asm ("  andl    $0x00FFFFFF, %eax");
  asm ("  andl    $0xFF000000, %edx");
  asm ("  orl     %edx, %eax");

  asm ("Clear24_Loop:");
  asm ("  subl    $3, %ecx");
  asm ("  jbe     Clear24_Exit");

  /* Write 4 bytes, advance 3 bytes */
  asm ("  stosl");
  asm ("  decl    %edi");
  asm ("  jmp     Clear24_Loop");
  
  asm ("Clear24_Exit:");
  asm ("  roll    $8, %eax"); /* Write last pixel */
  asm ("  movl    %eax, -1(%edi)");
  asm ("Clear24_Error:");
  asm ("  popl %edi");
}

void gfx_Clear32 (uint32_t color)
{
    uint32_t lfbsize = modeInfo.YResolution * modeInfo.BytesPerScanline;
  asm ("  pushl %edi");
  asm ("  cld");
  asm ("  movl    %0, %%edi" : :"m"(modeInfo.PhysBasePtr));
  asm ("  movl    %0, %%ecx" : :"m"(lfbsize));
  asm ("  movl    %0, %%eax" : :"m"(color));
  asm ("  testl   %edi, %edi");
  asm ("  jz      Clear32_Error");
  asm ("  shrl    $2, %ecx");
  asm ("  rep stosl");
  asm ("Clear32_Error:");
  asm ("  popl %edi");
}

void SetPixel (long x, long y, uint32_t color)
{
    uint8_t* lfb;

    if (x < 0 || y < 0 || x >= modeInfo.XResolution || y >= modeInfo.YResolution)
	return;

    lfb = (uint8_t*)(modeInfo.PhysBasePtr + (y * modeInfo.BytesPerScanline) + (x * ((modeInfo.BitsPerPixel + 7) / 8)));

    switch (modeInfo.BitsPerPixel)
    {
    case 8:
		*lfb = (uint8_t)color;
		break;
    case 15:
    case 16:
		*(uint16_t *)lfb = (uint16_t)color;
		break;
    case 24:
		*(uint16_t *)lfb = (uint16_t)color;
		//lfb[0] = (uint8_t)(color >> 0);
		//lfb[1] = (uint8_t)(color >> 8);
		lfb[2] = (uint8_t)(color >> 16);
		break;
    case 32:
		*(uint32_t *)lfb = (uint32_t)color;
		break;
    }

}

  /*
   *    Draws a solid rectangle from (x1, y1) to (x2, y2)
   */

void gfx_FillRect8 (long p_x1, long p_y1, long p_x2, long p_y2, uint32_t color)
{
    uint8_t* l_lfb;
    uint32_t l_width;
    uint32_t l_height;
    uint32_t l_skip;
    long  l_minx;
    long  l_miny;
    long  l_maxx;
    long  l_maxy;

    // Clip specified rectangle to context boundaries
    l_minx = 0;
    if( p_x1 >= l_minx )
      l_minx = p_x1;

    l_miny = 0;
    if( p_y1 >= l_miny )
      l_miny = p_y1;

    l_maxx = modeInfo.XResolution - 1;
    if( p_x2 <= l_maxx )
      l_maxx = p_x2;

    l_maxy = modeInfo.YResolution - 1;
    if( p_y2 <= l_maxy )
      l_maxy = p_y2;

    // Validate boundaries
    if( l_minx >= l_maxx )
      return;

    if( l_miny >= l_maxy )
      return;

    // Initialize loop variables
    l_width  = (l_maxx - l_minx) + 1;
    l_height = (l_maxy - l_miny) + 1;

    l_skip   = modeInfo.BytesPerScanline - l_width;

    // Calculate buffer offset
    l_lfb = (uint8_t*)(modeInfo.PhysBasePtr + (l_miny * modeInfo.BytesPerScanline) + l_minx);

    // Draw rectangle
    //gfx_FillRect8_asm( l_lfb, l_skip, l_width, l_height, p_pixel );
  asm ("  pushl %esi");
  asm ("  pushl %edi");
  asm ("  pushl %ebx");
  asm ("  cld");
  asm ("  movl    %0, %%edi" : :"m"(l_lfb));
  asm ("  movl    %0, %%edx" : :"m"(l_skip));
  asm ("  movl    %0, %%ecx" : :"m"(l_width));
  asm ("  movl    %0, %%ebx" : :"m"(l_height));
  asm ("  movl    %0, %%eax" : :"m"(color));
  asm ("  testl   %edi, %edi");
  asm ("  jz      FillRect8_Error");
  asm ("  movl    %ecx, %esi");
  asm ("FillRect8_Loop:");
  asm ("  movl    %esi, %ecx");
  asm ("  rep stosb");
  asm ("  addl    %edx, %edi");
  asm ("  decl    %ebx");
  asm ("  jnz      FillRect8_Loop");
  asm ("FillRect8_Error:");
  asm ("  popl %ebx");
  asm ("  popl %edi");
  asm ("  popl %esi");
}

void gfx_FillRect16 (long p_x1, long p_y1, long p_x2, long p_y2, uint32_t color)
{
    uint16_t* l_lfb;
    uint32_t  l_width;
    uint32_t  l_height;
    uint32_t  l_skip;
    long   l_minx;
    long   l_miny;
    long   l_maxx;
    long   l_maxy;

    // Clip specified rectangle to context boundaries
    l_minx = 0;
    if( p_x1 >= l_minx )
      l_minx = p_x1;

    l_miny = 0;
    if( p_y1 >= l_miny )
      l_miny = p_y1;

    l_maxx = modeInfo.XResolution - 1;
    if( p_x2 <= l_maxx )
      l_maxx = p_x2;

    l_maxy = modeInfo.YResolution - 1;
    if( p_y2 <= l_maxy )
      l_maxy = p_y2;

    // Validate boundaries
    if( l_minx >= l_maxx )
      return;

    if( l_miny >= l_maxy )
      return;

    // Initialize loop variables
    l_width  = (l_maxx - l_minx) + 1;
    l_height = (l_maxy - l_miny) + 1;

    l_skip   = modeInfo.BytesPerScanline - (l_width * 2);

    // Calculate buffer offset
    l_lfb = (uint16_t*)(modeInfo.PhysBasePtr + (l_miny * modeInfo.BytesPerScanline) + (l_minx * 2));

    // Draw rectangle
    //gfx_FillRect16_asm( l_lfb, l_skip, l_width, l_height, p_pixel );
  asm ("  pushl %esi");
  asm ("  pushl %edi");
  asm ("  pushl %ebx");
  asm ("  cld");
  asm ("  movl    %0, %%edi" : :"m"(l_lfb));
  asm ("  movl    %0, %%edx" : :"m"(l_skip));
  asm ("  movl    %0, %%ecx" : :"m"(l_width));
  asm ("  movl    %0, %%ebx" : :"m"(l_height));
  asm ("  movl    %0, %%eax" : :"m"(color));
  asm ("  testl   %edi, %edi");
  asm ("  jz      FillRect16_Error");
  asm ("  movl    %ecx, %esi");
  asm ("FillRect16_Loop:");
  asm ("  movl    %esi, %ecx");
  asm ("  rep stosw");
  asm ("  addl    %edx, %edi");
  asm ("  decl    %ebx");
  asm ("  jnz      FillRect16_Loop");
  asm ("FillRect16_Error:");
  asm ("  popl %ebx");
  asm ("  popl %edi");
  asm ("  popl %esi");
}

void gfx_FillRect24 (long p_x1, long p_y1, long p_x2, long p_y2, uint32_t color)
{
    uint8_t* l_lfb;
    uint32_t l_width;
    uint32_t l_height;
    uint32_t l_skip;
    long  l_minx;
    long  l_miny;
    long  l_maxx;
    long  l_maxy;

    // Clip specified rectangle to context boundaries
    l_minx = 0;
    if( p_x1 >= l_minx )
      l_minx = p_x1;

    l_miny = 0;
    if( p_y1 >= l_miny )
      l_miny = p_y1;

    l_maxx = modeInfo.XResolution - 1;
    if( p_x2 <= l_maxx )
      l_maxx = p_x2;

    l_maxy = modeInfo.YResolution - 1;
    if( p_y2 <= l_maxy )
      l_maxy = p_y2;

    // Validate boundaries
    if( l_minx >= l_maxx )
      return;

    if( l_miny >= l_maxy )
      return;

    // Initialize loop variables
    l_width  = (l_maxx - l_minx) + 1;
    l_height = (l_maxy - l_miny) + 1;

    l_skip   = modeInfo.BytesPerScanline - (l_width * 3);

    // Calculate buffer offset
    l_lfb = (uint8_t*)(modeInfo.PhysBasePtr + (l_miny * modeInfo.BytesPerScanline) + (l_minx * 3));

    // Draw rectangle
    //gfx_FillRect24_asm( l_lfb, l_skip, l_width, l_height, color );
  asm ("  pushl %esi");
  asm ("  pushl %edi");
  asm ("  pushl %ebx");
  asm ("  cld");
  asm ("  movl    %0, %%edi" : :"m"(l_lfb));
  asm ("  movl    %0, %%edx" : :"m"(l_skip));
  asm ("  movl    %0, %%ecx" : :"m"(l_width));
  asm ("  movl    %0, %%ebx" : :"m"(l_height));
  asm ("  movl    %0, %%eax" : :"m"(color));
  asm ("  testl   %edi, %edi");
  asm ("  jz      FillRect24_Error");
  asm ("  movl    %ecx, %esi");
  asm ("  addl    %ecx, %ecx");
  asm ("  addl    %esi, %ecx");
  asm ("  movl    %ecx, %esi");

  asm ("FillRect24_Loop:");
  asm ("  cmpl    $3, %ecx");
  asm ("  jbe     FillRect24_ExitLoop");

  /* Write 4 bytes, advance 3 bytes */
  asm ("  stosl");
  asm ("  decl    %edi");

  asm ("  subl    $3, %ecx");
  asm ("  jmp     FillRect24_Loop");

  asm ("FillRect24_ExitLoop:");
  /* Write last pixel */
  asm ("  movb    3(%edi), %cl");
  asm ("  andl    $0x00FFFFFF, %eax");
  asm ("  shll    $24, %ecx");
  asm ("  orl     %ecx, %eax");
  asm ("  stosl");
  asm ("  decl    %edi");

  asm ("  movl    %esi, %ecx");
  asm ("  addl    %edx, %edi");
  asm ("  decl    %ebx");
  asm ("  jnz     FillRect24_Loop");
  asm ("FillRect24_Error:");
  asm ("  popl %ebx");
  asm ("  popl %edi");
  asm ("  popl %esi");
}

  /*
   *  void gfx_FillRect32 (x1, y1, x2, y2, color)
   *
   *  Purpose:
   *    Draws a solid 32-BPP rectangle from (x1, y1)
   *    to (x2, y2) using specified pixel color,
   *
   *  Returns:
   *    Nothing
   */


void gfx_FillRect32 (long x1, long y1, long x2, long y2, uint32_t color)
{
    uint32_t* l_lfb;
    uint32_t  l_width;
    uint32_t  l_height;
    uint32_t  l_skip;
    long   l_minx;
    long   l_miny;
    long   l_maxx;
    long   l_maxy;

    // Clip specified rectangle to context boundaries
    l_minx = 0;
    if (l_minx < x1)
        l_minx = x1;

    l_miny = 0;
    if (l_miny < y1)
        l_miny = y1;

    l_maxx = modeInfo.XResolution - 1;
    if (l_maxx > x2)
        l_maxx = x2;

    l_maxy = modeInfo.YResolution - 1;
    if (l_maxy > y2)
        l_maxy = y2;

    // Validate boundaries
    if( l_minx >= l_maxx )
      return;

    if( l_miny >= l_maxy )
      return;

    // Initialize loop variables
    l_width  = (l_maxx - l_minx) + 1;
    l_height = (l_maxy - l_miny) + 1;

    l_skip   = modeInfo.BytesPerScanline - (l_width * 4);

    // Calculate buffer offset
    l_lfb = (uint32_t*)(modeInfo.PhysBasePtr + (l_miny * modeInfo.BytesPerScanline) + (l_minx * 4));

    // Draw rectangle
    //gfx_FillRect32_asm (l_lfb, l_skip, l_width, l_height, color);
  asm ("  pushl %esi");
  asm ("  pushl %edi");
  asm ("  pushl %ebx");
  asm ("  cld");
  asm ("  movl    %0, %%edi" : :"m"(l_lfb));
  asm ("  movl    %0, %%edx" : :"m"(l_skip));
  asm ("  movl    %0, %%ecx" : :"m"(l_width));
  asm ("  movl    %0, %%ebx" : :"m"(l_height));
  asm ("  movl    %0, %%eax" : :"m"(color));
  asm ("  testl   %edi, %edi");
  asm ("  jz      FillRect32_Error");
  asm ("  movl    %ecx, %esi");
  asm ("FillRect32_Loop:");
  asm ("  movl    %esi, %ecx");
  asm ("  rep stosl");
  asm ("  addl    %edx, %edi");
  asm ("  decl    %ebx");
  asm ("  jnz      FillRect32_Loop");
  asm ("FillRect32_Error:");
  asm ("  popl %ebx");
  asm ("  popl %edi");
  asm ("  popl %esi");

}

static int open_bmp(char *file)
{
	typedef unsigned long DWORD;
	typedef unsigned long LONG;
	typedef unsigned short WORD;
	struct { /* bmfh */ 
			WORD bfType;
			DWORD bfSize; 
			DWORD bfReserved1; 
			DWORD bfOffBits;
		} __attribute__ ((packed)) bmfh;
	struct { /* bmih */ 
		DWORD biSize; 
		LONG biWidth; 
		LONG biHeight; 
		WORD biPlanes; 
		WORD biBitCount; 
		DWORD biCompression; 
		DWORD biSizeImage; 
		LONG biXPelsPerMeter; 
		LONG biYPelsPerMeter; 
		DWORD biClrUsed; 
		DWORD biClrImportant;
	} __attribute__ ((packed)) bmih;
	unsigned long modeLineSize;
	unsigned char *mode_ptr;
	if (!open(file))
		return 0;
	if (! read((unsigned long long)(unsigned int)&bmfh,sizeof(bmfh),GRUB_READ) || bmfh.bfType != 0x4d42)
	{
		close();
		return !printf("Err: fil");
	}
	if (! read((unsigned long long)(unsigned int)&bmih,sizeof(bmih),GRUB_READ))
	{
		close();
		return !printf("Error:read1\n");
	}
	uint16_t*	modeList = (uint16_t*)drvInfo.VideoModePtr;
	uint16_t mode;
	printf("Info: %dX%dX%d\n",bmih.biWidth,bmih.biHeight,bmih.biBitCount);
	printf("BMP.size: %d\n",bmfh.bfSize);
	while ((mode = *modeList++) != 0xFFFF)
	{
		if (! GetModeInfo (mode) || 
			modeInfo.XResolution != bmih.biWidth
		|| modeInfo.YResolution != bmih.biHeight
		|| modeInfo.BitsPerPixel < bmih.biBitCount)
			continue;
		if (! SetMode (mode))
		{
			printf( "VBE mode %4x not supported\n", mode);
		}
		mode_ptr = (unsigned char*)modeInfo.PhysBasePtr;
		modeLineSize = modeInfo.BytesPerScanline;
		break;
	}

	if (mode == 0xffff)
	{
		printf("not supported!");
		close();
		getkey();
		return 0;
	}
	unsigned int LineSize=(bmih.biWidth*(bmih.biBitCount>>3)+3)&~3;
	char *buff=malloc(bmfh.bfSize);
	int x = 0,y = bmih.biHeight-1;
	char *p,*p1;
	if (!read((unsigned long long)(unsigned int)buff,bmfh.bfSize,GRUB_READ))
	{
		close();
		return 0;
	}
	close();
	p = buff;
	vbe_flags.img = malloc(modeInfo.BytesPerScanline*modeInfo.YResolution);
	for(y=bmih.biHeight-1;y>=0;--y)
	{
		p1 = vbe_flags.img + modeLineSize * y;
		for(x=0;x<bmih.biWidth;++x)
		{
			*p1++=*p++;
			*p1++=*p++;
			*p1++=*p++;
			if (modeInfo.BitsPerPixel==32)
				*++p1;
		}
		p = buff +(bmih.biHeight-y)*LineSize;
	}
	free(buff);
//	memmove(mode_ptr,vbe_flags.img,bmfh.bfSize);
//	getkey();
//	CloseMode();
	vbe_flags.mode = mode;
}

