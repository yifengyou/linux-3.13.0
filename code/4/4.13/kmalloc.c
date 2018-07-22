static __always_inline void *kmalloc(size_t size, gfp_t flags)
{
	//当申请空间大于8KB时，直接调用__get_free_pages向上2的n次方对齐，分配页面
	if (size > KMALLOC_MAX_CACHE_SIZE)//8K
		return kmalloc_large(size, flags);
	
	//..............
}

//直接调用__get_free_pages向上对齐，分配页面
static __always_inline void *kmalloc_large(size_t size, gfp_t flags)
{
	//根据size计算buddy所用的order
	unsigned int order = get_order(size);
	return kmalloc_order_trace(size, flags, order);
}

void *kmalloc_order_trace(size_t size, gfp_t flags, unsigned int order)
{
	void *ret = kmalloc_order(size, flags, order);
	return ret;
}
EXPORT_SYMBOL(kmalloc_order_trace);

static __always_inline void* kmalloc_order(size_t size, gfp_t flags, unsigned int order)
{
	void *ret;

	flags |= (__GFP_COMP | __GFP_KMEMCG);
	ret = (void *) __get_free_pages(flags, order);
	return ret;
}