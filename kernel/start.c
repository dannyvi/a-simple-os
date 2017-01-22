/**
  =====================================================================================
 *
 *       Filename:  start.c
 *
 *    Description:  definition of memcpy();
 *
 *        Version:  1.0
 *        Created:  2012年03月16日 15时32分01秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
  
#include "type.h"
#include "const.h"
#include "stdio.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "fs.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"


//!		cstart		{{{
PUBLIC void cstart()
{
	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
			"-----\"cstart\" begins-----\n");
//	// 将LOADER中的GDT复制到新的GDT里面 
	memcpy(&gdt,
				 (void*)(*((u32*)(&gdt_ptr[2]))),
				 *((u16*)(&gdt_ptr[0]))+1
					 );
//	// gdt_ptr[6]六个字节  用作sgdt/lgdt参数 
	u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);
	u32* p_gdt_base  = (u32*)(&gdt_ptr[2]);
	*p_gdt_limit = GDT_SIZE * sizeof(struct descriptor)-1;
	*p_gdt_base  = (u32)&gdt;
//
//	// idt_ptr[6] 共六个字节，0-15 limit 16-47 base   sidt/lidt 参数//
	u16* p_idt_limit = (u16*)(&idt_ptr[0]);
	u32* p_idt_base  = (u32*)(&idt_ptr[2]);
	*p_idt_limit = IDT_SIZE * sizeof(struct gate)-1;
	*p_idt_base  = (u32)&idt;


	init_prot();

	disp_str("-----\"cstart\" finished-----\n");
}

//}}}---------------------------------------------------end




