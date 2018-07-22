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
		prior = page->freelist;//本例中为0
		counters = page->counters;

		//使被释放对象的保存下一个空闲对象字段，指向page结构的freelist的第一个空闲对象
		set_freepointer(s, object, prior);
		
		new.counters = counters;
		was_frozen = new.frozen;//本例中没有被冻住
		new.inuse--;//此时slab是非活动slab，inuse代表正在使用的对象个数

		//若被释放对象所属的slab没有被冻住，且该slab中均是空闲对象（该slab应该位于某个node的部分空slab链表中）
		//若被释放对象所属的slab没有被冻住，但page->freelist为0的情况是：该slab之前是活动slab，但由于满后，即无空闲对象后，不再是活动slab。本地cache的page字段不指向它了，它也不在任何部分空链表中
		if ((!new.inuse || !prior) && !was_frozen) 
		{
			
			if (!prior)
			{
				//被释放对象所属的slab，其之前是活动slab，但由于满后，即无空闲对象后，不再是活动slab。
				//本地cache的page字段不指向它了，它也不在任何部分空链表中
				//该slab将会被放入本地cpu缓存的部分空slab链表中
				new.frozen = 1;
			}
			else 
			{ 
				//.................
			}
		}

	} while (!cmpxchg_double_slab(s, page,
		prior, counters,
		object, new.counters,
		"__slab_free"));

	if (new.frozen && !was_frozen) 
	{
		//该slab将会被放入本地cpu缓存的部分空slab链表中
		put_cpu_partial(s, page, 1);
	}
}

//将page对应的slab，插入到本地CPU缓存的部分空slab链表的头部。并且修改了page结构的pages、pobjects字段。
static void put_cpu_partial(struct kmem_cache *s, struct page *page, int drain)
{
	struct page *oldpage;
	int pages;
	int pobjects;

	do 
	{
		pages = 0;
		pobjects = 0;
		oldpage = this_cpu_read(s->cpu_slab->partial);

		if (oldpage) 
		{
			pobjects = oldpage->pobjects;
			pages = oldpage->pages;
			
			//若本地CPU部分空slab链表包括的对象数超过门限
			if (drain && pobjects > s->cpu_partial) 
			{
				unsigned long flags;
				
				local_irq_save(flags);

				//将本地cpu缓存中部分空slab链表中的所有slab，放入所属node的部分空slab链表中。
				//根据门限，其中全空的slab将会还给buddy
				unfreeze_partials(s, this_cpu_ptr(s->cpu_slab));

				local_irq_restore(flags);

				oldpage = NULL;
				pobjects = 0;
				pages = 0;
			}
		}

		pages++;
		pobjects += page->objects - page->inuse;

		page->pages = pages;
		page->pobjects = pobjects;
		page->next = oldpage;

	} while (this_cpu_cmpxchg(s->cpu_slab->partial, oldpage, page) != oldpage);
}