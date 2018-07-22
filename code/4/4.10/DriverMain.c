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

void WalkThroughPartialSlabLinkInCPUCache(struct kmem_cache *pKMemCache)
{
	struct kmem_cache_cpu *pCache;
	unsigned long flags;
	struct page *pPage;

	if(pKMemCache == NULL)
		return;

	local_irq_save(flags);

	pCache = this_cpu_ptr(pKMemCache->cpu_slab);

	for(pPage = pCache->partial; pPage != NULL; pPage = pPage->next)
	{
		DEBUG_PRINT(DEVICE_NAME "\n");

		DEBUG_PRINT(DEVICE_NAME " pPage frame no = 0x%lx\n", (unsigned long)page_to_pfn(pPage));
		DEBUG_PRINT(DEVICE_NAME " pPage->freelist = 0x%lx\n", (unsigned long)pPage->freelist);
		DEBUG_PRINT(DEVICE_NAME " pPage->inuse = 0x%lx\n", (unsigned long)pPage->inuse);
		DEBUG_PRINT(DEVICE_NAME " pPage->objects = 0x%lx\n", (unsigned long)pPage->objects);
		DEBUG_PRINT(DEVICE_NAME " pPage->frozen = 0x%lx\n", (unsigned long)pPage->frozen);
		DEBUG_PRINT(DEVICE_NAME " pPage->pages = 0x%lx\n", (unsigned long)pPage->pages);
		DEBUG_PRINT(DEVICE_NAME " pPage->slab_cache = 0x%lx\n", (unsigned long)pPage->slab_cache);
		DEBUG_PRINT(DEVICE_NAME " pKMemCache = 0x%lx\n", (unsigned long)pKMemCache);
	}
	
	local_irq_restore(flags);
}

static int DriverInitialize(void)
{
	SetPageReadAndWriteAttribute((unsigned long)DriverInitialize);

	DEBUG_PRINT(DEVICE_NAME " Initialize\n");

	WalkThroughPartialSlabLinkInCPUCache(kmalloc_caches[6]);
	
	return InitalizeCharDevice();
}

static void DriverUninitialize(void)
{
	DEBUG_PRINT(DEVICE_NAME " Uninitialize\n");

	UninitialCharDevice();
}

module_init(DriverInitialize);
module_exit(DriverUninitialize);
