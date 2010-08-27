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

gcc -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE g4d_off.c

 * disassemble:			objdump -d a.out 
 * confirm no relocation:	readelf -r a.out
 * generate executable:		objcopy -O binary a.out g4d_off.mod
 *
 * and then the resultant b.out will be grub4dos executable.
 */


#define		U_8				unsigned char
#define		U_16			unsigned short
#define		U_32			unsigned long
#define		U_64			unsigned long long
#define		NULL			((void*)0)
#define		INTERRUPTOFF()	asm ("cli")		

#define		BUFFER_START	((char *) & table_buffer[0])
#define		ACPI_SDTH		system_description_table_header
#define		ACPI_FLG		(pm1_ctr_tpy.pm1_cover.acpi_flg)
#define		PM1_CTR			(pm1_ctr_tpy.pm1_cover.pm1_top)
#define		PM1A			(pm1_ctr_tpy.pm1_cover.cover_reserved_1)
#define		PM1B			(pm1_ctr_tpy.pm1_cover.cover_reserved_2)
#define		PM1A_BIT_BLK	(pm1_ctr_tpy.pm1_ctr_reg_group.pm1a_blk)
#define		PM1B_BIT_BLK	(pm1_ctr_tpy.pm1_ctr_reg_group.pm1b_blk)

#define		G4D_ARG(x)		(memcmp((void*)&main - *(U_32*)(&main - 8), x, strlen(x)) == 0 ? 1 : 0)
#define 	TABLE_CHECK32(addr,signature) 	(header_check32(((void*)(addr)), (signature)) \
											&& check_sum(((void*)(addr)),(((struct ACPI_SDTH*)(addr))->length)))
											
#define		reg_inw(port, val) 	__asm__ __volatile__ ("inw %%dx, %%ax" : "=a" (val) : "d"(port))
#define		reg_outw(port, val) __asm__ __volatile__ ("outw %%ax, %%dx" : :"a"(val), "d"(port))
#define		reg_outb(port, val)	__asm__ __volatile__ ("outb %%al, %%dx" : :"a"(val), "d"(port))

#define		WRITE_REG_WORD(port,val)	do {if ((port) <= 0xffff) reg_outw(port,val);\
										 else *(U_16*)(port) = (U_16)(val);} while(0)
#define		READ_REG_WORD(port,val)		do {if ((port) <= 0xffff) reg_inw(port, val);\
										 else (val) = *(U_16*)(port);} while(0)
#define		WRITE_REG_BYTE(port,val)	do {if ((port) <= 0xffff) reg_outb(port,val);\
										 else *(U_16*)(port) = (U_8)(val);} while(0)
/*follows is grub4dos internal function*/										
#define 	strlen 		((int (*)(const char *))((*(int **)0x8300)[12]))
#define		memcmp 		((int (*)(const char *, const char *, int))((*(int **)0x8300)[22]))
#define 	cls 		((void (*)(void))((*(int **)0x8300)[6]))
#define 	putchar 	((void (*)(int))((*(int **)0x8300)[2]))
#define 	sprintf 	((int (*)(char *, const char *, ...))((*(int **)0x8300)[0]))
#define 	printf(...) sprintf(NULL, __VA_ARGS__)
#define 	mem64 		((int (*)(int, unsigned long long, unsigned long long,\
							unsigned long long))((*(int **)0x8300)[25]))
						

struct rsdp_revision_0
{
	unsigned char		signture[8] ; 
	unsigned char		checksum ; 
	unsigned char		oemid[6]; 
	unsigned char		revision ; 
	unsigned long		rsdtaddress ; 
} __attribute__ ((packed)) ; 


struct rsdp_table	/* acpi revision 2 above */
{
	unsigned char		signture[8] ; 
	unsigned char		checksum; 
	unsigned char		oemid[6]; 
	unsigned char		revision; 
	unsigned long		rsdtaddress; 
	unsigned long		length; 
	unsigned long		xsdtaddress; 
	unsigned long long	extended_checksum; 
	unsigned char		reservd[3]; 
} __attribute__ ((packed)); 


struct system_description_table_header
{
	unsigned char		signature[4]; 
	unsigned long		length; 
	unsigned char		revision; 
	unsigned char		checksum; 
	unsigned char		oemid[6]; 
	unsigned char		oem_table_id[8]; 
	unsigned char		oem_revison[4]; 
	unsigned char		creator_id[4]; 
	unsigned char		creator_revision[4]; 
} __attribute__ ((packed)); 


struct simplify_fadt_table 	/* refer to acpi 4.0 sepction */
{
	struct system_description_table_header	fadt_header;
	unsigned char		ignor_1[4]; 
	unsigned long		dsdt; 
	unsigned char		ignore_2[4]; 
	unsigned long		smi_cmd; 
	unsigned char		acpi_enable; 
	unsigned char		acpi_disable; 
	unsigned char		ignore_3[10]; 
	unsigned long		pm1a_cnt_blk; 
	unsigned long		pm1b_cnt_blk; 
	unsigned char		ignore_4[17]; 
	unsigned char		pm1_cnt_len; 
	unsigned char		ignore_5[50]; 
	unsigned long long	x_dsdt;
	unsigned char		ignore_6[24];
	unsigned char		x_pm1a_cnt_blk[12]; 
	unsigned char		x_pm1b_cnt_blk [12];
	unsigned char		ignore_7 [48];
} __attribute__ ((packed)); 


struct pm1_control_registers
{
	unsigned short		sci_en : 1; 
	unsigned short		bm_rld : 1; 
	unsigned short		gbl_rls : 1; 
	unsigned short		reserved_1 :6; 
	unsigned short		igneore : 1; 
	unsigned short		slp_typx : 3; 
	unsigned short		slp_en : 1; 
	unsigned short		reserved_2 : 2; 
} __attribute__ ((packed)); 


static union 
{
	struct {
		struct	pm1_control_registers	pm1a_blk; 
		struct	pm1_control_registers	pm1b_blk; 
	} __attribute__ ((packed)) pm1_ctr_reg_group; 
	
	struct {
		unsigned short	cover_reserved_1; 
		unsigned short	cover_reserved_2; 	
		unsigned short	pm1_top;
		unsigned short	acpi_flg;
	}__attribute__ ((packed)) pm1_cover;
	
} __attribute__ ((packed)) volatile pm1_ctr_tpy; 
	
static char		table_buffer[0x200000];
static struct	rsdp_table *						rsdp_addr = NULL; 
static struct 	system_description_table_header *	rsdt_addr = NULL; 
static struct	system_description_table_header *	xsdt_addr = NULL; 
static struct	system_description_table_header *	dsdt_addr = NULL; 
static struct	simplify_fadt_table *				buffer_fadt = NULL; 

static void *	get_rsdp 		(void);
static void * 	get_fadt		(void); 
static void *	get_dsdt 		(void);
static int		get_s5_sly 		( struct ACPI_SDTH* dsdt );
static void 	header_print 	(void * entry);
static void *	scan_entry 		(unsigned long long parent_table, const char parent_signature[],
									const char entry_signature[], unsigned long address_wide); 
static void * 	table_move_64 	( void * dest_addr, unsigned long long table_addr, 
									const char signature[] ); 
static inline void	translate_acpi		(int en_or_disable); 
static inline int   check_sum			( void *entry, unsigned long length );
static inline int   header_check32		( void *entry, const char signature[] );
static inline void  * scan_rsdp			( void *entry, unsigned long cx ); 
static inline void  write_pm1_reg_32	(struct simplify_fadt_table* fadt_32);
static inline void  write_pm1_reg_32	(struct simplify_fadt_table* fadt_32);
static inline void  write_smi_reg		(int set_acpi_state);



/*  
	a valid executable file/mode for grub4dos must end with these 8 bytes
	gcc treat the following lines as data and only it follow  global initialization which like above .
*/
asm(".long 0x03051805");
asm(".long 0xBCBAA7BA");


int main (void)
{
	cls;
	printf("programe runs in %x \nnote : 64_bit table FADT will moved to buffer %x, and DSDT moved to %x\n\n",&main, 			BUFFER_START, BUFFER_START + sizeof(struct simplify_fadt_table));
	if (! get_rsdp()) goto error;
	if (! get_fadt()) goto error;
	if (! get_dsdt()) goto error;
	if (! get_s5_sly(dsdt_addr)) goto error;
	if (! G4D_ARG("--display")) {
		INTERRUPTOFF();
		write_pm1_reg_32(buffer_fadt);
	}
error:
	translate_acpi(0);
	if (! G4D_ARG("--display"))
	printf("please run grub4dos halt command");
	return 0;
}


static void * get_rsdp ( void )
{
	void * p;
	
	if (rsdp_addr = scan_rsdp((void*)0xf0000, (0xfffff-0xf0000)/16)) goto found ; 
	if (rsdp_addr = scan_rsdp((void*)0xe0000, (0xeffff-0xe0000)/16)) goto found ; 
	p = (void*) ((U_32)*(U_16*)0x40e << 4) ; 
	if (p > (void*)0x90000
		&& ( rsdp_addr = scan_rsdp(p, 1024/16) ))
		goto found ; 
	else
		return rsdp_addr = NULL; 
found:
	printf("rsdp address at : 0x%x, revision is %x\n\n",rsdp_addr, rsdp_addr->revision);
	return rsdp_addr ; 
}


static inline void * scan_rsdp ( void *entry, unsigned long cx )
{
	for (; cx != 0 ; cx --, entry += 16) {
		if (memcmp(entry, "RSD PTR ", 8) != 0) continue ; 
		if ((((struct rsdp_table*)entry)->revision == 0) 
				? check_sum(entry, sizeof(struct rsdp_revision_0))
				: check_sum(entry, ((struct rsdp_table*)entry)->length))
			return entry ; 
	}
	return NULL; 
}


static inline int check_sum ( void *entry, unsigned long length )
{
	int		sum = 0; 
	char *	p = entry;
	
	for (; p < (char*)entry + length ; p++)
		sum = ( *p + sum ) & 0xff ; 
	if (sum) {
		printf("check sum error :\n");
		header_print(entry);
		return 0 ;
	}
	else 
		return 1; 
}


static inline int header_check32 ( void *entry, const char signature[] )
{	
	if (memcmp(entry, signature, strlen(signature) ) == 0 
		&& ((struct ACPI_SDTH*)entry)->revision 
		&& ((struct ACPI_SDTH*)entry)->length >= sizeof(struct ACPI_SDTH))
		return 1 ; 
	else {
			printf("header check info :\n");
			header_print(entry);
			return 0 ;
	}
}


static void * get_fadt (void)
{	
	if (! rsdp_addr) {
		printf("acpi entry : rsdp not found\n\n");
		goto error;
	}
	if (TABLE_CHECK32(rsdp_addr->rsdtaddress, "RSDT")) {		
		header_print(rsdt_addr = (struct ACPI_SDTH*)rsdp_addr->rsdtaddress); 
		buffer_fadt = (struct simplify_fadt_table*) scan_entry(rsdp_addr->rsdtaddress, "RSDT", "FACP", 4) ; 
	}		
	if (! buffer_fadt && rsdp_addr->revision )
		buffer_fadt = (struct simplify_fadt_table*) scan_entry(rsdp_addr->xsdtaddress , "XSDT", "FACP", 8); 
	if (buffer_fadt)
		return buffer_fadt ;
	else
error:
		return buffer_fadt = NULL;
}


static void * scan_entry ( unsigned long long parent_table, const char parent_signature[],
							const char entry_signature[], unsigned long address_wide )
{
	struct ACPI_SDTH * parent_start; 
	struct ACPI_SDTH * parent_end;
	struct ACPI_SDTH * entry_start; 
	
	if (! parent_table) {
		ACPI_FLG = 0; 
		goto error;
	}
	if (parent_table > 0xffffffff) {
		if (! (parent_start = table_move_64(BUFFER_START, parent_table, parent_signature)))
			goto error ;
	} else {
		parent_start = (void *)(U_32)parent_table; 
	}
	parent_end = (void*)parent_start + parent_start->length;
	for (entry_start =  (void*)parent_end - address_wide;
		(void*)entry_start >= (void*)parent_start + sizeof(struct ACPI_SDTH);
		entry_start = (void *)entry_start - address_wide) {
		if (address_wide == 8 && *((U_32*)(entry_start) + 1)) {			
			if (table_move_64((void *)parent_end, *(U_64*)entry_start, entry_signature)) {
				header_print(parent_end);
				ACPI_FLG = 64;
				return  parent_end;
			}
			else continue ;
		} else {
			if (TABLE_CHECK32((*(U_32*)entry_start), entry_signature)) goto found ;
			else continue ;
		}
	}
error:
	printf ("%s search is over,\tflag is %d,\taddress_wide is %d,\n but %s entry not found\n", 
			parent_signature, ACPI_FLG, address_wide, entry_signature);
	ACPI_FLG = 0;
	return NULL ;
found:
	header_print((void*)*(U_32*)entry_start);
	ACPI_FLG = 32;
	return (void*)(*(U_32*)entry_start);

}


static void * table_move_64 ( void * dest_addr, unsigned long long table_addr, const char signature[] )
{	
	mem64(1, (U_32)dest_addr, table_addr, sizeof(struct ACPI_SDTH)); 
	if (header_check32(dest_addr, signature )) {
		mem64(1, (U_32)dest_addr, table_addr, ((struct ACPI_SDTH*)dest_addr)->length); 
		if (check_sum(dest_addr, ((struct ACPI_SDTH*)dest_addr)->length)) {
			header_print(dest_addr);
			return dest_addr ;
		}
		else goto error ;
	}
	else
error:
		return NULL ;
}


static void * get_dsdt (void)
{
	void * dsdt;

	switch (ACPI_FLG) {
	case 0 :
			printf("fadt not found\n\n\n");
			goto error;	
	case 64:
			printf("use 64bit x_dsdt, start at 0x%x%x\n", buffer_fadt->x_dsdt, *((U_32*)&(buffer_fadt->x_dsdt)+1));
			dsdt = BUFFER_START+sizeof(struct simplify_fadt_table);
			if (table_move_64(dsdt, buffer_fadt->x_dsdt,"DSDT")) goto found;
			else goto error; 
	case 32:
			if (buffer_fadt->fadt_header.revision > 2 && buffer_fadt->x_dsdt < 0xffffffff) 
				dsdt = (void*)(U_32)(buffer_fadt->x_dsdt);
			else dsdt = (void*)buffer_fadt->dsdt;
			if (TABLE_CHECK32(dsdt, "DSDT")) goto found;
	default:
error:
			printf("dsdt not found");
			return dsdt_addr = NULL;
found:
			header_print(dsdt);
			return dsdt_addr = dsdt ;
	}
}


static int get_s5_sly ( struct ACPI_SDTH* dsdt )
{ 
	unsigned char *p = (void*)dsdt + sizeof(struct ACPI_SDTH);

	if (!dsdt) goto error;
	for (; p < (unsigned char*)dsdt + dsdt->length; p++) {
		if (*p != 0x08) continue;
		/* some bios Sn name space have no root path "\" */
		if ((*(U_32*)(p+1) == 0x5f35535f) || *(U_32*)(p+1) == 0x35535f5c /*  "\_S5"  */) {
			printf("S5 name space possible start at 0x%x\n\n", p);
			*(p+1) == 0x5c ? (p += 6) : (p += 5) ;
			if (*p != 0x12) continue;
			else 
				goto name_found;
		}
	}
	printf("S5 name space not found");
	goto error;
name_found:
	p += 3;
	if (*p == 0x0a && *(p + 1) <= 7) {
		printf("found s5_sly_typ a at 0x%x\n", p + 1);
		PM1A = 0;
		PM1A_BIT_BLK.slp_typx = *( p + 1 );
		printf("found s5_slp_typx_a value is: %x\n", PM1A_BIT_BLK.slp_typx);
	} else {
		printf ("slp_typ_a error, value is %x\n", *(p + 1)) ;
		goto error;
	}
	if (buffer_fadt->pm1_cnt_len > 2) {
		if (*(p + 2) == 0x0a && *(p+3) <= 7) {
			PM1B = 0;
			PM1B_BIT_BLK.slp_typx = *( p + 3 );
			printf("found s5_slp_typ_b value is: %x\n", PM1B_BIT_BLK.slp_typx);
			goto found;
		} else {
			printf ("slp_typ_b error, value is %z\n", *(p + 3)) ;
			goto error;
		}
	}
found:
	return 1; 
error:
	return 0; 

}


void header_print (void * entry)
{
	int j;
	char *p = entry;
	
	for (j = 4; j; j--)
		putchar(*p++);	
	printf(" table  at: 0x%x, length is 0x%x, revision is %x, OEMID is ", entry, 
			((struct ACPI_SDTH*)entry)->length, ((struct ACPI_SDTH*)entry)->revision) ; 
	for (p =((struct ACPI_SDTH*)entry)->oemid,j = 6; j; j--)
		putchar(*p++);
	printf("\n\n");
	return;
}


static inline void read_pm1_reg_32 (struct simplify_fadt_table* fadt_32)
{
	PM1_CTR = PM1A = PM1B = 0;
	READ_REG_WORD(fadt_32->pm1a_cnt_blk, PM1A);
	if (fadt_32->pm1_cnt_len > 2 )
		READ_REG_WORD(fadt_32->pm1b_cnt_blk, PM1B);
	PM1_CTR = PM1A | PM1B;
	return; 
}


static inline void write_pm1_reg_32 (struct simplify_fadt_table* fadt_32)
{
	PM1A_BIT_BLK.slp_en = PM1A_BIT_BLK.slp_en = 1;
	WRITE_REG_WORD(fadt_32->pm1a_cnt_blk, PM1A);
	if (fadt_32->pm1_cnt_len > 2 )
		WRITE_REG_WORD(fadt_32->pm1b_cnt_blk, PM1B);
	return;
}


static inline void write_smi_reg (int set_acpi_state)
{
	(set_acpi_state == 1 ? (set_acpi_state = buffer_fadt->acpi_enable) 
						: (set_acpi_state = buffer_fadt->acpi_disable));
	WRITE_REG_BYTE(buffer_fadt->smi_cmd, set_acpi_state); 
	return;
}	
	

static inline void  translate_acpi (int en_or_disable) /* input: 0 for disable ,1 for enable; */
{
	if (buffer_fadt && buffer_fadt->smi_cmd && buffer_fadt->acpi_disable && buffer_fadt->pm1a_cnt_blk) {
		read_pm1_reg_32(buffer_fadt);
		if(PM1_CTR & 1) {
			write_smi_reg(en_or_disable); 
			read_pm1_reg_32(buffer_fadt);
		} else if (PM1_CTR & 1 == 1)
			printf("\nonly supports ACPI mode \n");
		else {
			printf("\nenter legacy mode,  ");
			printf("SMI port is %x,  PM1_ctr_reg is  %x,  sci_en is %x\n\n", buffer_fadt->smi_cmd,
			PM1_CTR, PM1A_BIT_BLK.sci_en);
		}
	}
	return;
}

