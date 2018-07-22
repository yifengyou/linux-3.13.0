void kfree(const void *x)
{
	struct page *page;
	void *object = (void *)x;

	//得到x所在slab的第一个页框的page结构
	page = virt_to_head_page(x);
	if (unlikely(!PageSlab(page))) //大于8KB的对象申请时，没有设置PG_slab标志
	{
		//大对象释放
		//实际上调用了__free_pages释放页面
		return;
	}

	slab_free(page->slab_cache, page, x, _RET_IP_);
}
EXPORT_SYMBOL(kfree);

static __always_inline void slab_free(struct kmem_cache *s, struct page *page, void *x, unsigned long addr)
{
	void **object = (void *)x;
	struct kmem_cache_cpu *c;
	unsigned long tid;

redo:
	preempt_disable();
	c = __this_cpu_ptr(s->cpu_slab);

	tid = c->tid;
	preempt_enable();

	//当被释放对象所在的slab是活动slab时，使用cas操作将被释放对象加入到本地cpu缓存freelist链表头
	if (likely(page == c->page)) 
	{
		//......
	} 
	else
		__slab_free(s, page, x, addr);//针对非活动slab内的对象释放
}

static void __slab_free(struct kmem_cache *s, struct page *page, void *x, unsigned long addr)
{
	void *prior;
	void **object = (void *)x;
	int was_frozen;
	struct page new;
	unsigned long counters;
	struct kmem_cache_node *n = NULL;
	
	
	//将被释放对象放入slab内page->freelist指向的空闲对象链表头部
	do 
	{
		prior = page->freelist;//本例中prior不为0
		counters = page->counters;

		//使被释放对象的保存下一个空闲对象字段，指向page结构的freelist的第一个空闲对象
		set_freepointer(s, object, prior);
		
		new.counters = counters;
		was_frozen = new.frozen;
		new.inuse--;//此时slab是非活动slab，inuse代表正在使用的对象个数

	} while (!cmpxchg_double_slab(s, page,
		prior, counters,
		object, new.counters,
		"__slab_free"));
}
