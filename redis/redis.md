# redis
---

## 数据结构
1. 字符串
2. 列表
3. 哈希表
4. set
5. sort set
6. Bitmaps
7. BloomFilter
8. HyperLogLog
9. Pub/Sub
10. Redis Module
11. RedisSearch
12. Redis-ML

> [redis数据结构](https://www.cnblogs.com/neooelric/p/9621736.html)
---
- **sds字符串**

```
struct sdshdr {
	//字符长度
    unsigned int len;
    //当前可用空间
    unsigned int free;
    //具体存放字符的buf
    char buf[];
};
```
1. 通过预分配内存和维护字符串长度，实现动态字符串   
2. 减少修改字符串带来的内存分配次数(空间预分配和惰性空间释放)   
(1)空间预分配：小于1MB，多分配与len同样大小的未使用空间，大于1MB，多分配1MB的未使用空间    
(2)惰性空间释放：字符串收缩多余的的字节存放在free等待将来使用  


---
- **list列表**

```
typedef struct list {
	//列表头结点
    listNode *head;
    //列表尾结点
    listNode *tail;
    
    /* 下面3个方法为所有结点公用的方法, 分别在相应情况下回调用 */
    //复制函数指针
    void *(*dup)(void *ptr);
    //释放函数指针
    void (*free)(void *ptr);
   	//匹配函数指针
    int (*match)(void *ptr, void *key);
    //列表长度
    unsigned long len;
} list;
```
1. 双端链表
2. 无环
3. 多态: void*保存节点值
4. 轻量级消息队列   
(1)维护两个队列：pending 队列和 doing 表（hash表）   
(2)由 pending 队列出队后，workers 分配一个线程去处理任务，并将时间戳和当前线程名称以及任务 id 写入 doing 表，如果 worker 完成消息的处理则自行擦除 doing 表中该任务 id 信息，如果处理业务失败，则主动回滚   
(3)启用一个定时任务，每隔一段时间去扫描doing队列，检查每隔元素的时间戳，如果超时，把任务 rollback，即把该任务从 doing 表中删除，再重新 push 进 pending 队列

> [用redis实现消息队列](https://segmentfault.com/a/1190000012244418)   
> [利用Redis 实现消息队列](https://blog.csdn.net/ZuoAnYinXiang/article/details/50263945)

---
- **哈希表hash**

```
/* 字典结构体, 保存K-V值的结构体 */
typedef struct dictEntry {
	//字典key函数指针
    void *key;
    union {
        void *val;
        //无符号整型值
        uint64_t u64;
        //有符号整型值
        int64_t s64;
        double d;
    } v;
    //下一字典结点
    struct dictEntry *next;
} dictEntry;

/* 哈希表结构体 */
typedef struct dictht {
	//字典实体
    dictEntry **table;
    //表格可容纳字典数量
    unsigned long size;
	//哈希表大小掩码用于计算索引值, 总等于size-1
    unsigned long sizemask;
    //正在被使用的数量
    unsigned long used;
} dictht;

/* 字典主操作类 */
typedef struct dict {
	//字典类型dictType结构保存一簇用于操作特定类型键值对的函数
    dictType *type;
    //私有数据指针, 传递给类型特定函数的可选参数
    void *privdata;
    //字典哈希表, 共2张, 一张旧的, 一张新的
    dictht ht[2];
    //重定位哈希时的下标rehash进度, 没有rehash值为1
    long rehashidx; /* rehashing not in progress if rehashidx == -1 */
    //当前迭代器数量
    int iterators; /* number of iterators currently running */
} dict;
```
1. 链地址法解决hash冲突
2. rehash负载因子: used/size
3. 平滑扩容   
（1）后续每一次的插入,替换,查找操作,都插入到ht[1]指向的哈希表中   
（2）每一次插入,替换,查找操作执行时,会将旧表ht[0]中的一个bucket索引位持有的结点链表,迁移到ht[1]中去，迁移的进度保存在rehashidx这个字段中。在旧表中由于冲突而被链接在同一索引位上的结点,迁移到新表后,可能会散布在多个新表索引中去   
（3）当迁移完成后, ht[0]指向的旧表会被释放,之后会将新表的持有权转交给ht[0], 再重置ht[1]指向NULL
4. 字典遍历迭代器   
（1）安全迭代器: 禁止rehash(bgaofrewrite和bgsave指令需要遍历所有对象进行持久化, 不允许出现重复)  
（2）不安全迭代器: rehash中会发生元素重复遍历
5. dictScan反向二进制迭代器

```
顺序迭代: 字典rehash扩容和收缩会造成大量重复或者缺失
高位序扫描方式: 反向二进制位迭代器原理(高位进1): 

(0)000------(0)0000
	  +-----(8)1000
(4)100------(4)0100
	  +-----(12)1100
(2)010------(2)0010
	  +-----(10)1010
(6)110------(6)0110
	  +-----(14)1110
(1)001------(1)0001
	  +-----(9)1001
(5)101------(5)0101
	  +-----(13)1101
(3)011------(3)0011
	  +-----(11)1011
(7)111------(7)0111
	  +-----(15)1111
```

> [Redis-字典遍历内部实现原理源码](https://blog.csdn.net/zanpengfei/article/details/86519134)   
> [Redis Rehash机制的探索和实践](https://www.cnblogs.com/meituantech/p/9376472.html)   
> [Redis dictScan反向二进制迭代器](https://blog.csdn.net/u011863942/article/details/46428321)   
> [Redis源码解析——字典遍历](https://blog.csdn.net/breaksoftware/article/details/53509986)

---
- **set**   

当对象保存的所有元素都是整数值和对象保存的所有元素都是整数值用insert结构, 否则用hash table结构   
（1）如果要添加的元素value所需的数据编码比当前intset的编码要大，那么则调用intsetUpgradeAndAdd将intset的编码进行升级后再插入value，intsetUpgradeAndAdd会把原来intset中的每个元素取出来，再用新的编码重新写入新的位置   
（2）调用intsetResize对intset进行内存扩充，使得它能够容纳新添加的元素。因为intset是一块连续空间，因此这个操作会引发内存的realloc（参见http://man.cx/realloc）。这有可能带来一次数据拷贝。同时调用intsetMoveTail将待插入位置后面的元素统一向后移动1个位置，这也涉及到一次数据拷贝。在intsetMoveTail中是调用memmove完成这次数据拷贝的，memmove保证了在拷贝过程中不会造成数据重叠或覆盖

```
typedef struct intset {
	// 编码方式
	uint32_t encoding;
	
    // 集合包含的元素数量
    uint32_t length;

    // 保存元素的数组
    int8_t contents[];

} intset;
```

> [intset源码分析](https://blog.csdn.net/yangbodong22011/article/details/78671625)   
> [intset内部数据结构详解](https://blog.csdn.net/yangbodong22011/article/details/78671625)


---
- **sort set**   

1. 当数据量少时, sorted set是由ziplist来实现的   

（1）zlbytes存储整个ziplist所占用的内存的字节数      
（2）zltail指的是ziplist中最后一个entry的偏移量  
（3）zllen指的是整个ziplit中entry的数量   
（4）zlend是一个终止字节, 其值为全F, 即0xff   
（5）每个entry中prevlen存储了它前一个entry所占用的字节数，encoding存储着当前结点的类型:所谓的类型,包括当前结点存储的数据是什么(二进制,还是数值),如何编码(如果是数值,数值如何存储,如果是二进制数据,二进制数据的长度)，data存储真实的数据
2. 当sorted set中的元素个数, 即(数据, score)对的数目超过128的时候或者当sorted set中插入的任意一个数据的长度超过了64, sorted set是由dict + skiplist来实现   
（1）skiplist 在有序链表和多层链表的基础上发展起来的，每个节点随机出一个层数（level），除了最下面第1层链表之外，它会产生若干层稀疏的链表，这些链表里面的指针故意跳过了一些节点（而且越高层的链表跳过的节点越多），这就使得我们在查找数据的时候能够先在高层的链表中进行查找，然后逐层降低，最终降到第1层链表来精确地确定数据位置，同时插入操作只需要修改插入节点前后的指针，而不需要对很多节点都进行调整，降低了插入操作的复杂度   
（2）redis skiplist：   
a）分数(score)允许重复，即skiplist的key允许重复，经典skiplist中是不允许的   
b）在比较时，不仅比较分数（相当于skiplist的key），还比较数据本身，在Redis的skiplist实现中，数据本身的内容唯一标识这份数据，而不是由key来唯一标识，当多个元素分数相同的时候，还需要根据数据内容来进字典排序   
c）第1层链表不是一个单向链表，而是一个双向链表，为了方便以倒序方式获取一个范围内的元素   
d）level[]是一个柔性数组，存放指向各层链表后一个节点的指针（后向指针），另外，每个后向指针还对应了一个span值，它表示当前的指针跨越了多少个节点，span用于很方便地计算出元素排名(rank)   

```
typedef struct zskiplistNode {
    robj *obj;  	/*成员对象*/
    double score;   /*分值*/
    struct zskiplistNode *backward; /*后退指针*/
    struct zskiplistLevel { /*层*/
        struct zskiplistNode *forward;  /*前进指针*/
        unsigned int span;  /*跨度*/
    } level[];
} zskiplistNode;

typedef struct zskiplist {
    struct zskiplistNode *header, *tail;
    unsigned long length;
    int level;
} zskiplist;
```

skiplist和各种平衡树以及哈希表
1. skiplist和各种平衡树(如AVL、红黑树等)的元素是有序排列的, 而哈希表不是有序的,在哈希表上只能做单个key的查找, 不适宜做范围查找
2. 在做范围查找的时候, 平衡树比skiplist操作要复杂, 在平衡树上, 我们找到指定范围的小值之后,还需要以中序遍历的顺序继续寻找其它不超过大值的节点
3. 平衡树的插入和删除操作可能引发子树的调整,逻辑复杂, 而skiplist的插入和删除只需要修改相邻节点的指针, 操作简单又快速
4. 从内存占用上来说, skiplist比平衡树更灵活一些, 平衡树每个节点包含2个指针(分别指向左右子树),而skiplist每个节点包含的指针数目平均为1/(1-p)
5. 查找单个key,skiplist和平衡树的时间复杂度都为O(log n), 大体相当；而哈希表在保持较低的哈希值冲突概率的前提下, 查找时间复杂度接近O(1), 性能更高一些
6. skiplist易于开发和调试

> [Redis 为什么用跳表而不用平衡树？](https://juejin.im/post/57fa935b0e3dd90057c50fbc#heading-0)   
> [浅析SkipList跳跃表原理及代码实现](https://blog.csdn.net/ict2014/article/details/17394259)   
> [Redis中的跳跃表](https://blog.csdn.net/universe_ant/article/details/51134020)

---
- **Bitmaps**

1. String类型上的一组面向bit操作的集合,最大长度是512m, 所以bitmaps能最大设置2^32个不同的bit
2. Bitmaps的最大优点就是存储信息时可以节省大量的空间
3. Bitmaps局限性：样本分布不均匀,造成空间上的浪费，以及key值如果是url等可能hash出相同的int值,降低Bitmap准确性
4. Bitmaps使用场景:各种实时分析和存储与对象ID关联的节省空间并且高性能的布尔信息（例如使用 bitmap 实现用户上线次数统计、统计活跃用户）

---
- **Bloom Filter**

1. 概念：Bloom Filter是一个很长的二进制向量和一系列随机映射函数，布隆过滤器可以用于检索一个元素是否在一个集合中
2. 原理：当一个元素被加入集合时，通过K个散列函数将这个元素映射成一个位数组中的K个点，把它们置为1
3. Bloom Filter与单哈希函数Bit-Map不同之处在于：Bloom Filter使用了k个哈希函数，每个字符串跟k个bit对应，从而降低了冲突的概率
4. 缺点   
（1）存在误判，可能要查到的元素并没有在容器中，但是hash之后得到的k个位置上值都是1。如果bloom filter中存储的是黑名单，那么可以通过建立一个白名单来存储可能会误判的元素   
（2）删除困难。一个放入容器的元素映射到bit数组的k个位置上是1，删除的时候不能简单的直接置为0，可能会影响其他元素的判断

> [Redis-避免缓存穿透的利器之BloomFilter](https://juejin.im/post/5db69365518825645656c0de)


---
## HyperLogLog

- **基数计数基本概念**

1. 基数计数通常用来统计一个集合中不重复的元素个数

- **基数计数作用**

1. 统计每日访问 IP 数
2. 统计页面实时 UV 数
3. 统计用户每天搜索不同词条的个数
4. 数据分析、网络监控

- **基数计数方法**

1. B树：用B树存储要统计的数据，可以快速判断新来的数据是否已经存在，并快速将元素插入B树。要计算基数值，只需要计算B树的节点个数，但没有节省存储内存
2. bitmap：bit是数据的最小存储单位，因此能大量节省空间，也可以将整个bit数据一次性load到内存计算，如果要统计1亿个数据的基数值，大约需要内存：100000000/8/1024/1024 \approx ≈ 12M，bitmap 对于内存的节约量是显而易见的，但还是不够，统计一个对象的基数值需要12M，如果统计10000个对象，就需要将近120G了，同样不能广泛用于大数据场景
3. 概率算法：HLL中实际存储的是一个长度为 m 的大数组 S，将待统计的数据集合划分成 m 组，每组根据算法记录一个统计值存入数组中   
（1）通过hash函数计算输入值对应的比特串   
（2）比特串的低t（2^t=m）   
（3）t+1 位开始找到第一个1出现的位置 k，将 k 记入数组 S[i] 位置
（4）基于数组 S 记录的所有数据的统计值，计算整体的基数值
4. hyperloglog 原理   
（1）一直抛硬币直到出现正面，记录下投掷次数 k，将这种抛硬币多次直到出现正面的过程记为一次伯努利过程，对于 n 次伯努利过程，我们会得到 n 个出现正面的投掷次数值k1，k2……kmax，其中最大值记为 kmax   
（2）总结：进行了 n 次进行抛硬币实验，每次分别记录下第一次抛到正面的抛掷次数 k，那么可以 n 次实验中最大的抛掷次数 kmax 来预估实验组数量 n：n = 2^kmax   
（3）回到基数统计的问题，我们需要统计一组数据中不重复元素的个数，集合中每个元素的经过hash函数后可以表示成0和1构成的二进制数串，一个二进制串可以类比为一次抛硬币实验，1是抛到正面，0是反面。二进制串中从低位开始第一个1出现的位置可以理解为抛硬币试验中第一次出现正面的抛掷次数 k，那么基于上面的结论，我们可以通过多次抛硬币实验的最大抛到正面的次数来预估总共进行了多少次实验，同样可以可以通过第一个1出现位置的最大值 kmax 来预估总共有多少个不同的数字
5. 分桶平均：分桶平均的基本原理是将统计数据划分为 m 个桶，每个桶分别统计各自的 kmax 并能得到各自的基数预估值 n，最终对这些 n 求平均得到整体的基数估计值，分桶数组是为了消减因偶然性带来的误差，提高预估的准确性
6. 偏差修正：引入一种小范围修正方法，当HLL算法中统计数组已满的时候，需要统计的数据基数很大，这时候hash空间会出现很多碰撞情况，这种阶段引入一种大范围修正方法

> [HyperLogLog算法](http://www.rainybowe.com/blog/2017/07/13/%E7%A5%9E%E5%A5%87%E7%9A%84HyperLogLog%E7%AE%97%E6%B3%95/index.html?utm_source=tuicool&utm_medium=referral)   
> [高压缩空间占用的 Hyper LogLog 算法](https://blog.csdn.net/heiyeshuwu/article/details/41248379?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)


---
## Pub/Sub


```
struct redisServer {
    dict *pubsub_channels;  //key=channel，value=list，list是clients列表
    list *pubsub_patterns;  //存放模式匹配的subscribe的clients列表
}

pubsubPublishMessage找到字典中的channel这个key，获取到clients之后，遍历client的来发送信息
```

> [redis 发布订阅模式](https://blog.csdn.net/w05980598/article/details/80444717)
---

## redis过期时间策略和原理(懒汉式删除+定期删除)


- **定时删除**   
设置键的过期时间时，创建一个Timer，当过期时间到临时，立刻删除键，内存友好型Cpu不友好型策略
- **惰性删除**   
惰性删除策略不会在键过期的时候立马删除，而是当外部指令获取这个键的时候才会主动删除如果一些键长期没有被访问，会造成内存泄露（垃圾数据占用内存）
- **定期删除**   
通过合理的删除操作执行的时长和频率，达到合理的删除过期键


---
- **RDB持久化过期键处理**   
（1）Master 载入RDB时，文件中的未过期的键会被正常载入，过期键则会被忽略   
（2）Slave 载入RDB时，文件中的所有键都会被载入，当同步进行时，会和Master保持一致
- **AOF持久化过期键处理**   
数据库键空间的过期键的过期但并未被删除释放的状态会被正常记录到 AOF文件中，当过期键发生释放删除时，DEL也会被同步到 AOF 文件中去
- **主从复制过期键处理**   
（1）Master 删除过期 Key 之后，会向所有 Slave 服务器发送一个DEL命令让从服务器删除这些过期 Key   
（2）Slave 在被动的读取过期键时，不会做出操作，而是继续返回该键，只有当Master发送DEL通知来，才会删除过期键，保证主从服务器的数据一致性


---
## redis过期淘汰配置

- volatile-lru:在设置了过期时间的键空间中,移除最近最少使用的key
- allkeys-lru:移除最近最少使用的key
- 
- volatile-random:在设置了过期时间的键空间中,随机移除一个键
- allkeys-random:直接在键空间中随机移除一个键
- 
- volatile-ttl:在设置了过期时间的键空间中,有更早过期时间的key优先移除
- noeviction:不做过键处理, 只返回一个写操作错误

> [Redis过期策略以及内存淘汰机制](https://blog.csdn.net/qq_28018283/article/details/80764518?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [Redis有效时间设置及时间过期处理](https://blog.csdn.net/qq_35981283/article/details/70156422?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)


---
## Redis数据备份与恢复

- **RDB持久**化

1. RDB方式的持久化是通过快照（snapshotting）完成的，当符合一定条件时Redis会自动将内存中的所有数据进行快照并存储在硬盘上，SAVE(阻塞)，BGSAVE(子进程并发处理)

```
两个参数构成：时间和改动的键的个数

save 900 1    # 900秒内有至少1个键被更改则进行快照
save 300 10   # 300秒内有至少10个键被更改则进行快照
save 60 10000 # 60秒内有至少10000个键被更改则进行快照
```

2. Redis实现快照的过程   
（1）Redis使用fork函数复制一份当前进程（父进程）的副本（子进程）   
（2）父进程继续接收并处理客户端发来的命令，而子进程开始将内存中的数据写入硬盘中的临时文件   
（3）当子进程写入完所有数据后会用该临时文件替换旧的RDB文件，至此一次快照操作完成

在执行fork的时候操作系统（类Unix操作系统）会使用写时复制（copy-on-write）策略，即fork函数发生的一刻父子进程共享同一内存数据，当父进程要更改其中某片数据时（如执行一个写命令），操作系统会将该片数据复制一份以保证子进程的数据不受影响，所以新的RDB文件存储的是执行fork一刻的内存数据

Redis在进行快照的过程中不会修改RDB文件，只有快照结束后才会将旧的文件替换成新的，也就是说任何时候RDB文件都是完整的,这使得我们可以通过定时备份RDB文件来实现Redis数据库备份

- **AOF(Append-only-file)持久化**

1. 开启AOF持久化后每执行一条会更改Redis中的数据的命令，Redis就会将该命令写入硬盘中的AOF文件

```
配置redis自动重写AOF文件的条件

auto-aof-rewrite-min-size 64mb   # 允许重写的最小AOF文件大小

配置写入AOF文件后，要求系统刷新硬盘缓存的机制

# appendfsync always   # 每次执行写入都会执行同步，最安全也最慢
appendfsync everysec   # 每秒执行一次同步操作
# appendfsync no       # 不主动进行同步操作，而是完全交由操作系统来做（即每30秒一次），最快也最不安全

如果不要求性能，在每条写指令时都sync一下磁盘，就不会丢失数据。
但是在高性能的要求下每次都sync是不现实的，一般都使用定时sync，比如1s1次，这个时候最多就会丢失1s的数据
```

2. AOF重写   
(1)AOF后台重写开启后父进程将请求写入AOF重写缓冲区   
(2)子进程不需要对先有AOF文件进行任何读取分析, 通过读取服务器当前数据库状态实现,用一条命令去记录键值对代替之前记录这个键值对的多条命令   
(3)子进程完成AOF重写后,父进程将AOF重写缓冲区写入新的AOF文件中

- **RDB和AOF总结**

1. 与AOF相比，RDB适合大规模的数据恢复，适合冷备
2. RDB数据丢失风险大,适合对数据完整性和一致性要求不高场景
3. 与AOF相比，RDB持久化方式可以最大化redis的性能，但RDB在生成数据快照的时候，如果文件很大，客户端可能会暂停几毫秒甚至几秒


> [Redis持久化](https://blog.csdn.net/u010648555/article/details/73433717?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [Redis数据备份与恢复](https://blog.csdn.net/wzzfeitian/article/details/42081969?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [Redis持久化之RDB和AOF](https://www.cnblogs.com/itdragon/p/7906481.html)


---

## redis主从同步

- **全量同步**

1. Redis全量复制一般发生在Slave初始化阶段，这时Slave需要将Master上的所有数据都复制一份   
2. 同步过程   
（1）从服务器连接主服务器，发送SYNC命令   
（2）主服务器接收到SYNC命名后，开始执行BGSAVE命令生成RDB文件并使用缓冲区记录此后执行的所有写命令   
（3）主服务器BGSAVE执行完后，向所有从服务器发送快照文件，并在发送期间继续记录被执行的写命令   
（4）从服务器收到快照文件后丢弃所有旧数据，载入收到的快照   
（5）主服务器快照发送完毕后开始向从服务器发送缓冲区中的写命令   
（6）从服务器完成对快照的载入，开始接收命令请求，并执行来自主服务器缓冲区的写命令

- **增量同步**   

1. Redis增量复制是指Slave初始化后开始正常工作时主服务器发生的写操作同步到从服务器的过程   
2. 增量复制的过程主要是主服务器每执行一个写命令就会向从服务器发送相同的写命令，从服务器接收并执行收到的写命令

- **PSYNC同步**

1. 断线后重复制效率低，PSYNC提供了部分复制的能力，PSYNC执行过程中比较重要的3个概念：runid、offset（复制偏移量）以及复制积压缓冲区
2. 同步过程   
（1）客户端向服务器发送SLAVEOF命令，让当前服务器成为Slave   
（2）当前服务器根据自己是否保存Master的runid来判断是否是第一次复制，如果是第一次同步则跳转到3，否则跳转到4   
（3）向Master发送PSYNC -1 命令来进行完整同步   
（4）向Master发送PSYNC runid offset   
（5）Master接收到PSYNC命令后首先判断runid是否和本机的id一致，如果一致则会再次判断offset偏移量和本机的偏移量相差有没有超过复制积压缓冲区大小，如果没有那么就给Slave发送CONTINUE，此时Slave只需要等待Master传回失去连接期间丢失的命令   
（6）如果runid和本机id不一致或者双方offset差距超过了复制积压缓冲区大小，那么就会返回FULLRESYNC runid offset，Slave将runid保存起来，并进行完整同步

> [Redis主从同步原理-SYNC](https://blog.csdn.net/sk199048/article/details/50725369?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task/)   
> [Redis主从同步原理-PSYNC](https://blog.csdn.net/sk199048/article/details/77922589)


---
## redis集群

- **cluster**
1. Redis cluster 支撑 N 个 Redis master node，每个master node都可以挂载多个 slave node，主从同步读写分离的
2. Redis 集群没有使用一致性 hash, 而是引入了哈希槽的概念，Redis 集群有16384个哈希槽,每个 key 通过 CRC16 校验后对16384取模来决定放置哪个槽,集群的每个节点负责一部分 hash 槽，整个 Redis 就可以横向扩容了   
如果你要支撑更大数据量的缓存，那就横向扩容更多的 master 节点，每个 master 节点就能存放更多的数据了
3. 集群操作命令：客户端拿到一个key后到底该向哪个节点发请求呢？把集群里的那套key和节点的映射关系理论搬到客户端来就行了   
所以客户端需要实现一个和集群端一样的哈希函数，先计算出key的哈希值，然后再对16384取余，这样就找到了该key对应的哈希槽，利用客户端缓存的槽和节点的对应关系信息，就可以找到该key对应的节点了   
集群已经发生了变化，客户端的缓存还没来得及更新，redis 节点只处理自己拥有的key，对于不拥有的key将返回重定向错误，即-MOVED key 127.0.0.1:6381，客户端重新向这个新节点发送请求   
让一组相关的key映射到同一个节点上是非常有必要的，这样可以提高效率，通过多key命令一次获取多个值，redis支持key哈希标签{user1000}.following仅使用Key中的位于{和}间的字符串参与计算哈希值，这样可以保证哈希值相同，落到相同的节点上

- **Sentinel**
1. 作用   
（1）监控(Monitoring): 哨兵(sentinel) 会不断地检查你的Master和Slave是否运作正常   
（2）提醒(Notification):当被监控的某个 Redis出现问题时, 哨兵(sentinel) 可以通过 发布订阅发送通知   
（3）自动故障迁移(Automatic failover):当一个Master不能正常工作时，哨兵(sentinel) 会开始一次自动故障迁移操作,它会将失效Master的其中一个Slave升级为新的Master, 并让失效Master的其他Slave改为复制新的Master; 当客户端试图连接失效的Master时,集群也会向客户端返回新Master的地址,使得集群可以使用Master代替失效Master

- **一致性 hash**
1. 一致性hash是一个0-2^32-1的环形闭合圆
2. 计算一致性哈希是采用的是如下步骤：   
(1)对节点进行hash，通常使用其节点的ip或者是具有唯一标示的数据进行hash(ip)，将其值分布在这个闭合圆上   
(2)将存储的key进行hash(key)，然后将其值要分布在这个闭合圆上   
(3)从hash(key)在圆上映射的位置开始顺时针方向找到的一个节点即为存储key的节点
3. 为了防止删除节点发生热点数据造成下一节点崩溃的雪崩情况，以及节点太少数据倾斜的情况，采用虚拟节点，就是将真实节点计算多个哈希形成多个虚拟节点并放置到哈希环上，定位算法不变，只是多了一步虚拟节点到真实节点映射的过程
4. 一致性哈希算法在分布式系统中的使用场景：   
(1)负载均衡：对客户端IP地址或者会话ID计算哈希值，顺时针方向找到的一个节点服务器地址   
(2)数据分片：搜索日志数据分片多台机器   
(3)分布式存储：海量的数据、海量的用户的数据存储

> [redis集群](https://www.cnblogs.com/lixinjie/p/a-key-point-of-redis-in-interview.html)   
> [Redis的哨兵机制或者心跳机制模式原理详解](https://blog.csdn.net/u012240455/article/details/81843714?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)  
> [redis主从复制和集群实现原理](https://blog.csdn.net/nuli888/article/details/52136822?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [那些年用过的Redis集群架构](https://www.cnblogs.com/rjzheng/p/10360619.html)

> [一致性哈希和哈希槽对比](https://www.jianshu.com/p/4163916a2a8a)   
> [一致性哈希算法在分布式系统中的使用场景](https://blog.csdn.net/m0_37609579/article/details/100901237)

---
## 缓存雪崩、击穿、穿透

- **缓存穿透**
1. 查询一个一定不存在的数据，由于缓存是不命中时被动写的，并且出于容错考虑，如果从存储层查不到数据则不写入缓存，这将导致这个不存在的数据每次请求都要到存储层去查询，失去了缓存的意义
2. 解决方案   
(1)根据业务做参数校验   
(2)采用布隆过滤器，将所有可能存在的数据哈希到一个足够大的bitmap中，一个一定不存在的数据会被这个bitmap拦截掉，从而避免了对底层存储系统的查询压力

- **缓存雪崩**
1. 缓存雪崩是指在我们设置缓存时采用了相同的过期时间，导致缓存在某一时刻同时失效，请求全部转发到DB，DB瞬时压力过重雪崩
2. 解决方案   
(1)大多数系统设计者考虑用加锁或者队列的方式保证缓存的单线程/进程写，从而避免失效时大量的并发请求落到底层存储系统上   
(2)缓存失效时间分散开,在原有的失效时间基础上增加一个随机值，比如1-5分钟随机，这样每一个缓存的过期时间的重复率就会降低，就很难引发集体失效的事件   

- **缓存击穿**
1. 缓存在某个时间点过期的时候，恰好在这个时间点对这个Key有大量的并发请求过来，这些请求发现缓存过期一般都会从后端DB加载数据并回设到缓存，这个时候大并发的请求可能会瞬间把后端DB压垮
2. 解决方案   
(1)设置热点数据永远不过期   
(2)加上互斥锁

> [缓存雪崩、击穿、穿透](https://juejin.im/post/5dbef8306fb9a0203f6fa3e2)   
> [缓存穿透，缓存击穿，缓存雪崩解决方案分析](https://blog.csdn.net/zeb_perfect/article/details/54135506)
---
## 数据库与redis缓存数据一致性解决方案

1. 写完数据库后是否需要马上更新缓存还是直接删除缓存？   
(1)如果对于那种写数据频繁而读数据少的场景并不合适这种解决方案，因为也许还没有查询就被删除或修改了，这样会浪费时间和资源   
(2)写入缓存中的数据需要经过几个表的关联计算后得到的结果插入缓存中，那就没有必要马上更新缓存，只有删除缓存即可，等到查询的时候在去把计算后得到的结果插入到缓存中即可   
2. 先更新数据库，在删除缓存，如果删除缓存失败导致数据不一致问题
3. 先删除缓存，在更新数据库，在更新数据库的未完成的情况下发生了读到脏数据后更新了缓存
4. 数据库与缓存双写需要强一致性（在高并发的情况下，如果当删除完缓存的时候，这时去更新数据库，但还没有更新完，另外一个请求来查询数据，发现缓存里没有，就去数据库里查拿到旧的数据）   
(1)用队列解决问题，创建几个队列，如20个，根据商品的ID去做hash值，然后对队列个数取摸，当有数据更新请求时，先把它丢到队列里去，当更新完后在从队列里去除，如果在更新的过程中，遇到以上场景，先去缓存里看下有没有数据，如果没有，可以先去队列里看是否有相同商品ID在做更新，如果有也把查询的请求发送到队列里去，然后同步等待缓存更新完成   
(2)优化点：如果发现队列里有一个查询请求了，那么就不要放新的查询操作进去了，用一个while（true）循环去查询缓存，循环个200MS左右，如果缓存里还没有则直接取数据库的旧数据，一般情况下是可以取到的   
(3)注意点：队列中挤压了大量的更新操作，造成读请求超时直接请求数据库，一般做好压力测试。热点商品请求路由到一个队列，造成队列请求倾斜，做好测试

> [数据库与缓存数据一致性解决方案](https://blog.csdn.net/simba_1986/article/details/77823309?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [分布式之数据库和缓存双写一致性方案解析](https://www.cnblogs.com/rjzheng/p/9041659.html)


---
## redis分布式锁
- **分布式锁**
1. 分布式锁：当多个进程不在同一个系统中，用分布式锁控制多个进程对资源的访问

- **setex**
1. 从 Redis 2.6.12 版本开始，执行 SET key value EX seconds 的效果等同于执行 SETEX key seconds value + EXPIRE key seconds
2. 缺点：试想一下，某线程A获取了锁并且设置了过期时间为10s，然后在执行业务逻辑的时候耗费了15s，此时线程A获取的锁早已被Redis的过期机制自动释放了，在线程A获取锁并经过10s之后，改锁可能已经被其它线程获取到了，当线程A执行完业务逻辑准备解锁（DEL key）的时候，有可能删除掉的是其它线程已经获取到的锁，所以最好的方式是在解锁时判断锁是否是自己的，我们可以在设置key的时候将value设置为一个唯一值uniqueValue，例如机器号+线程号的组合、签名等

- **lua分布式锁**
1. 加锁机制： 
```
// 锁成功
if (redis.exists(key) == 0) then
    redis.hset(key, id, client_id, times, 1)
    redis.ttl(key, expire_time)
    return true
end
// 可重入加锁，增加times
if (redis.hget(key, id) == client_id) then
    redis.incr(key, times)
    redis.ttl(key, expire_time)
    return true
end
// 锁互斥，返回锁剩余时间
return redis.pttl(key)
```

2. 释放锁机制：

```
// 释放锁
if (redis.hget(key, id) == client_id) then
    redis.decr(key, times)
    if (redis.hget(key, times) == 0) then
        redis.del(key)
    end
end
```
3. watchdog的概念，翻译过来就是看门狗，它会在你获取锁之后，每隔10秒帮你把key的超时时间设为30s
4. 缺点：在redis master实例宕机的时候，redis slave未同步master数据，可能导致多个客户端同时完成加锁，导致各种脏数据的产生

- **RedLock算法**
1. 假设redis的部署模式是redis cluster，总共有5个master节点，通过以下步骤获取一把锁：   
(1)获取当前时间戳，单位是毫秒   
(2)轮流尝试在每个master节点上创建锁，过期时间设置较短，一般就几十毫秒   
(3)尝试在大多数节点上建立一个锁，比如5个节点就要求是3个节点（n / 2 +1）   
(4)客户端计算建立好锁的时间，如果建立锁的时间小于超时时间，就算建立成功了   
(5)要是锁建立失败了，那么就依次删除这个锁   
(6)只要别人建立了一把分布式锁，你就得不断轮询去尝试获取锁   
2. Redlock会失效的情况：时钟发生跳跃，长时间的网络延迟


> [Redis分布式锁的作用及实现](https://blog.csdn.net/L_BestCoder/article/details/79336986?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [基于Redis实现分布式锁](https://blog.csdn.net/ugg/article/details/41894947?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [Redis分布式锁的正确实现方式](https://www.cnblogs.com/linjiqin/p/8003838.html#!comments)   
> [基于Redis实现分布式锁](https://blog.csdn.net/ugg/article/details/41894947)

> [拜托，面试请不要再问我Redis分布式锁的实现原理！](https://juejin.im/post/5bf3f15851882526a643e207)   
> [面试不懂分布式锁？那得多吃亏](https://juejin.im/post/5d26266de51d454f71439d70#heading-1)   
> [大家所推崇的 Redis 分布式锁，真的万无一失吗？](https://juejin.im/post/5d41c94bf265da03a715b18f)

---

## 面试

- **为啥Redis那么快？**

1. 完全基于内存，绝大部分请求是纯粹的内存操作，非常快速。它的，数据存在内存中，类似于HashMap，HashMap的优势就是查找和操作的时间复杂度都是O(1)   
2. 数据结构简单，对数据操作也简单，Redis中的数据结构是专门进行设计的   
3. 采用单线程，避免了不必要的上下文切换和竞争条件，也不存在多进程或者多线程导致的切换而消耗 CPU，不用去考虑各种锁的问题，不存在加锁释放锁操作，没有因为可能出现死锁而导致的性能消耗   
4. 使用多路I/O复用模型，非阻塞IO，事件模型I/O多路复用: select, poll, epoll   
5. Redis直接自己构建了VM 机制 ，因为一般的系统调用系统函数的话，会浪费一定的时间去移动和请求

- **redis是单线程的，我们现在服务器都是多核的，那不是很浪费？**

1. 是单线程的，但是，我们可以通过在单机开多个Redis实例

- **redis是怎么进行数据交互的？以及Redis是怎么进行持久化的？Redis数据都在内存中，一断电或者重启不就木有了嘛？RDB/AOF各自优缺点是啥？**
- **主从同步原理？数据传输的时候断网了或者服务器挂了怎么办？**
- **内存淘汰机制？LRU代码？redis过期策略？**
- **redis数据结构特性和使用场景？**
1. String普通的 set 和 get，做简单的 KV 缓存，应用与缓存功能、计数器、共享用户Session
2. Hash一般可以将结构化的数据，比如一个对象缓存在 Redis 里，简单的商品类型、标签、单价、数量等
3. List 是有序列表，可以做简单的消息队列，可以通过 List 存储一些列表型的数据结构，类似粉丝列表、文章的评论列表之类的东西，也可以通过 lrange 命令，读取某个闭区间内的元素，可以基于 List 实现分页查询
4. Set 是无序集合，会自动去重，Set 有交集、并集、差集的操作，可以把两个人的好友列表整一个交集，看看俩人的共同好友是谁
5. Sorted set 是排序的 Set，去重但可以排序，写进去的时候给一个分数，自动根据分数排序，Sorted set可以利用分数进行成员间的排序，例如视频网站需要对用户上传的视频做排行榜，榜单维护可能是多方面：按照时间、按照播放量、按照获得的赞数等

- **数据一致性的问题如何解决？**
- **如果有大量的key需要设置同一时间过期，一般需要注意什么？**   
1. 如果大量的key过期时间设置的过于集中，到过期的那个时间点，Redis可能会出现短暂的卡顿现象，严重的话会出现缓存雪崩
2. 我们一般需要在时间上加一个随机值，使得过期时间分散一些

- **redis分布式锁？**
- **Redis里面有1亿个key，其中有10w个key是以某个固定的已知的前缀开头的，如何将它们全部找出来？**
1. 使用keys指令可以扫出指定模式的key列表，但是redis是单线程，keys指令会导致线程阻塞一段时间，线上服务会停顿，直到指令执行完毕，服务才能恢复
2. scan指令，scan指令可以无阻塞的提取出指定模式的key列表，但是会有一定的重复概率，在客户端做一次去重就可以了，但是整体所花费的时间会比直接用keys指令长

- **Redis做异步队列么，你是怎么用的？**
1. 一般使用list结构作为队列，rpush生产消息，lpop消费消息。当lpop没有消息的时候，要适当sleep一会再重试
2. 也可以不sleep，list还有个指令叫blpop，在没有消息的时候，它会阻塞住直到消息到来
- **能不能生产一次消费多次呢？**
1. 使用pub/sub主题订阅者模式，可以实现 1:N 的消息队列
2. 但是订阅者模式在消费者下线的情况下，生产的消息会丢失，得使用专业的消息队列如RocketMQ等
- **Redis如何实现延时队列？**
1. 使用sortedset，拿时间戳作为score，消息内容作为key调用zadd来生产消息，消费者用zrangebyscore指令获取N秒之前的数据轮询进行处理

- **Redis是怎么持久化的？服务主从数据怎么交互的？**
- **Pipeline有什么好处，为什么要用pipeline？**
1. 可以将多次IO往返的时间缩减为一次，前提是pipeline执行的指令之间没有因果相关性
2. 使用redis-benchmark进行压测的时候可以发现影响redis的QPS峰值的一个重要因素是pipeline批次指令的数目

> [Redis，哨兵、持久化、主从、手撕LRU](https://blog.csdn.net/qq_35190492/article/details/102958250)   
> [Redis双写一致性、并发竞争、线程模型](https://blog.csdn.net/qq_35190492/article/details/103004235)   
> [《吊打面试官》系列- Redis基础](https://juejin.im/post/5db66ed9e51d452a2f15d833)   
> [Redis常见面试题总结](https://blog.csdn.net/qq_35190492/article/details/103041932)   
> [redis详解（三）-- 面试题](https://blog.csdn.net/guchuanyun111/article/details/52064870)   
> [面试中关于Redis的问题看这篇就够了](https://blog.csdn.net/qq_34337272/article/details/80012284)   
> [50道Redis面试题史上最全，以后面试再也不怕问Redis了](https://juejin.im/post/5cb13b4d6fb9a0687b7dd0bd)



---
## Redis 源码

> [超强、超详细Redis入门教程](https://blog.csdn.net/liqingtx/article/details/60330555)   
> [Redis源码从哪里读起？](http://zhangtielei.com/posts/blog-redis-how-to-start.html)   
> [Redis源码剖析](https://blog.csdn.net/xiejingfa/category_9273801.html)   
> [Redis源码](https://blog.csdn.net/androidlushangderen/category_2647211.html)


 

---

https://segmentfault.com/a/1190000013758197   
http://kaiyuan.me/2017/06/12/leveldb-05/   
https://leveldb-handbook.readthedocs.io/zh/latest/cache.html   
https://www.jianshu.com/p/d1e7efacc394   
https://www.jianshu.com/p/9e7773432772   
https://bean-li.github.io/leveldb-LRUCache/   

红黑树   
分布式一致性算法Raft