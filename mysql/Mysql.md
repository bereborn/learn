# Mysql

---
## 基础架构

- **server层**   

1. 连接器：管理连接，权限验证   
使用长连接后，你可能会发现，有些时候 MySQL 占用内存涨得特别快，这是因为 MySQL 在执行过程中临时使用的内存是管理在连接对象里面的。这些资源会在连接断开的时候才释放。所以如果长连接累积下来，可能导致内存占用太大，被系统强行杀掉(OOM)，从现象看就是 MySQL 异常重启了   
两种解决方案：   
(1)  定期断开长连接。使用一段时间，或者程序里面判断执行过一个占用内存的大查询后，断开连接，之后要查询再重连   
(2) 如果使用的是 MySQL 5.7 或更新版本，可以在每次执行一个比较大的操作后，通过执行 mysql_reset_connection 来重新初始化连接资源。这个过程不需要重连和重新做权限验证，但是会将连接恢复到刚刚创建完时的状态
2. 查询缓存   
Mysql 提供了对于默认的 SQL 语句都不使用查询缓存的方式
3. 分析器：词法、语法解析
4. 优化器：生成执行计划，索引选择
5. 执行器：操作引擎，返回结果

- **存储层**

1. 负责数据存储和提取，插件式支持InnoDB、MyISAM多个存储引擎

---
## 日志系统

- **redo log(重做日志)**

1. InnoDB引擎的日志，redo log 保证数据库异常重启之前提交的记录不会丢失(crash-safe)
2. 在一条更新语句进行执行的时候，InnoDB引擎会把更新记录写到 redo log 日志中，然后更新内存，此时算是语句执行完了，然后在空闲的时候或者是按照设定的更新策略将 redo log 中的内容更新到磁盘中，这里涉及到WAL即Write Ahead logging技术(先写日志再写磁盘)
3. redo log 是物理日志，记录的是在某个数据页上做了什么修改
4. redo log是循环写，空间固定会用完

- **binlog(归档日志)**

1. server层日志，记录了MySQL对数据库执行更改的所有操作，没有crash-safe能力
2. binlog是逻辑日志，记录的是记录所有数据库表结构变更（例如CREATE、ALTER、TABLE…）以及表数据修改（INSERT、UPDATE、DELETE…）的二进制日志
3. binlog采用追加写的模式
4. 用途：   
(1)恢复：binlog日志恢复数据库数据   
(2)复制：主库有一个log dump线程，将binlog传给从库，从库有两个线程，一个I/O线程，一个SQL线程，I/O线程读取主库传过来的binlog内容并写入到relay log，SQL线程从relay log里面读取内容，写入从库的数据库   
(3)审计：用户可以通过二进制日志中的信息来进行审计，判断是否有对数据库进行注入攻击
5. binlog常见格式

format|定义|优点|缺点
:-----|:--|:---|:---
statement|记录的是修改SQL语句|日志文件小，节约IO，提高性能|准确性差，有些语句的执行结果是依赖于上下文命令可能会导致主备不一致(delete带limit，很可能会出现主备数据不一致的情况)
row|记录的是每行实际数据的变更|准确性强，能准确复制数据的变更|日志文件大，较大的网络IO和磁盘IO
mixed|statement和row模式的混合|准确性强，文件大小适中|有可能发生主从不一致问题

- **两段提交**   

1. 两段提交保证数据库binlog状态和日志redo log恢复出来的数据库状态保持一致
2. &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;   时刻A   &emsp;&emsp;&emsp;&emsp;时刻B    
两段提交: 写入redo log处于prepare阶段 --- 写入bin log --- 提交事务处于commit状态   
(1)时刻A崩溃恢复: redo log未提交， bin log 未写，不会传到备库，这时事务会回滚   
(2)时刻B崩溃恢复: 如果 redo log 事务完整有commit标识则直接提交，如果 redo log 事务只有完整的prepare，则判断对应事务 bin log 是否完整，是提交事务，否则回滚事务
3. bin log完整性判断:   
(1)statement格式最后有commit   
(2)row格式最有有一个XID event（redo log 和 bin log关联：有一个共同字段XID）

- **undo log**
1. undo log是逻辑日志，可以认为当delete一条记录时，undo log中会记录一条对应的insert记录，反之亦然，当update一条记录时，它记录一条对应相反的update记录
2. undo log 作用   
(1)提供回滚   
(2)多个行版本控制(MVCC)

- **Mysql抖动**
1. 当内存数据页跟磁盘数据页内容不一致的时候，称这个内存页为脏页，把内存里的数据写入磁盘的过程是flush
2. InnoDB 的 redo log 写满了，系统会停止所有更新操作，把 checkpoint 对应的所有脏页都 flush 到磁盘
3. 系统内存不足，当需要新的内存页，而内存不够用的时候，就要淘汰一些数据页，空出内存给别的数据页使用。如果淘汰的是"脏页"，就要先将脏页写到磁盘
4. MySQL 认为系统"空闲"的时候，会flush脏页
5. MySQL 正常关闭的情况，MySQL 会把内存的脏页都 flush 到磁盘上
6. InnoDB 的刷盘速度参考两个因素：一个是脏页比例，一个是 redo log 写盘速度

> [MySQL事务日志](https://www.cnblogs.com/f-ck-need-u/p/9010872.html)   
> [binlog知识](https://www.cnblogs.com/rjzheng/p/9721765.html)   
> [MySQL日志—— Undo | Redo](https://www.cnblogs.com/Bozh/archive/2013/03/18/2966494.html)


---
## 索引

- **索引的本质是数据结构**
- **索引分类**
1. 主键索引：不允许重复，不允许空值（可做外键，不允许空值，每个表只能有一个主键）
2. 唯一索引：索引列的值必须是唯一的，允许空值（不可做外键，允许空值）
3. 普通索引：用表中的普通列构建的索引，没有任何限制
4. 组合索引：用多个列组合构建的索引，这多个列中的值不允许有空值
5. 覆盖索引：如果索引包含所有满足查询需要的数据的索引成为覆盖索引，不需要回表操作


- **磁盘读取原理**
1. 磁盘组成：   
(1)一个磁盘由大小相同且同轴的圆形盘片组成，磁盘可以转动（各个磁盘必须同步转动）在磁盘的一侧有磁头支架，磁头支架固定了一组磁头，每个磁头负责存取一个磁盘的内容，磁头不能转动   
(2)磁道：盘片被划分成一系列同心环，圆心是盘片中心，每个同心环叫做一个磁道，所有半径相同的磁道组成一个柱面   
(3)扇区：磁道被沿半径线划分成一个个小的段，每个段叫做一个扇区，每个扇区是磁盘的最小存储单元   
(4)读取过程：当需要从磁盘读取数据时，系统会将数据逻辑地址传给磁盘，磁盘的控制电路按照寻址逻辑将逻辑地址翻译成物理地址，即确定要读的数据在哪个磁道，哪个扇区。为了读取这个扇区的数据，需要将磁头放到这个扇区上方，为了实现这一点，磁头需要移动对准相应磁道，这个过程叫做寻道，所耗费时间叫做寻道时间，然后磁盘旋转将目标扇区旋转到磁头下，这个过程耗费的时间叫做旋转时间
2. 局部性原理和磁盘预读   
(1)局部性原理：当一个数据被用到时，其附近的数据也通常会马上被使用   
(2)由于磁盘顺序读取的效率很高（不需要寻道时间，只需很少的旋转时间），因此对于具有局部性的程序来说，预读可以提高I/O效率

- **B-/+Tree索引的性能分析**
1. B-Tree：根据B-Tree的定义，可知检索一次最多需要访问h个节点，数据库系统的设计者巧妙利用了磁盘预读原理，将一个节点的大小设为等于一个页，这样每个节点只需要一次I/O就可以完全载入。为了达到这个目的，在实际实现B-Tree还需要使用如下技巧：每次新建节点时，直接申请一个页的空间，这样就保证一个节点物理上也存储在一个页里，加之计算机存储分配都是按页对齐的，就实现了一个node只需一次I/O
2. 红黑树：红黑树这种结构，h明显要深的多，由于逻辑上很近的节点（父子）物理上可能很远，无法利用局部性，所以红黑树的I/O效率明显比B-Tree差很多
3. B+Tree：B+Tree更适合外存索引，由于B+Tree内节点去掉了data域，因此可以拥有更大的出度，拥有更好的性能

- **B树**
1. B树定义：   
(1)树中每个结点最多含有m个子树   
(2)除根结点和叶子结点外，其它每个结点至少有[ceil(m / 2)]个子树（其中ceil(x)是一个取上限的函数）   
(3)若根结点不是叶子结点，则至少有2个孩子   
(4)所有叶子结点都出现在同一层，叶子结点不包含任何关键字信息(可以看做是外部接点或查询失败的接点，实际上这些结点不存在，指向这些结点的指针都为null)   
(5)每个非终端结点中包含有n个关键字信息： (n，P0，K1，P1，K2，P2，......，Kn，Pn)，其中：   
（a）Ki (i=1...n)为关键字，且关键字按顺序升序排序K(i-1)< Ki   
（b）Pi为指向子树根的接点，且指针P(i-1)指向子树种所有结点的关键字均小于Ki，但都大于K(i-1)   
（c）关键字的个数n必须满足： [ceil(m / 2)-1]<= n <= m-1


- **B+树**
1. B树定义：   
(1)非叶子结点中含有m个关键字（B树是m-1个）  
(2)所有的叶子结点中包含了全部关键字的信息，及指向含有这些关键字记录的指针，且叶子结点本身依关键字的大小自小而大的顺序链接（而B树的叶子节点并没有包括全部需要查找的信息）   
(3)B+树非叶节点中存放的关键码并不指示数据对象的地址指针，非叶节点只是索引部分，所有数据都保存在叶子节点（而B 树的非终节点也包含需要查找的有效信息）

- **B+-tree比B树更适合实际应用中操作系统的文件索引和数据库索引**
1. B+-tree的非叶节点结点并没有指向关键字具体信息的指针，因此其内部结点相对B树更小，一次性读入内存中的需要查找的关键字也就越多
2. 任何关键字的查找必须走一条从根结点到叶子结点的路，所有关键字查询的路径长度相同，查询效率稳定

- **MyISAM非聚簇索引**
1. MyISAM的索引文件仅仅保存数据记录的地址（非聚簇索引，数据表和索引表是分开存储）
2. 非聚簇索引的主索引和辅助索引几乎是一样的，只是主索引不允许重复，不允许空值，他们的叶子结点的key都存储指向键值对应的数据的物理地址

- **InnoDB聚簇索引**
1. InnoDB中，表数据文件本身就是按B+Tree组织的一个索引结构，这棵树的叶节点data域保存了完整的数据记录（聚簇索引，数据和主键索引存储在一起），因为InnoDB的数据文件本身要按主键聚集，所以InnoDB要求表必须有主键（MyISAM可以没有），如果没有显式指定，则MySQL系统会自动选择一个可以唯一标识数据记录的列作为主键或者自动为InnoDB表生成一个隐含字段作为主键
2. InnoDB的辅助索引data域存储相应记录主键的值而不是地址，辅助索引搜索需要检索两遍索引：首先检索辅助索引获得主键，然后用主键到主索引中检索获得记录

- **索引使用策略**
1. 什么时候要使用索引？   
(1)主键自动建立唯一索引   
(2)经常作为查询条件在 WHERE或者 ORDER BY 语句中出现的列要建立索引   
(3)作为排序的列要建立索引   
(4)查询中与其他表关联的字段，外键关系建立索引
2. 什么时候不要使用索引？   
(1)经常增删改的列不要建立索引   
(2)有大量重复的列不建立索引   
3. 索引失效情况：   
(1)在组合索引中不能有列的值为NULL，如果有，那么这一列对组合索引就是无效的   
(2)LIKE操作中，'%aaa%'不会使用索引，也就是索引会失效，但是‘aaa%’可以使用索引，在查询条件中使用正则表达式时，只有在搜索模板的第一个字符不是通配符的情况下才能使用索引   
(3)在索引的列上使用表达式或者函数最左前缀匹配会使索引失效   
(4)在查询条件中使用 IS NULL或者 IS NOT NULL 会导致索引失效   
(5)范围列可以用到索引（必须是最左前缀），但是范围列后面的列无法用到索引   
(6)where查询类型不一致会导致索引失效   
(7)在查询条件中使用OR连接多个条件会导致索引失效，除非OR链接的每个条件都加上索引，这时应该改为两次查询，然后用UNION ALL连接起来   

- **索引优化**
1. 最左前缀匹配：在组合索引中，当查询条件精确匹配索引的左边==连续==一个或几个列时，组成的最左前缀，中间某个条件未提供会造成索引失效
2. 在组合索引中，中间某个条件未提供，可使用隔离列的优化方法：可以考虑用"IN"来填补这个"坑"从而形成最左前缀，但如果列的不同值很多，填坑就不合适了，必须建立辅助索引
3. 覆盖索引


- **InnoDB的主键选择与插入优化**
1. 自增字段做主键：   
(1)在使用InnoDB存储引擎时，如果没有特别的需要，请永远使用一个与业务无关的自增字段作为主键   
(2)InnoDB使用聚集索引，数据记录本身被存于主索引（一颗B+Tree）的叶子节点上。这就要求同一个叶子节点内（大小为一个内存页或磁盘页）的各条数据记录按主键顺序存放，因此每当有一条新的记录插入时，MySQL会根据其主键将其插入适当的节点和位置，如果页面达到装载因子（InnoDB默认为15/16），则开辟一个新的页（节点）   
(3)如果表使用自增主键，那么每次插入新的记录，记录就会顺序添加到当前索引节点的后续位置，当一页写满，就会自动开辟一个新的页，这样就会形成一个紧凑的索引结构，近似顺序填满，由于每次插入时也不需要移动已有数据，因此效率很高，也不会增加很多开销在维护索引上   
(4)如果使用非自增主键（如果身份证号或学号等），由于每次插入主键的值近似于随机，因此每次新纪录都要被插到现有索引页得中间某个位置，此时MySQL不得不为了将新记录插到合适位置而移动数据，甚至目标页面可能已经被回写到磁盘上而从缓存中清掉，此时又要从磁盘上读回来，这增加了很多开销，同时频繁的移动、分页操作造成了大量的碎片，得到了不够紧凑的索引结构，后续不得不通过OPTIMIZE TABLE来重建表并优化填充页面   
2. 普通索引和唯一索引的选择：   
(1)当需要更新一个数据页时，如果数据页在内存中就直接更新，而如果这个数据页还没有在内存中的话，在不影响数据一致性的前提下，InooDB 会将这些更新操作缓存在 change buffer 中，这样就不需要从磁盘中读入这个数据页了。在下次查询需要访问这个数据页的时候，将数据页读入内存，然后执行 change buffer 中与这个页有关的操作。通过这种方式就能保证这个数据逻辑的正确性   
(2)将 change buffer 中的操作应用到原数据页，得到最新结果的过程称为 purge。除了访问这个数据页会触发 purge 外，系统有后台线程会定期 purge。在数据库正常关闭(shutdown)的过程中，也会执行 purge 操作   
(3)显然，如果能够将更新操作先记录在 change buffer，减少读磁盘，语句的执行速度会得到明显 的提升。而且，数据读入内存是需要占用 buffer pool 的，所以这种方式还能够避免占用内存，提高内存利用率   
(4)对于唯一索引来说，所有的更新操作都要先判断这个操作是否违反唯一性约束，这必须要将数据页读入内 存才能判断，如果都已经读入到内存了，那直接更新内存会更快，就没必要使用 change buffer 了   
(5)change buffer 的使用场景：对于写多读少的业务来说，页面在写完以后马上被访问到的概率比较小，此时 change buffer 的使用效果最好，这种业务模型常见的就是账单类、日志类的系统
3. 字符串添加索引：   
(1) MySQL 是支持前缀索引的，可以定义字符串的一部分作为索引，使用前缀索引，定义好长度，就可以做到既节省空间，又不用额外增加太多的查询成本，我们在建立索引时关注的是区分度，区分度越高越好，可以通过统计索引上有多少个不同的值来判断要使用多长的前缀   
(2)hash索引：身份证索引区分度不高，占用空间大，用 crc32 算出来的值小的冲突概率，虽然有额外的存储和计算消耗，但查询性能稳定，不支持范围查询和覆盖索引


- **explain**
1. table | type | possible_keys | key | key_len | ref | rows | Extra
2. type：连接类型   
(1)const：表最多有一个匹配行，const用于比较primary key 或者unique索引，因为只匹配一行数据，所以很快（explain SELECT * FROM `asj_admin_log` where log_id = 111）   
(2)eq_ref：对于每个来自于前面的表的行组合，从该表中读取一行，它用在一个索引的所有部分被联接使用并且索引是UNIQUE或PRIMARY KEY，常见于主键或唯一索引扫描（explain select * from uchome_spacefield，uchome_space where uchome_spacefield.uid = uchome_space.uid）   
(3)ref：对于每个来自于前面的表的行组合，所有有匹配索引值的行将从这张表中读取，如果联接只使用键的最左边的前缀，或如果键不是 UNIQUE 或 PRIMARY KEY，则使用ref，这个类型严重依赖于根据索引匹配的记录多少—越少越好，常见于使用非唯一索引或唯一索引的前缀查找（explain select * from uchome_space where uchome_space.friendnum = 0）   
(4)range：给定范围内的检索，使用一个索引来检查行，常见于between、<、>等的查询（explain select * from uchome_space where uid < 2 ）   
(5)index：对于每个来自于前面的表的行组合，进行完全扫描，这通常比ALL快，因为索引文件通常比数据文件小   
(6)ALL：对于每个来自于前面的表的行组合，进行完整的表扫描
3. possible_keys：显示可能应用在这张表中的索引
4. keys：表明实际使用的索引
5. key_len：使用的索引的长度，在不损失精确性的情况下，长度越短越好
6. ref：显示索引的哪一列被使用了，如果可能的话是一个常数
7. rows：MYSQL执行查询的行数，数值越大越不好，说明没有用好索引
8. Extra：MySQL解决查询的详细信息   
(1)Distinct：一旦mysql找到了与行相联合匹配的行， 就不再搜索了   
(2)Not exists：mysql优化了LEFT JOIN， 一旦它找到了匹配LEFT JOIN标准的行， 就不再搜索了   
(3)Range checked for each Record：没有找到合适的索引，因此对从前面表中来的每一个行组合，mysql检查使用哪个索引   
(4)using filesort：查询就需要优化了，mysql需要进行额外的步骤来发现如何对返回的行排序，根据连接类型以及存储排序键值和匹配条件的全部行的行指针来排序全部行（select city,name,age from t where city='杭州' order by name limit 1000; 不给name添加索引数据库使用sort_buffer对name数据排序，内存sort_buffer不够则使用外部排序，将数据分割多个文件排序再组合，或者使用rowid排序，sort_buffer只有主键id和name，name排序完再根据主键id从表中取数据）   
(5)using index：列数据是从仅仅使用了索引中的信息而没有读取实际的行动的表返回的，这发生在对表的全部的请求列都是同一个索引的部分的时候   
(6)Using temporary：查询需要优化了，mysql需要创建一个临时表来存储结果，这通常发生在对不同的列集进行ORDER BY上（select word from words order by rand() limit 3;从表取出主键id并且生成一个随机值，sort_buffer对其排序，取前三个主键id从表中取数据，随机算法：取表行数C，取得 Y = floor(C * rand())，在用limit Y，1 取得一行）   
(7)using where：使用了WHERE从句来限制哪些行将与下一张表匹配或者是返回给用户   



> [MySQL索引原理及BTree结构详解](https://blog.csdn.net/u013967628/article/details/84305511)   
> [深入理解MySQL索引原理和实现](https://blog.csdn.net/tongdanping/article/details/79878302?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [b+树图文详解](https://blog.csdn.net/qq_26222859/article/details/80631121?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [从B树、B+树、B*树谈到R树](https://blog.csdn.net/v_JULY_v/article/details/6530142?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [MYSQL explain详解](https://blog.csdn.net/zhuxineli/article/details/14455029?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [MySQL explain执行计划解读](https://blog.csdn.net/xifeijian/article/details/19773795?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)

---
## 锁

- **全局锁**
1. 使用场景：全库逻辑备份
2. mysqldump 使用参数–single-transaction 的时候，导数据之前就会启动一个事务，来确保拿到一致性视图，而由于 MVCC 的支持，这个过程中数据是可以正常更新的，single-transaction 方法只适用于所有的表使用事务引擎的库

- **表级锁**
1. 表锁和元数据锁（MDL）
2. 当对一个表做增删改查操作的时候，加 MDL 读锁，读锁之间不互斥
3. 当要对表做结构变更操作的时候，加 MDL 写锁，写锁之间是互斥的
4. 给一个小表加个字段，导致整个库挂了的情况：session A/session B 查询表 t 需要对表 t 加一个 MDL 读锁，session C 修改表结构需要对表 t 加一个 MDL 写锁，只能被阻塞，之后所有要在表 t 上新申请 MDL 读锁的请 求也会被 session C 阻塞，如果某个表上的查询语句频繁，而且客户端有重试机制，也就是说超时后会再起一个新 session 再请求的话，这个库的线程很快就会爆满
5. 安全地给小表加字段：比较理想的机制是，在 alter table 语句 里面设定等待时间，如果在这个指定的等待时间里面能够拿到 MDL 写锁最好，拿不到也不要阻 塞后面的业务语句，先放弃，之后开发人员或者 DBA 再通过重试命令重复这个过程
6. 意向锁，允许行锁和表锁共存，实现多粒度锁机制   
(1)意向共享锁 IS：事务在给一个数据行加共享锁前必须先取得该表的IS锁   
(2)意向排他锁 IX：事务在给一个数据行加排他锁前必须先取得该表的IX锁

- **行锁**
1. 两阶段锁协议：在 InnoDB 事务中，行锁是在需要的时候才加上的，但并不是不需要了就立刻释放，而是要等到事务结束时才释放
2. 如果你的事务中需要锁多个行，要把最可能造成锁冲突、最可能影响并 发度的锁的申请时机尽量往后放
3. 行锁模式：   
(1)共享锁 s：多个事务只能读数据不能改数据   
(2)排他锁 x：其他事务不能再在其上加其他的锁（select语句默认不会加任何锁类型）
4. InnoDB行锁实现方式：InnoDB行锁是通过给索引上的索引项加锁来实现的，只有通过索引条件检索数据，InnoDB才使用行级锁，==否则，InnoDB将使用表锁==，当表有多个索引的时候，不同的事务可以使用不同的索引锁定不同的行，即便在条件中使用了索引字段，但是否使用索引来检索数据是由MySQL通过判断不同执行计划的代价来决定的，如果MySQL认为全表扫描效率更高，比如对一些很小的表，这种情况下InnoDB将使用表锁而不是行锁


- **死锁**
1. Mysql死锁策略   
(1)直接进入等待，直到超时，这个超时时间可以通过参数 innodb_lock_wait_timeout 来设置   
(2)发起死锁检测，发现死锁，主动回滚死锁链条中的某一个事务，让其他事务得以继续执行，将参数 innodb_deadlock_detect 设置为 on，表示开启这个逻辑
2. 避免死锁的常用方法：   
(1)在应用中，如果不同的程序会并发存取多个表，应尽量约定以相同的顺序来访问表，这样可以大大降低产生死锁的机会   
(2)在程序以批量方式处理数据的时候，如果事先对数据排序，保证每个线程按固定的顺序来处理记录，也可以大大降低出现死锁的可能   
(3)在RR隔离级别下，如果两个线程同时对相同条件记录用 SELECT...FOR UPDATE 加排他锁，在没有符合该条件记录情况下，两个线程都会加间隙锁成功，程序发现记录尚不存在，就试图插入一条新记录，如果两个线程都这么做，就会出现死锁，这种情况下，将隔离级别改成RC不会产生间隙锁，就可避免问题   
(4)在RC隔离级别下，如果两个线程都先执行SELECT...FOR UPDATE，判断是否存在符合条件的记录，如果没有，就插入记录，如果两个线程都先执行 SELECT...FOR UPDATE，判断是否存在符合条件的记录，如果没有，就插入记录   
(5)在程序设计中总是捕获并处理死锁异常是一个很好的编程习惯，如果出现死锁，可以用 show innodb status \G 命令来确定最后一个死锁产生的原因，返回结果中包括死锁相关事务的详细信息，如引发死锁的SQL语句，事务已经获得的锁，正在等待什么锁，以及被回滚的事务等


- **间隙锁**
1. 间隙锁的主要作用是为了防止出现幻读，但是它会把锁定范围扩大，innodb_locks_unsafe_for_binlog 配置是否启用间隙锁
2. 间隙锁锁定的区域:根据检索条件向左寻找最靠近检索条件的记录值A，作为左区间，向右寻找最靠近检索条件的记录值B作为右区间，即锁定的间隙为（A，B)
3. innodb自动使用间隙锁的条件：   
(1)必须在RR级别下   
(2)检索条件必须有索引（没有索引的话，mysql会全表扫描，那样会锁定整张表所有的记录，包括不存在的记录，此时其他事务不能修改不能删除不能添加）
4. 间隙锁保证Mysql恢复和复制正确性   
(1)MySQL的恢复是SQL语句级的，也就是重新执行BINLOG中的SQL语句，MySQL的Binlog是按照事务提交的先后顺序记录的，恢复也是按这个顺序进行的   
(2)在BINLOG中，更新操作的位置在INSERT...SELECT之前，如果使用这个BINLOG进行数据库恢复，恢复的结果与实际的应用逻辑不符；如果进行复制，就会导致主从数据库不一致！   
(3) INSERT...SELECT...和 CREATE TABLE...SELECT... 语句，InnoDB会给源表加共享锁   
(4)INSERT...SELECT...和 CREATE TABLE...SELECT... 语句 where 带范围，InnoDB会给源表加共享锁+间隙锁


- **next-key锁**
1. next-key锁其实包含了记录锁和间隙锁，即锁定一个范围，并且锁定记录本身，InnoDB默认加锁方式是next-key 锁


> [innodb下的记录锁，间隙锁，next-key锁](https://blog.csdn.net/bigtree_3721/article/details/73731377?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [innodb下的记录锁，间隙锁，next-key锁](https://www.jianshu.com/p/bf862c37c4c9)   
> [MySQL中的锁（表锁、行锁，共享锁，排它锁，间隙锁）](https://blog.csdn.net/soonfly/article/details/70238902?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [MySQL详解－－锁](https://blog.csdn.net/xifeijian/article/details/20313977?depth_1-utm_source=distribute.pc_relevant.none-task&utm_source=distribute.pc_relevant.none-task)   
> [MySQL中的锁（表锁、行锁）](https://www.cnblogs.com/chenqionghe/p/4845693.html)   
> [select for update 引发死锁分析](https://www.cnblogs.com/micrari/p/8029710.html)

---
## 事务

- **特性**
1. 原子性
2. 一致性
3. 隔离性
4. 持久性

- **事务隔离**
1. Read uncommitted   
脏读问题: 一个事务可以读取另一个未提交事务的数据(程序员看到老板还没提交事务时的误操作工资3.9w)
2. Read committed   
不可重复读: 事务开启， 允许其他事务的UPDATE修改操作， 一个事务范围内两个相同的查询却返回了不同数据(程序员看到卡里3.6w余额准备享受生活，这时妻子把钱全部转出充当家用并提交，程序员收费系统准备扣款时再检测卡里的金额发现已经没钱了)
3. Repeatable read   
可重复读: 事务A在读到一条数据之后，此时事务B对该数据进行了修改并提交，那么事务A再读该数据，读到的还是原来的内容   
幻读: 事务开启，允许其他事务的INSERT插入操作(妻子查看程序员消费2千元打印清单时发现花了1.2万元)
4. Serializable   
事务串行化顺序执行

> [事务的四种隔离级别](https://www.cnblogs.com/ubuntu1/p/8999403.html)   
> [MySQL——事务(Transaction)详解](https://blog.csdn.net/w_linux/article/details/79666086)   
> [事务原理](https://www.jianshu.com/p/62ccfa64fed1)


---
## MVCC(多版本并发控制机制)

- **MVCC使用及其原理、实现机制**
1. 作用: 事务型存储引擎使用MVCC可以在大多数情况下代替行级锁能降低其系统开销，实现了非阻塞的读操作，写操作也只锁定了必要的行，MVCC主要适用于Mysql的RC，RR隔离级别
2. 实现: 通过保存数据在某个时间点的快照来实现的，每行记录后面保存创建时系统版本号，行删除时系统版本号   
(1)insert: 为新插入的每一行保存当前系统版本号作为版本号   
(2)delete: 删除的每一行保存当前系统的版本号(事务的ID)作为删除标识   
(3)update: 实际上是新插入了一行记录，并保存其创建时间为当前事务的ID，同时保存当前事务ID到要UPDATE的行的删除时间   
(4)select: 同时满足以下两个条件才会返回：  
只会查找版本早于当前事务版本的数据行(行的系统版本号小于或等于事务的系统版本号)，这样可以确保事务读取的行，要么是在事务开始前已经存在的，要么是事务自身插入或者修改过的   
行的删除版本要么未定义，要么大于当前事务版本号，这可以确保事务读取到的行，在事务开始之前未被删除

- **Mysql MVCC原理**
1. transaction id：InnoDB 里面每个事务有一个唯一的事务 ID，叫作 transaction id，它是在事务开始的时候向 InnoDB 的事务系统申请的，是按申请顺序严格递增的
2. row trx_id：每行数据也都是有多个版本的，每个版本有自己的 row trx_id，每次事务更新数据的时候，都会生成一个新的数据版本，并且把 transaction id 赋值给这个数据版本的事务 ID，记为 row trx_id
3. up_limit_id：一个事务只需要在启动的时候，找到所有已经提交的事务 ID 的最大值，记为 up_limit_id，如果一个数据版本的 row trx_id 大于 up_limit_id，我就不认，我必须要找到它的上一个版本，当然，如果一个事务自己更新的数据，它自己还是要认的，InnoDB 利用了"所有数据都有多个版本"的这个特性，实现了"秒级创建快照"的能力
4. 更新数据都是先读后写的，而这个读是当前读，==当前读读取的是最新已提交数据，并且都会加行锁==，当前读语句: select ... lock in share mode，select ... for update，insert，update，delete
5. 可重复读的核心就是一致性读：事务更新数据的时候，只能用当前读，如果当前的记录的行锁被其他事务占用的话，就需要进入锁等待
6. 读提交和可重复读逻辑：   
(1)在读提交隔离级别下，每一个语句执行前都会重新算一次 up_limit_id 的值   
(2)在可重复读隔离级别下，只需要在事务开始的时候找到那个 up_limit_id，之后事务里的其他 查询都共用这个 up_limit_id

- **一致性读**
1. 一致性读：事务更新数据的时候，只能用当前读，如果当前的记录的行锁被其他事务占用的话，就需要进入锁等待
2. 一致性非锁定读：   
(1)在RC隔离级别下，一致性非锁定读总是读取被锁定行的最新一份快照数据，产生了不可重复读的问题   
(2)在RR隔离级别下，一致性非锁定读总是读取事务开始时的行数据版本，解决不可重复读的问题
3. 一致性锁定读：   
(1)在RC隔离级别下，读操作不加锁，写操作加锁，读事务每次都读最新版本，产生了不可重复读的问题   
(2)在RR隔离级别下，第一次读数据的时候就将数据加行锁(共享锁)，使其他事务不能修改当前数据，即可实现可重复读，但是不能锁住insert进来的新的数据，当前事务读取或者修改的同时，另一个事务还是可以insert提交，造成幻读

> [Mysql中MVCC的使用及原理详解](https://blog.csdn.net/w2064004678/article/details/83012387)   
> [轻松理解MYSQL MVCC 实现机制](https://blog.csdn.net/whoamiyang/article/details/51901888)   
> [Mysql中的MVCC](https://blog.csdn.net/chen77716/article/details/6742128)   
> [探究InnoDB可重复读](https://blog.csdn.net/ghsau/article/details/78258389)

---
## 表删除数据，表文件大小不变

- **数据删除流程**
1. 如果我们要删掉 R4 这个记录，InnoDB 引擎只会把 R4 这个记录标记为删除，如果之后要再插入一个可以在 R4 位置上的记录时，可能会复用这个位置
2. delete命令把整个表的数据删除，其实是把所有的数据页都会被标记为可复用，但磁盘文件的大小是不会变的
3. 通过 delete 命令是不能回收表空间的，这些可以复用，而没有被使用的空间，看起来就像是"空洞"，大量增删改的表，都是可能是存在空洞的

- **重建表**
1. 新建一个与表 A 结构相同的临时表 B，然后按照主键 ID 递增的顺序插入到临时表 B 中
2. 生成临时文件的过程中，将所有对 A 的操作记录在一个日志文件（row log）中
3. 临时表 B 生成后，将日志文件中的操作应用到临时表 B 
4. 用临时表 B 替换表 A
5. optimize table t = recreate + analyze


---
## count(*)

- **count(\*)实现方式**
1. MyISAM 引擎把一个表的总行数存在了磁盘上，因此执行 count(*) 的时候会直接返回这个数，效率很高
2. InnoDB 引擎执行 count(\*) 的时候，需要把数据一行一行地从引擎里面读出 来，然后累积计数，InnoDB 默认的隔离级别是可重复读，对于 count(\*) 请求来说，每一行记录都要判断自己是否对这个会话可见

- **缓存保存计数**
1. redis保存，存在丢失更新以及逻辑上不精确
2. Mysql保存，可以保证崩溃恢复，可重复读事务保证逻辑一致

- **不同count做法**
1. count(1)：InnoDB 引擎遍历整张表，但不取值，server层对于返回的每一行，放一个数字"1"进去，判断是不可能为空的，按行累加,比count(主键 id)快，count(主键 id)涉及解析数据行以及拷贝字段值的操作
2. count(*)：并不会把全部字段取出来，而是专门做了优化，不取值
3. count(主键 id)：InnoDB 引擎会遍历整张表，把每一行的 id 值都取出来，返回给 server 层，server 层拿到 id 后，判断是不可能为空的，就按行累加
4. count(字段)：如果这个字段定义为 not null，一行行地从记录里面读出这个字段，判断不能
为 null，按行累加，如果这个字段定义允许为 null，还要把值取出 来再判断一下，不是 null 才累加


---
## - Mysql性能优化21条最佳实践

1. EXPLAIN 你的 SELECT 查询
2. 为搜索字段建索引
3. 在Join表的时候使用相同类型的字段，并将其索引
4. 千万不要 ORDER BY RAND()
5. 避免 SELECT *，养成一个需要什么就取什么的好的习惯
6. 永远为每张表设置一个ID，除非要自定义字段主键作为外键
7. 字段的取值是有限而且固定的，使用 ENUM 而不是 VARCHAR
8. 选择正确的存储引擎，MyISAM 适合于一些需要大量查询的应用，但其对于有大量写操作并不是很好，甚至你只是需要update一个字段，整个表都会被锁起来，另外，MyISAM 对于 SELECT COUNT(*) 这类的计算是超快无比的，InnoDB 的趋势会是一个非常复杂的存储引擎，对于一些小的应用，它会比 MyISAM 还慢。他是它支持“行锁” ，于是在写操作比较多的时候，会更优秀，并且，他还支持更多的高级应用，比如：事务

> [Mysql数据库调优和性能优化21条最佳实践](https://www.jianshu.com/p/9d438bbd2afc)

---
## 分布式ID方案总结

- **数据库自增ID**

```
// 数据库表
CREATE TABLE SEQID.SEQUENCE_ID (
	id bigint(20) unsigned NOT NULL auto_increment, 
	stub char(10) NOT NULL default '',
	PRIMARY KEY (id),
	UNIQUE KEY stub (stub)
) ENGINE=MyISAM;

// 获取一个自增id
// stub字段在这里并没有什么特殊的意义，只是为了方便的去插入数据，只有能插入数据才能产生自增id
// 对于插入我们用的是replace，replace会先看是否存在stub指定值一样的数据，如果存在则先delete再insert，如果不存在则直接insert
begin;
replace into SEQUENCE_ID (stub) VALUES ('anyword');
select last_insert_id();
commit;

# 业务系统每次需要一个ID时，都需要请求数据库获取，性能低，并且如果此数据库实例下线了，那么将影响所有的业务系统
```

- **数据库多主模式**

```
// 单独给每个Mysql实例配置不同的起始值和自增步长

// 第一台Mysql实例配置：
set @@auto_increment_offset = 1;     -- 起始值
set @@auto_increment_increment = 2;  -- 步长

第二台Mysql实例配置：
set @@auto_increment_offset = 2;     -- 起始值
set @@auto_increment_increment = 2;  -- 步长

// 这两个Mysql实例生成的id序列如下：
// mysql1,起始值为1，步长为2，ID生成的序列为：1，3，5，7，9... 
// mysql2，起始值为2，步长为2，ID生成的序列为：2，4，6，8，10...

# 实行这种方案后，就算其中某一台Mysql实例下线了，仍然可以利用另外一台Mysql来生成ID
# 但是这种方案的扩展性不太好，如果两台Mysql实例不够用，需要新增Mysql实例来提高性能时，这时就会比较麻烦
# 第一，mysql1、mysql2的步长肯定都要修改为3，而且只能是人工去修改，这是需要时间的
# 第二，因为mysql1和mysql2是不停在自增的，对于mysql3的起始值我们可能要定得大一点，以给充分的时间去修改mysql1，mysql2的步长
# 第三，在修改步长的时候很可能会出现重复ID，要解决这个问题，可能需要停机才行
```

- **号段模式**

```
CREATE TABLE id_generator (
  id int(10) NOT NULL,
  current_max_id bigint(20) NOT NULL COMMENT '当前最大id',
  increment_step int(10) NOT NULL COMMENT '号段的长度',
  version int(10) NOT NULL COMMENT '版本',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

// 乐观锁并发控制
update id_generator set current_max_id=#{newMaxId}, version=version+1 where version = #{version}

// 批量获取自增id
// 如果DistributIdService重启，会丢失一段ID，导致ID空洞
```

- **Redis**

```
127.0.0.1:6379> set seq_id 1     // 初始化自增ID为1
OK
127.0.0.1:6379> incr seq_id      // 增加1，并返回
(integer) 2
127.0.0.1:6379> incr seq_id      // 增加1，并返回
(integer) 3

// Redis支持RDB和AOF两种持久化的方式，如果Redis挂掉了，重启Redis后会出现ID重复，
```

- **雪花算法**

```
// 分布式ID固定是一个long型的数字，一个long型占8个字节，也就是64个bit
// 第一个bit位是标识部分，在java中由于long的最高位是符号位，一般为0
// 时间戳部分占41bit，这个是毫秒级的时间，41位的时间戳可以使用69年
// 工作机器id占10bit，可以部署1024个节点
// 序列号部分占12bit，支持同一毫秒内同一个节点可以生成4096个ID

// 原始的snowflake算法需要人工去为每台机器去指定一个机器id，并配置在某个地方从而让snowflake从此处获取机器id，但是在大厂里，机器是很多的，人力成本太大且容易出错
// 所以大厂对snowflake进行了改造，应用在启动时会往数据库表(uid-generator需要新增一个WORKER_NODE表)中去插入一条数据，数据插入成功后返回的该数据对应的自增唯一id就是该机器的workId
```

> [大型互联网公司分布式ID方案总结](https://juejin.im/post/5d6fc8eff265da03ef7a324b#heading-1)



