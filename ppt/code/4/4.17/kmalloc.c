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

	//..............

load_freelist:
	
	//让freelist指向下一个空闲对象，第一个对象返回给用户
	c->freelist = get_freepointer(s, freelist);
	
	c->tid = next_tid(c->tid);

	local_irq_restore(flags);
	
	return freelist;
	
new_slab:

	if (c->partial) 
	{
		//.....
	}
	
	freelist = new_slab_objects(s, gfpflags, node, &c);

	page = c->page;

	goto load_freelist;
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

	//........
}

//依次从指定节点、远端节点的部分空slab链表处，获取若干slab，并返回作为活动slab的第一个空闲对象的地址
static void *get_partial(struct kmem_cache *s, gfp_t flags, int node, struct kmem_cache_cpu *c)
{
	void *object;
	int searchnode = (node == NUMA_NO_NODE) ? numa_node_id() : node;

	//从node的部分空slab链表中获取若干slab，并返回作为活动slab的第一个空闲对象的地址
	object = get_partial_node(s, get_node(s, searchnode), c, flags);
	if (object || node != NUMA_NO_NODE)
		return object;

	//根据NUMA距离，选择其他节点的部分空slab链表获取slab
	return get_any_partial(s, flags, c);
}

//从node的部分空slab链表中获取若干slab，并返回作为活动slab的第一个空闲对象的地址
//当得到的空闲对象数超过cpu_partial/2时，不再从node的部分空链表获取slab
static void *get_partial_node(struct kmem_cache *s, struct kmem_cache_node *n, struct kmem_cache_cpu *c, gfp_t flags)
{
	struct page *page, *page2;
	void *object = NULL;
	int available = 0;
	int objects;

	//访问node的slab资源时，需要加锁
	spin_lock(&n->list_lock);

	list_for_each_entry_safe(page, page2, &n->partial, lru) 
	{
		void *t;

		//该函数从node的部分空slab链表中取出page所属的slab，并设置page结构字段：freelist、inuse、frozen
		t = acquire_slab(s, n, page, object == NULL, &objects);
		if (!t)
			break;

		//available跟踪从node部分空slab链表中，得到的空闲对象数目
		available += objects;

		if (!object) 
		{//说明当前page所属的slab，将被当作活动slab
			//设置本地CPU缓存，使得slab成为活动slab
			c->page = page;
			object = t;
		} 
		else 
		{
			//当page所属的slab不作为活动slab时，放入本地CPU缓存的部分空slab链表中
			put_cpu_partial(s, page, 0);
		}

		if (!kmem_cache_has_cpu_partial(s) || available > s->cpu_partial / 2)
			break;

	}
	spin_unlock(&n->list_lock);
	return object;
}