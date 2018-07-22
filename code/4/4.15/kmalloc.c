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

redo:
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
	else 
	{//当前CPU缓存的活动slab有空闲对象，
	 //此时获取当前活动slab中的第一个空闲对象
	 //即freelist指向的对象
		
		//获取下一个空闲对象的地址
		void *next_object = get_freepointer_safe(s, object);

		//失败的原因可能是：
		//由于没有关中断，当前进程可能被其他进程抢占，此时其他进程也在该活动slab分配
		//或者其他进程在其他CPU上向该活动slab释放对象等
		if (unlikely(!this_cpu_cmpxchg_double(
				s->cpu_slab->freelist, s->cpu_slab->tid,
				object, tid,
				next_object, next_tid(tid)))) 
			goto redo;
		
		prefetch_freepointer(s, next_object);
	}

	if (unlikely(gfpflags & __GFP_ZERO) && object)
		memset(object, 0, s->object_size);

	return object;
}

static inline void *get_freepointer_safe(struct kmem_cache *s, void *object)
{
	void *p;

	p = get_freepointer(s, object);

	return p;
}

static inline void *get_freepointer(struct kmem_cache *s, void *object)
{
	return *(void **)(object + s->offset);
}