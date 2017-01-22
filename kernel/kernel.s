##/* kernel.s*//*{{{*/
##compile 
##	$ rm -f kernel.bin
## $ as -o kernel.o kernel.s
## $ as -o string.o string.s 
## $ as -o klib.o klib.s 
## $ gcc -c -o start.o start.c
## $ ld  -s -Ttext 0x30400 -o kernel.bin kernel.o string.o start.o klib.o
## $ rm -f kernel.o string.o start.o 
## $/*}}}*/
/** @file kernel.s  */
## @{
.include "sconst.s"
##----------------------------- 导入函数-------------------------{{{
## 导入函数
.extern cstart
.extern kernel_main
.extern exception_handler
.extern spurious_irq
.extern disp_str
.extern delay
.extern irq_table

.extern k_reenter

.extern gdt_ptr
.extern idt_ptr
.extern p_proc_ready
.extern tss
.extern disp_pos
.extern clock_handler
.extern sys_call_table

clock_int_msg:		.byte '^',0
##}}}-------------------------------------------------------------end
##-----------------------------STACK-------------------------{{{
.section .bss
##.lcomm StackSpace,2048
StackSpace: .fill 2048,1,0
StackTop:   
##}}}-------------------------------------------------------------end
##----------------------------------------------globl func------{{{
.section .text
.code32
.align 32,0x90909090
.globl restart
.globl sys_call

.globl divide_error
.globl single_step_exception
.globl nmi
.globl breakpoint_exception
.globl overflow
.globl bounds_check
.globl inval_opcode
.globl copr_not_available
.globl double_fault
.globl copr_seg_overrun
.globl inval_tss
.globl segment_not_present
.globl stack_exception
.globl general_protection
.globl page_fault
.globl copr_error
.globl hwint00
.globl hwint01
.globl hwint02
.globl hwint03
.globl hwint04
.globl hwint05
.globl hwint06
.globl hwint07
.globl hwint08
.globl hwint09
.globl hwint10
.globl hwint11
.globl hwint12
.globl hwint13
.globl hwint14
.globl hwint15
##}}}-------------------------------------------------------------end
##-----------------------------_start-------------------------{{{
.globl _start

_start:
##-..................................................MEM INFO--{{{
	## 此时内存看上去是这样的（更详细的内存情况在 LOADER.ASM 中有说明）：
	##              ┃                                    ┃
	##              ┃                 ...                ┃
	##              ┣━━━━━━━━━━━━━━━━━━┫
	##              ┃■■■■■■Page  Tables■■■■■■┃
	##              ┃■■■■■(大小由LOADER决定)■■■■┃ PageTblBase
	##    00101000h ┣━━━━━━━━━━━━━━━━━━┫
	##              ┃■■■■Page Directory Table■■■■┃ PageDirBase = 1M
	##    00100000h ┣━━━━━━━━━━━━━━━━━━┫
	##              ┃□□□□ Hardware  Reserved □□□□┃ B8000h ← gs
	##       9FC00h ┣━━━━━━━━━━━━━━━━━━┫
	##              ┃■■■■■■■LOADER.BIN■■■■■■┃ somewhere in LOADER ← esp
	##       90000h ┣━━━━━━━━━━━━━━━━━━┫
	##              ┃■■■■■■■KERNEL.BIN■■■■■■┃
	##       80000h ┣━━━━━━━━━━━━━━━━━━┫
	##              ┃■■■■■■■■KERNEL■■■■■■■┃ 30400h ← KERNEL 入口 (KernelEntryPointPhyAddr)
	##       30000h ┣━━━━━━━━━━━━━━━━━━┫
	##              ┋                 ...                ┋
	##              ┋                                    ┋
	##           0h ┗━━━━━━━━━━━━━━━━━━┛ ← cs, ds, es, fs, ss
	##
	##
	## GDT 以及相应的描述符是这样的：
	##
	##		              Descriptors               Selectors
	##              ┏━━━━━━━━━━━━━━━━━━┓
	##              ┃         Dummy Descriptor           ┃
	##              ┣━━━━━━━━━━━━━━━━━━┫
	##              ┃         DESC_FLAT_C    (0～4G)     ┃   8h = cs
	##              ┣━━━━━━━━━━━━━━━━━━┫
	##              ┃         DESC_FLAT_RW   (0～4G)     ┃  10h = ds, es, fs, ss
	##              ┣━━━━━━━━━━━━━━━━━━┫
	##              ┃         DESC_VIDEO                 ┃  1Bh = gs
	##              ┗━━━━━━━━━━━━━━━━━━┛
	##
	## 注意! 在使用 C 代码的时候一定要保证 ds, es, ss 这几个段寄存器的值是一样的
	## 因为编译器有可能编译出使用它们的代码, 而编译器默认它们是一样的. 比如串拷贝操作会用到 ds 和 es.
	##
	##
##}}}-------------------------------------------------------------end
##-----------------------------startaction--------------------{{{

##  把esp从LOADER 移到 KERNEL
	movl $StackTop, %esp										

	movl $0, (disp_pos)											

	sgdt (gdt_ptr)												
	call cstart													
	lgdt (gdt_ptr)										
	
	lidt (idt_ptr)

	ljmpl $SELECTOR_KERNEL_CS, $csinit	
##}}}-------------------------------------------------------------end
##}}}-------------------------------------------------------------end
##-----------------------------csinit-------------------------{{{	
csinit:

	xorl %eax, %eax
	movw $SELECTOR_TSS, %ax
	ltr  %ax

	jmp  kernel_main

##	sti
##	hlt
##-----------------------------interrupts&xxx--------------{{{
.macro hwint_master	irq
		call save
		inb $INT_M_CTLMASK, %al
		orb $(1<<\irq), %al
		outb %al, $INT_M_CTLMASK
		movb $EOI, %al
		outb %al, $INT_M_CTL
		sti
    pushl $\irq
		call *(irq_table+(4*\irq  ))
		popl %ecx
		cli
		inb $INT_M_CTLMASK, %al
		andb $(~(1<<\irq)), %al
		outb %al, $INT_M_CTLMASK
		ret
.endm

.align 16
hwint00:
		hwint_master			0
.align 16 
hwint01:						## Interrupt routine for irq	 1 (keyboard)
		hwint_master			1
.align 16 
hwint02:						## Interrupt routine for irq	 2 (cascade!)
		hwint_master			2
.align 16 
hwint03:						## Interrupt routine for irq  3 (second serial)
		hwint_master			3
.align 16 
hwint04:						## Interrupt routine for irq  4 (first serial)
		hwint_master			4
.align 16 
hwint05:						## Interrupt routine for irq  5 (XT winchester)
		hwint_master			5
.align 16 
hwint06:						## Interrupt routine for irq  6 (floppy)
		hwint_master			6
.align 16 
hwint07:						## Interrupt routine for irq  7 (printer)
		hwint_master			7


.macro hwint_slave	irq
	call save
	inb	 $INT_S_CTLMASK, %al
	orb  (1 << (\irq-8)), %al
	outb %al, $INT_S_CTLMASK
	movb $EOI, %al
	outb %al, $INT_M_CTL
	nop
	outb %al, $INT_S_CTL
	sti
	pushl $\irq
	call *(irq_table+\irq*4)
	popl  %ecx
	cli
	inb  $INT_S_CTLMASK, %al
	andb $(~(1<<(\irq -8))), %al
	outb %al, $INT_S_CTLMASK
	ret
.endm

.align 16 
hwint08:						## Interrupt routine for irq 8 (realtime clock)
		hwint_slave			8
.align 16 
hwint09:						## Interrupt routine for irq 9 (irq 2redirected)
		hwint_slave			9
.align 16 
hwint10:						## Interrupt routine for irq 10 (
		hwint_slave			10
.align 16 
hwint11:						## Interrupt routine for irq 11
		hwint_slave			11
.align 16 
hwint12:						## Interrupt routine for irq 12
		hwint_slave			12
.align 16 
hwint13:						## Interrupt routine for irq 13 (FPU exception)
		hwint_slave			13
.align 16 
hwint14:						## Interrupt routine for irq 14 (AT winchester)
		hwint_slave			14
.align 16 
hwint15:						## Interrupt routine for irq 15
		hwint_slave			15













##}}}-------------------------------------------------------------end
##}}}-------------------------------------------------------------end
##-----------------------------interrupt-------------------------{{{
## 中断和异常 -- 异常
divide_error: 
	pushl $0xFFFFFFFF
	pushl $0
	jmp exception
single_step_exception: 
	pushl $0xFFFFFFFF
	pushl $1
	jmp exception
nmi: 
	pushl $0xFFFFFFFF
	pushl $2
	jmp exception
breakpoint_exception: 
	pushl $0xFFFFFFFF
	pushl $3
	jmp exception
overflow: 
	pushl $0xFFFFFFFF
	pushl $4
	jmp exception
bounds_check: 
	pushl $0xFFFFFFFF
	pushl $5
	jmp exception
inval_opcode: 
	pushl $0xFFFFFFFF
	pushl $6
	jmp exception
copr_not_available: 
	pushl $0xFFFFFFFF
	pushl $7
	jmp exception
double_fault: 
	pushl $8
	jmp exception
copr_seg_overrun: 
	pushl $0xFFFFFFFF
	pushl $9
	jmp exception
inval_tss: 
	pushl $10
	jmp exception
segment_not_present: 
	pushl $11
	jmp exception
stack_exception: 
	pushl $12
	jmp exception
general_protection: 
	pushl $13
	jmp exception
page_fault: 
	pushl $14
	jmp exception
copr_error: 
	pushl $0xFFFFFFFF
	pushl $16
	jmp exception

exception:
	call exception_handler
	addl $8, %esp
	jmp .
	hlt

save:
		pushal 
		pushl %ds
		pushl %es
		pushl %fs
		pushl %gs

		movl %edx, %esi

		movw	%ss, %dx
		movw  %dx, %ds
		movw  %dx, %es
		movw  %dx, %fs

		movl %esi, %edx

		movl %esp, %esi

		incl (k_reenter)
		cmpl $0, (k_reenter)
		jne		.save1
		movl $StackTop, %esp
		pushl $restart
		jmp  *(RETADR-P_STACKBASE)(%esi)
.save1:
		pushl $restart_reenter
		jmp  *(RETADR-P_STACKBASE)(%esi)
##}}}-------------------------------------------------------------end
##//---------------sys_call-------------------------{{{-
sys_call:
		call save

		sti
		pushl %esi

		pushl (p_proc_ready)
		pushl %edx
		pushl %ecx
		pushl %ebx

		call *sys_call_table(,%eax,4)
	
		addl $(4*4), %esp
		popl %esi
		movl %eax,(EAXREG-P_STACKBASE)(%esi)

		cli

		ret
##//}}}----------------------------------------------end
##//---------------rstart-------------------------{{{


restart:
	movl (p_proc_ready), %esp
	lldt P_LDT_SEL(%esp)
	leal P_STACKTOP(%esp),%eax
	movl %eax, (tss+TSS3_S_SP0)
restart_reenter:
	decl (k_reenter)
	popl %gs
	popl %fs
	popl %es
	popl %ds
	popal

	addl $4, %esp

	iretl

##//}}}-----------------------------------------------end
##@}
