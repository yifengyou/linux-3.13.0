static __always_inline void *kmalloc(size_t size, gfp_t flags)
{
	//当申请空间大于8KB时，直接调用__get_free_pages向上对齐，分配页面
	if (size > KMALLOC_MAX_CACHE_SIZE)//8K
		return kmalloc_large(size, flags);

	//内核分配器维护了14个缓冲区，即返回index：0到13
	int index = kmalloc_index(size);

	if (!index)
		return ZERO_SIZE_PTR;//((void *)16)

	return kmem_cache_alloc_trace(kmalloc_caches[index], flags, size);
}

static __always_inline int kmalloc_index(size_t size)
{
	if (!size)
		return 0;

	if (size > 64 && size <= 96)
		return 1;

	if (size > 128 && size <= 192)
		return 2;

	if (size <=          8) return 3;
	if (size <=         16) return 4;
	if (size <=         32) return 5;
	if (size <=         64) return 6;
	if (size <=        128) return 7;
	if (size <=        256) return 8;
	if (size <=        512) return 9;
	if (size <=       1024) return 10;
	if (size <=   2 * 1024) return 11;
	if (size <=   4 * 1024) return 12;
	if (size <=   8 * 1024) return 13;
	
	return -1;
}

void *kmem_cache_alloc_trace(struct kmem_cache *s, gfp_t gfpflags, size_t size)
{
	void *ret = slab_alloc(s, gfpflags, _RET_IP_);
	return ret;
}
EXPORT_SYMBOL(kmem_cache_alloc_trace);

static __always_inline void *slab_alloc(struct kmem_cache *s,
										gfp_t gfpflags, unsigned long addr)
{
	return slab_alloc_node(s, gfpflags, NUMA_NO_NODE, addr);
}

static __always_inline void *slab_alloc_node(struct kmem_cache *s,
		gfp_t gfpflags, int node, unsigned long addr)
{
	void **object;
	struct kmem_cache_cpu *c;
	struct page *page;
	unsigned long tid;

	preempt_disable();
	
	c = __this_cpu_ptr(s->cpu_slab);
	
	tid = c->tid;
	
	preempt_enable();

	object = c->freelist;
	page = c->page;

	if (unlikely(!object || !node_match(page, node)))
	{
		//当当前CPU的活动slab没有空闲对象，或者page不属于该node，
		//或者当前CPU根本就没有活动slab，则调用以下函数。
		object = __slab_alloc(s, gfpflags, node, addr, c);
	}
	
	//......................

	return object;
}

static void *__slab_alloc(struct kmem_cache *s, gfp_t gfpflags, int node,
			  unsigned long addr, struct kmem_cache_cpu *c)
{
	void *freelist;
	struct page *page;
	unsigned long flags;

	//虽然这里关了中断，但是如果用户调用kmalloc时允许睡眠（gfpflags中设置了__GFP_WAIT），
	//则会在allocate_slab函数中开中断，然后调用buddy分配页框，之后再关中断
	//所以就可能出现睡醒后，被交换到了其他CPU运行
	local_irq_save(flags);

#ifdef CONFIG_PREEMPT
	/*
	 * We may have been preempted and rescheduled on a different
	 * cpu before disabling interrupts. Need to reload cpu area
	 * pointer.
	 */
	//由于内核抢占的原因，需要重新获取当前CPU缓存。这样一来，等同于全新地进行分配操作
	c = this_cpu_ptr(s->cpu_slab);
#endif

	page = c->page;
	if (!page)
		goto new_slab;

	//..................

load_freelist:
	
	//让freelist指向下一个空闲对象，第一个对象返回给用户
	c->freelist = get_freepointer(s, freelist);
	
	c->tid = next_tid(c->tid);

	local_irq_restore(flags);
	
	return freelist;

new_slab:

	if (c->partial) 
	{
		//.............
	}

	freelist = new_slab_objects(s, gfpflags, node, &c);

	page = c->page;

	goto load_freelist;
	
	//....................
}

static inline void *get_freepointer(struct kmem_cache *s, void *object)
{
	return *(void **)(object + s->offset);
}

static inline void *new_slab_objects(struct kmem_cache *s, gfp_t flags, int node, struct kmem_cache_cpu **pc)
{
	void *freelist;
	struct kmem_cache_cpu *c = *pc;
	struct page *page;

	//依次从指定节点、远端节点的部分空slab链表处，获取若干slab
	//并返回作为活动slab的第一个空闲对象的地址
	freelist = get_partial(s, flags, node, c);
	if (freelist)
		return freelist;

	page = new_slab(s, flags, node);
	if (page) 
	{
		//虽然这里关了中断，但是如果用户调用kmalloc时允许睡眠（gfpflags中设置了__GFP_WAIT），
		//则会在allocate_slab函数中（new_slab调用的）开中断，然后调用buddy分配页框，之后再关中断
		//所以就可能出现睡醒后，被交换到了其他CPU运行
		//所以这里再次读取本地cpu缓存。如果交换到了其他cpu，则c->page可能有效，
		//此时将该slab清理到所属的node的部分空slab链表、全满slab链表，或者归还给buddy
		//显然，新申请到的slab可能不属于交换后的node
		c = __this_cpu_ptr(s->cpu_slab);
		if (c->page)
			flush_slab(s, c);

		/*
		 * No other reference to the page yet so we can
		 * muck around with it freely without cmpxchg
		 */
		freelist = page->freelist;
		page->freelist = NULL;

		c->page = page;
		*pc = c;
	} 
	else
		freelist = NULL;

	return freelist;
}

static struct page *new_slab(struct kmem_cache *s, gfp_t flags, int node)
{
	struct page *page;
	void *start;
	void *last;
	void *p;
	int order;

	//申请slab所需的若干连续页框，并将第一个页框page结构的objects字段，设为该slab包含的对象个数
	page = allocate_slab(s, flags & (GFP_RECLAIM_MASK | GFP_CONSTRAINT_MASK), node);
	if (!page)
		goto out;

	//新分配的slab所对应的kmem_cache_node的nr_slabs++,total_objects增加相应的对象数
	inc_slabs_node(s, page_to_nid(page), page->objects);

	//从page结构的slab_cache字段，回指到缓冲区结构kmem_cache
	page->slab_cache = s;

	start = page_address(page);

	//设置每个对象的保存下一个空闲对象地址的字段，
	//使其实际上指向物理相邻的高地址空闲对象。
	last = start;
	for_each_object(p, s, start, page->objects) 
	{
		setup_object(s, page, last);
		set_freepointer(s, last, p);
		last = p;
	}
	setup_object(s, page, last);
	set_freepointer(s, last, NULL);

	//设置page结构的freelist，使其指向slab中的第一个空闲对象
	page->freelist = start;
	page->inuse = page->objects;
	page->frozen = 1;

out:
	return page;
}

static struct page *allocate_slab(struct kmem_cache *s, gfp_t flags, int node)
{
	struct page *page;
	struct kmem_cache_order_objects oo = s->oo;
	gfp_t alloc_gfp;

	//如果用户调用kmalloc时允许睡眠，则会开中断，然后调用buddy分配页框，之后再关中断
	if (flags & __GFP_WAIT)
		local_irq_enable();

	flags |= s->allocflags;

	//首先使用kmem_cache的oo中规定的order，分配页框。
	//若失败，再使用kmem_cache的min中规定的order分配页框
	page = alloc_slab_page(alloc_gfp, node, oo);
	if (unlikely(!page)) 
	{
		oo = s->min;
		page = alloc_slab_page(flags, node, oo);
	}

	if (flags & __GFP_WAIT)
		local_irq_disable();

	page->objects = oo_objects(oo);

	return page;
}

static inline struct page *alloc_slab_page(gfp_t flags, int node, struct kmem_cache_order_objects oo)
{
	int order = oo_order(oo);

	if (node == NUMA_NO_NODE)
		return alloc_pages(flags, order);
	else
		return alloc_pages_exact_node(node, flags, order);
}