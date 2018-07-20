#include "DriverMain.h"
#include "DriverFileOperations.h"
#include "ToolFunctions.h"

MODULE_LICENSE("Dual BSD/GPL");
 
struct SLDriverParameters gslDriverParameters = {0};

struct file_operations gslNvmDriverFileOperations = 
{
	.owner = THIS_MODULE,
	.open  = DriverOpen,
	.release = DriverClose,
	.read  = DriverRead,
	.write = DriverWrite,
	.unlocked_ioctl = DriverIOControl,
	.mmap = DriverMMap,
};

int InitalizeCharDevice(void)
{
	int result;
	struct device *pdevice;

	result = alloc_chrdev_region(&(gslDriverParameters.uiDeviceNumber), 0, 1, DEVICE_NAME);
	if(result < 0)
	{
		printk(KERN_ALERT DEVICE_NAME " alloc_chrdev_region error\n");
		return result;
	}

	gslDriverParameters.pslDriverClass = class_create(THIS_MODULE, DEVICE_NAME);
	if(IS_ERR(gslDriverParameters.pslDriverClass)) 
	{
		printk(KERN_ALERT DEVICE_NAME " class_create error\n");

		result = PTR_ERR(gslDriverParameters.pslDriverClass);
		goto CLASS_CREATE_ERROR;
	}

	cdev_init(&(gslDriverParameters.slCharDevice), &gslNvmDriverFileOperations);
	gslDriverParameters.slCharDevice.owner = THIS_MODULE;

	result = cdev_add(&(gslDriverParameters.slCharDevice), gslDriverParameters.uiDeviceNumber, 1);
	if(result < 0) 
	{
		printk(KERN_ALERT DEVICE_NAME " cdev_add error\n");
		goto CDEV_ADD_ERROR;
	}

	pdevice = device_create(gslDriverParameters.pslDriverClass, NULL, gslDriverParameters.uiDeviceNumber, NULL, DEVICE_NAME);
	if(IS_ERR(pdevice)) 
	{
		printk(KERN_ALERT DEVICE_NAME " device_create error\n");

		result = PTR_ERR(pdevice);
		goto DEVICE_CREATE_ERROR;
	}

	return 0;

DEVICE_CREATE_ERROR:
	cdev_del(&(gslDriverParameters.slCharDevice));

CDEV_ADD_ERROR:
	class_destroy(gslDriverParameters.pslDriverClass);

CLASS_CREATE_ERROR:
	unregister_chrdev_region(gslDriverParameters.uiDeviceNumber, 1);

	return result;
}

void UninitialCharDevice(void)
{
	device_destroy(gslDriverParameters.pslDriverClass, gslDriverParameters.uiDeviceNumber);

	cdev_del(&(gslDriverParameters.slCharDevice));

	class_destroy(gslDriverParameters.pslDriverClass);

	unregister_chrdev_region(gslDriverParameters.uiDeviceNumber, 1);
}

struct kmem_cache_node 
{
	spinlock_t list_lock;

	unsigned long nr_partial;
	struct list_head partial;

	atomic_long_t nr_slabs;
	atomic_long_t total_objects;
	struct list_head full;
};

void WalkThroughSlabLinksInNode(struct kmem_cache *pKMemCache)
{
	int i, slab_num = 0;
	struct page *pPage;
	struct kmem_cache_node *pNode;
	struct list_head *pos, *head;

	if(pKMemCache == NULL)
		return;

	for(i = 0; i < MAX_NUMNODES; i++)
	{
		pNode = pKMemCache->node[i];
		if(pNode == NULL)
			break;

		DEBUG_PRINT(DEVICE_NAME " node %d, nr_partial = %lx\n", i, (unsigned long)pNode->nr_partial);

		spin_lock(&pNode->list_lock);

		head = &(pNode->partial);

		list_for_each(pos, head)
		{
			pPage = list_entry(pos, struct page, lru);

			DEBUG_PRINT(DEVICE_NAME "\n");

			DEBUG_PRINT(DEVICE_NAME " pPage frame no = 0x%lx\n", (unsigned long)page_to_pfn(pPage));
			DEBUG_PRINT(DEVICE_NAME " pPage->freelist = 0x%lx\n", (unsigned long)pPage->freelist);
			DEBUG_PRINT(DEVICE_NAME " pPage->inuse = 0x%lx\n", (unsigned long)pPage->inuse);
			DEBUG_PRINT(DEVICE_NAME " pPage->objects = 0x%lx\n", (unsigned long)pPage->objects);
			DEBUG_PRINT(DEVICE_NAME " pPage->frozen = 0x%lx\n", (unsigned long)pPage->frozen);
			DEBUG_PRINT(DEVICE_NAME " pPage->slab_cache = 0x%lx\n", (unsigned long)pPage->slab_cache);
			DEBUG_PRINT(DEVICE_NAME " pKMemCache = 0x%lx\n", (unsigned long)pKMemCache);

			slab_num++;

			DEBUG_PRINT(DEVICE_NAME " slab_num = %x\n", slab_num);
		}

		spin_unlock(&pNode->list_lock);
	}
}

static int DriverInitialize(void)
{
	SetPageReadAndWriteAttribute((unsigned long)DriverInitialize);

	DEBUG_PRINT(DEVICE_NAME " Initialize\n");

	WalkThroughSlabLinksInNode(kmalloc_caches[6]);
	
	return InitalizeCharDevice();
}

static void DriverUninitialize(void)
{
	DEBUG_PRINT(DEVICE_NAME " Uninitialize\n");

	UninitialCharDevice();
}

module_init(DriverInitialize);
module_exit(DriverUninitialize);
