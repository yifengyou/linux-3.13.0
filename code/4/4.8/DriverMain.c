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

void PrintKMemCache(struct kmem_cache *pKMemCache)
{
	if(pKMemCache == NULL)
		return ;

	DEBUG_PRINT(DEVICE_NAME " kmem_cache.name = %s\n", pKMemCache->name);
	DEBUG_PRINT(DEVICE_NAME " kmem_cache.flags = %lx\n", pKMemCache->flags);

	DEBUG_PRINT(DEVICE_NAME " kmem_cache.align = %x\n", pKMemCache->align);
	DEBUG_PRINT(DEVICE_NAME " kmem_cache.object_size = %x\n", pKMemCache->object_size);
	DEBUG_PRINT(DEVICE_NAME " kmem_cache.inuse = %x\n", pKMemCache->inuse);
	DEBUG_PRINT(DEVICE_NAME " kmem_cache.size = %x\n", pKMemCache->size);
	DEBUG_PRINT(DEVICE_NAME " kmem_cache.offset = %x\n", pKMemCache->offset);

	DEBUG_PRINT(DEVICE_NAME " kmem_cache.allocflags = %x\n", pKMemCache->allocflags);
	DEBUG_PRINT(DEVICE_NAME " kmem_cache.oo = %lx\n", pKMemCache->oo.x);

	DEBUG_PRINT(DEVICE_NAME " kmem_cache.min_partial = %lx\n", pKMemCache->min_partial);
	DEBUG_PRINT(DEVICE_NAME " kmem_cache.cpu_partial = %x\n", pKMemCache->cpu_partial);
}

void ConstructObject(void *ptr)
{
	return;
}

static int DriverInitialize(void)
{
	int i;

	SetPageReadAndWriteAttribute((unsigned long)DriverInitialize);

	DEBUG_PRINT(DEVICE_NAME " Initialize\n");

	for(i = 0; i < 14; i++)
	{
		PrintKMemCache(kmalloc_caches[i]);
		DEBUG_PRINT(DEVICE_NAME "\n");
	}

	return InitalizeCharDevice();
}

static void DriverUninitialize(void)
{
	DEBUG_PRINT(DEVICE_NAME " Uninitialize\n");

	UninitialCharDevice();
}

module_init(DriverInitialize);
module_exit(DriverUninitialize);
