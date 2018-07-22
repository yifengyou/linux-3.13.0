/*
 * Figure out which kmalloc slab an allocation of a certain size
 * belongs to.
 * 0 = zero alloc
 * 1 =  65 .. 96 bytes
 * 2 = 120 .. 192 bytes
 * n = 2^(n-1) .. 2^n -1
 */
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