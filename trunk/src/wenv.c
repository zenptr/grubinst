/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004  Free Software Foundation, Inc.
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

gcc -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE wenv.c

 * disassemble:			objdump -d a.out 
 * confirm no relocation:	readelf -r a.out
 * generate executable:		objcopy -O binary a.out wenv
 *
 */
/*
 * to get this source code & binary: http://grub4dos-chenall.google.com
 * For more information.Please visit web-site at http://chenall.net/grub4dos_wenv/
 * 2010-06-20
 2010-10-05
   1.添加字符串处理功能,和linux shell类似.
      1).${VAR:x:y}  从x开始提取y个字符,如果x为负数则从倒数x个开始提取.
      
      以下提取的内容都不包含STRING字符串.
      2).${VAR#STRING}	提取STRING字符串后面的内容,从左往右第一个位置开始.
      3).${VAR##STRING} 提取STRING字符串后面的内容,从右往左第一个位置开始.
      4).${VAR%STRING}	提取STRING字符串前面的内容,从右往左第一个位置开始.
      5).${VAR%%STRING}	提取STRING字符串前面的内容,从左往右第一个位置开始.
 2010-10-04
   1.函数调整.
 2010-09-30
   1.调整calc函数。
 2010-09-06
	1.修正一个逻辑错误（在释放变量的时候）。
	http://bbs.wuyou.com/viewthread.php?tid=159851&page=25#pid2033990
 2010-08-08 1.read 函数读取的文件允许使用":"符号注释.使用":"开头,这一行将不会执行.
	2.解决当使用read可能导致出错的问题.
 */
#include "grub4dos.h"
//#define DEBUG
#define BASE_ADDR 0x45000 //变量保存地址
#define VAR ((char (*)[8])BASE_ADDR) //变量索引表 共计 8*64 = 512
#define ENVI ((char (*)[512])(BASE_ADDR + 0x200)) //变量对应的值 共计512*64 = 32768
#define VAR_MAX 64

#define MAX_ARG_LEN 1024
#define MAX_VAR_LEN	8
#define MAX_ENV_LEN	512

static char arg_new[MAX_ARG_LEN];

static int wenv_func(char *arg, int flags);
static int wenv_help(void);
static int grub_memcmp (const char *s1, const char *s2, int n);

/* 
arg比较来源字符串,string要对比的字符串.
功能:
参数比较(类似于strcmp，但是这里作了扩展)
1.arg可以比string长;使用空格' ',制表符'\t',等于号'=' 分隔参数;
2.如果string字符串全部是小写字符,则不区分大小写进行比较.
*/
#define strcmp_ex(arg,string) grub_memcmp(arg,string,0)
static char *strstrn(const char *s1,const char *s2,const int n);

static int envi_cmd(const char var[MAX_VAR_LEN],char env[MAX_ENV_LEN],int flags);
#define set_envi(var,val) envi_cmd(var,val,0)
#define get_env(var,val) envi_cmd(var,val,1)
/*
  变量替换主要函数,把in里面的变量扩展后输出到out 
  如果flags值为真则,如果碰到前导*的数字,尝试读取该内存指向的字符串数据.
*/
static int replace_str(char *in,char *out,int flags); 


static int read_val(char **str_ptr,unsigned long long *val); /*读取一个数值(用于计算器).*/
static unsigned long calc (char *arg);/*简单计算器模块,参数是一个字符串*/

/*把ch指针指向的字符串按unicode编码并写入到地址addr*/
static int ascii_unicode(const char *ch,char *addr);

static int read_file(char *arg);

static char *lower(char *string);

static char *upper(char *string);

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
	return wenv_func (arg , flags);
}

static int wenv_func(char *arg, int flags)
{
	int i;
	char var[MAX_VAR_LEN] = "\0";
	char value[512] = "\0";
	
	if (! strcmp_ex((char *)BASE_ADDR,"__wenv") == 0 )
	{
		memset((char *)BASE_ADDR,0,512);//只清除变量索引表
		sprintf((char *)BASE_ADDR,"__wenv");
	}
	
	if (*arg)
	{
		memset(arg_new, 0, 1024);
		if (strcmp_ex(arg,"read") == 0)
		{
			arg = skip_to (0,arg);
			return read_file(arg);
		}
		else if (strcmp_ex(arg,"set") == 0)
		{
			arg=skip_to(0,arg);
			if (! *arg) return envi_cmd(NULL,arg_new,2);
			int ucase=0;

			for (i=0;i<MAX_VAR_LEN;i++)
			{
				if ((arg[i] == '=') || (arg[i] < 48)) break;
				var[i] = arg[i];
			}
			
			arg = skip_to(1,arg);
			
			if (! *arg) return envi_cmd(var,"\0",0);
			
			if (*arg=='$')
			{
				if (arg[1] == 'U' || arg[1] == 'u')
				{
					ucase = 1;
					arg +=3 ;
					while (*arg == ',' || *arg == ' ' || *arg == '\t') arg++;
				} 
				else if (arg[1] == 'L' || arg[1] == 'l')
				{
					ucase = 2;
					arg +=3 ;
					while (*arg == ',' || *arg == ' ' || *arg == '\t') arg++;
				}
			}
			
			 if (grub_memcmp(arg, "$input,", 7) == 0)
			{
				arg += 7;
				struct get_cmdline_arg get_cmdline_str;
				get_cmdline_str.prompt = arg;
				get_cmdline_str.maxlen = MAX_ENV_LEN;
				get_cmdline_str.cmdline = arg_new;
				get_cmdline_str.echo_char = 0;
				get_cmdline_str.readline = 1;
				if (get_cmdline(get_cmdline_str))
					return 0;
			}
			else
			{
				if (!replace_str(arg,arg_new,1))
				    return 0;
			}
			
			if (ucase == 1)
				upper(arg_new);
			else if (ucase == 2)
				lower(arg_new);
			
			return set_envi(var,arg_new);
		}
		  else if (strcmp_ex(arg, "get") == 0)
		{
			arg=skip_to(0,arg);
			if (! *arg) return envi_cmd(NULL,arg_new,2);
			for (i=0;i<MAX_VAR_LEN;i++)
			{
				if ((arg[i] == '=') || (arg[i] < 48)) break;
				var[i] = arg[i];
			}
			
			memset(value,0,MAX_ENV_LEN);
			get_env(var,value);
			if (value[0])
			{
				arg=skip_to(1,arg);
				if (*arg)
				{
					unsigned long long addr;
					if (! safe_parse_maxint(&arg,&addr))
						errnum = 0;
					else
					{
						return ascii_unicode(value,(char *)(int)addr);
					}
				}
				if (debug > 0)
					printf("%s=%s",&var,&value);
				return 1;
			}
			return 0;
		}
		  else if(strcmp_ex(arg,"run") == 0)
		{
			arg = skip_to(0,arg);
			if (!replace_str(arg,arg_new,1)) return 0;
			#ifdef DEBUG
			if (debug >1)
			printf("%s\n",arg_new);
			#endif
			int ret;
			char *p = arg_new;
			char *cmd = arg_new;
			while (p = strstr(p,"]]"))
			{
				if (p[2] == '&' || p[2] == '|' || p[2] == ']')
				{
					*p=0;
					p += 2;
					arg = skip_to(0,cmd);
					nul_terminate(cmd);
					ret = builtin_cmd(cmd, arg, flags);
					errnum = 0;
					if ((*p == '&' && ret) || (*p == '|' && ! ret) || *p == ']')
					{
						p++;
						while (*p == ' ' || *p == '\t') p++;
						cmd = p;
						continue;
					}
					else 
					{
						sprintf(arg_new,"0x%X\0",ret);
						set_envi ("__wenv",arg_new);
						return ret;
					}
				}
				else
				{
					p += 2;
					continue;
				}
			}
			arg = skip_to(0,cmd); /* 取得参数部份 */
			nul_terminate(cmd); /* 取得命令部份 */
			if (strcmp_ex(cmd, "echo") == 0)
			{
				ret = printf("%s\n",arg);
			}
			else
			{
				ret = builtin_cmd(cmd, arg, flags);
			}
			if (errnum) return 0;
			sprintf(arg_new,"0x%X\0",ret);
			set_envi ("__wenv",arg_new);
			return ret;
		}
		  else if (strcmp_ex(arg, "calc") == 0)
		{
			arg = skip_to(0,arg);
			if (!replace_str(arg,arg_new,0)) return 0;
			return calc(arg_new);
		}
		else if (strcmp_ex(arg, "reset") == 0)
		{
			memset((char *)BASE_ADDR,0,512);
			return sprintf((char *)BASE_ADDR,"__wenv");
		}
	}
	return wenv_help();
};

static int wenv_help(void)
{
return printf("\n WENV Using variables in GRUB4DOS,Compiled time: %s %s\n\
\n WENV set VARIABLE=[$[U|L],] [$input,] STRING\n\
\n WENV get [VARIABLE]\n\
\n WENV read FILE\n\
\n WENV calc [[VARIABLE | [*]INTEGER]=] [*]INTEGER OPERATOR [[*]INTEGER]\n\
\n WENV run COMMAND\n\tthe COMMAND can use VARIABLE with ${VARIABLE} or *ADDR\n\
\n For more information.Please visit web-site at http://chenall.net/tag/grub4dos\n\
 to get this source and bin: http://grub4dos-chenall.googlecode.com\n",__DATE__,__TIME__);
};

static int replace_str(char *in, char *out, int flags)
{
   char *po = out;
   char *p = in;
   int i;
   unsigned long long addr;
   char ch[MAX_VAR_LEN]="\0";
   char value[512];
   memset(out, 0, MAX_ENV_LEN);
   while (*in && out - po < MAX_ENV_LEN)
   {
      switch(*(p = in))
      {
         case '*':/*当flags为真时 替换命令参数为内存地址指向的字符串*/
            if (! flags)
            {
               *out++ = *in++;
               continue;
            }
            p++;
            if (! safe_parse_maxint (&p, &addr))
            {
               errnum = 0;
               *out++ = *in++;
               continue;
            }
            in = p;
            p = (char *)(unsigned long)addr;
            while (*p && *p != '\r' && *p != '\n') *out++ = *p++;
            break;
            
         case '$':/*替换变量,如果这个变量不符合不替换保持原来的字符串*/
            
            if (p[1] != '{')
            {
               *out++ = *in++;
               continue;
            }
            
            p += 2;
            
            long long start = 0LL;//提取字符起始位置
            unsigned long long len=-1ULL;//提取字符串长度
            int str_flag = 0;
            str_flag = 0;
            memset(ch,0,MAX_VAR_LEN);
            for (i=0;i<MAX_VAR_LEN;i++)
            {
               if (*p == '}') //提取变量成功.
               {
                  break;
               }
               else if (*p == ':') //截取指定长度字符串关键字符
	       {
		  p++;
		  if (! safe_parse_maxint (&p, &start)) //起始位置
		  {
		     break;//不符号要求,则不处理.如果后面刚好是一个"}"则相当于过滤掉了":"
		  }
		  if (*p == ':' && (p++,! safe_parse_maxint (&p, &len)))//长度,非必要的
		  {
		     break;//同上
		  }
		  break;//不再判断,直接退出.
	       }
	       else if (*p == '#')
	       {
		  if (*++p == '#')
		  {
		     p++;
		     str_flag = 3;
		  }
		  else
		     str_flag = 2;

		  if (*p == '*') p++;
		  start = (long long)(int)p;//设定起始位置(保存的是一个指针数值)
		  len = 0;//字符串长度,后面需要跳过这些字符.
		  while (*p && *p != '}')//跳过后面的字符
		  {
		     len++;
		     p++;
		  }
		  break;
	       }
	       else if (*p == '%')
	       {
       		  if (*++p == '%')
       		  {
		     str_flag = 4;
		     p++;
		  }
		  else
		     str_flag = 5;

		  start = (long long)(int)p;//设定起始位置(保存的是一个指针数值);
		  while (*p && *p != '}') p++;//跳过后面的字符
		  if (*(p-1) == '*') *(p-1) = '\0'; //截断
		  break;
	       }
               ch[i] = *p++;
            }
            errnum = 0;
            if (*p != '}')
            {
               *out++ = *in++;
               continue;
            }
	    *p = '\0'; //截断
            in = ++p;
            
            memset(value,0,MAX_ENV_LEN);
            if (get_env(ch,value))
            {
	       if (str_flag)
	       {
		  if (! (p = strstrn(value, (char *)(int)start, str_flag & 1)))
		     continue;
		  if (str_flag & 4)
		  {
		     len = p - value;
		     p = value;
		  }
		  else
		  {
		     p += len;
		     len = -1;
		  }
	       }
	       else
	       {
		  if (start < 0) start += strlen(value);
		  p = value + start;
	       }

               while (*p && len)
               {
		  *out++ = *p++;
		  len--;
	       }
            }
            
            break;

         default:/* 默认不替换直接复制字符*/
            *out++ = *in++;
            break;
      }
   }
   #ifdef DEBUG
      if (debug > 1)
         printf("replace_str:%s\n",po);
   #endif
   return 1;
};

static int envi_cmd(const char var[MAX_VAR_LEN],char env[MAX_ENV_LEN],int flags)
{
	int i,j;
	char ch[MAX_VAR_LEN]="\0";

	if (flags == 2)//显示所有变量信息
	{
		for(i=0;i <= VAR_MAX && VAR[i][0];i++)
		{
			if (VAR[i][0] < 'A') continue;
			for (j=0;j<8 && VAR[i][j];j++)
				putchar(VAR[i][j]);
			putchar('=');
			for (j=0;j<60 && ENVI[i][j];j++)
				putchar(ENVI[i][j]);
			if (ENVI[i][j]) printf("...");
			putchar('\n');
		}
		return 1;
	}

	if (strlen(var) == 0) return !printf("Err2");//如果var为空。
	strcpy(ch,var);
	j = 0;
	for(i=0;i <= VAR_MAX && VAR[i][0];i++)
	{
		if (memcmp(VAR[i],ch,8) == 0)
		{
			if (! flags) 
			{
				goto SETVAR;
			}
			memmove(env,ENVI[i],MAX_ENV_LEN);
			return 1;
		}
		if (!j && VAR[i][0] == '@') j = i;
	}
	if (flags || i > VAR_MAX)
		return 0;
	i = (j?j:i);
	memmove(VAR[i],ch,MAX_VAR_LEN);
	SETVAR:
	if (env[0] == 0)
	{
		VAR[i][0] = '@';
		memset(ENVI[i],0,MAX_ENV_LEN);
	}
	else
		memmove(ENVI[i],env,MAX_ENV_LEN);
	return 1;
};

static int read_val(char **str_ptr,unsigned long long *val)
{
      char *p;
      char *arg = *str_ptr;
      while (*arg == ' ' || *arg == '\t') arg++;
      p = arg;
      if (*arg == '*') arg++;
      
      if (! safe_parse_maxint_with_suffix (&arg, val, 0))
      {
	 return 0;
      }
      
      if (*p == '*')
      {
	 *val = *((unsigned long long *)(int)*val);
      }
      
      while (*arg == ' ' || *arg == '\t') arg++;
      *str_ptr = arg;
      return 1;
}

static unsigned long calc (char *arg)
{
   #ifdef DEBUG
   if (debug > 1)
       printf("calc_arg:%s\n",arg);
   #endif
   char envi[600] = "\0";;
   unsigned long long val1 = 0;
   unsigned long long val2 = 0;
   unsigned long long *p_result = &val1;
   char O;
   
   if (*arg == '*')
   {
      arg++;
      if (! safe_parse_maxint_with_suffix (&arg, &val1, 0))
      {
	 return 0;
      }
      p_result = (unsigned long long *)(int)val1;
   }
   else
   {
      if (!read_val(&arg, p_result))
      {
	 int i;
	 for (i=0;i<MAX_VAR_LEN;i++)
	 {
	    if ((*arg == '=') || (*arg < 48)) break;
	    envi[i] = *arg++;
	 }
	 envi[i] = 0;
	 errnum = 0;
      }
   }
   
   while (*arg == ' ') arg++;

   if ((arg[0] == arg[1]) && (arg[0] == '+' || arg[0] == '-'))
   {
      if (arg[0] == '+')
	 (*p_result)++;
      else
	 (*p_result)--;
      arg += 2;
      while (*arg == ' ') arg++;
   }

   if (*arg == '=')
   {
      arg++;
      if (! read_val(&arg, p_result))
	 return 0;
   }
   else 
   {
      envi[0] = 0;
      if (p_result != &val1)
      {
	 val1 = *p_result;
	 p_result = &val1;
      }
   }
   while (*arg)
   {
      val2 = 0ULL;
      O = *arg;
      arg++;

      if (O == '>' || O == '<')
      {
	 if (*arg != O)
		 return 0;
	 arg++;
      }
      
      if (! read_val(&arg, &val2))
	 return 0;

      switch(O)
      {
	 case '+':
		 *p_result += val2;
		 break;
	 case '-':
		 *p_result -= val2;
		 break;
	 case '*':
		 *p_result *= val2;
		 break;
	 case '/':
		 *(unsigned long *)p_result /= (unsigned long)val2;
		 break;
	 case '%':
		 *(unsigned long *)p_result %= (unsigned long)val2;
		 break;
	 case '&':
		 *p_result &= val2;
		 break;
	 case '|':
		 *p_result |= val2;
		 break;
	 case '^':
		 *p_result ^= val2;
		 break;
	 case '<':
		 *p_result <<= val2;
		 break;
	 case '>':
		 *p_result >>= val2;
		 break;
	 default:
		 return 0;
      }
   }
   
   if (debug > 0)
	  printf(" %ld (HEX:0x%lX)\n",*p_result,*p_result);
   
   if (envi[0] != 0)
   {
      sprintf(&envi[8],"%ld\0",*p_result);
      set_envi(envi,&envi[8]);
   }
   errnum = 0;
   return *p_result;
};

static int ascii_unicode(const char *ch,char *addr)
{
	int i = 0;
	while (*ch)
	{
		sprintf(&addr[i],"\\x%02X\\0",*ch++);
		i += 6;
	}
	sprintf(&addr[i],"\\0\\0\0");
#ifdef DEBUG
	sprintf(arg_new,"0x%X\0",i);
	set_envi ("_wenv_",arg_new);
#endif
	return 1;
}

static char *upper(char *string)
{
#if 0
	char *P=string;
	while (*P)
	{
		if (*P >= 'a' && *P <= 'z') *P &= 0xDF;//位5设为0就是大写
		P++;
	}
#else
   char *P=string;
   while (*P != '\0')
   {
      if ((unsigned char)(*P - 'a') < 26)
      {
	 *P &= 0xDF;
      }
      P++;
   }
#endif
	return string;
}

static char *lower(char *string)
{
#if 0
	char *P=string;
	while (*P)
	{
		if (*P >= 'A' && *P <= 'Z') *P |= 32;//位5设为1就是小写
		P++;
	}
#else
   char *P = string;
   while (*P != '\0')
   {
      if ((unsigned char)(*P - 'A') < 26)
      {
	 *P |= 32;
      }
      P++;
   }
#endif
	return string;
}

static int
grub_memcmp (const char *s1, const char *s2, int n)
{//如果n=0,则执行扩展比较也就是strcmp_ex
   int i = n;
   if (i == 0) i = strlen(s2);
   for (;i;*s1++,*s2++,i--)
   {
      if  (*s1 == * s2 || ((unsigned char)(*s1 - 'A') < 26 && *s1 | 32 == *s2))
      {
	 continue;
      }
      else return (*s1 > *s2)?1:-1;
   }

   if (n != 0 || (*s1 == '\0' || *s1 == ' ' || *s1 == '\t' || *s1 == '='))
      return 0;

   return 1;
}

/*
   static char *strstrn(const char *s1,const char *s2,const int n)
   功能：从字符串s1中寻找s2的位置（不比较结束符NULL).
   说明：如果没找到则返回NULL。n为0 时返回指向第一次出现s1位置的指针，否则返回最后一次出现的指针.
*/
static char *strstrn(const char *s1,const char *s2,const int n)
{
   if (n == 0)//n为0直接调用strstr返回第一次出现的指针,注:strstr 如果没有找到时会返回NULL.
   {
      return strstr(s1,s2);
   }
   else
   {
      char *p = NULL;
      char *s;
      int lens2 = strlen(s2);
      for (s=strstr(s1,s2);s != NULL;s=strstr(s,s2)) //在s1中查找s2直到最后一个
      {
	 p = s;
	 s += lens2;
      }
      return p;
   }
}

static char* next_line(char *arg)
{
	char *P=arg;
	
	while(*P++)
	{
		if (*P == '\r' || *P == '\n') 
		{
			*P++ = 0;
			while (*P == '\n' || *P == '\r' || *P == 0x20 || *P == '\t') P++;
			if (*P == ':') continue;
			if (*P) return P;
			break;
		}
	}
	return 0;
}
/*从指定文件中读取WENV的命令序列，并依次执行*/
static int read_file(char *arg)
{
	if (! open(arg)) return 0;
	
	if (filemax > 0x8200) return 0;/*限制文件大小不能超过32KB+512字节*/
	char *P,*P1;	
	P=(char *)0x600000;
	if (read((unsigned long long)(unsigned int)P,-1,GRUB_READ) != filemax) return 0;
	P[filemax] = 0;

	if ( *P == ':' )
		P1 = next_line (P);
	else
		P1 = P;

	while (P = next_line(P1))/*每次读取一行next_line会把行尾的回车符改成0截断，并返回下一行的起始地址*/
	{
		wenv_func(P1,0);
		P1 = P;
	}
	
	return wenv_func(P1,0);
}