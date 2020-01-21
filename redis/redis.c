

sds字符串
struct sdshdr {
	//字符长度
    unsigned int len;
    //当前可用空间
    unsigned int free;
    //具体存放字符的buf
    char buf[];
};
sds和c字符串区别
常数复杂度获取字符串长度
杜绝缓冲区溢出 strcat扩容
减少修改字符串带来的内存分配次数(空间预分配和惰性空间释放)
空间预分配: 小于1MB, 多分配与len同样大小的未使用空间	大于1MB, 多分配1MB的未使用空间
惰性空间释放: 字符串收缩多余的的字节存放在free等待将来使用

list
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
双端链表
无环
多态: void*保存节点值

ziplist采用了一系列特殊编码的内存块组成顺序型数据结构, 整个ziplist其实是超级长的字符串, 通过里面各个结点的长度, 上一个结点的长度等信息, 通过快速定位实现相关操作
zlbytes---zltail---zllen---entryX---zlend
typedef struct zlentry {
	//prevrawlen为上一个数据结点的长度, prevrawlensize为记录该长度数值所需要的字节数
    unsigned int prevrawlensize, prevrawlen;
    //len为当前数据结点的长度, lensize表示表示当前长度表示所需的字节数
    unsigned int lensize, len;
    //数据结点的头部信息长度的字节数
    unsigned int headersize;
    //编码的方式保存数据类型和长度
    unsigned char encoding;
    //数据结点的数据(已包含头部等信息), 以字符串形式保存
    unsigned char *p;
} zlentry;

哈希表hash
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

链地址法解决hash冲突
rehash负载因子: used/size

dictScan反向二进制迭代器
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
	  +-----(5)1111

字典遍历迭代器:
安全迭代器: 禁止rehash(bgaofrewrite和bgsave指令需要遍历所有对象进行持久化, 不允许出现重复)
不安全迭代器: rehash中会发生元素重复遍历


set
当对象保存的所有元素都是整数值和对象保存的所有元素都是整数值用insert结构, 否则用hash table结构
typedef struct intset {
	// 编码方式
	uint32_t encoding;
	
    // 集合包含的元素数量
    uint32_t length;

    // 保存元素的数组
    int8_t contents[];

} intset;



sort set
跳表SkipList
二分法引出单链表随机生成一个结点指向后续结点

Redis中sorted set的使用
sorted set是一个有序的数据集合: zrevrank由数据查询它对应的排名, zscore由数据查询它对应的分数, zrevrange根据一个排名范围, 查询排名在这个范围内的数据, zrevrangebyscore根据分数区间查询数据集合


redis skiplist实现
当数据量少时, sorted set是由ziplist来实现的
当sorted set中的元素个数, 即(数据, score)对的数目超过128的时候或者当sorted set中插入的任意一个数据的长度超过了64, sorted set是由dict + skiplist来实现

typedef struct zskiplistNode {
    robj *obj;  	/*成员对象*/
    double score;   /*分值*/
    struct zskiplistNode *backward; /*后退指针*/
    struct zskiplistLevel { /*层*/
        struct zskiplistNode *forward;  /*前进指针*/
        unsigned int span;  /*跨度*/
    } level[];
} zskiplistNode;

skiplist和各种平衡树(如AVL、红黑树等)的元素是有序排列的, 而哈希表不是有序的, 在哈希表上只能做单个key的查找, 不适宜做范围查找
在做范围查找的时候, 平衡树比skiplist操作要复杂, 在平衡树上, 我们找到指定范围的小值之后, 还需要以中序遍历的顺序继续寻找其它不超过大值的节点
平衡树的插入和删除操作可能引发子树的调整, 逻辑复杂, 而skiplist的插入和删除只需要修改相邻节点的指针, 操作简单又快速
从内存占用上来说, skiplist比平衡树更灵活一些, 平衡树每个节点包含2个指针(分别指向左右子树), 而skiplist每个节点包含的指针数目平均为1/(1-p)
查找单个key, skiplist和平衡树的时间复杂度都为O(log n), 大体相当；而哈希表在保持较低的哈希值冲突概率的前提下, 查找时间复杂度接近O(1), 性能更高一些


Bitmaps
String类型上的一组面向bit操作的集合, 由于strings是二进制安全的blob, 并且它们的最大长度是512m, 所以bitmaps能最大设置 2^32 个不同的bit
Bitmaps的最大优点就是存储信息时可以节省大量的空间
Bitmaps使用场景: 各种实时分析和存储与对象ID关联的节省空间并且高性能的布尔信息
例如你想知道访问你的网站的用户的最长连续时间, 可以简单的用当前时间减去初始时间并除以 3600*24 (结果就是你的网站公开的第几天)当做这个bit的位置

redis过期时间策略和原理(懒汉式删除+定期删除)
redis Slave过期key等待Master发送del通知才会删除过期键, redis删除过期键时会del写入AOF文件
定时删除
为每一个设置过期时间的key创建一个定时器保证内存被尽快释放, 但造成CPU性能影响严重
懒汉式删除
key过期时间不删除, 通过key获取值检查是否过期(key->ttl>cur_time), 过期返回null并且删除key, 缺点: 若大量的key在超出超时时间后, 很久一段时间内, 都没有被获取过, 那么可能发生内存泄露
定期删除
每隔一段时间执行一次删除过期key操作(redis随机抽样REDIS_EXPIRELOOKUPS_PER_CRON, 如果失效主键占抽样个数比例超过25%, 则遍历删除过期key)


redis过期淘汰配置
volatile-lru: 在设置了过期时间的键空间中, 移除最近最少使用的key
allkeys-lru: 移除最近最少使用的key

volatile-random: 在设置了过期时间的键空间中, 随机移除一个键
allkeys-random ： 直接在键空间中随机移除一个键

volatile-ttl: 在设置了过期时间的键空间中, 有更早过期时间的key优先移除
noeviction: 不做过键处理, 只返回一个写操作错误


RDB(Redis DataBase)
SAVE(阻塞)	BGSAVE(子进程并发处理)
周期性保存和多少次修改保存(RDB是间隔一段时间进行持久化, 如果持久化之间redis发生故障, 会发生数据丢失, 所以这种方式更适合数据要求不严谨的时候)

AOF(Append-only-file)
appendfsync选项: 
always每个事件循环把aof_buf缓存写入同步AOF文件
everysec每隔一秒子线程对AOF文件同步
no由操作系统控制何时同步

AOF文件重写
BGREWRITEAOF
不需要对先有AOF文件进行任何读取分析, 通过读取服务器当前数据库状态实现, 用一条命令去记录键值对代替之前记录这个键值对的多条命令
实际避免缓冲区溢出, 重写AOF检查键包含元素个数, 超过REDIS_AOF_REWRITE_ITEMS_PER_CMD用多条命令记录

AOF后台重写
子进程带有服务器进程的数据副本执行AOF重写, 父进程处理客户端请求, 造成当前数据库状态和重写后AOF保存的数据看状态不一致
AOF后台重写开启后将父进程请求写入AOF重写缓冲区, 子进程完成AOF重写后, 父进程将AOF重写缓冲区写入新的AOF文件中

redis ae事件驱动
事件类型总共2个一个FileEvent, TimeEvent
事件模型I/O多路复用: select, poll, epoll

redis主从复制
完整同步: 从服务发送SYNC命令, 主服务器收到SYNC后生成RDB文件发送给从服务器, 并将新的redis命令写进缓存区, 从服务器收到RDB文件载入并执行主服务器缓冲区的命令
部分同步(处理断线后重复复制): 主从服务器分别维护一个复制偏移量, 主服务器维护一个固定长度的先进先出队列作为复制积压缓冲区, 从服务缺少的偏移量在复制积压缓冲区进行部分同步, 否则执行完整同步

redis事务
事务队列实现, WATCH命令是个乐观锁监视键是否被修改
事务因为命令入队出错被服务器拒绝执行, 事务中所有命令都不会执行
redis事务不支持回滚, 某条命令执行期间出现错误也会继续执行下去直到事务中所有命令执行完成

redis sentinel
连接: sentinel向主服务器创建两个异步网络连接, 一个是命令连接用于像主服务器发送命令接受回复, 另一个是订阅频道(sentinel频道不会丢失任何信息)
获取主服务器信息: sentinel默认每十秒向主服务器发送INFO命令, 返回的主服务器信息更新sentinel中主服务器的实例结构, 从服务器信息用于更新或创建sentinel中从服务器的实例结构并创建sentinel到从服务器命令连接和订阅连接
获取从服务器信息: sentinel默认每十秒向从服务器发送INFO命令, 返回的从服务器信息用于更新sentinel中从服务器的实例结构
创建sentinel之间的链接: sentinel每秒像主从服务器订阅频道发送sentinel本身信息和主从服务器信息, 其他sentinel接收到订阅频道信息后更新或创建sentinels结构并创建sentinel之间的命令连接
检测主观下线: sentinel每秒像所有创建连接的实例包括主从服务器其他sentinel发送PING命令, 在down-after-milliseconds没有收到有效回复则认为该服务主观下线
检查客观下线: sentinel将一个主服务器判定主观下线后, 向其他监视这个主服务器的sentinel发送询问, 从其他sentinel接收到足够数量下线状态, 判定为该主服务器为客观下线
选举领头sentinel: Raft算法
故障转移: 	选出从服务器: 过滤断线未回复的从服务器, 根据优先级高和复制偏移量大以及运行ID小的选出作为主服务器(slaveof no one)	
			修改从服务器的复制目标: 发送slaveof ip port
			将旧主服务器变为从服务器: 旧主服务器上线后发送slaveof ip port
			
redis集群




150逆波兰式(栈)
由相应的语法树的后序遍历的结果得到(A+B*(C-D)-E*F / A B C D - * + E F * -)


110平衡二叉树(递归最大深度)

116完美二叉树填充每个节点的下一个右侧节点指针(递归)

117二叉树填充每个节点的下一个右侧节点指针(层序遍历)

129求根到叶子节点数字之和(前序遍历递归)

199二叉树右视图(递归、层序遍历)

257二叉树所有路径(递归)



225队列实现栈
232栈实现队列

394字符串解码(编码规则k[encoded_string], 3[a]2[bc]返回aaabcbc, 辅助栈)




378有序矩阵中第K小的元素(二分查找)
230二叉搜索树第K小的元素(递归)


198打家劫舍I(输入[1,2,3,1]输出4)

198打家劫舍I
213打家劫舍II(I基础上环形数组)
337打家劫舍III


263丑数
264丑数II(找出第n个丑数 动态规划)
313超级丑数(超级丑数是指其所有质因数都是长度为k的质数列表primes中的正整数 动态规划)

239滑动窗口最大值(双端单调队列 动态规划)

55跳跃游戏I(动态规划 贪心算法)
45跳跃游戏II(贪心算法)

121买股票最佳时机I(输入[7,6,4,3,1]输出5)

[7,1,5,3,6,4]


54螺旋矩阵
59螺旋矩阵II 生成一个包含 1 到 n2 所有元素，且元素按顺时针顺序螺旋排列的正方形矩阵
885螺旋矩阵III 手搓螺旋丸

74搜索二维矩阵I 每行中的整数从左到右按升序排列, 每行的第一个整数大于前一行的最后一个整数(二分法)
240搜索二维矩阵II 每行的元素从左到右升序排列, 每列的元素从上到下升序排列





