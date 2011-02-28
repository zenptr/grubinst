/*
 * Distributed as public domain by SvOlli (email available @ svolli.org)
 * It is free as in both free beer and free speech.
 *
 * THIS SOFTWARE IS PROVIDED BY SvOlli ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL SvOlli BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	Modify By chenall at 2011-01-28,	http://chenall.net
 *	Report bugs to website:  http://code.google.com/p/grubutils/issues
 */
 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mbr.h"
#include "xdio.h"
#include "utils.h"
#define WEE_SIGN 0xCE1A02B0
#define MAX_DATA_SIZE (63<<9)
typedef unsigned char uchar;
typedef unsigned short uchar2;
typedef unsigned long uchar4;
struct fb_mbr
{
  uchar jmp_code;
  uchar jmp_ofs;
  uchar boot_code[0x1ab];
  uchar max_sec;		/* 0x1ad  */
  uchar2 lba;			/* 0x1ae  */
  uchar spt;			/* 0x1b0  */
  uchar heads;			/* 0x1b1  */
  uchar2 boot_base;		/* 0x1b2  */
  uchar4 fb_magic;		/* 0x1b4  */
  uchar mbr_table[0x46];	/* 0x1b8  */
  uchar2 end_magic;		/* 0x1fe  */
} PACK;

void
read_disk (xd_t *xd, void *buf, int size)
{
  if (xd_read (xd, buf, size))
  {
    printf ("xd_read fails at offset %ld, size %d", xd->ofs, size);
    exit(1);
  }
}

void
write_disk (xd_t *xd, void *buf, int size)
{
  if (xd_write (xd, buf, size))
  {
    printf ("xd_write fails at offset %ld, size %d", xd->ofs, size);
    exit(1);
  }
}

void
seek_disk (xd_t *xd, unsigned long ofs)
{
  if (xd_seek (xd, ofs))
  {
    printf ("xd_seek fails at offset %ld", ofs);
    exit(1);
  }
}

void
list_devs (void)
{
	int i;
	char name[16];

	for (i = 0; i < MAX_DISKS; i++)
	{
		xd_t *xd;

		sprintf (name, "(hd%d)", i);
		xd = xd_open (name, 1);
		if (xd)
		{
			unsigned long size;
			int s;
			char c;
			struct fb_mbr m;

			size = xd_size (xd);
			if (size == XD_INVALID_SIZE)
				goto quit;

			if (xd_read (xd, (char *) &m, 1))
				goto quit;

			if (size >= (3 << 20))
			{
				s = (size + (1 << 20)) >> 21;
				c = 'g';

			}
			else
			{
				s = (size + (1 << 10)) >> 11;
				c = 'm';
			}

			printf ("%s: %lu (%d%c)\n", name, size, s, c);

			quit:
			xd_close (xd);
		}
	}
}

static int readfile( const char *filename, char *buf, int size)
{
	int fd;
	int dataread;
	fd = open( filename, O_RDONLY | O_BINARY);
	dataread = read( fd, buf, size);
	if( dataread <= 0 )
	{
		printf( "%s: read error:%d\n", filename,dataread);
		exit( 2 );
	}
	close( fd );
	return dataread;
}

int check_mbr_data(char *mbr)
{
	if (*(unsigned short *)(mbr + 510) != 0xAA55 || *(unsigned long *)(mbr + 0x1B4) == 0x46424246 || memcmp(mbr + 0x200,"EFI PART",8) == 0)
	{
		return 1;
	}

	int n;
	unsigned long ofs;
	ofs=0xFFFFFFFF;
	for (n=0x1BE;n<0x1FE;n+=16)
	{
		if (mbr[n+4])
		{
			if (ofs>valueat(mbr[n],8,unsigned long))
				ofs=valueat(mbr[n],8,unsigned long);
		}
	}
	if (ofs<63)
	{
		fprintf(stderr,"Not enough room to install mbr");
		return 1;
	}
	return 0;
}

int help(void)
{
	printf("weesetup v1.2.\n");
	printf("Usage:\n\tweesetup [OPTIONS] DEVICE\n\nOPTIONS:");
	printf("\n\t-i wee63.mbr\t\tUse a custom wee63.mbr file.\n");
	printf("\n\t-o outfile\t\tExport new wee63.mbr to outfile.\n");
	printf("\n\t-s scriptfile\t\tImport script from scriptfile.\n");
	printf("\n\t-m mbrfile\t\tRead mbr from mbrfile(must use with option -o).\n");
	printf("\n\t-f\t\t\tForce install.\n");
	printf("\t-u\t\t\tUpdate.\n");
	printf("\t-b\t\t\tBackup mbr to second sector(default is nt6mbr).\n");
	printf("\t-l\t\t\tList all disks in system and exit\n");
	printf("\nReport bugs to website:\n\thttp://code.google.com/p/grubutils/issues\n");
	printf("\nThanks:\n");
	printf("\twee63.mbr (minigrub for mbr by tinybit)\n\t\thttp://bbs.znpc.net/viewthread.php?tid=5838\n");
	printf("\twee63setup.c by SvOlli,xdio by bean");
	return 0;
}

int main( int argc, char *argv[] )
{
   xd_t* xd;
   int  opt;
   int  size;
   int  i;
   int  fd;
   int  install_flag	 = 0;
   char *mbr_data        = 0;
   char *wee63_data      = (char *)wee63_mbr;
   char *script_data     = 0;
   char *in_filename     = 0;
   char *out_filename    = 0;
   char *mbr_filename    = 0;
   char *script_filename = 0;
   if (argc == 1)
   {
   	return help();
  }
   while ((opt = getopt(argc, argv, "lbufi:m:o:s:")) != -1)
   {
      switch (opt)
      {
         case 'l':
           list_devs();
           return 1;
         case 'b':
            install_flag |= 1;
            break;
         case 'u':
            install_flag |= 2;
            break;
        case 'f':
            install_flag |= 16;
            break;
         case 'i':
            in_filename = optarg;
            break;
         case 'm':
            mbr_filename = optarg;
            break;
         case 'o':
            out_filename = optarg;
            break;
         case 's':
            script_filename = optarg;
            break;
         default:
            return help();
      }
   }

	if (in_filename)
	{
		size = readfile(in_filename, wee63_data, 63<<9);
	}
	i = *(unsigned long *)(wee63_data + 0x84) & 0xFFFF;
	if (*(unsigned long *)(wee63_data + 0x86) != WEE_SIGN
	 || *(unsigned long *)(wee63_data + 0x86) != *(unsigned long *)(wee63_data + i))
	{
		printf("Err wee63.mbr file!");
		exit(3);
	}

	i += 0x10;	//wee63 script offset;
	script_data = wee63_data + i;

	mbr_data = malloc(MAX_DATA_SIZE);
	if (! mbr_data)
	{
		fprintf(stderr,"not enough memory.!");
		exit(1);
	}
	memset(mbr_data,0,MAX_DATA_SIZE);

	if (mbr_filename)
	{
		readfile(mbr_filename, mbr_data,0x400);
	}
	else
	{
		mbr_filename = argv[argc-1];
		if (*mbr_filename == '(')
		{
			xd = xd_open (mbr_filename, 1);
			if (xd)
			{
				read_disk(xd, mbr_data ,63);
				xd_close (xd);
				fd = open( "backup.mbr", O_WRONLY | O_CREAT | O_BINARY | O_TRUNC, 0666 );
				size = write(fd, mbr_data, MAX_DATA_SIZE);
				close(fd);
				if (size != MAX_DATA_SIZE)
				{
					goto quit;
				}
				printf("Backup %s MBR to backup.mbr.\n",mbr_filename);
				install_flag |= 4;
			}
		}
		else
		{
			install_flag |= 8;
		}
	}
	if (!(install_flag & 4) && !out_filename)
	{
		help();
		goto quit;
	}

	if (script_filename)
	{
		size = readfile( script_filename, script_data, MAX_DATA_SIZE - i - 1 );
		memset(script_data + size,0,MAX_DATA_SIZE - i - size);
		printf("Imported Script from file: %s\n",script_filename);
	}

	if (!(install_flag & 8))
	{
		if (*(unsigned long *)(mbr_data + 0x86) == WEE_SIGN && !(install_flag & 2))
		{
			fprintf (stderr,"Wee already Installed,Use -u option!");
			goto quit;
		}

		int fs;
		fs=get_fstype(mbr_data);

		if (fs == FST_MBR2)
		{
			if (install_flag & 16)
			{
				fs = FST_MBR;
			}
			else
			{
				printf("Bad partition table, Use -f option to force install.");
				goto quit;
			}
		}
		if ( fs != FST_MBR || check_mbr_data(mbr_data))
		{
			fprintf (stderr,"Err: Not Support MBR.");
			goto quit;
		}

		memmove(wee63_data + 0x1B8 , mbr_data + 0x1B8 , 0x48);

		if (install_flag & 1)
		{
			memmove(wee63_data + 0x200 , mbr_data, 0x200);
		}
		else
		{
			memmove(wee63_data + 0x3B8, mbr_data + 0x1B8 , 0x48);
		}
		if (memcmp(wee63_data + 0x1B8, mbr_data + 0x1B8,0x48))
		{
			printf("Error: Partition table");
			goto quit;
		}
	}

	if (out_filename)
	{
		fd = open( out_filename, O_WRONLY | O_CREAT | O_BINARY | O_TRUNC, 0666 );
		size = write(fd, wee63_data,MAX_DATA_SIZE);
		close(fd);
		printf("Saved outfile %s.\n",out_filename);
	}

	if (install_flag & 4)
	{
		xd = xd_open (mbr_filename, 1);
		opt = 0;
		for (i = 0; i< 63; ++i)
		{
			if (memcmp(wee63_data,mbr_data + (i<<9),0x200))
			{
				seek_disk (xd, i);
				write_disk(xd, wee63_data , 1);
				++opt;
			}
			wee63_data += 0x200;
		}
		
		seek_disk (xd, 0);
		read_disk(xd, wee63_mbr + 0x200 , 1);
		if (memcmp(wee63_mbr,wee63_mbr + 0x200,0x200))
		{
			printf("Unknow Error.");
			seek_disk (xd, 0);
			write_disk(xd, mbr_data , 1);
			xd_close(xd);
		}
		else
		{
			xd_close (xd);
			printf("Install wee to %s succeed!\n %d sectors changed.\n",mbr_filename,opt);
		}
	}
	free(mbr_data);
	return 0;
	quit:
	free(mbr_data);
	return 1;
}
