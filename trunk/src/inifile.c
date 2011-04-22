#include "grub4dos.h"
static char *find_section(char *section,char *file);
static void skip_eol(char *line);
int GRUB = 0x42555247;/* this is needed, see the following comment. */
/* gcc treat the following as data only if a global initialization like the
 * above line occurs.
 */

asm(".long 0x534F4434");
asm(ASM_BUILD_DATE);
/* a valid executable file for grub4dos must end with these 8 bytes */
asm(".long 0x03051805");
asm(".long 0xBCBAA7BA");

/* thank goodness gcc will place the above 8 bytes at the end of the b.out
 * file. Do not insert any other asm lines here.
 */

static int main (char *arg,int flags)
{
	char *filename=arg;
	char *section;
	char *item;
	char *file_buf;
	char *value;
	section = skip_to(0x200,arg);
	if (*section != '[')
		return 0;
	item = section;
	while (*++item != ']')
		;
	item = skip_to(0x200,item);
	value = skip_to(0x201,item);
	if (section[1] == ']')
		section =NULL;
	if (!open(filename))
		return 0;
	if ((file_buf=malloc(filemax))==NULL)
	{
		close();
		return 0;
	}
	read((unsigned long long)(int)file_buf,-1,GRUB_READ);
	file_buf[filemax] = '\0';
	char *tmp = section;
	int ret = 0;
	if ((section = find_section(section,file_buf)))
	{
		if (substring(item,"/remove",1) == 0)
		{
			filepos = section - file_buf;
			ret = read((unsigned long long)(unsigned int)"[;",2,GRUB_WRITE);
		}
		else
		{
			char *p;
			if (tmp)
				section = skip_to(0x100 | ';',section);
			while ((tmp=section) && *tmp != '[')
			{
				section = skip_to(0x100 | ';',tmp);
				p = skip_to(0x201,tmp);
				skip_eol(p);
				if (*item == '\0')
				{
					printf("%s=%s\n",tmp,p);
					++ret;
				}
				else if (substring(item,tmp,1) == 0)
				{
					if (*value)
					{
						int len = sprintf(file_buf,"%s=%s\n;",tmp,value);
						if (len > (section - tmp - 2))
						{
							len = section - tmp - 2;
						}
						filepos = tmp - file_buf;
						read((unsigned long long)(unsigned int)file_buf,len,GRUB_WRITE);
					}
					else
					{
						printf("%s\n",p);
					}
					++ret;
					break;
				}
			}
		}
	}
	free(file_buf);
	close();
	return ret;
}

static void skip_eol(char *line)
{
	if (line == NULL)
		return;
	while (*line)
	{
		if (*line == ';')
			break;
		if (*line == '\"')
		{
			while (*++line && *line != '\"')
				;
		}
		if (*line)
			++line;
	}
	if (*line != ';')
		return;
	while (isspace(*--line))
	{
		;
	}
	line[1]='\0';
	return;
}

static char *find_section(char *section,char *file)
{
	if (section == NULL)
		return (*file == ';')?skip_to(0x100 | ';',file):file;
	while (file)
	{
		if (*file == '[' && substring(section,file,1) != 1)
		{
			return file;
		}
		file = skip_to(0x100 | ';',file);
	}
	return NULL;
}