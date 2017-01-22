/*{{{
 * =====================================================================================
 *
 *       Filename:  i8259.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月17日 09时34分33秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 }}}*/
//!		include		{{{
#include "type.h"
#include "const.h"
#include "stdio.h"
#include "protect.h"
#include "proc.h"
#include "fs.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
//}}}----------------------------------------------end
//!		init_8259A		{{{
PUBLIC void init_8259A()
{
	out_byte(INT_M_CTL, 0x11);/* Master 8259, ICW1. */
	out_byte(INT_S_CTL, 0x11);/* Slave 8259, ICW1. */
	out_byte(INT_M_CTLMASK, INT_VECTOR_IRQ0);/* Mseter 8259, ICW2,  */
	out_byte(INT_S_CTLMASK, INT_VECTOR_IRQ8);/* Slave 8259, ICW2,   */
	out_byte(INT_M_CTLMASK,0x4);/* Master 8259, ICW3, IR2   */
	out_byte(INT_S_CTLMASK,0x2);/* Slave 8259, ICW3. 对应‘主8259’IR2. */
	out_byte(INT_M_CTLMASK,0x1);/* Master 8259, ICW4. */
	out_byte(INT_S_CTLMASK,0x1);/* Slave 8259,ICW4 */

	out_byte(INT_M_CTLMASK,0xFF);/* Master 8259,OCW1 */
	out_byte(INT_S_CTLMASK,0xFF);/* Slave 8259, OCW1. */

	int i;
	for (i = 0; i < NR_IRQ; i++){
		irq_table[i] = spurious_irq;
	}
}
//}}}-------------------------------------------------------------end
//!		spurious_irq		{{{
/*==================================================================
 *														spurious_irq 
 =================================================================*/
PUBLIC void spurious_irq(int irq)
{
			disp_str("spurious_irq: ");
			disp_int(irq);
			disp_str("\n");
}
//}}}-----------------------------------------------end
//!		put_irq_handler		{{{
PUBLIC void put_irq_handler(int irq, irq_handler handler)
{
	disable_irq(irq);
	irq_table[irq] = handler;
}
//}}}-----------------------------------------------end




