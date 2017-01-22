/*{{{
 * =====================================================================================
 *
 *       Filename:  clock.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月22日 21时57分43秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 }}}*/ 
//!		head		{{{
#include "type.h"
#include "const.h"
#include "stdio.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
//}}}----------------------------------------------end
//!		clock_handler		{{{
PUBLIC void clock_handler(int irq)
{
/*  	disp_str("#"); */
	if (++ticks >= MAX_TICKS)
		ticks = 0;

	if (p_proc_ready->ticks)
		p_proc_ready->ticks--;

	if (key_pressed)
		inform_int(TASK_TTY);

	if(k_reenter != 0) {
//		disp_str("!");
		return;
	}
	if(p_proc_ready->ticks>0){
		return;
	}
	schedule();
}
PUBLIC void milli_delay(int milli_sec)
{
	int t = get_ticks();
	while(((get_ticks() - t) * 1000 /HZ) < milli_sec) {}
}
//}}}----------------------------------------------end
//!		/* init_clock */		{{{
PUBLIC void init_clock()
{
	/*  初始化 8253 PIT */
	out_byte(TIMER_MODE, RATE_GENERATOR);
	out_byte(TIMER0, (u8)(TIMER_FREQ/HZ) );
	out_byte(TIMER0, (u8)((TIMER_FREQ/HZ) >> 8 ));

	put_irq_handler(CLOCK_IRQ, clock_handler);
	enable_irq(CLOCK_IRQ);
}
//}}}-----------------------------------------------end
