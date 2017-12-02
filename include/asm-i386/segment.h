#ifndef _ASM_SEGMENT_H
#define _ASM_SEGMENT_H

#define __KERNEL_CS	0x10
#define __KERNEL_DS	0x18

#define __USER_CS	0x23
#define __USER_DS	0x2B

#endif
/*
 * 这里定义了四种不同的段寄存器，我们可以用他们的具体数值，
 * 与段式管理中段寄存器的定义进行.发现他们的TI都为0,也就是说全部使用GDT
 */
