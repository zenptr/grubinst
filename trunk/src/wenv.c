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
 2010-10-05 醉书生在svn版本9基础上修改
	变量名必须由非数字开头的字母、数字、下划线组成(不能以数字开头)
	变量名长度须完全匹配
		WENV set A12345678 非法，不会匹配  A1234567
		WENV set A1- 非法，不会匹配 A1
	set命令在debug未关闭时回显结果
	set命令变量名后必须有=，"WENV set TT" 将不会清掉TT
	
	reset命令复位__wenv
	reset命令扩展批量清除
		WENV reset ABC 等效于 WENV set ABC=
		WENV reset ABC* 清理所有 ABC 开头的变量
		
	帮助信息整理，增加了reset，子命令出错只显示该命令帮助
	set/calc/run/read命令无参数时，显示相应命令语法
 */
/*
 * to get this source code & binary: http://grub4dos-chenall.google.com
 * For more information.Please visit web-site at http://chenall.net/grub4dos_wenv/
 * 2010-06-20
 
 2010-10-08
   1.修正cal ++VARIABLE没有成功的BUG。
   2.修正比较符BUG。
   3.变量提取支持${VARIABLE:X:-Y}的形式，-Y代表提取到-Y个字符(即倒数Y个字符都不要).

 2010-10-07
   1.CALC支持自增/自减运算符++/--,和C语言的语法一样
   例子,注意执行的顺序.
   wenv calc a=b++ //结果a=b b=b+1
   wenv calc a=++b //结果b=b+1 a=b
 2010-10-06
   1.添加 >=,<= 比较符.
   2.支持按数字比较.
		如果要比较的第一个串是一个数字就按数字进行比较.
	3.数字比较时支持使用内存地址.*ADDR

 2010-10-05
   1.修正calc命令除法计算的bug.
   2.整合醉书生的修改版,并修复几个小bug.
   3.添加字符串比较功能. == <>
 2010-10-04
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

#define MAX_ARG_LEN 1024
#define MAX_VARS 64
#define MAX_VAR_LEN	8
#define MAX_ENV_LEN	512
#define MAX_BUFFER	(MAX_VARS * (MAX_VAR_LEN + MAX_ENV_LEN))

// 变量保存地址
#define BASE_ADDR 0x45000
// 变量名
#define VAR ((char (*)[MAX_VAR_LEN])BASE_ADDR)
// 变量值
#define ENVI ((char (*)[MAX_ENV_LEN])(BASE_ADDR + MAX_VARS * MAX_VAR_LEN))

// 缓冲
static char arg_new[MAX_ARG_LEN];
static int wenv_func(char *arg, int flags);		/* 功能入口 */
static int wenv_help_ex(int index);			/* 显示帮助 */
#define wenv_help()	wenv_help_ex(0)

// 字符串比较
static int grub_memcmp(const char *s1, const char *s2, int n);

/* 
arg比较来源字符串,string要对比的字符串.
功能:
参数比较(类似于strcmp，但是这里作了扩展)
1.arg可以比string长;使用空格' ',制表符'\t',等于号'=' 分隔参数;
2.如果string字符串全部是小写字符,则不区分大小写进行比较.
*/
#define strcmp_ex(arg,string) grub_memcmp(arg,string,0)
static char *strstrn(const char *s1, const char *s2, const int n);	// 查找子串

// 环境变量功能函数
static int envi_cmd(const char *var, char *env, int flags);
#define set_envi(var, val)			envi_cmd(var, val, 0)
#define get_env(var, val)			envi_cmd(var, val, 1)
#define get_env_all()				envi_cmd(NULL, NULL, 2)
#define reset_env_all()				envi_cmd(NULL, NULL, 3)

static int replace_str(char *in, char *out, int flags);	// 变量替换, 主要功能函数
static int read_val(char **str_ptr,unsigned long long *val);	// 读取一个数值(用于计算器)
static unsigned long calc(char *arg);			// 简单计算器模块, 参数是一个字符串

static int ascii_unicode(const char *ch, char *addr);	// unicode编码转换
static int read_file(char *arg);				// 读外部文件命令集
static char *lower(char * const string);			// 小写转换
static char *upper(char * const string);			// 大写转换
static char* next_line(char *arg);				// 读下一命令行
static void strcpyn(char *dest,const char *src,int n);	// 复制字符串，最多n个字符

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

int main()
{
	void *p = &main;
	char *arg = p - (*(int *)(p - 8));
	int flags = (*(int *)(p - 12));
	return wenv_func (arg , flags);
}

static int wenv_func(char *arg, int flags)
{
   if( ! *arg )
      return wenv_help();
   int i;
   char var[MAX_VAR_LEN+1] = "\0";
   char value[MAX_ENV_LEN + 1] = "\0";

   // 检查默认变量
   if( 0 != strcmp_ex(VAR[0], "__wenv") )
      reset_env_all();

   memset(value,0,MAX_ENV_LEN+1);

   if( 0 == strcmp_ex(arg, "read") )
   {
      arg = skip_to(0, arg);
      if(! *arg)
         return wenv_help_ex(5);
      return read_file(arg);
   }
   else if (strcmp_ex(arg, "set") == 0)
   {
      arg=skip_to(0, arg);
      if(! *arg || *arg == '=' || (unsigned char)(*arg - '0') < 9 )	//不能以数字开头或空
         return wenv_help_ex(1);


      for(i=0; i<MAX_VAR_LEN && arg[i]; ++i)
      {
         if( arg[i] == '=' || (arg[i] < '0') ) //无效变量名
              break;
         if (! (arg[i] == '_'
            || (arg[i] >= '0' && arg[i] <= '9')
            || (arg[i] >= 'a' && arg[i] <= 'z')
            || (arg[i] >= 'A' && arg[i] <= 'Z'))) //变量名只能使用字母,数字或下划线组成.
            {
               if( debug > 0 )
               {
                  printf(" VARIABLE illegally\n\n");
                  wenv_help_ex(1);
               }
               return 0;
            }
         var[i] = arg[i];
      }

      while(arg[i] == ' ') i++;//去掉空格

      if (arg[i] != '=') //变量后面不是'='非法,显示帮助信息并退出
         return wenv_help_ex(1);

      arg = skip_to(1, arg);
      if(! *arg) return set_envi(var, NULL); //值为空清除变量

      int ucase=0;
      if (*arg=='$')
      {
         if ((arg[1] | 32) == 'u')
         {
            ucase = 1;
            arg +=3 ;
            while (*arg == ',' || *arg == ' ' || *arg == '\t') arg++;
         }
         else if ((arg[1] | 32) == 'l')
         {
            ucase = 2;
            arg +=3 ;
            while (*arg == ',' || *arg == ' ' || *arg == '\t') arg++;
         }
      }

      //memset(value, 0, MAX_ENV_LEN);
      if (grub_memcmp(arg, "$input,", 7) == 0)
      {
         arg += 7;
         struct get_cmdline_arg get_cmdline_str;
         get_cmdline_str.prompt = arg;
         get_cmdline_str.maxlen = MAX_ENV_LEN;
         get_cmdline_str.cmdline = value;
         get_cmdline_str.echo_char = 0;
         get_cmdline_str.readline = 1;
         if (get_cmdline(get_cmdline_str))
            return 0;
      }
      else
      {
         if (!replace_str(arg,value,1))
            return 0;
      }

      if (ucase == 1)
         upper(value);
      else if (ucase == 2)
         lower(value);
         
      if( set_envi(var, value) )
      {
         return 1;
      }
      else
         return 0;
   }
   else if (strcmp_ex(arg, "get") == 0)
   {
      arg=skip_to(0,arg);
      if(! *arg)
         return get_env_all();
      for (i=0;i<MAX_VAR_LEN;i++)
      {
         if ((arg[i] == '=') || (arg[i] < '0')) break;
         var[i] = arg[i];
      }
      
      if (arg[i] == '*') //显示包含指定字符的变量
         return envi_cmd(var,NULL,2);
      if (arg[i] && arg[i] != '=' && arg[i] != ' ') return 0;//超长
      
      if(get_env(var, value))
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
            printf("%s=%s",var,value);
         return 1;
      }
      return 0;
   }
   else if(strcmp_ex(arg,"run") == 0)
   {
      //memset(arg_new, 0, MAX_ARG_LEN);
      arg = skip_to(0,arg);
      if(! *arg)
            return wenv_help_ex(6);
      if (!replace_str(arg,arg_new,1))
         return 0;
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
               sprintf(value,"0x%X\0",ret);
               set_envi ("__wenv",value);
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
         if (! (ret = builtin_cmd(cmd, arg, flags))) //优先使用内置的命令,如果不存在(旧版GRUB4DOS)就使用自带的
            ret = printf("%s\n",arg);
      }
      else
      {
         ret = builtin_cmd(cmd, arg, flags);
      }
      if (errnum) return 0;
      sprintf(value,"0x%X\0",ret);
      set_envi ("__wenv",value);
      return ret;
   }
   else if (strcmp_ex(arg, "calc") == 0)
   {
      arg = skip_to(0,arg);
      if(! *arg)
         return wenv_help_ex(4);
   //memset(arg_new, 0, MAX_ARG_LEN);
      replace_str(arg,arg_new,0);
      return calc(arg_new);
   }
   else if (strcmp_ex(arg, "reset") == 0)
   {
      arg = skip_to(0, arg);
      if(! *arg)
         return reset_env_all();
      for(i=0; i<MAX_VAR_LEN; ++i)
      {
         //if ((arg[i] == '*') || (arg[i] < 48)) //不需要判断,因为'*'本来就小于48
         if (arg[i] < 48)
            break;
         var[i] = arg[i];
      }

      if (MAX_VAR_LEN==i && arg[i] != ' ' && arg[i] != 0)//输入的变量名超过8个退出.
      {
         if(debug>0)
            printf(" VARIABLE illegally\n\n");
         return 0;
      }
      
      if (arg[i] == '*') //按前缀清除
      {
         int j, count=0;
         for(j=1; i < MAX_VARS && VAR[j][0] != '\0'; j++)
         {
            if (VAR[j][0] == '@') //已经删除的
               continue;
            if (memcmp(VAR[j], var, i) == 0)//匹配成功作删除标记
            {
               VAR[j][0] = '@';
			   ++count;
            }
         }
		 printf("\treset counts = %d", count);
      }
      else
      {
         return set_envi(var, NULL);
      }
   }
   else/* if (strcmp_ex(arg, "if") == 0)*/
   {
		if (! replace_str(arg, value, 0))
			return 0;
		unsigned int op=-1; // 0:==, 1:<>, 2:>=, 3:<=, 判断优先级从小到大
      char *p1,*p2;
      p1 = value;

      if (strstr(value,"${") != NULL) //二次替换
      {
			replace_str(p1, arg_new, 0);
			p1 = arg_new;
		}
		p2 = p1;
		while(*p2)
		{
			if (*p2 == '\"')
			{
				while (*++p2 && *p2 != '\"');
				if (*p2 == '\"')
					p2++;
				else
					break;
			}
			switch(*(unsigned short*)p2)
			{
				case 0x3D3D://==
					op = 0;
					break;
				case 0x3E3C://<>
					op = 1;
					break;
				case 0x3D3E://>=
					op = 2;
					break;
				case 0x3D3C://<=
					op = 3;
					break;
				default:
					p2++;
					continue;
			}
			break;
		}
		if (op == -1)
		{
			errnum = ERR_BAD_ARGUMENT;
			return wenv_help_ex(7);
		}
		#if 0
		if( NULL != (p2=strstr(p1,"==") ) )
			op =0;
		else if( NULL != (p2=strstr(p1,"<>") ) )
			op =1;
		else if( NULL != (p2=strstr(p1,">=") ) )
			op =2;
		else if( NULL != (p2=strstr(p1,"<=") ) )
			op =3;
		else
			return wenv_help_ex(7);
		#endif
		for (i = 1;*(p2-i) == ' ';i++)	// 滤掉操作符前面的空格
		{
			;
		}
		*(p2-i+1) = '\0';//截断
		i = p2 - p1 - i;
		p2++;

		while(*++p2 == ' ')	// 滤掉操作符后面的空格
		{
			;
		}
		
		long long v1 = 0LL,v2=0LL;
		if ( !read_val(&p1,&v1) || !read_val(&p2,&v2) ) // 读数字失败就按字符串比较
		{
			v1 = 0LL;
			v2 = grub_memcmp (p2, p1, 0);
		}
		errnum = 0; //清除错误信息
		switch(op)
		{
			case 0:
				return (v1 == v2);
			case 1:
				return (v1 != v2);
			case 2:
				return (v1 >= v2);
			case 3:
				return (v1 <= v2);
		}
   }
   return 0;
}

static int wenv_help_ex(int index)
{
	switch(index)
	{
		case 0:	// information
			printf(" WENV Using variables in GRUB4DOS, Compiled time: %s %s\n", __DATE__, __TIME__);
		case 1: // set
			printf("\n WENV set VARIABLE=[$[U|L],] [$input,] STRING\n");
			printf("\tVARIABLE name is made of characters \"_/A-Z/a-z/0-9\"\n");
			printf("\tmax length=8, and the first character is not 0-9\n");
			if(index>0) break;
		case 2:	// get
			printf(" WENV get [VARIABLE] | [PREFIX[*]]\n");
			if(index>0) break;
		case 3:	// reset
			printf(" WENV reset [VARIABLE] | [PREFIX[*]]\n");
			if(index>0) break;
		case 4:	// calc
			printf(" WENV calc [[VARIABLE | [*]INTEGER]=] [*]INTEGER OPERATOR [[*]INTEGER]\n");
			if(index>0) break;
		case 5:	// read
			printf(" WENV read FILE\n");
			if(index>0) break;
		case 6:	// run
			printf(" WENV run COMMAND\n\tthe COMMAND can use VARIABLE with ${VARIABLE} or *ADDR\n");
			if(index>0) break;
		case 7: // 比较
			printf("\n WENV <string|VARIABLE|[*]INTEGER> OPERATOR <string|VARIABLE|[*]INTEGER>\n");
			printf("\tOPERATOR support ==,<>,>=,<=\n");
			if(index>0) break;
		default:
			printf("\nFor more information:  http://chenall.net/tag/grub4dos\n");
			printf("to get source and bin: http://grub4dos-chenall.googlecode.com\n");
	}
	return 0;
}

static int replace_str(char *in, char *out, int flags)
{
   if (out == arg_new)
   {
      memset(out, 0, MAX_ARG_LEN);
   }
   char *po = out;
   char *p = in;
   int i;
   unsigned long long addr;
   char ch[MAX_VAR_LEN + 1]="\0";
   char value[MAX_ENV_LEN + 1];

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
            long long len = 0xFFFFFFFFULL;//提取字符串长度
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
		     break;//不符合要求,则不处理.如果后面刚好是一个"}"则相当于过滤掉了":"
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

		  //if (*p == '*') p++;
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
		  //if (*(p-1) == '*') *(p-1) = '\0'; //截断
		  break;
	       }
               else if (*p < '0') //非法字符
               {
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
            
            memset(value,0,MAX_ENV_LEN+1);
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
		     len = 0xFFFFFFFF;
		  }
	       }
	       else
	       {
		  int str_len = strlen(value);
		  if (start < 0LL) start += str_len;//确定起始位置
		  if (len < 0LL) len += str_len - start;//确定结束位置
		  p = value + start;
	       }
               while (*p && len >0)
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
}

static void strcpyn(char *dest,const char *src,int n)
{
#ifdef DEBUG
	if( NULL == dest || NULL == src || n <= 0
		|| dest >= src && (int)(dest-src) < n
		|| dest <  src && (int)(src-dest) < n )
	{
		printf("strcpyn: bad parameter\n");
		return;
	}
#endif
   while(*src && n--)
   {
      *dest++ = *src++;
   }
}

static int envi_cmd(const char *var,char *env,int flags)
{
#ifdef DEBUG
	if( flags < 0 || flags > 3 )
	{
		printf("envi_cmd: bad parameter\n");
		return 0;
	}
#endif
   int i,j;
   //	char ch[MAX_VAR_LEN]="\0";

   if(flags == 3)//重置
   {
      #ifdef DEBUG
      memset((char *)BASE_ADDR,0,MAX_VARS * MAX_VAR_LEN);
      sprintf(VAR[0],"__wenv");
      #else
      memset( (char *)BASE_ADDR, 0, MAX_BUFFER );
      sprintf(VAR[0], "__wenv");
      #endif
      return 1;
   }
   if (flags == 2)//显示所有变量信息或包含指定字符的变量
   {
		int count=0;
      for(i=1; i < MAX_VARS && VAR[i][0]; ++i)//因为第一个固定 __wenv,所以跳过第一个的显示
      {
         if (VAR[i][0] < 'A')
            continue;//非法或已删除变量.
         if (var != NULL)//显示包含特定字符的变量
         {
            for (j=0;j<MAX_VAR_LEN && var[j] != '\0';j++)
            {
               if (VAR[i][j] != var[j])
               {
                  j = -1;
                  break;
               }
            }
            if (j == -1) continue; //不匹配退出
         }
		   ++count;
         for (j=0;j<MAX_VAR_LEN && VAR[i][j];j++)//不能直接PRINTF因为我们的变量索引表之间没有分隔符
                 putchar(VAR[i][j]);
         putchar('=');
         for (j=0;j<60 && ENVI[i][j];j++)//同样的没有分隔,不能直接PRINTF
                 putchar(ENVI[i][j]);
         if (ENVI[i][j]) printf("...");//如果要显示的字符太长了只显示60个字符.
         putchar('\n');
      }
		printf("\tcounts = %d", count);
		if( NULL == var)
			printf("    max = %d", MAX_VARS-1);
      return 1;
   }

   char ch[MAX_VAR_LEN +1] = "\0"; //使用ch中转可以保证字符截取正确.
   strcpyn(ch,var,MAX_VAR_LEN);
   j = 0;
   /*遍历变量名表
   注: 添加 VAR[i][0] 判断,还有使用删除标记符"@"
      使用得我们不需要遍历整个表.
   */
   for(i=0;i < MAX_VARS && VAR[i][0];i++)
   {
      if (memcmp(VAR[i], ch, MAX_VAR_LEN) == 0)//找到了
      {
         j = i;//如果完全匹配设置j = 1并退出循环,这时j会等于i,其它情况都不可能相等.
         break;
      }

      if (!j && VAR[i][0] == '@') j = i;//记录一个可设变量的位置,如果是添加新的变量时使用.
   }
   
   //读取
   if (flags == 1)
   {//有找到匹配的变量时正常返回,否则返回0
      if (j == i)
      {
         memmove(env,ENVI[i],MAX_ENV_LEN);
         return env[0];
      }
      return 0;
   }//如果flags = 1到这里就结束了,直接返回,后面不以不需要再判断.

   if (j == 0 && i >= MAX_VARS) //没有可用空间也没有找到匹配的记录.
	{
		if( debug > 0 ) // 这里最好给用户一个提示
			printf(" variables max=%d", MAX_VARS-1);
      return 0;
   }
	if (env == NULL || env[0] == '\0') //env为空,删除变量
   {
		if(j == i)//有找到匹配的记录
      {
         VAR[i][0] = '@';//只简单设置删除标记,不做其它操作
         return 1;//直接返回
         //memset(ENVI[i],0,MAX_ENV_LEN);//清除变量的值.
      }
		return 1; // 变量不存在
   }
   
   if (j != i) //添加新的变量
   {
      i = (j?j:i);//优先使用已删除的空间.
      memmove(VAR[i] ,ch ,MAX_VAR_LEN);//添加变量名
   }
   //添加或修改
   memmove(ENVI[i],env,MAX_ENV_LEN);
   return 1;
}

static int read_val(char **str_ptr,unsigned long long *val)
{
      while (**str_ptr == ' ' || **str_ptr == '\t') (*str_ptr)++;
      char *arg = *str_ptr;
      unsigned long long *val_ptr = val;
      char *str_ptr1 = NULL;
		char *p = arg;
      if (arg[0] == arg[1] && (arg[0] == '+' || arg[0] == '-'))
      {
         str_ptr1 = arg;
         arg += 2;
      }

      if (*arg == '*')
      {
			p = arg;
			arg++;
		}
      
      if (! safe_parse_maxint_with_suffix (&arg, val, 0))
      {
	 return 0;
      }
      
      if (*p == '*')
      {
         val_ptr = (unsigned long long *)(int)*val;
         *val = *val_ptr;
         if (str_ptr1 == NULL)
         {
            str_ptr1 = arg;
         }
         if (str_ptr1[0] == str_ptr1[1])
         {
            switch(str_ptr1[0])
            {
               case '+':
                  (*val_ptr)++;
                  str_ptr1 += 2;
                  break;
               case '-':
                  (*val_ptr)--;
                  str_ptr1 += 2;
                  break;
               default:
                  break;
            }
         }
          if (str_ptr1 < arg)//如果是前面的++/--
            *val = *val_ptr;
          else
            arg = str_ptr1;
      }
      while (*arg == ' ') arg++;
      *str_ptr = arg;
      return (int)val_ptr;
}

static int read_envi_val(char **str_ptr,char *envi,unsigned long long *val)
{
   int i;
   char *p = *str_ptr;
   char *p1 = &envi[8];
   char tmp = '\0';
   long long val_tmp = 0LL;
   memset(envi,0,MAX_VAR_LEN + MAX_ENV_LEN + 1);
   while (*p == ' ') p++;
   if (p[0] == p[1] && (p[0] == '+' || p[0] == '-'))
   {
      if (*p == '+')
         val_tmp = 1LL;
      else
         val_tmp = -1LL;
      p += 2;
   }

   if (*p == 0 || *p == '=' || (unsigned char)(*p - '0') < 9)
      return 0;
   for (i=0;i<MAX_VAR_LEN;i++)
   {
      if ((*p == '=') || (*p < 48)) break;
      envi[i] = *p++;
   }
   
   p1 = &envi[8];
   get_env(envi,p1);
   read_val(&p1,val);

   if (val_tmp != 0LL) //计算++X
   {
      *val += val_tmp;
      val_tmp = *val;
      sprintf(p1,"%ld\0",val_tmp);
      set_envi(envi,p1);
   }
   else if (p[0] == p[1] && (p[0] == '+' || p[0] == '-')) //计算x++
   {
      val_tmp = *val;
      if (*p == '+')
         val_tmp++;
      else
         val_tmp--;
      p += 2;
      sprintf(p1,"%ld\0",val_tmp);
      set_envi(envi,p1);
   }
   *str_ptr = p;
   return 1;
}

static unsigned long calc (char *arg)
{
   #ifdef DEBUG
   if (debug > 1)
       printf("calc_arg:%s\n",arg);
   #endif
   char var[MAX_VAR_LEN + 1] = "\0";
   char envi[MAX_VAR_LEN + MAX_ENV_LEN + 1] = "\0";
   unsigned long long val1 = 0LL;
   unsigned long long val2 = 0LL;
   unsigned long long *p_result = &val1;
   unsigned long long var_val = 0LL;
//   char *p;
   char O;
   
   if (*arg == '*')
   {
      if ((p_result = (unsigned long long*)read_val(&arg, &val1)) == 0)
         return 0;
   }
   else
   {
      if (!read_val(&arg, &val1) && read_envi_val(&arg,envi,&val1) == 0)
      {
         return 0;
      }
   }
   while (*arg == ' ') arg++;

   if (*arg == '=')
   {
      arg++;
      strcpyn(var,envi,8);
      if (! read_val(&arg, &val1) && read_envi_val(&arg,envi,&val1) == 0)
	 return 0;
   }
   else 
   {
      envi[0] = 0;
      p_result = &val1;
   }
   for(;;)
   {
      while (*arg == ' ') arg++;//过滤空格
      if (*arg < 37) break;//非法字符
      val2 = 0ULL;
      O = *arg;
      arg++;

      if (O == '>' || O == '<')
      {
	 if (*arg != O)
		 return 0;
	 arg++;
      }
      
      if (read_val(&arg, &val2) == 0 && read_envi_val(&arg,envi,&val2) == 0)
	 return 0;

      switch(O)
      {
	 case '+':
		 val1 += val2;
		 break;
	 case '-':
		 val1 -= val2;
		 break;
	 case '*':
		 val1 *= val2;
		 break;
	 case '/':
		 val1 = ((long)val1 / (long)val2);
		 break;
	 case '%':
		 val1 = ((long)val1 % (long)val2);
		 break;
	 case '&':
		 val1 &= val2;
		 break;
	 case '|':
		 val1 |= val2;
		 break;
	 case '^':
		 val1 ^= val2;
		 break;
	 case '<':
		 val1 <<= val2;
		 break;
	 case '>':
		 val1 >>= val2;
		 break;
	 default:
		 return 0;
      }
   }
   if (p_result != &val1)
      *p_result = val1;
   if (debug > 0)
	  printf(" %ld (HEX:0x%lX)\n",*p_result,*p_result);
   
   if (var[0] != 0)
   {
      sprintf(&envi[8],"%ld\0",*p_result);
      set_envi(var,&envi[8]);
   }
   errnum = 0;
   return *p_result;
}

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

// 小写转大写
static char *upper(char * const string)
{
   char *P=string;
   while (*P)
   {
      if ((unsigned char)(*P - 'a') < 26) //字符如果在'a'-'z'之间
      {
         *P &= 0xDF;//0XDF=11011111B 把第5位设为0就是大写,好像用位操作可以加快运算速度.
      }
      P++;
   }
   return string;
}

static char *lower(char * const string)
{
   char *P=string;
   while (*P)
   {
      if ((unsigned char)(*P - 'A') < 26)
      {
         *P |= 32;//32 = 00100000B,使用位操作只需要把第5位设为1就是小写了.
      }
      P++;
   }
   return string;
}

/* 比较字符串前n个字符(s1不分大小写), 相等返回0
 * n>0 时，仅比较前n具字符
 * n=0 时，s1须以结束符、空格、制表符、等号分隔才会返回相等的结果
 */
static int grub_memcmp(const char *s1, const char *s2, int n)
{
	if( s1 == s2 )
		return 0;
	// 基础函数严谨一点比较好
#ifdef DEBUG
	if( NULL == s1 || NULL == s2 || n <= 0
		|| s1 > s2 && (int)(s1-s2) < n
		|| s1 < s2 && (int)(s2-s1) < n )
	{
		printf("grub_memcmp: bad parameter\n");
		return;
	}
#endif
   int len = (n>0) ? n : strlen(s2);

   while(len--)
   {
      if  (*s1 == * s2 || ((unsigned char)(*s1 - 'A') < 26 && (*s1 | 32) == *s2))
      {
         s1++;
         s2++;
         continue;
      }
      else return (*s1 > *s2)?1:-1;
   }

      // 前面len字节已匹配，如n=0作额外的结束符检查
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
#ifdef DEBUG
	if( NULL == s1 || NULL == s2 || '\0' == *s1 || '\0' == *s2 )
   {
		printf("strstrn: bad parameters\n");
		return NULL;
   }
#endif

	int len1 = strlen(s1);
	int len2 = strlen(s2);
	if( len1 < len2 )
		return NULL;
	char *s = strstr(s1,s2);
	if( 0 == n )
		return s;
	const char *p = s1 + len1;
	do
   {
		p -= len2;
		if(p < s1)
			p = s1;
	} while( NULL == (s=(strstr(p, s2))) && p > s1 );
	return s;
}

static char* next_line(char *arg)
{
   char *P=arg;

   while(*P++)
   {
      if(*P == '\r' || *P == '\n')
      {
         *P++ = 0;
         while(*P == '\n' || *P == '\r' || *P == ' ' || *P == '\t')
            P++;

         if(*P == ':')
            continue;

         if(*P)
            return P;

         break;
      }
   }
   return 0;
}
/*从指定文件中读取WENV的命令序列，并依次执行*/
static int read_file(char *arg)
{
   if(! open(arg))
      return 0;

   if(filemax > 32*1024+512)	// 限制文件大小不能超过32KB+512字节
      return 0;

   char *P=(char *)0x600000;	// 6M
   char *P1;

   if (read((unsigned long long)(unsigned int)P,-1,GRUB_READ) != filemax)
      return 0;

   P[filemax] = 0;

   if( *P == ':' )
      P1 = next_line(P);
   else
      P1 = P;

   while (P = next_line(P1))	// 每次读取一行next_line会把行尾的回车符改成0截断，并返回下一行的起始地址
   {
      wenv_func(P1, 0);
      P1 = P;
   }

   return wenv_func(P1, 0);
}