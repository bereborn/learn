



sysAlloc
	*不能超出arena大小限制
	*调用sysMap申请内存
		*系统调用mmap申请虚拟内存
	*扩张bitmap和spans array内存
	
	
	
向arena区域申请新span的函数是mheap类的grow函数
grow
	*申请大小是64kb倍数, 最小1MB
	*调用sysAlloc像操作系统申请虚拟内存
	*创建span来管理刚申请的内存
	*填充heap spans数组的信息
	*调用freeSpanLocked尝试合并左邻右舍的span,构成更大的块
		*计算当前span在arena起始位置
		*通过spans数组访问左边的相邻的span
		*不为空且为空闲span则合并并且更新span属性, h.free释放原来左侧的span
		*检查右侧span,不为空与检查左侧span做法一样
		*根据npages插入free/freelarge链表
	
	
type mheap struct {
	free [_MaxMHeapList]mspan 	//页数小于_MaxMHeapList(128页=1M)的自由span都会在free列表中
	freelarge mspan 			//页数大于_MaxMHeapList的自由span都会在freelarge列表中
	central [67*2]struct {
		mcentral mcentral
		pad [sys.CacheLineSize - unsafe.Sizeof(mcentral{})%sys.CacheLineSize]byte
	}
	
	spans []*mspan	//spans: 指向mspans区域, 用于映射mspan和page的关系, freeSpanLocked通过这个spans数组访问左右两边的相邻的span
}
	
调用allocSpanLocked从heap分配span, allocSpanLocked函数要求当前已经对mheap上锁
首先从页数相同的链表查找, 如果没有则查找页数更多的链表甚至超大块或者申请新块
如果返回更大的span, 为避免浪费, 将多余部分切出来重新放到heap链表, 同时尝试合并相邻闲置span空间, 减少碎片
allocSpanLocked
	*尝试在mheap中的自由列表h.free分配, 分配成功跳转到HaveSpan
	*free列表找不到则调用allocLarge查找freelarge列表, 查找不到就调用grow向arena区域申请一个新的span加到freelarge中, 然后再查找freelarge列表
		*h.freelarge.remove(npage)
	*HaveSpan
		*如果获取到的span页数比要求的页数多, 分割剩余的页数到另一个span并且调用freeSpanLocked放到自由列表中, 并重新设置spans区域的指针
	
	
	
mcentral向mheap申请一个新的span会使用grow函数
grow
	*根据mcentral.spanclass计算需要申请多少页npage, 对象字节数size, 可分配的对象个数那n
	*调用mheap_.alloc向mheap申请一个新的span, 以页8K为单位
		*调用h.alloc_m
			*调用allocSpanLocked从heap分配span
			*将span放入mheap_.sweepSpans[mheap_.sweepgen/2%2]链表 (GC时候sweep span时候用)
			*设置span属性
			*上面的grow函数large传入true, 添加已分配的span到busy列表, 如果页数超过_MaxMHeapList(128页=8K*128=1M)则放到busylarge列表
	*分配并初始化span的allocBits和gcmarkBits
	

type mheap struct {
	sweepgen uint32 // sweep generation, see comment in mspan
	sweepdone uint32 // all spans are swept
}
type mspan struct {
	// sweepgen每次GC都会增加2
    // - sweepgen == 全局sweepgen, 表示span已经sweep过
    // - sweepgen == 全局sweepgen-1, 表示span正在sweep
    // - sweepgen == 全局sweepgen-2, 表示span等待sweep
	sweepgen uint32
}

type mheap struct {
	free [_MaxMHeapList]mspan 	//页数小于_MaxMHeapList(128页=1M)的自由span都会在free列表中
	freelarge mspan 			//页数大于_MaxMHeapList的自由span都会在freelarge列表中
	central [67*2]struct {
		mcentral mcentral
		pad [sys.CacheLineSize - unsafe.Sizeof(mcentral{})%sys.CacheLineSize]byte
	}
}

type mcentral struct {
	spanclass 	spanClass
	nonempty 	mspan 	// 链表span尚有空闲object可用, 如果span已经sweep过, nonempty链表中的span确定最少有一个未分配的元素
	empty 		mspan 	// 链表span可能没有空闲object可用或者已被cache取走
}
垃圾回收每次都会累加这个类似代龄的计数值sweepgen, 从heap里闲置的span不会被垃圾回收器关注, 但central里的span却有可能正在被清理, 所以当cache从central提取span时, 该属性值十分重要


向mcentral申请一个新的span会通过cacheSpan函数
mcentral首先尝试从内部的链表复用原有的span, 如果复用失败则向mheap申请
cacheSpan
	*执行一些sweep操作
	*遍历nonempty链表(使用前都要保证span已经sweep, 否则会发生在span在此分配了对象却被GC任务中sweep误认为是空闲对象)
		*如果span等待sweep, 尝试原子修改sweepgen为全局mheap_.sweepgen-1
			*修改成功则把span移到empty链表的最后, 尝试sweep它然后跳到havespan
		*如果这个span正在被其他线程sweep, 就跳过
		*span已经sweep过, 则把span移到empty链表, sweep它然后跳到havespan
	*遍历empty链表
		*如果span等待sweep, 尝试原子修改sweepgen为全局mheap_.sweepgen-1
			*尝试sweep
			*检测是否有未分配的对象, 如果有则跳到havespan, 没有则重头从nonempty链表遍历
		*如果这个span正在被其他线程sweep, 就跳过
		*找不到有未分配对象的span跳出遍历循环
	*找不到有未分配对象的span, 需要调用grow从mheap分配, 并把分配到的span放到到empty链表的最后
	*havespan
		*统计span中未分配的元素数量, 加到mcentral.nmalloc中
		*统计span中未分配的元素总大小, 加到memstats.heap_live中
		*如果当前在GC中, 因为heap_live改变了, 调用revise函数重新调整G辅助标记工作的值
		*设置span的incache属性, 表示span正在mcache中
		*根据freeindex初始化allocCache
		

		
mcache中指定类型的span已满, 就需要调用refill函数申请新的span
refill
	*确保当前的span所有元素都已分配
	*调用mheap_.central[spc].mcentral.cacheSpan()向mcentral申请一个新的span
	*分配失败throw("out of memory")
	*设置新的span到mcache中
	
		

从span里面分配对象, 首先会调用nextFreeFast尝试快速分配
nextFreeFast
	*freeindex + allocCache在分配时跳过已分配的元素, 返回未分配的元素对象的起始地址
	*如果span没有则返回0


如果在freeindex后无法快速找到未分配的元素, 就需要调用nextFree做出更复杂的处理
nextFree
	*mcache.alloc索引spc获取对应的span
	*如果span里面所有元素都已分配, 则需要调用refill获取新的span
	*返回元素所在的地址
	*添加已分配的元素计数

type mspan struct {
	freeindex uintptr	//freeindex标记下一次分配对象时应该开始搜索的地址, 分配后freeindex会增加, 在freeindex之前的元素都是已分配的, 在freeindex之后的元素有可能已分配, 也有可能未分配, sweep会重置freeindex
	nelems uintptr
	allocCache uint64	//allocCache用于缓存freeindex开始的bitmap, 缓存的bit值与原值相反
	
	allocBits  *gcBits	//allocBits用于标记哪些元素是已分配的, 哪些元素是未分配的
	gcmarkBits *gcBits	//gcmarkBits用于在gc时标记哪些对象存活, 每次gc以后gcmarkBits会变为allocBits
}

type mcache struct {
	alloc [numSpanClasses]*mspan
	tiny             uintptr	//当前小块的开头, 或者如果没有当前的小块, 则为零, tiny是一个堆指针, 由于mcache在非GC内存中, 我们通过在标记终止期间在releaseAll中清除它来处理它
	tinyoffset       uintptr
}

go从堆分配对象时会调用newobject函数, newobject调用了mallocgc函数
mallocgc(size uintptr, typ *_type, needzero(true) bool) unsafe.Pointer
	*判断是否要辅助GC工作, gcBlackenEnabled在GC的标记阶段会开启
	*增加当前G对应的M的lock计数, 防止这个G被抢占
	*获取当前G对应的M对应的P的本地span缓存(mcache)
	
	*判断是否小对象, 小于32k, maxSmallSize当前的值是32K
		*如果type == nil或者typ.kind&kindNoPointers != 0, 即对象不包含指针, 并且对象小于maxTinySize 16byte(这里是针对非常小的对象的优化, 因为span的元素最小只能是8 byte, 如果对象更小那么很多空间都会被浪费掉, 非常小的对象可以整合在"class 2 noscan"的元素(大小为16 byte)中)
			*从mcache.tiny + c.tinyoffset获取小对象的起始地址
			*调用nextFreeFast快速从这个span中分配
			*如果分配失败, 调用nextFree分配
		*普通分配小对象
			*根据size查表确定sizeClass
			*调用makeSpanClass根据sizeClass和noscan生成mcache.alloc索引spc
			*从mcache.alloc获取对应spc索引的span
			*调用nextFreeFast快速从这个span中分配
				*freeindex + allocCache在分配时跳过已分配的元素, 返回未分配的元素对象的起始地址
			*如果分配失败, 调用nextFree, 可能需要从mcentral或者mheap中获取(如果从mcentral或者mheap获取了新的span, 则shouldhelpgc会等于true, shouldhelpgc会等于true时会在下面判断是否要触发GC)
	*大对象直接调用largeAlloc从mheap分配, 这里的s是一个特殊的span, 它的class是0
		*调用mheap.alloc向mheap申请一个新的span
	
	*如果对象不包含指针, 则设置arena对应的bitmap, 记录哪些位置包含了指针, GC会使用bitmap扫描所有可到达的对象
	*如果当前在GC中, 需要立刻标记分配后的对象为黑色, 防止它被回收
	*解锁重新允许当前的G被抢占
	*如果之前获取了新的span, 则判断是否需要后台启动GC
			
	
	
	
	


nextFreeIndex返回span中空闲对象的索引
nextFreeIndex
	*如果freeindex等于nelems, 则直接返回freeindex
	*获取allocCache二进制数末尾连续0有多少个bitIndex
	*如果bitIndex等于64则循环(allocCache为unit64)
		*freeindex移动到64个索引值
		*如果freeindex等于nelems, 则直接返回freeindex
		*调用refillAllocCache更新freeindex开始的allocCache
		*获取新的allocCache二进制数末尾连续0有多少个bitIndex
	*计算新的freeindex值(加上bitIndex)
	*allocCache右移bitIndex
	*返回span中空闲对象的索引
	
		