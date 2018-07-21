# Linux 3.13.0 内核学习笔记

## 本仓库内容

* 《Linux操作系统内核技术》上课笔记
* 《Linux操作系统内核技术》PPT分享

```
Something I hope you know before go into the coding~
First, please watch or star this repo, I'll be more happy if you follow me.
Bug report, questions and discussion are welcome, you can post an issue or pull a request.
```

## 相关站点

本仓库已经开始作为gitbook仓库，访问地址

<https://yifengyou.gitbooks.io/linux-3-13-0/content/>

GitHub访问地址

<https://github.com/yifengyou/linux-3-13-0/>




## 目录

* [第一讲-前言](docs/第一讲-前言/第一讲-前言.md)
    * [Linux内核简介](docs/第一讲-前言/Linux内核简介.md)
    * [内核实证](docs/第一讲-前言/内核实证.md)
    * [学习资料推荐](docs/第一讲-前言/学习资料推荐.md)
* [第二讲-地址转换](docs/第二讲-地址转换/第二讲-地址转换.md)
    * [Flat内存模式](docs/第二讲-地址转换/Flat内存模式.md)
    * [Linux的分段管理](docs/第二讲-地址转换/Linux的分段管理.md)
    * [Linux的分页管理](docs/第二讲-地址转换/Linux的分页管理.md)
* [第三讲-页框管理](docs/第三讲-页框管理/第三讲-页框管理.md)
    * [内存探测](docs/第三讲-页框管理/内存探测.md)
    * [page+section+root](docs/第三讲-页框管理/page+section+root.md)
    * [node与zone](docs/第三讲-页框管理/node与zone.md)
    * [页框的分配与释放](docs/第三讲-页框管理/页框的分配与释放.md)
* [第四讲-内核内存分配器](docs/第四讲-内核内存分配器/第四讲-内核内存分配器.md)
    * [内核堆概述](docs/第四讲-内核内存分配器/内核堆概述.md)
    * [内核内存分配器基本思想](docs/第四讲-内核内存分配器/内核内存分配器基本思想.md)
    * [Slub分配器的实现](docs/第四讲-内核内存分配器/Slub分配器的实现.md)
    * [Slab分配器概述](docs/第四讲-内核内存分配器/Slab分配器概述.md)
    * [Slob分配器概述](docs/第四讲-内核内存分配器/Slob分配器概述.md)
    * [内存分配接口](docs/第四讲-内核内存分配器/内存分配接口.md)
* [第五讲-内核线性地址空间管理](docs/第五讲-内核线性地址空间管理/第五讲-内核线性地址空间管理.md)
    * [32位内核线性地址空间](docs/第五讲-内核线性地址空间管理/32位内核线性地址空间.md)
    * [64位内核线性地址空间](docs/第五讲-内核线性地址空间管理/64位内核线性地址空间.md)
    * [vmalloc+ioremap空间管理](docs/第五讲-内核线性地址空间管理/vmalloc+ioremap空间管理.md)

## 参考资料

电子科技大学 李林 副教授 《Linux操作系统内核技术》课程PPT

## 总结

1. 基础永远值得花费90%的精力去学习加强。厚积而薄发~
2. 要理解一个软件系统的真正运行机制，一定要阅读其源代码~
