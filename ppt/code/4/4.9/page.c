//struct page：大小是0x40
struct page 
{
	unsigned long flags;

	union 
	{
		//用于页高速缓存或匿名区使用
		struct address_space *mapping;  /* If low bit clear, points to
										* inode address_space, or NULL.
										* If page mapped as anonymous
										* memory, low bit is set, and
										* it points to anon_vma object:
										* see PAGE_MAPPING_ANON below.
										*/
		void *s_mem;/* slab first object *///仅用于slab而非slub，表示slab中第一个对象的地址，无论是否正在被使用
	};  

	struct 
	{
		union 
		{
			//若当前page结构对应的页框是pgd，则index保存pgd表所属进程的mm结构体指针
			pgoff_t index;          /* Our offset within mapping. */
			void *freelist;         /* sl[aou]b first free object *///slub中第一个空闲对象的地址。当该slab是活动slab的，该值为0；其他情况，都指向slab中的第一个空闲对象。
			bool pfmemalloc;        /* If set by the page allocator,
									* ALLOC_NO_WATERMARKS was set
									* and the low watermark was not
									* met implying that the system
									* is under some pressure. The
									* caller should try ensure
									* this page is only used to
									* free other pages.
									*/
		};  

		union 
		{
			/* Used for cmpxchg_double in slub */
			unsigned long counters;

			struct 
			{
				union 
				{
					/* Count of ptes mapped in mms, to show when page is mapped & limit reverse map searches.
					Used also for tail pages refcounting instead of _count. Tail pages cannot be mapped 
					and keeping the tail page _count zero at all times guarantees get_page_unless_zero() 
					will never succeed on tail pages. 
					页框中页表项数目
					*/
					atomic_t _mapcount;

					struct 
					{ /* SLUB */
						unsigned inuse:16;//跟踪了正在使用的对象个数
						unsigned objects:15;//一个slab被划分成的对象数，即包含的对象数目
						unsigned frozen:1;//当slab位于本地cpu缓存时，即为活动slab或位于本地cpu缓存部分空slab链表时，即处于冻住状态，该字段为1
					};

					int units;      /* SLOB *///slob中空闲单元的个数。4K页面，一个单元的大小是2字节；64K以上页面大小时，一个单元的大小是4字节
				};
				//页引用计数。若该字段为-1，则表示相应的页框空闲。
				//调用page_count检查该字段，返回0表示页框空闲
				atomic_t _count;    /* Usage count, see below. */
			};

			unsigned int active;    /* SLAB *///针对的是slab而非slub，该字段是slab中第一个空闲对象的下标
		};
	};

	union 
	{
		//在buddy系统中，用于构成空闲页框链表
		//若当前描述的页框是pgd，则此时lru字段用于组成pgd_list链表
		//构成了slub分配器中，node的部分空slab链表、node的全满slab链表。
		struct list_head lru;   // Pageout list, eg. active_list protected by zone->lru_lock !

		struct 
		{                
			/* slub per cpu partial pages */
			//用于本地CPU缓存中的部分空slab链表
			struct page *next;      /* Next partial slab *///构成本地CPU缓存中部分空slab链表的指针
			int pages;      /* Nr of partial slabs left *///本地CPU缓存中部分空slab链表，包含的slab个数，只有链表头的page结构的该字段才有意义
			int pobjects;   /* Approximate # of objects */
		};

		struct list_head list;  /* slobs list of pages *///用于链入三个slob链表之一
		struct slab *slab_page; /* slab fields */
		struct rcu_head rcu_head;       // Used by SLAB when destroying via RCU

		pgtable_t pmd_huge_pte; /* protected by page->ptl */
	};

	union 
	{
		//当page结构位于buddy系统空闲链表时，若该page结构为某个空闲页框块的第一个页框的page结构，则private字段存放order。即该空闲页框块的大小为2的order次方。
		unsigned long private; /* Mapping-private opaque data: usually used for buffer_heads
							   * if PagePrivate set; used for swp_entry_t if PageSwapCache;
							   * indicates order in the buddy system if PG_buddy is set.*/
		spinlock_t ptl;

		struct kmem_cache *slab_cache;  /* SL[AU]B: Pointer to slab *///slub中的第一个page的page结构的slab_cache字段，回指该slub所属的缓冲区，即kmem_cache结构
		struct page *first_page;        /* Compound tail pages *///分配页框时设置了__GFP_COMP标志：当调用budyy系统分配一个页框块时，该页框块的所有非第一个page结构的该字段都指向第一个page结构
	};

	/*
	* On machines where all RAM is mapped into kernel address space,
	* we can simply calculate the virtual address. On machines with
	* highmem some memory is mapped into kernel virtual memory
	* dynamically, so we need a place to store that address.
	* Note that this field could be 16 bits on x86 ... ;)
	*
	* Architectures with slow multiplication can define
	* WANT_PAGE_VIRTUAL in asm/page.h
	*/
	//页框对应的内核线性地址
	void *virtual;/* Kernel virtual address (NULL if not kmapped, ie. highmem) */
};