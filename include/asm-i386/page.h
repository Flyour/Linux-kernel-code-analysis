#ifndef _I386_PAGE_H
#define _I386_PAGE_H

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

#ifdef __KERNEL__
#ifndef __ASSEMBLY__

#include <linux/config.h>

#ifdef CONFIG_X86_USE_3DNOW

#include <asm/mmx.h>

#define clear_page(page)	mmx_clear_page(page)
#define copy_page(to,from)	mmx_copy_page(to,from)

#else

/*
 *	On older X86 processors its not a win to use MMX here it seems.
 *	Maybe the K6-III ?
 */

#define clear_page(page)	memset((void *)(page), 0, PAGE_SIZE)
#define copy_page(to,from)	memcpy((void *)(to), (void *)(from), PAGE_SIZE)

#endif

#define clear_user_page(page, vaddr)	clear_page(page)
#define copy_user_page(to, from, vaddr)	copy_page(to, from)

/*
 * These are used to make use of C type-checking..
 */
#if CONFIG_X86_PAE //对应的36位页内存管理
typedef struct { unsigned long pte_low, pte_high; } pte_t;
typedef struct { unsigned long long pmd; } pmd_t;
typedef struct { unsigned long long pgd; } pgd_t;
#define pte_val(x)	((x).pte_low | ((unsigned long long)(x).pte_high << 32))
#else //对应32位页内存管理
typedef struct { unsigned long pte_low; } pte_t;
typedef struct { unsigned long pmd; } pmd_t;
typedef struct { unsigned long pgd; } pgd_t;
#define pte_val(x)	((x).pte_low)
#endif
/*
 * 两种定义的区别在于36位中的pmd,pgd定义为long long 型整数，之所以不全部定义为long 型，是为了gcc 在编译时加以更严格的类型检查
 */
#define PTE_MASK	PAGE_MASK

typedef struct { unsigned long pgprot; } pgprot_t;

#define pmd_val(x)	((x).pmd)
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)

#define __pte(x) ((pte_t) { (x) } )
#define __pmd(x) ((pmd_t) { (x) } )
#define __pgd(x) ((pgd_t) { (x) } )
#define __pgprot(x)	((pgprot_t) { (x) } )
/*
 * 注意，以上的define 定义，实际上是为我们的页目录定义了一些方法，为结构对象定义方法，这里吸收了面向对象的程序设计手法
 */

#endif /* !__ASSEMBLY__ */

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

/*
 * This handles the memory map.. We could make this a config
 * option, but too many people screw it up, and too few need
 * it.
 *
 * A __PAGE_OFFSET of 0xC0000000 means that the kernel has
 * a virtual address space of one gigabyte, which limits the
 * amount of physical memory you can use to about 950MB.
 *
 * If you want more physical memory than this then see the CONFIG_HIGHMEM4G
 * and CONFIG_HIGHMEM64G options in the kernel configuration.
 */

#define __PAGE_OFFSET		(0xC0000000)
/*
 * 32位地址，共有4G的虚存空间，linux内核将4G字节的空间分成两部分，将最高的1G字节（从0xC0000000 至 0xFFFFFFFF ）用与内核本身，称为‘系统空间’，
 * 而较低的3G字节（从0x0 至 0xBFFFFFFF）用作各个进程的‘用户空间’。
 * 虚存空间中最高的1G，在物理内存中却总是从最低的地址（0）开始，所以对于内核来说，‘系统空间’的虚存地址和物理地址有有一定的位移量。
 * 因此定义了__PAGE_OFFSET 来作为偏移量。
 */

#ifndef __ASSEMBLY__

/*
 * Tell the user there is some problem. Beep too, so we can
 * see^H^H^Hhear bugs in early bootup as well!
 */
#define BUG() do { \
	printk("kernel BUG at %s:%d!\n", __FILE__, __LINE__); \
	__asm__ __volatile__(".byte 0x0f,0x0b"); \
} while (0)

#define PAGE_BUG(page) do { \
	BUG(); \
} while (0)

/* Pure 2^n version of get_order */
extern __inline__ int get_order(unsigned long size)
{
	int order;

	size = (size-1) >> (PAGE_SHIFT-1);
	order = -1;
	do {
		size >>= 1;
		order++;
	} while (size);
	return order;
}

#endif /* __ASSEMBLY__ */

#define PAGE_OFFSET		((unsigned long)__PAGE_OFFSET)
#define __pa(x)			((unsigned long)(x)-PAGE_OFFSET) /* 从虚拟地址减去偏移量，得到物理地址，系统空间虚地址的映射到物理地址的映射是线性映射 */
#define __va(x)			((void *)((unsigned long)(x)+PAGE_OFFSET)) /* 起始就是上一个宏的逆操作 */
#define virt_to_page(kaddr)	(mem_map + (__pa(kaddr) >> PAGE_SHIFT))
/*
 * 根据虚存地址找到相应的物理页面的page数据结构,mem_map 是一个指针指向一个page数据结构的数组
 * 初始指针加上偏移的下标，得到所需page数据结构的指针呢。所以该宏返回的也是一各page数据结构的指针呢
 * 注意，page数据结构在文件include/linux/mm.h中定义
 */
#define VALID_PAGE(page)	((page - mem_map) < max_mapnr)
/*
 * 对于‘系统空间’而言，给定一个虚存地址x,这对应的物理地址__pa (physics address)为：x-PAGE_OFFSET
 * 给定一个物理地址x，则对应的虚拟地址为： x+PAGE_OFFSET
 */


#endif /* __KERNEL__ */

#endif /* _I386_PAGE_H */
