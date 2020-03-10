EXISTS 和 NOT EXISTS
查询选修了所有课程的学生id, name (即这一个学生没有一门课程他没有选的)
select * from t_student ts where not exists
	 (select * from course c where not exists
  		(select * from select_course sc where sc.student_id=ts.id and sc.course_id=c.id))

查询没有选择所有课程的学生, 即没有全选的学生（存在这样的一个学生, 他至少有一门课没有选
select id,name from t_student where exists
	(select * from course where not exists
		(select * from select_course sc where student_id=t_student.id and course_id=course.id))

查询一门课也没有选的学生 (不存这样的一个学生, 他至少选修一门课程)
select id,name from t_student where not exists
	(select * from course where exists
		(select * from select_course sc where student_id=t_student.id and course_id=course.id))

查询至少选修了一门课程的学生
select id,name from t_student where exists
	(select * from course where  exists
		(select * from select_course sc where student_id=t_student.id and course_id=course.id))


mysql中的in语句是把外表和内表作hash连接, 而exists语句是对外表作loop循环, 每次loop循环再对内表进行查询

exists执行顺序
首先执行一次外部查询 --- 对于外部查询中的每一行分别执行一次子查询 --- 使用子查询的结果来确定外部查询的结果集
in执行顺序
子查询先产生结果集 --- 然后主查询再去结果集里去找符合要求的字段列表去
not exists的执行顺序
在表中查询, 是根据索引查询的, 如果存在就返回true, 如果不存在就返回false, 不会每条记录都去查询
not in的执行顺序
在表中一条记录一条记录的查询(查询每条记录)符合要求的就返回结果集, 也就是说为了证明找不到, 所以只能查询全部记录才能证明, 并没有用到索引

in适合外大内小, exists适合外小内大, not exists查询的效率远远高与not in查询的效率



// 索引的本质：索引是数据结构

// 顺序查找、二分查找、二叉排序树查找

// 平衡多路搜索树B树 B Tree


// 索引

// 代价
// 索引需要占物理空间, 除了数据表占数据空间之外, 每一个索引还要占一定的物理空间, 如果要建立聚簇索引, 那么需要的空间就会更大
// 当对表中的数据进行增加、删除和修改的时候, 索引也要动态的维护, 这样就降低了数据的维护速度

// 使用索引列
// 经常需要搜索的列
// 主键的列
// 经常用在连接的列
// 经常需要根据范围进行搜索的列上
// 经常需要排序的列上
// 经常使用在WHERE子句中的列


// 唯一索引、主键索引、聚集索引

// 唯一索引
// 不允许两行具有相同的索引值

// 主键索引
// 主键索引是唯一索引的特殊类型


// 唯一索引和主键索引区别
// 主健可作外健, 唯一索引不可
// 主健不可为空, 有not null属性, 唯一索引可
// 主健也可是多个字段的组合
// 每个表只能有一个主健

// 聚集索引
// 在聚集索引中, 表中行的物理顺序与键值的逻辑(索引)顺序相同, 一个表只能包含一个聚集索引

// 覆盖索引
// 如果索引包含所有满足查询需要的数据的索引成为覆盖索引, 不需要回表操作
// 如果要使用覆盖索引, 一定要注意SELECT 列表值取出需要的列, 不可以是SELECT *, 因为如果将所有字段一起做索引会导致索引文件过大, 查询性能下降

// 哈希索引: 等值查询效率高, 不能排序, 不能进行范围查询
// 联合主键primary key(a, b)	索引key(c, a)没有必要	InnoDB索引最后会使用联合主键索引


// 局部性原理与磁盘预读
// 当一个数据被用到时, 其附近的数据也通常会马上被使用

// B-/+Tree索引的性能分析
// 将一个节点的大小设为等于一个页, 这样每个节点只需要一次I/O就可以完全载入, 每次新建节点时, 直接申请一个页的空间, 这样就保证一个节点物理上也存储在一个页里
// 一般实际应用中, 出度d是非常大的数字, 通常超过100, 因此h非常小(通常不超过3)

// B树
// 每个结点最多有m-1个关键字
// 非根结点至少有Math.ceil(m/2)-1个关键字
// 每个结点中的关键字都按照从小到大的顺序排列, 每个关键字的左子树中的所有关键字都小于它, 而右子树中的所有关键字都大于它
// 每个结点中存储了关键字(key)和关键字对应的数据(data), 以及孩子结点的指针
// B树中每个节点包含了键值和键值对于的数据对象存放地址指针, 所以成功搜索一个对象可以不用到达树的叶节点
// 成功搜索包括节点内搜索和沿某一路径的搜索, 成功搜索时间取决于关键码所在的层次以及节点内关键码的数量

// B+树
// 非叶子结点中含有m个关键字(b树是m-1个)
// 中间节点不含有实际数据, 只有子树的最大数据和子树指针, 因此磁盘页中可以容纳更多节点元素
// 查找必会查到叶子节点, 更加稳定
// B+树非叶节点中存放的关键码并不指示数据对象的地址指针, 非叶节点只是索引部分
// 所有的叶节点在同一层上, 包含了全部关键码和相应数据对象的存放地址指针, 且叶节点按关键码从小到大顺序链接

// 带有顺序访问指针的B+Tree
// 在B+Tree的每个叶子节点增加一个指向相邻叶子节点的指针, 就形成了带有顺序访问指针的B+Tree



// MyISAM索引(非聚集索引)
// 叶节点的data域存放的是数据记录的地址
// MyISAM索引文件和数据文件是分离的, 索引文件仅保存数据记录的地址
// 在MyISAM中, 主索引和辅助索引(Secondary key)在结构上没有任何区别, 只是主索引要求key是唯一的, 而辅助索引的key可以重复

// InnoDB索引(聚集索引)
// 叶节点data域保存了完整的数据记录
// InnoDB的数据文件本身要按主键聚集, 所以InnoDB要求表必须有主键
// InnoDB的所有辅助索引都引用主键作为data域


// explain
// table | type | possible_keys | key | key_len | ref | rows | Extra
// type: 显示了连接使用了哪种类别, 有无使用索引 const、eq_reg、ref、range、indexhe、ALL
// system: 表只有一行, const连接类型的特殊情况
// const: 表中的一个记录的最大值能够匹配这个查询(索引可以是主键或惟一索引), 因为只有一行, 这个值实际就是常数, 因为MYSQL先读这个值然后把它当做常数来对待
// eq_ref: 在连接中, MYSQL在查询时, 从前面的表中, 对每一个记录的联合都从表中读取一个记录, 它在查询使用了索引为主键或惟一键的全部时使用
// ref: 这个连接类型只有在查询使用了不是惟一或主键的键或者是这些类型的部分(比如, 利用最左边前缀)时发生, 对于之前的表的每一个行联合, 全部记录都将从表中读出, 这个类型严重依赖于根据索引匹配的记录多少—越少越好
// range: 这个连接类型使用索引返回一个范围中的行, 比如使用>或<查找东西时发生的情况
// index: 这个连接类型对前面的表中的每一个记录联合进行完全扫描(比ALL更好, 因为索引一般小于表数据)
// ALL: 这个连接类型对于前面的每一个记录联合进行完全扫描, 这一般比较糟糕, 应该尽量避免

// possible_keys: 显示可能应用在这张表中的索引
// key: 表明实际使用的索引, 很少的情况下, MYSQL会选择优化不足的索引。这种情况下, 可以在SELECT语句中使用USE INDEX（indexname）来强制使用一个索引或者用IGNORE INDEX（indexname）来强制MYSQL忽略索引
// key_len: 使用的索引的长度, 在不损失精确性的情况下, 长度越短越好
// ref: 显示索引的哪一列被使用了, 如果可能的话是一个常数 
// rows: MYSQL认为必须检查的用来返回请求数据的行数

// extra: 列返回的描述的意义
// Distinct: 一旦mysql找到了与行相联合匹配的行, 就不再搜索了
// Not exists: mysql优化了LEFT JOIN, 一旦它找到了匹配LEFT JOIN标准的行, 就不再搜索了
// Range checked for each Record: 没有找到理想的索引, 因此对从前面表中来的每一个行组合, mysql检查使用哪个索引, 并用它来从表中返回行, 这是使用索引的最慢的连接之一
// Using filesort: 查询就需要优化了, mysql需要进行额外的步骤来发现如何对返回的行排序, 根据连接类型以及存储排序键值和匹配条件的全部行的行指针来排序全部行
// Using index: 列数据是从仅仅使用了索引中的信息而没有读取实际的行动的表返回的, 这发生在对表的全部的请求列都是同一个索引的部分的时候
// Using temporary: 查询需要优化了, mysql需要创建一个临时表来存储结果, 这通常发生在对不同的列集进行ORDER BY上, 而不是GROUP BY上
// Where used: 使用了WHERE从句来限制哪些行将与下一张表匹配或者是返回给用户, 如果不想返回表中的全部行, 并且连接类型ALL或index, 这就会发生, 或者是查询有问题


// explain  select author_id from "article" where category_id = 1 and comments > 1 order by views desc limit 1\G
// index(category_id, comments, views)		type: range/Extra: Using filesort	(comments使用了范围索引所以views没有使用索引)
// index(category_id, views)				type: ref/Extra: Using where

// explain select * from class left join book on class.card = book.card\G
// book index(card)								type: ref/Extra:
// class index(card)								type: ALL/Extra:


// 联合索引
// 最左前缀原理


// 索引失效
// 最左前缀匹配(中间索引不要断), 如果出现范围查找(>、<、between、like)就不能进一步命中
// 不要在索引上做任何操作(计算、函数、自动/手动类型转换) where id + 1 = ? 改成 where id = 1 - ?
// 尽量使用覆盖索引, 减少使用select*
// 索引字段使用!=会导致索引失效
// 索引字段使用is null / is not null 判断时, 会导致索引失效
// 使用like以通配符开头(%'字符串')会导致索引失效
// 索引字段使用or时, 会导致索引失效
















锁

MyISAM和InnoDB都支持表锁, InnoDB还支持事务和行锁

MyISAM表锁
show status like 'table%'可以通过检查table_locks_waited和table_locks_immediate状态变量来分析系统上的表锁定争夺
MySQL的表级锁有两种模式: 表共享读锁(Table Read Lock)和表独占写锁(Table Write Lock)

读锁
对MyISAM表的读操作, 不会阻塞其他用户对同一表的读请求, 但会阻塞对同一表的写请求
写锁
对MyISAM表的写操作, 则会阻塞其他用户对同一表的读和写操作

如何加表锁
在用LOCK TABLES READ/WRITE给表显式加表锁时, 必须同时取得所有涉及到表的锁, 并且MySQL不支持锁升级
在执行LOCK TABLES后, 只能访问显式加锁的这些表, 不能访问未加锁的表；同时, 如果加的是读锁, 那么只能执行查询操作, 而不能执行更新操作
使用LOCK TABLES时, 不仅需要一次锁定用到的所有表, 而且, 同一个表在SQL语句中出现多少次, 就要通过与SQL语句中相同的别名锁定多少次, 否则也会出错

并发插入
MyISAM表的读和写是串行的, MyISAM表也支持查询和插入操作的并发进行
MyISAM存储引擎有一个系统变量concurrent_insert, 专门用以控制其并发插入的行为, 其值分别可以为0、1或2
*当concurrent_insert设置为0时, 不允许并发插入
*当concurrent_insert设置为1时, 如果MyISAM表中没有空洞(即表的中间没有被删除的行), MyISAM允许在一个进程读表的同时, 另一个进程从表尾插入记录。这也是MySQL的默认设置
*当concurrent_insert设置为2时, 无论MyISAM表中有没有空洞, 都允许在表尾并发插入记录


MyISAM的锁调度
写进程先获得锁, 大量的更新操作会造成查询操作很难获得读锁, 从而可能永远阻塞




InnoDB锁
支持事务和采用行级锁

共享锁
允许一个事务去读一行, 阻止其他事务获得相同数据集的排他锁(LOCK IN SHARE MODE)
排他锁
允许获得排他锁的事务更新数据, 阻止其他事务取得相同数据集的共享读锁和排他写锁(FOR UPDATE)

InnoDB行锁实现方式
InnoDB行锁是通过给索引上的索引项加锁来实现的, 这意味着: 只有通过索引条件检索数据, InnoDB才使用行级锁, 否则, InnoDB将使用表锁, 可能导致大量的锁冲突, 从而影响并发性能
MySQL的行锁是针对索引加的锁, 不是针对记录加的锁, 所以虽然是访问不同行的记录, 但是如果是使用相同的索引键, 是会出现锁冲突的
当表有多个索引的时候, 不同的事务可以使用不同的索引锁定不同的行

乐观锁和悲观锁都是针对读(select)来说的

乐观锁
对加锁持有一种乐观的态度, 即先进行业务操作, 不到最后一步不进行加锁
一般是在表添加version版本字段或者timestamp时间戳字段

悲观锁
对数据加锁持有一种悲观的态度, 整个数据处理过程中, 将数据处于锁定状态
悲观锁实际使用了排他锁来实现(select **** for update)

幻读原因: 行锁只能锁住行, 但是新插入记录这个动作, 要更新的是记录之间的间隙(对于键值在条件范围内但并不存在的记录, 叫做间隙)
间隙锁(next-key lock)
间隙锁可能导致同样的语句死锁, 影响并发度
防止幻读    在Repeatable read级别下且必须有索引使用间隙锁
MySQL数据库的恢复和主从复制
MySQL的恢复机制: 
MySQL的恢复是SQL语句级的, 也就是重新执行BINLOG中的SQL语句
MySQL的Binlog是按照事务提交的先后顺序记录的, 恢复也是按这个顺序进行的
MySQL的恢复机制要求: 在一个事务未提交前, 其他并发事务不能插入满足其锁定条件的任何记录, 也就是不允许出现幻读
逻辑备份的时候, mysqldump要把备份线程设置成可重复读

死锁
在应用中, 如果不同的程序会并发存取多个表, 应尽量约定以相同的顺序来访问表, 这样可以大大降低产生死锁的机会
在程序以批量方式处理数据的时候, 如果事先对数据排序, 保证每个线程按固定的顺序来处理记录, 也可以大大降低出现死锁的可能
在REPEATABLE-READ隔离级别下, 如果两个线程同时对相同条件记录用SELECT...FOR UPDATE加排他锁, 在没有符合该条件记录情况下, 两个线程都会加锁成功(两个线程都会加间隙锁, 间隙锁之间不会相互阻塞)。程序发现记录尚不存在, 就试图插入一条新记录, 如果两个线程都这么做, 就会出现死锁(一个线程会先获取插入意向间隙锁与另一个线程间隙锁是冲突而阻塞, 另一个线程同样也会先获取插入意向间隙锁, MySQL检测到死锁回滚)
当隔离级别为READ COMMITTED时, 如果两个线程都先执行SELECT...FOR UPDATE, 判断是否存在符合条件的记录, 如果没有, 就插入记录。此时, 只有一个线程能插入成功, 另一个线程会出现锁等待, 当第1个线程提交后, 第2个线程会因主键重出错, 但虽然这个线程出错了, 却会获得一个排他锁！这时如果有第3个线程又来申请排他锁, 也会出现死锁, 对于这种情况, 可以直接做插入操作, 然后再捕获主键重异常, 或者在遇到主键重错误时, 总是执行ROLLBACK释放获得的排他锁

死锁策略
直接进入等待到超时, innodb_lock_wait_timeout设置超时时间(时间长不可接受, 时间短误伤)
发起死锁检测, 发现死锁后主动回滚死锁链条中某个事务, 让其他事务可以执行, 将参数innodb_deadlock_detect设置为on(死锁检测耗费大量CPU资源)
控制并发度减少死锁: 考虑通过将一行改成逻辑上的多行来减少锁冲突(影院账户分成10个记录, 随机选其中一条记录做处理)




// 事务的四种隔离级别
// https://www.cnblogs.com/ubuntu1/p/8999403.html
// 事务原理
// https://www.jianshu.com/p/62ccfa64fed1

// 事务
// 原子性、一致性、隔离性、持久性

// 事务隔离
// Read uncommitted
// 脏读问题: 一个事务可以读取另一个未提交事务的数据(程序员看到老板还没提交事务时的误操作工资3.9w)

// Read committed
// 不可重复读: 事务开启, 允许其他事务的UPDATE修改操作, 一个事务范围内两个相同的查询却返回了不同数据(程序员看到卡里3.6w余额准备享受生活, 这时妻子把钱全部转出充当家用并提交, 程序员收费系统准备扣款时再检测卡里的金额发现已经没钱了)

// Repeatable read
// 可重复读: 事务A在读到一条数据之后，此时事务B对该数据进行了修改并提交，那么事务A再读该数据，读到的还是原来的内容
// 幻读: 事务开启, 允许其他事务的INSERT插入操作(妻子查看程序员消费2千元打印清单时发现花了1.2万元)

// Serializable
// 事务串行化顺序执行


// MVCC使用及其原理、实现机制
// https://blog.csdn.net/w2064004678/article/details/83012387
// https://blog.csdn.net/whoamiyang/article/details/51901888
// https://www.cnblogs.com/Allen-win/p/8283102.html

// MVCC(多版本并发控制机制)
// 作用: 事务型存储引擎使用MVCC可以在大多数情况下代替行级锁能降低其系统开销, 实现了非阻塞的读操作, 写操作也只锁定了必要的行, MVCC主要适用于Mysql的RC,RR隔离级别
// 实现: 通过保存数据在某个时间点的快照来实现的, 每行记录后面保存创建时系统版本号, 行删除时系统版本号
// insert: 为新插入的每一行保存当前系统版本号作为版本号
// select: 同时满足以下两个条件才会返回
// 只会查找版本早于当前事务版本的数据行(行的系统版本号小于或等于事务的系统版本号), 这样可以确保事务读取的行, 要么是在事务开始前已经存在的, 要么是事务自身插入或者修改过的
// 行的删除版本要么未定义,要么大于当前事务版本号,这可以确保事务读取到的行, 在事务开始之前未被删除
// delete: 删除的每一行保存当前系统的版本号(事务的ID)作为删除标识
// update: 实际上是新插入了一行记录, 并保存其创建时间为当前事务的ID, 同时保存当前事务ID到要UPDATE的行的删除时间


// 事务实现原理: MVCC、锁
// 一致性非锁定读:
// 在read committed隔离级别下, 一致性非锁定读总是读取被锁定行的最新一份快照数据, 产生了不可重复读的问题
// 在repeatable read 事务隔离级别下, 一致性非锁定读总是读取事务开始时的行数据版本, 解决不可重复读的问题
// 一致性锁定读:
// 在read committed隔离级别下, 读操作不加锁, 写操作加锁, 读事务每次都读最新版本, 产生了不可重复读的问题
// 在repeatable read 事务隔离级别下, 第一次读数据的时候就将数据加行锁(共享锁), 使其他事务不能修改当前数据, 即可实现可重复读, 但是不能锁住insert进来的新的数据, 当前事务读取或者修改的同时, 另一个事务还是可以insert提交, 造成幻读


// redo log
// redo log记录的是对数据页更改的物理日志
// redo log就是保存执行的SQL语句到一个指定的Log文件, 当Mysql执行recovery时重新执行redo log记录的SQL操作即可
// 当客户端执行每条SQL(更新语句)时, redo log会被首先写入log buffer, 当客户端执行COMMIT命令时, log buffer中的内容会被视情况刷新到磁盘, redo log在磁盘上作为一个独立的文件存在, 即Innodb的log文件

// undo log
// 与redo log相反, undo log是为回滚而用, 具体内容就是copy事务前的数据库内容(行)到undo buffer, 在适合的时间把undo buffer中的内容刷新到磁盘

// Innodb的事务回滚实现方式
// 事务以排他锁的形式修改原始数据
// 表的内容保存指向undo日志的指针,  把修改前的数据存放于undo log, rollback根据这个指针来获取就数据并覆盖新数据
// 修改成功(commit)啥都不做, 失败则恢复undo log中的数据(rollback)












MYSQL uuid	MYSQL数据保持一致
在备份期间, 备份线程用的是可重复读, 而业务线程用的是读提交。同时存在两种事务隔离级别, 会不会有问题？
如果把隔离级别设置为读提交的话, 就没有间隙锁了。但同时, 你要解决可能出现的数据和日志不一致问题, 需要把binlog格式设置为row。这, 也是现在不少公司使用的配置组合








// MYSQL分为server层和存储引擎层
// server层: 连接器、查询缓存、分析器、优化器、执行器, 涵盖MYSQL大多数核心服务功能以及所有的内置函数(日期、时间数学和加密函数等), 所有跨存储引擎的功能都在这一层实现如存储过程、触发器、视图
// 存储层: 负责数据存储和提取, 插件式支持InnoDB、MyISAM多个存储引擎

// 连接器: 跟客户端连接获取维持和管理连接
// 查询缓存: 静态表适合查询缓存
// 分析器: 词法分析和语法分析
// 优化器: 决定使用哪个索引, 多表关联(join)决定各个表的连接顺序
// 执行器: 遍历引擎接口返回表的每一行, 如果符合条件则存在结果集中, 遍历完成返回结果集

// 日志:
// crash-safe保证:
// 如果客户端收到事务成功的消息, 事务就一定持久化了
// 如果客户端收到事务失败（比如主键冲突、回滚等）的消息, 事务就一定失败了
// 如果客户端收到"执行异常"的消息, 应用需要重连后通过查询当前状态来继续后续的逻辑, 此时数据库只需要保证内部(数据和日志之间, 主库和备库之间)一致就可以了

// redo log(重做日志): 
// InnoDB引擎的日志, redo log保证数据库异常重启之前提交的记录不会丢失(crash-safe), WAL技术(先写日志再写磁盘) 
// redo log是物理日志, 记录的是在某个数据页上做了什么修改
// redo log是循环写, 空间固定会用完

// binlog(归档日志): server层日志, 记录了MySQL对数据库执行更改的所有操作, 没有crash-safe能力
// binlog是逻辑日志, 记录的是这个语句的原始逻辑
// binlog采用追加写的模式
// bin log 三种模式
// statement格式: 记录sql语句, 有些语句的执行结果是依赖于上下文命令可能会导致主备不一致(delete带limit, 很可能会出现主备数据不一致的情况)
// row格式: 非常清楚的记录下每一行数据修改的细节(可能会产生大量的日志内容)
// mixed格式: MySQL自己会判断这条SQL语句是否可能引起主备不一致, 如果有可能, 就用row格式, 否则就用statement格式

// 数据恢复到某个时间点的状态: 找到最近的一次全量备份, 从这个备份binlog恢复到某个时间点的状态
// 两段提交保证数据库binlog状态和日志redo log恢复出来的数据库状态保持一致


// 									 时刻A			 时刻B
// 两段提交: 写入redo log处于prepare阶段 --- 写入bin log --- 提交事务处于commit状态
// 时刻A崩溃恢复: redo log未提交, bin log未写, 不会传到备库, 这时事务会回滚
// 时刻B崩溃恢复: 如果redo log事务完整有commit标识则直接提交
// 			   如果redo log事务只有完整的prepare, 则判断对应事务bin log是否完整, 是提交事务, 否则回滚事务
			   
// bin log完整性判断:
// statement格式最后有commit
// row格式最有有一个XID event
// redo log 和 bin log关联
// 有一个共同字段XID

// 两段提交commit设计目的: 数据与备份一致性
// 在时刻B写完bin log发生崩溃, 这时候bin log已经写入, 之后会被从库(或者用这个bin log恢复出来的库)使用, 主库提交这个事务, 保证主从数据一致性

// redo log写完再写bin log
// 事务持久性问题: redo log提交完成不能回滚(如果还允许回滚就可能覆盖别的事务更新), 如果redo log直接提交, bin log写入失败, InnoDB又不可回滚, 数据和bin log日志不一致








redo log大小设置:
设置太小: 日志写满自动切换另外一个日志, 而且触发数据库的检查点checkpoint, 导致InnoDB缓存脏页小批量刷新, 降低InnoDB性能
设置太大: 减少了checkpoint提高了I/O性能, 意外宕机恢复提交的事务需要时间很长


业务题:
业务上有这样的需求, A、B 两个用户, 如果互相关注, 则成为好友, 但是如果 A、B 同时关注对方, 会出现不会成为好友的情况, 即使使用了排他锁也不行, 因为记录不存在, 行锁无法生效
设计like表里的数据保证 user_id < liker_id, 这样不论是 A 关注 B, 还是 B 关注 A, 在操作"like"表的时候, 如果反向的关系已经存在, 就会出现行锁冲突


// 全局锁(FTWRL):
// 整个库处于只读状态, 其他线程数据更新、数据定义、事务提交会被阻塞
// 使用场景: 全库逻辑备份
// 官方自带的逻辑备份工具是 mysqldump, 当 mysqldump 使用参数–single-transaction 的时候, 导数据之前就会启动一个事务, 来确保拿到一致性视图, 而由于 MVCC 的支持, 这个过程中数据是可以正常更新的
// 一致性读是好, 但前提是引擎要支持这个隔离级别, MyISAM不支持事务引擎


// 表级元数据锁(DML):
// DML会直到事务提交才释放, 在做表结构变更的时候, 你一定要小心不要导致锁住线上查询和更新



// 事务与MVCC
// 快照读: InnoDB引擎下是基于undo log(undo log的存在解决了两个问题, 一是数据回滚, 二是实现了MVCC), 快照读语句: select
// RC快照读遵循规则: 优先读取当前事务修改的数据, 其次读取最新已提交数据
// RR快照读遵循规则: 优先读取当前事务修改的数据, 其次读取小于当前事务id的最新一条已提交数据, 通过这样的机制, 保证了快照读的可重复读, 但读取到的数据很可能已经过期了
// 当前读: 读取的是最新已提交数据, 并且都会加行锁, 当前读语句: select ... lock in share mode, select ... for update, insert, update, delete
// RC当前读: 事务A select ... for update, 事务B insert 可以成功执行, 会造成幻读
// RR当前读: 事务A select ... for update, 事务B insert 会锁等待(RR隔离级别下, 会对上下两个数据间隙加间隙锁)
// 业务题: 事务隔离级别是可重复读RR, 把字段c和id值相等的行的c值清零, 发生改不掉的情况, 构造出这种情况并说明原理

// 普通索引和唯一索引的选择:
// 查询:
// 普通索引查到满足条件的第一个记录后, 继续查找下一个记录, 直到第一个不满足条件的记录
// 唯一索引, 由于索引唯一性, 查到第一个满足条件的记录后停止检索, 但是两者的性能差距微乎其微, 因为InnoDB根据数据页来读写的
// 更新:
// change buf: 如果数据页在内存中(buffer pool中时)就直接更新, 否则InooDB会将这些更新操作缓存在change buffer中, 在下次查询需要访问这个数据页时, 将数据页读入内存, 然后执行 change buffer 中与这个页有关的操作
// merge: 将 change buffer 中的操作应用到原数据页, 得到最新结果, 避免大量的磁盘随机访问I/O
// 唯一索引的更新就不能使用 change buffer：对于唯一索引, 所有的更新操作都要先判断这个操作是否违反唯一性约束, 那么必须将数据页读入内存才能判断
// change buf适用场景: 一个对于写多读少的业务来说, change buffer记录的变更越多越划算, 例如账单类日志类
redo log 主要节省的是随机写磁盘的I/O消耗(转成顺序写), 而change buffer主要节省的则是随机读磁盘的I/O消耗(将更新操作先记录在change buffer, 减少读磁盘)

MySQL选错索引:
对于由于索引统计信息不准确导致的问题, 你可以用analyze table来解决
对于其他优化器误判的情况, 你可以在应用端用force index来强行指定索引





高效编程
运算符和数据库函数
多条件表达式(AND, OR, NOT): 有运算符优先顺序, 用括号避免
结果排序: order by(ASC, DESC), MYSQL把NULL当做最小值
数据分组: group by 主要统计函数(AVG, COUNT, MAX, MIN, SUM)


DML语句
truncate, delete, drop区别
truncate 和 delete 只删除数据不删除表的结构(定义), drop 语句将删除表的结构被依赖的约束、触发器、索引
delete 语句是数据库操作语言(DML)这个操作会放到 rollback segement 中, 事务提交之后才生效, truncate、drop 是数据库定义语言(DDL), 操作立即生效, 原数据不放到 rollback segment 中, 不能回滚
delete 语句不影响表所占用的 extent, 高水线(high watermark)保持原位置不动, drop 语句将表所占用的空间全部释放, truncate 语句缺省情况下见空间释放到 minextents个 extent, 除非使用reuse storage
速度一般来说: drop> truncate > delete
小心使用 drop 和 truncate, 尤其没有备份的时候, 否则哭都来不及, 想删除部分数据行用 delete, 注意带上where子句

多表连接
内连接: inner join on 两个表的交集 (select * from a_table a inner join b_table b on a.a_id = b.b_id)
左连接: left join on  左表记录+右表符合搜索的记录 (select * from a_table a left join b_table b on a.a_id = b.b_id)
右连接: left join on  右表记录+左表符合搜索的记录 (select * from a_table a right join join b_table b on a.a_id = b.b_id)
完全连接: full join 在内连接的基础上包含两个表中所有不符合条件的数据行(MYSQL不支持)
交叉连接: cross join 笛卡尔积

数据检索
exists

EXISTS 和 NOT EXISTS
查询选修了所有课程的学生id, name (即这一个学生没有一门课程他没有选的)
select * from t_student ts where not exists
	 (select * from course c where not exists
  		(select * from select_course sc where sc.student_id=ts.id and sc.course_id=c.id))

查询没有选择所有课程的学生, 即没有全选的学生（存在这样的一个学生, 他至少有一门课没有选)
select id,name from t_student where exists
	(select * from course where not exists
		(select * from select_course sc where student_id=t_student.id and course_id=course.id))

查询一门课也没有选的学生 (不存这样的一个学生, 他至少选修一门课程)
select id,name from t_student where not exists
	(select * from course where exists
		(select * from select_course sc where student_id=t_student.id and course_id=course.id))

查询至少选修了一门课程的学生
select id,name from t_student where exists
	(select * from course where  exists
		(select * from select_course sc where student_id=t_student.id and course_id=course.id))


mysql中的in语句是把外表和内表作hash连接, 而exists语句是对外表作loop循环, 每次loop循环再对内表进行查询

exists执行顺序
首先执行一次外部查询 --- 对于外部查询中的每一行分别执行一次子查询 --- 使用子查询的结果来确定外部查询的结果集
in执行顺序
子查询先产生结果集 --- 然后主查询再去结果集里去找符合要求的字段列表去
not exists的执行顺序
在表中查询, 是根据索引查询的, 如果存在就返回true, 如果不存在就返回false, 不会每条记录都去查询
not in的执行顺序
在表中一条记录一条记录的查询(查询每条记录)符合要求的就返回结果集, 也就是说为了证明找不到, 所以只能查询全部记录才能证明, 并没有用到索引

in适合外大内小, exists适合外小内大, not exists查询的效率远远高与not in查询的效率

表维护和改造
改变列的数据类型
int（20）中20的涵义最大显示宽度为20, 但仍占4字节存储, 存储范围不变
char、varchar、text设计
char(n)和varchar(n(中括号中n代表字符的个数, 并不代表字节个数, 所以当使用了中文的时候(UTF8)意味着可以插入m个中文, 但是实际会占用m*3个字节
同时char和varchar最大的区别就在于char不管实际value都会占用n个字符的空间, 而varchar只会占用实际字符应该占用的空间+1, 并且实际空间+1<=n
超过char和varchar的n设置后, 字符串会被截断
char的上限为255字节, varchar的上限65535字节, text的上限为65535
char在存储的时候会截断尾部的空格, varchar和text不会
varchar会使用1-3个字节来存储长度, text不会

char: 存定长, 速度快, 存在空间浪费的可能, 会处理尾部空格, 上限255
varchar: 存变长, 速度慢, 不存在空间浪费, 不处理尾部空格, 上限65535, 但是有存储长度实际65532最大可用
text: 存变长大数据, 速度慢, 不存在空间浪费, 不处理尾部空格, 上限65535, 会用额外空间存放数据长度, 顾可以全部使用65535

复制表
表列构造+数据复制: create table customH select *from custom
表列构造复制: create table customG like custom
数据复制: insert into customG select *from custom

视图
视图(view)是一种虚拟存在的表, 是一个逻辑表, 本身并不包含数据, 视图将我们不需要的数据过滤掉, 将相关的列名用我们自定义的列名替换, 为了保障数据安全性, 提高查询效率

存储
一组可编程的函数, 是为了完成特定功能的SQL语句集, 经编译创建并保存在数据库中, 用户可通过指定存储过程的名字并给定参数(需要时)来调用执行

触发器
事先为某张表绑定一段代码, 当表中的某些内容发生改变(增、删、改)的时候, 系统会自动触发代码并执行, 触发时间分为事件类型前和后








MySQL的复制原理以及流程
从: 连接master, 并且请求某个binlog, position之后的内容
主: 接受到slave发送来的日志请求信息, 将binlog内容和position返回给slave线程
从: 收到binlog内容后写入relaylog日志
从: 监控relaylog日志是否更新, 有则执行sql语句

读延迟如何解决?



MySQL中MyISAM与InnoDB的区别
InnoDB支持事物, 	MyISAM不支持事物
InnoDB支持行级锁, 	MyISAM支持表级锁
InnoDB支持MVCC, 	MyISAM不支持
InnoDB支持外键, 	MyISAM不支持
InnoDB不支持全文索引, MyISAM支持

外键: 一张表中有一个非主键的字段指向另外一张表的主键, 保持数据一致性, 完整性, 主要目的是控制存储在外键表中的数据
foreign key(外键字段) + references + 外部表名(主键字段)  alter table + 表名 + add[constraint + 外键名字] + foreign key(外键字段) + references + 外部表名(主键字段)


InnoDB引擎的4大特性
插入缓冲提升插入性能
只对于非聚集索引(非唯一)的插入和更新有效, 对于每一次的插入不是写到索引页中, 而是先判断插入的非聚集索引页是否在缓冲池中, 如果在则直接插入；若不在, 则先放到Insert Buffer 中, 再按照一定的频率进行合并操作
只对于非聚集索引(非唯一)的插入和更新有效的限制: 聚集索引肯定是唯一的, 判断唯一性得到索引页中获取判断, 每次插入都得进行IO操作

两次写提高可靠性
部分写失效: 当数据库正在从内存向磁盘写一个数据页时, 数据库宕机, 从而导致这个页只写了部分数据, 这就是部分写失效, 它会导致数据丢失, 无法通过重做日志恢复的, 因为重做日志记录的是对页的物理修改, 如果页本身已经损坏, 重做日志也无能为力
原理: 
当刷新缓冲池脏页时, 并不直接写到数据文件中, 而是先拷贝至内存中的两次写缓冲区
接着从两次写缓冲区分两次写入磁盘共享表空间中, 每次写入1MB
待第2步完成后, 再将两次写缓冲区写入数据文件
部分写失效的问题解决: 在磁盘共享表空间中已有数据页副本拷贝, 如果数据库在页写入数据文件的过程中宕机, 在实例恢复时, 可以从共享表空间中找到该页副本, 将其拷贝覆盖原有的数据页, 再应用重做日志即可

自适应哈希索引
实时监控表上索引的使用情况, 如果认为建立哈希索引可以提高查询效率, 则自动在内存中的自适应哈希索引缓冲区建立哈希索引

预读
线性预读: 将下一个extent提前读取到buffer pool中
随机预读: 随机预读着眼于将当前extent中的剩余的page提前读取到buffer pool中


MySQL数据库cpu飙升到500%的话他怎么处理
列出所有进程  show processlist  观察所有进程  多秒没有状态变化的(干掉)
查看超时日志或者错误日志


count(*)
MyISAM把一个表的总行数存在了磁盘上, 因此执行count(*)的时候会直接返回, 效率高
InnoDB需要把数据一行一行地从引擎中读取出来, 然后累计计数, InnoDB事务设计是MVCC实现的, count(*)请求需要判断每一行记录都要判断自己是否对这个会话可见
基于InnoDB引擎的count()语义:
count(字段): 一行行地从记录里面读出这个字段, 判断不能为null, 按行累加
count(主键id): InnoDB 引擎会遍历整张表, 把每一行的id值都取出来, 返回给server 层, server 层拿到id后, 判断是不可能为空的, 就按行累加(引擎返回id会涉及到解析数据行, 以及拷贝字段值的操作)
count(1): InnoDB 引擎遍历整张表, 但不取值, server 层对于返回的每一行, 放一个数字"1"进去, 判断是不可能为空的, 按行累加
count(*): 并不会把全部字段取出来, 而是专门做了优化, 不取值, count(*)肯定不是 null, 按行累加

order by工作原理
全字段排序: sort_buffer_size, 就是MySQL为排序开辟的内存(sort_buffer)的大小, 如果要排序的数据量小于sort_buffer_size, 排序就在内存中完成, 但如果排序数据量太大, 内存放不下, 则不得不利用磁盘临时文件辅助排序
rowid排序(排序的单行长度太大): 排序过程中一次可以排序更多行, 但是需要再回到原表去取数据

查询一行仍很慢问题排查
等DML锁: 使用 show processlist 命令查看 Waiting for table metadata lock 
等行锁: sys.innodb_lock_waits 表查到
没有走索引: slow log
一致性读: 事务update100w次生成100w回滚日志undo log, select一致性读需要执行100w次得到一开始的事务结果


自增id:
自增id的存储:
MyISAM 引擎的自增值保存在数据文件中
InnoDB 引擎的自增值, 其实是保存在了内存里, 重启第一次打开表会找自增的最大值max(id), 并且到了MySQL 8.0 版本后, 才有了自增值记录在redo log持久化的能力
自增id不连续现象: 唯一键冲突和事务回滚

join
Index_Nested-Loop_Join(NLJ): 遍历t1表, 取出t1表中每行数据中a值, 去t2表查找满足条件, 用上了被驱动表的索引(小表做驱动表, 被驱动表必须使用索引)
Simple_Nested-Loop_Join: 被驱动表没有使用索引, 查找满足条件时t1和t2表都要做全表扫描
Block_Nested-Loop_Join(BNL): 被驱动表没有使用索引, 把t1表数据读入线程内存join_buffer中(内存放不下t1表数据采用分段放), 把t2表每行取出来和join_buffer中数据做对比

join优化
Multi-Range_Read优化: 尽量使用顺序读盘
如果随着a的值递增顺序查询的话, 回表的时候id的值就变成随机的, 那么就会出现随机访问, 性能相对较差
根据索引a, 定位到满足条件的记录(范围查询, 足够多的主键id才能体现MRR优势), 将id值放入read_rnd_buffer中, 将read_rnd_buffer中的id进行递增排序, 排序后的id数组, 依次到主键id索引中查记录, 并作为结果返回
Batched_Key_Acess(BAK): 针对BLJ优化, 从t1表取出一部分数据放到临时内存join_buffer中










