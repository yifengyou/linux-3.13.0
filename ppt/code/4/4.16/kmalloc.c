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
	
	//由于内核抢占的原因，需要重新获取当前CPU缓存。这样一来，等同于全新地进行分配操作
	c = this_cpu_ptr(s->cpu_slab);
#endif

	page = c->page;
	if (!page)
		goto new_slab;

redo:
	//..................

	//当前page所属的slab已经没有空闲对象了，从kmalloc开始流程将会运行到此处。
	//刚进入函数时，本地cpu缓存page字段还指向该page，page->freelist=0，且被冻住
	//运行get_freelist后，该slab被解冻，即page结构的frozen设为0
	freelist = get_freelist(s, page);

	if (!freelist) {
		c->page = NULL;
		goto new_slab;
	}

load_freelist:
	
	//让freelist指向下一个空闲对象，第一个对象返回给用户
	c->freelist = get_freepointer(s, freelist);
	
	c->tid = next_tid(c->tid);

	local_irq_restore(flags);
	
	return freelist;

new_slab:

	if (c->partial) 
	{
		page = c->page = c->partial;
		c->partial = page->next;
		c->freelist = NULL;
		goto redo;
	}
	
	//....................
}

//当前page所属的slab已经没有空闲对象了，从kmalloc开始流程将会运行到此处。
//刚进入函数时，本地cpu缓存page字段还指向该page，page->freelist=0，且被冻住
//运行get_freelist后，该slab被解冻，即page结构的frozen设为0
static inline void *get_freelist(struct kmem_cache *s, struct page *page)
{
	struct page new;
	unsigned long counters;
	void *freelist;

	do {
		freelist = page->freelist;
		counters = page->counters;

		new.counters = counters;
		VM_BUG_ON(!new.frozen);

		new.inuse = page->objects;
		new.frozen = freelist != NULL;

	} while (!__cmpxchg_double_slab(s, page,
		freelist, counters,
		NULL, new.counters,
		"get_freelist"));

	return freelist;
}

static inline void *get_freepointer(struct kmem_cache *s, void *object)
{
	return *(void **)(object + s->offset);
}
