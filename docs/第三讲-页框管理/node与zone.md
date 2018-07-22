<!-- TOC depthFrom:1 depthTo:6 withLinks:1 updateOnSave:1 orderedList:0 -->

- [node与zone](#node与zone)
	- [node](#node)
	- [zone](#zone)
	- [添加ZONE_MOVABLE](#添加zonemovable)
	- [page结构转node和zone](#page结构转node和zone)
	- [参考博客](#参考博客)
	- [END](#end)

<!-- /TOC -->
# node与zone

![1532244295819.png](image/1532244295819.png)

![1532244633326.png](image/1532244633326.png)


* 在NUMA模型中，每个CPU都有自己的本地内存节点（memory node），而且还可以**通过QPI总线访问其他CPU下挂的内存节点，只是访问本地内存要比访问其他CPU下的内存的速度高许多，一般经过一次QPI要增加30%的访问时延。**
* Linux中对所有的内存进行统一管理，但由于关联不同的CPU导致访问速度不同，因此又将内存划分为节点（node）；在节点内部，又进一步细分为内存域（zone）。


## node


![1532244257314.png](image/1532244257314.png)

![1532244659443.png](image/1532244659443.png)

![1532244726337.png](image/1532244726337.png)

![1532244744078.png](image/1532244744078.png)

![1532244764181.png](image/1532244764181.png)

![1532244771514.png](image/1532244771514.png)

![1532244981026.png](image/1532244981026.png)

![1532244991833.png](image/1532244991833.png)

![1532245009997.png](image/1532245009997.png)



## zone

*  由于一些特殊的应用场景，导致只能分配特定地址范围内的内存（比如老式的ISA设备DMA时只能使用前16M内存；比如kmalloc只能分配低端内存，而不能分配高端内存），因此在node中又将内存细分为zone。

![1532244477447.png](image/1532244477447.png)

![1532244783431.png](image/1532244783431.png)

![1532244791602.png](image/1532244791602.png)

![1532244802355.png](image/1532244802355.png)

![1532244811833.png](image/1532244811833.png)

![1532244831496.png](image/1532244831496.png)

![1532244872471.png](image/1532244872471.png)

* 可以对比16MB DMA zone以下reserved页框数目spanned_pages-managed_pages

![1532244939476.png](image/1532244939476.png)

## 添加ZONE_MOVABLE

![1532244960265.png](image/1532244960265.png)

## page结构转node和zone

![1532245038533.png](image/1532245038533.png)

![1532245054208.png](image/1532245054208.png)



## 参考博客

<http://zhaoxinfeng.blog.chinaunix.net/uid-30282771-id-5171166.html>


## END
