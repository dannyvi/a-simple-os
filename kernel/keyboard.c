/*
 * =====================================================================================
 *
 *       Filename:  keyboard.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年03月24日 14时48分21秒
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
#include "keyboard.h"
#include "keymap.h"

PRIVATE	struct kb_inbuf	kb_in;
PRIVATE int code_with_E0; 
PRIVATE int shift_l;
PRIVATE int shift_r;
PRIVATE int alt_l;
PRIVATE int alt_r;
PRIVATE int ctrl_l;
PRIVATE int ctrl_r;
PRIVATE int caps_lock;
PRIVATE int num_lock;
PRIVATE int scroll_lock;
PRIVATE int column;

//PRIVATE int caps_lock;	/* Caps Lock */
//PRIVATE int num_lock;		/* Num Lock */
//PRIVATE int scroll_lock;/* Scroll Lock */

PRIVATE u8	get_byte_from_kb_buf();
PRIVATE void set_leds();
PRIVATE void kb_wait();
PRIVATE void kb_ack();

//!		keyboard_handler		{{{
PUBLIC void keyboard_handler(int irq)
{
	u8 scan_code = in_byte(KB_DATA);

	if(kb_in.count < KB_IN_BYTES) {
		*(kb_in.p_head) = scan_code;
		kb_in.p_head++;
	if (kb_in.p_head == kb_in.buf + KB_IN_BYTES) {
		kb_in.p_head = kb_in.buf;
	}
	kb_in.count++;
	}
	key_pressed = 1;
}
//}}}----------------------------------------------end
//!		init_keyboard		{{{
PUBLIC void init_keyboard()
{
	kb_in.count = 0;
	kb_in.p_head = kb_in.p_tail = kb_in.buf;

	shift_l	= shift_r = 0;
	alt_l	= alt_r   = 0;
	ctrl_l	= ctrl_r  = 0;

	caps_lock				= 0;
	num_lock				= 1;
	scroll_lock			= 0;

	column		= 0;

	set_leds();

	put_irq_handler(KEYBOARD_IRQ, keyboard_handler);
	enable_irq(KEYBOARD_IRQ);
}
//}}}----------------------------------------------end
//!		keyboard_read		{{{
PUBLIC void keyboard_read(TTY* tty)
{
	u8 scan_code;

	int make;

	u32 key = 0; /*   用整型来表示一个键。 */
	u32* keyrow;		/*   指向 keymap[] 的某一行 */



	while(kb_in.count > 0) {
		code_with_E0 = 0;
		scan_code = get_byte_from_kb_buf();

		/*  下面开始解析扫描码 */
		if (scan_code == 0xE1) {
			int i;
			u8 pausebreak_code[] = {0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5};
			int is_pausebreak = 1;
			for(i=1;i<6;i++){
				if (get_byte_from_kb_buf() != pausebreak_code[i]){
					is_pausebreak = 0;
					break;
				}
			}
			if (is_pausebreak){
				key = PAUSEBREAK;
			}
		}
		else if (scan_code == 0xE0) {
			code_with_E0 = 1;
			scan_code = get_byte_from_kb_buf();
			/* PrintScreen  被按下 */
			if(scan_code == 0x2A){
			code_with_E0 = 0;
				if ((scan_code = get_byte_from_kb_buf()) == 0xE0){
					code_with_E0 = 1;
					if ((scan_code = get_byte_from_kb_buf()) == 0x37){
						key = PRINTSCREEN;
						make = 1;
					}
				}
			}
			/* PrintScreen  被释放 */
			if(scan_code == 0xB7){
				code_with_E0 = 0;
				if ((scan_code = get_byte_from_kb_buf()) == 0xE0) {
					code_with_E0 = 1;
					if ((scan_code = get_byte_from_kb_buf()) == 0xAA){
						key = PRINTSCREEN;
						make = 0;
					}
				}
			}
//			/*  不是PrintScreen, 此时scan_code 为0xE0紧跟的那个值 */
//			if (key == 0){
//				code_with_E0 = 1;
//			}
		}
		if ((key != PAUSEBREAK) && (key != PRINTSCREEN)){
			int caps;
			/*  首先判断make code 还是 Break Code */
			make = (scan_code & FLAG_BREAK ? 0:1);
			/*  先定位到keymap中的行 */
			keyrow = &keymap[(scan_code & 0x7F) * MAP_COLS];

			column = 0;
			caps = shift_l || shift_r;

			if (caps_lock &&
			    keyrow[0] >= 'a' && keyrow[0] <= 'z')
				caps = !caps;

			if (caps)
				column = 1;

			if (code_with_E0)
				column = 2;

			key = keyrow[column];

			switch(key) {
				case SHIFT_L:
					shift_l = make;
					break;
				case SHIFT_R:
					shift_r = make;
					break;
				case CTRL_L:
					ctrl_l = make;
					break;
				case CTRL_R:
					ctrl_r = make;
					break;
				case ALT_L:
					alt_l = make;
					break;
				case ALT_R:
					alt_l = make;
					break;
				case CAPS_LOCK:
					if(make) {
						caps_lock = !caps_lock;
						set_leds();
					}
					break;
				case NUM_LOCK:
					if (make) {
						num_lock	= !num_lock;
						set_leds();
					}
					break;
				case SCROLL_LOCK:
					if (make) {
						scroll_lock = !scroll_lock;
						set_leds();
					}
					break;
				default:
					break;
			}
    }
				
				/*  如果是Make Code 就打印，Break Code 则不做处理 */
			if(make){
				int pad = 0;

				/*  首先处理小键盘 */
				if ((key >= PAD_SLASH) && (key <= PAD_9)){
					pad = 1;
					switch(key) {
						case PAD_SLASH:
							key = '/';
							break;
						case PAD_STAR:
							key = '*';
							break;
						case PAD_MINUS:
							key = '-';
							break;
						case PAD_PLUS:
							key = '+';
							break;
						case PAD_ENTER:
							key = ENTER;
							break;
						default:
							if (num_lock &&
									(key >= PAD_0) &&
									(key <= PAD_9)) {
								key = key - PAD_0 + '0';
									}
							else if (num_lock &&
									(key == PAD_DOT)) {
								key = '.';
							}
							else{
								switch(key) {
									case PAD_HOME:
										key = HOME;
										break;
									case PAD_END:
										break;
									case PAD_PAGEUP:
										key = PAGEUP;
										break;
									case PAD_PAGEDOWN:
										key = PAGEDOWN;
										break;
									case PAD_INS:
										key = INSERT;
										break;
									case PAD_UP:
										key = UP;
										break;
									case PAD_DOWN:
										key = DOWN;
										break;
									case PAD_LEFT:
										key = LEFT;
										break;
									case PAD_RIGHT:
										key = RIGHT;
										break;
									case PAD_DOT:
										key = DELETE;
										break;
									default:
										break;
								}
							}
							break;
					}

				}


				key |= shift_l ? FLAG_SHIFT_L	:0;
				key |= shift_r ? FLAG_SHIFT_R	:0;
				key |= ctrl_l  ? FLAG_CTRL_L	:0;
				key |= ctrl_r	 ? FLAG_CTRL_R	:0;
				key |= alt_l	 ? FLAG_ALT_L		:0;
				key |= alt_r	 ? FLAG_ALT_R		:0;
				key |= pad		 ? FLAG_PAD			:0;

				in_process(tty,key);
			}
		}
	}

//}}}----------------------------------------------end
//!		get_byte_from_kb_buf()		{{{
PRIVATE u8 get_byte_from_kb_buf()
{
	u8 scan_code;

	while (kb_in.count <= 0){}

	disable_int();
	scan_code = *(kb_in.p_tail);
	kb_in.p_tail++;
	if (kb_in.p_tail == kb_in.buf + KB_IN_BYTES) {
		kb_in.p_tail = kb_in.buf;
	}
	kb_in.count--;
	enable_int();

	return scan_code;
}
//}}}----------------------------------------------end
//!		kb_wait		{{{
PRIVATE void kb_wait()
{
	u8 kb_stat;

	do {
		kb_stat = in_byte(KB_CMD);
	} while (kb_stat & 0x02);
}
//}}}----------------------------------------------end
//!		kb_ack		{{{
PRIVATE void kb_ack()
{
	u8 kb_read;

	do {
		kb_read = in_byte(KB_DATA);
	} while (kb_read != KB_ACK);
}
//}}}----------------------------------------------end
//!		set_leds		{{{
PRIVATE void set_leds()
{
	u8 leds = (caps_lock << 2) | (num_lock << 1) | scroll_lock;

	kb_wait();
	out_byte(KB_DATA, LED_CODE);
	kb_ack();

	kb_wait();
	out_byte(KB_DATA, leds);
	kb_ack();
}
//}}}----------------------------------------------end

