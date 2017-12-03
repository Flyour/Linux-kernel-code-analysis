#ifndef _LINUX_MMZONE_H
#define _LINUX_MMZONE_H

#ifdef __KERNEL__
#ifndef __ASSEMBLY__

#include <linux/config.h>
#include <linux/spinlock.h>
#include <linux/list.h>

/*
 * Free memory management - zoned buddy allocator.
 */

#define MAX_ORDER 10
/* 空闲区间队列 */
typedef struct free_area_struct {
	struct list_head	free_list; /* list_head 是一个通用的用来维持双向链队列的机构，内核中需要用双向链队的地方都使用这个数据结构 */
	unsigned int		*map;
} free_area_t;

struct pglist_data;

typedef struct zone_struct {
	/*
     * 每个管理分区有一个zone_strucg 结构
	 * Commonly accessed fields:
	 */
	spinlock_t		lock;
	unsigned long		offset; /* offset 表示该分区在mem_map 中的起始页面号 */
	unsigned long		free_pages;
	unsigned long		inactive_clean_pages;
	unsigned long		inactive_dirty_pages;
	unsigned long		pages_min, pages_low, pages_high;

	/*
	 * free areas of different sizes
	 */
	struct list_head	inactive_clean_list;
	free_area_t		free_area[MAX_ORDER]; /* 定义了一组空闲区间队列,用一个队列来保持一些离散（连续长度为MAX_ORDER)的物理页面，‘一组’，代表着每一种大小的连续块，都有一个队列进行记录 */

	/*
	 * rarely used fields:
	 */
	char			*name;
	unsigned long		size;
	/*
	 * Discontig memory support fields.
	 */
	struct pglist_data	*zone_pgdat; /* 指向该管理区所属存储节点的pglist_data 数据结构 */
	unsigned long		zone_start_paddr;
	unsigned long		zone_start_mapnr;
	struct page		*zone_mem_map;
} zone_t;

#define ZONE_DMA		0
#define ZONE_NORMAL		1
#define ZONE_HIGHMEM		2
#define MAX_NR_ZONES		3

/*
 * One allocation request operates on a zonelist. A zonelist
 * is a list of zones, the first one is the 'goal' of the
 * allocation, the other zones are fallback zones, in decreasing
 * priority.
 *
 * Right now a zonelist takes up less than a cacheline. We never
 * modify it apart from boot-up, and only a few indices are used,
 * so despite the zonelist table being relatively big, the cache
 * footprint of this construct is very small.
 */
typedef struct zonelist_struct {
	zone_t * zones [MAX_NR_ZONES+1]; // NULL delimited
    /*
     * zones是个指针，各个元素按特定的次序指向具体的页面管理区，表示分配页面是先尝试zones[0]所指的管理分区，如果不能满足，再去尝试zones[1],以此类推
     * 每一个zonelist_struct 结构体都代表了一种查询顺序
     */
	int gfp_mask;
} zonelist_t;

#define NR_GFPINDEX		0x100

struct bootmem_data;
/* 对于NUMA结构的系统，每个存储节点都有一个pglist_data 数据结构,所有的pgliist_data 可以通过指针node_next 形成一个单链队列 */
typedef struct pglist_data {
	zone_t node_zones[MAX_NR_ZONES]; /* 指向该节点的最多3个的页面管理区 */
	zonelist_t node_zonelists[NR_GFPINDEX]; /* 一个zonelist_struct 的数组，代表了NR_GFPINDEX( 0x100 )种的查询顺序 */
	struct page *node_mem_map; /* 这个page 型指针执行该节点的apge结构数组 */
	unsigned long *valid_addr_bitmap;
	struct bootmem_data *bdata;
	unsigned long node_start_paddr;
	unsigned long node_start_mapnr;
	unsigned long node_size;
	int node_id;
	struct pglist_data *node_next;
} pg_data_t;

extern int numnodes;
extern pg_data_t *pgdat_list;

#define memclass(pgzone, tzone)	(((pgzone)->zone_pgdat == (tzone)->zone_pgdat) \
			&& (((pgzone) - (pgzone)->zone_pgdat->node_zones) <= \
			((tzone) - (pgzone)->zone_pgdat->node_zones)))

/*
 * The following two are not meant for general usage. They are here as
 * prototypes for the discontig memory code.
 */
struct page;
extern void show_free_areas_core(pg_data_t *pgdat);
extern void free_area_init_core(int nid, pg_data_t *pgdat, struct page **gmap,
  unsigned long *zones_size, unsigned long paddr, unsigned long *zholes_size,
  struct page *pmap);

extern pg_data_t contig_page_data;

#ifndef CONFIG_DISCONTIGMEM

#define NODE_DATA(nid)		(&contig_page_data)
#define NODE_MEM_MAP(nid)	mem_map

#else /* !CONFIG_DISCONTIGMEM */

#include <asm/mmzone.h>

#endif /* !CONFIG_DISCONTIGMEM */

#define MAP_ALIGN(x)	((((x) % sizeof(mem_map_t)) == 0) ? (x) : ((x) + \
		sizeof(mem_map_t) - ((x) % sizeof(mem_map_t))))

#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */
#endif /* _LINUX_MMZONE_H */
