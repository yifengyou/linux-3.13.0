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

//得到x所在slab的第一个页框的page结构
static inline struct page *virt_to_head_page(const void *x)
{
	//得到对象x所对应的page结构
	struct page *page = virt_to_page(x);

	//得到x所在slab的第一个页框的page结构
	return compound_head(page);
}

//获得page所在页框块的第一个页框的page结构
static inline struct page *compound_head(struct page *page)
{
	//page结构是否设置了PG_tail位
	if (unlikely(PageTail(page)))
		return page->first_page;

	return page;
}

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
		//使被释放对象的保存下一个空闲对象字段，指向freelist的第一个空闲对象
		set_freepointer(s, object, c->freelist);

		if (unlikely(!this_cpu_cmpxchg_double(
				s->cpu_slab->freelist, s->cpu_slab->tid,
				c->freelist, tid,
				object, next_tid(tid)))) 
		{
			goto redo;
		}
	} 
	else
		__slab_free(s, page, x, addr);//针对非活动slab内的对象释放
}