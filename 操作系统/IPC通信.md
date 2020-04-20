# IPC通信

---
## IPC

1. 标识符（ID）：ID是IPC结构的内部名，用来确保使用同一个通讯通道，消息队列、信号量和共享存储段都属于内核中的IPC结构，它们都用标识符（ID）来指明IPC通讯
2. 键值（key）：key是IPC结构的外部名，在System V IPC机制中，建立两端联系的路由方法是和IPC关键字（key）直接相关的
3. IPC命令：   
(1)ipcs -q：只显示消息队列   
(2)ipcs -s：只显示信号量   
(3)ipcs -m：只显示共享内存   
(4)ipcs：查看IPC

> [IPC结构的键(key)与标识符(ID)](https://blog.csdn.net/chapterzj/article/details/8205207)

---
## 管道

- **Linux管道的实现机制**
1. 借助了文件系统的file结构和VFS的索引节点inode，通过将两个 file 结构指向同一个临时的 VFS 索引节点，而这个 VFS 索引节点又指向一个物理页面而实现的，内核将磁盘上的数据块复制到内核缓冲区中，该缓冲区的大小为1页，即4K字节，它被设计成为环形的数据结构，以便管道可以被循环利用
2. 局限性：   
(1)数据自己读不能自己写   
(2)数据一旦被读走，便不在管道中存在，不可反复读取   
(3)由于管道采用半双工通信方式，因此，数据只能在一个方向上流动   
(4)只能在有公共祖先的进程间使用管道
3. FIFO文件类型：FIFO只是借用了文件系统为管道命名，内核缓冲区的大小为1页，即4K字节，它被设计成为环形的数据结构，以便管道可以被循环利用，FIFO的好处在于我们可以通过文件的路径来识别管道，从而让没有亲缘关系的进程之间建立连接

> [Linux下管道的原理](https://blog.csdn.net/guang11cheng/article/details/17144907)   
> [Linux 管道pipe的实现原理](https://segmentfault.com/a/1190000009528245)   
> [linux管道pipe详解](https://blog.csdn.net/qq_42914528/article/details/82023408)   
> [Linux进程间通信](https://cloud.tencent.com/developer/article/1023072)


---
## 消息队列

1. 消息队列就是一个消息的链表
2. 标识符（ID）：ID是IPC结构的内部名，用来确保使用同一个通讯通道，消息队列、信号量和共享存储段都属于内核中的IPC结构，它们都用标识符（ID）来指明IPC通讯
3. 键值（key）：key是IPC结构的外部名，在System V IPC机制中，建立两端联系的路由方法是和IPC关键字（key）直接相关的

> [Linux消息队列之原理实现篇（转）](https://my.oschina.net/uvwxyz/blog/90601)   
> [Linux消息队列原理与应用](http://blog.chinaunix.net/uid-20620288-id-3240392.html)


---
## 信号与信号量

```
struct semaphore {
    spinlock_t lock;
    unsigned int count;
    struct list_head wait_list;
};
```

> [Linux系统中的信号量机制](https://my.oschina.net/kelvinxupt/blog/226100)   
> [Linux信号量的理解和探讨](https://blog.csdn.net/pang9998/article/details/80833345)


---
## 共享内存

- **IPC共享内存原理**
1. shmget获得或创建一个共享内存区，初始化该共享内存区相应的shmid_kernel结构注同时，还将在特殊文件系统shm中，创建并打开一个同名文件，并在内存中建立起该文件的相应dentry及inode结构

```
struct shmid_kernel
{
    struct kern_ipc_perm shm_perm;
    struct file * shm_file;     // 存储了将被映射文件的地址
    int id;
    unsigned long shm_nattch;
    unsigned long shm_segsz;
    time_t shm_atim;
    time_t shm_dtim;
    time_t shm_ctim;
    pid_t shm_cprid;
    pid_t shm_lprid;
};
```
2. shmat在把共享内存区域映射到进程空间
3. 建立内存映射并没有实际拷贝数据，MMU在地址映射表中是无法找到与ptr相对应的物理地址的，也就是MMU失败，将产生一个缺页中断，缺页中断的中断响应函数会在swap中寻找相对应的页面，如果找不到（也就是该文件从来没有被读入内存的情况），从硬盘上将文件读取到物理内存中，这个过程与内存映射无关





