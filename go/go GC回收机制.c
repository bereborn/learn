go 函数栈
lo stackguard0 																	hi
+---------------+---------------------------------------------------------------+
| StackGuard 	| 																|
+---------------+---------------------------------------------------------------+
																			<-- SP

每个goroutine都有自己的栈, 分配2kb, 每次执行函数调用时Go的runtime都会进行检测, 若当前栈的大小不够用, 则会触发"中断", 从当前函数进入到Go的运行时库, Go的运行时库会保存此时的函数上下文环境, 然后分配一个新的足够大的栈空间, 将旧栈的内容拷贝到新栈中, 并做一些设置, 使得当函数恢复运行时, 函数会在新分配的栈中继续执行
*如何检测栈不够用
*分配新的栈空间
*旧栈内容拷贝到新栈
*指针失效问题: 指向旧栈对象的指针如何重新指向新栈
*函数返回后如何进行栈缩小

c程序通过栈指针寄存器和栈基址寄存器确定函数的栈, goroutine存了stackbase和stackguard, 用于保存的栈空间信息
每个Go函数调用的前几条指令, 比较栈指针寄存器sp和stackguard进行栈空间大小判断, 栈空间不足的时候, 通过调用runtime.morestack_noctxt来完成栈的扩容, morestack_noctxt函数清空rdx寄存器并调用morestack函数, morestack函数会保存G的状态到g.sched, 切换到g0和g0的栈空间, 然后调用newstack函数
runtime.newstack完成栈扩容
	*preempt = true抢占相关代码(详见goroutine)
	*设置g.status = _Gcopystack, 用于防止GC扫描
	*调用copystack
		*调用stackalloc从系统分配的栈或者go分配的堆获取新栈
		*调用memmove把旧栈数据复制到新栈
		*调用gentraceback调整向旧栈对象的指针重新指向新栈, 对当前栈帧之前的每一个栈帧, 对其中的每一个指针, 检测指针指向的地址, 如果指向地址是落在旧栈范围内的, 则将它加上一个偏移使它指向新栈的相应地址。这个偏移值等于新栈基地址减旧栈基地址
		*调用stackfree释放旧栈
		
栈的释放和缩栈
scanstack扫描栈时, 如果未完成标记则调用shrinkstack, markroot标记工作中执行到fixedRootFreeGStacks这一步调用shrinkstack释放已中止的G的栈
shrinkstack
	*如果g.status = _Gdead, 调用stackfree释放旧栈
	*如果栈的使用空间大于总栈空间的 1/4, 或者正在进行系统调用, 则不进行缩栈 
	*调用copystack把栈缩小为原来的一半


辅助GC
为了防止heap增速太快, 在GC执行的过程中如果同时运行的G分配了内存, 那么这个G会被要求辅助GC做一部分的工作.
在GC的过程中同时运行的G称为"mutator", "mutator assist"机制就是G辅助GC做一部分工作的机制.

辅助GC做的工作有两种类型, 一种是标记(Mark), 另一种是清扫(Sweep).
辅助标记的触发可以查看上面的mallocgc函数, 触发时G会帮助扫描"工作量"个对象, 工作量的计算公式是:	debtBytes * assistWorkPerByte

revise
	*等待扫描的对象数量 = 未扫描的对象数量 - 已扫描的对象数量	scanWorkExpected := int64(memstats.heap_scan) - gcControllerState.scanWork
	*距离触发GC的Heap大小 = 期待触发GC的Heap大小 - 当前的Heap大小, 注意next_gc的计算跟gc_trigger不一样, next_gc等于heap_marked * (1 + gcpercent / 100)	heapDistance := int64(memstats.next_gc) - int64(atomic.Load64(&memstats.heap_live))
	*每分配1 byte需要辅助扫描的对象数量 = 等待扫描的对象数量 / 距离触发GC的Heap大小
	*c.assistWorkPerByte = float64(scanWorkExpected) / float64(heapDistance)
	*c.assistBytesPerWork = float64(heapDistance) / float64(scanWorkExpected)


*调用stopTheWorldWithSema停止整个世界, 这个函数必须在g0中运行
*空闲队列之中的p以及处于系统调用之中的未运行的p, 通过直接设置其状态为_Pgcstop来阻止工作线程绑定它们
*正在运行go代码的p, 只能通过设置抢占标记gp.preempt = true来停止他们
*当前go代码的p, 直接设置其状态为_Pgcstop防止抢占
	*需要停止的P数量 sched.stopwait = gomaxprocs
	*设置gc等待标记, 调度时看见此标记会进入等待	sched.gcwaiting = gomaxprocs
	*调用preemptall遍历所有p, 对每个p.status == _Prunning调用preemptone函数设置gp.preempt = true和gp.stackguard0 = stackPreempt抢占所有运行中的G
	*停止当前的P p.status = _Pgcstop	(当前P在newstack发生抢占的时候, 由于p.status != _Prunning而不进行抢占, 保证了整个世界只有当前P可以执行)
	*减少需要停止的P数量(当前的P算一个) sched.stopwait--
	*抢占所有在Psyscall状态的P, 通过设置p.status = _Pgcstop, 防止它们重新参与调度
	*防止所有空闲的_Pidle状态的P重新参与调度
	*如果仍有需要停止的P, 则等待它们停止
	*逻辑正确性检查所有P.status = _Pgcstop, 所有运行中的G都会变为待运行, 并且所有的P都不能被M获取, 也就是说所有的go代码(除了当前的)都会停止运行, 并且不能运行新的go代码

*调用startTheWorldWithSema重新启动世界
	*调用procresize会返回含有可运行任务g的P的链表
		*设置p.status = _Pidle
		*如果p没有任务g, 调用pidleput把P添加到空闲P链表中
		*如果p有任务g, 调用mget从空闲M链表sched.midle获取一个空闲的M给p.m, 添加入可运行任务g的P的链表
	*解除gc等待标记sched.gcwaiting = 0
	*遍历p链表, 调用notewakeup唤醒可运行p
		*如果p.m存在, 则m.nextp = p.m, 调用notewakeup唤醒
		*如果p.m不存在, 则调用newm新建一个M
			*newm会新建一个m的实例, 所有新建的m添加到allm链表, 且不被释放, 设置m启动函数mspinning(设置自旋状态spinning=true), m的实例包含一个新创建的g0, 然后调用newosproc创建一个系统线程(allm链表超出最大限制会导致进程崩溃, 默认10000)
			*设置自旋状态spinning=true
			*设置nextp
	
gcStart
	*如果mode == gcBackgroundMode, 调用gcBgMarkStartWorkers启动扫描任务
		*分别对每个P启动一个gcBgMarkWorker用于启动后台标记任务, 这个gcBgMarkWorker后台标记任务在协程M获取G时调用的findRunnableGCWorker中触发
		
	*调用gcResetMarkState重置标记相关的状态
	
	*调用stopTheWorldWithSema停止所有运行中的G, 并禁止它们运行
		
	*世界已停止(STW)！！！！！
	
	*调用finishsweep_m清扫上一轮GC未清扫的span, 确保上一轮GC已完成
		*循环调用sweepone返回^uintptr(0)
	
	*调用clearpools清理sched.sudogcache和sched.deferpool, 让它们的内存可以被回收
	
	*如果mode == gcBackgroundMode
		*调用startCycle标记开始了新一轮的GC
			*计算可以同时执行的后台标记任务的数量
			*重置P中的辅助GC所用的时间统计 p.gcAssistTime = 0
			*调用revise计算辅助GC的参数
		
		*调用setGCPhase(_GCmark)设置全局变量中的GC状态为_GCmark, 然后启用写屏障, 然后启用写屏障
			*gcphase = _GCmark
			*writeBarrier.needed = gcphase == _GCmark
			*writeBarrier.enabled = writeBarrier.needed
			
		*调用gcBgMarkPrepare重置后台标记任务的计数
			*work.nproc = 0
			*work.nwait = 0
			
		*调用gcMarkRootPrepare计算扫描根对象的任务数量
			*计算所有p的mcache数量 nFlushCacheRoots = gomaxprocs
			*遍历activeModules计算扫描可读写的全局变量的数量 nDataRoots = datap.edata - datap.data
			*遍历activeModules计算扫描只读的全局变量的数量 nBSSRoots = datap.ebss - datap.bss
			*计算扫描span数量 nSpanRoots = mheap_.sweepSpans[mheap_.sweepgen/2%2] (内存分配调用mheap_.alloc向mheap申请一个新的span时会加入, 单个span执行sweep仍使用也会加入)
			*计算扫描各个G的栈的数量 nStackRoots = allglen
			*计算总数量, 后台标记任务会对markrootNext进行原子递增, 来决定做哪个任务
			*markrootJobs = fixedRootCount + nFlushCacheRoots + nDataRoots + nBSSRoots + nSpanRoots  + nStackRoots
		
		*调用gcMarkTinyAllocs标记所有tiny alloc等待合并的对象
			
		*启用辅助GC gcBlackenEnabled = 1
		
		*调用startTheWorldWithSema重新启动世界
		
		*世界已重新启动！！！！！
	
	

/*************************************标记工作*****************************************************/
P
gcw：P的GC工作缓冲区缓存, 由写barriers填补, 由mutator assists助手消耗


重启世界后各个M会重新开始调度, 调度时会优先使用上面提到的findRunnableGCWorker函数查找任务, 之后就有大约25%的P运行后台标记任务
gcBgMarkWorker函数用于后台标记任务
gcBgMarkWorker
	*for循环
		*让当前G进入休眠, findRunnableGCWorker会唤醒当前G
		*切换g0, 判断后台标记的任务模式 _p_.gcMarkWorkerMode
			*gcMarkWorkerDedicatedMode
				*调用gcDrain(&_p_.gcw, gcDrainUntilPreempt|gcDrainFlushBgCredit), 这个模式下P应该专心执行标记, 直到被抢占, 并且需要计算后台的扫描量来减少辅助GC和唤醒等待中的G
				*被抢占时把本地运行队列中的所有G都踢到全局运行队列
				*继续执行标记, 直到无更多任务, 并且需要计算后台的扫描量来减少辅助GC和唤醒等待中的G
			*gcMarkWorkerFractionalMode
				*调用gcDrain(&_p_.gcw, gcDrainUntilPreempt|gcDrainFlushBgCredit), 这个模式下P应该适当执行标记, 直到被抢占, 并且需要计算后台的扫描量来减少辅助GC和唤醒等待中的G
			*gcMarkWorkerIdleMode
				*调用gcDrain(&_p_.gcw, gcDrainIdle|gcDrainUntilPreempt|gcDrainFlushBgCredit), 这个模式下P只在空闲时执行标记, 执行标记, 直到被抢占或者达到一定的量, 并且需要计算后台的扫描量来减少辅助GC和唤醒等待中的G
		*判断是否所有后台标记任务都完成, 并且没有更多的任务
			*调用gcMarkDone准备进入完成标记阶段


gcDrain函数用于执行标记
gcDrain
	*如果根对象未扫描完, 则先扫描根对象
		*从根对象扫描队列取出一个值work.markrootNext, 调用markroot执行根对象扫描工作
	*从本地标记队列p.gcw中获取对象
	*调用scanobject扫描获取到的对象
	*done
		*把扫描的对象数量添加到全局
		
/*************************************标记工作*****************************************************/
		
		
		
/******扫描根, 标记对象为灰色*****/
/*对象包含指针(所在span的类型是scan) 标记为灰色*/
markroot函数用于执行根对象扫描工作, 根据work.markrootNext的数值决定执行哪种任务
markroot
	*fixedRootCount <= work.markrootNext
		*调用flushmcache释放p.mcache中的所有span, 要求STW
			*循环遍历p.mcache.alloc的span, 调用mheap_.central[i].mcentral.uncacheSpan函数释放这个span
				*如果span中有可分配的对象, 则将span从empty移除并插入nonempty
	*baseData(baseFlushCache + uint32(work.nFlushCacheRoots)) <= work.markrootNext
		*遍历activeModules调用markrootBlock扫描可读写的[datap.data, datap.edata-datap.data]全局变量
	*baseBSS(baseData + uint32(work.nDataRoots)) <= work.markrootNext
		*遍历activeModules调用markrootBlock扫描只读的[datap.bss, datap.ebss-datap.bss]全局变量
	*fixedRootFinalizers == work.markrootNext
		*scanblock扫描析构器队列
	*fixedRootFreeGStacks == work.markrootNext
		*释放已中止的G的栈, 调用shrinkstack
	*baseSpans(baseBSS + uint32(work.nBSSRoots)) <= work.markrootNext
		*扫描各个span中特殊对象(析构器列表)
	*default扫描各个G的栈
		*baseStacks(baseSpans + uint32(work.nSpanRoots)) <= work.markrootNext
		*work.markrootNext - baseStacks获取需要扫描的G
		*调用scang扫描G的栈
	
markrootBlock函数负责扫描内存块[b0, b0+n0]全局变量
markrootBlock
	*调用scanblock扫描内存块[b0, b0+n0]

scanblock函数是一个通用的扫描函数, 扫描全局变量和栈空间都会用它, 和scanobject不同的是bitmap需要手动传入
scanblock
	*枚举扫描的地址
		*找到bitmap中对应的byte
			*枚举byte
				*如果该地址的对象包含指针且对象堆中分配, 调用greyobject标记在该地址的对象存活, 并把它加到标记队列(该对象变为灰色)
	
	
greyobject用于标记一个对象存活, 并把它加到标记队列(该对象变为灰色)
greyobject
	*设置对象所在的span中的gcmarkBits对应的bit为1
	*如果确定对象不包含指针(所在span的类型是noscan), 则不需要把对象放入标记队列(对象提前标记为黑色)
	*把含有指针的对象放入标记队列gcw
	
scang函数负责扫描G的栈
scang
	*循环扫描
		*获取当前G的状态
		*_Gdead
			*G已中止, 不需要扫描它
		*_Gcopystack
			*G的栈正在扩展, 下一轮重试
		*_Grunnable, _Gsyscall, _Gwaiting
			*G不在运行中, 调用scanstack扫描它的栈
		*_Grunning
			*G正在运行, 设置preemptscan为true抢占G, 抢占成功时G会调用scanstack扫描它自己
			
scanstack函数用于扫描栈
scanstack
	*shrinkstack计算当前使用的空间, 小于栈空间的 1/4 的话, 执行栈的收缩, 将栈收缩为现在的 1/2, 否则直接返回
	*scanframeworker会根据代码地址(pc)获取函数信息, 然后找到函数信息中的stackmap.bytedata, 它保存了函数的栈上哪些地方有指针, 再调用scanblock来扫描函数的栈空间, 同时函数的参数也会这样扫描
	*gentraceback枚举所有调用帧, 分别调用scanframe函数
	
/******扫描根, 标记对象为灰色*****/


	
/******扫描标记队列中对象*****/
gcDrain函数扫描完根对象, 就会开始消费标记队列, 对从标记队列中取出的对象调用scanobject函数
scanobject
	*获取对象对应的bitmap
	*获取对象所在的span
	*获取span.elemsize对象的大小
	*取出指针的值
	*如果指针的值在arena区域中, 则调用greyobject标记对象并把对象放到标记队列中
	
/******扫描标记队列中对象*****/
	

/******完成标记阶段*****/
在所有后台标记任务都把标记队列消费完毕时, 会执行gcMarkDone函数准备进入完成标记阶段(mark termination):
在并行GC中gcMarkDone会被执行两次, 第一次会禁止本地标记队列然后重新开始后台标记任务, 第二次会进入完成标记阶段(mark termination)
gcMarkDone
	*暂时禁止启动新的后台标记任务
	*如果本地标记队列未禁用
		*禁用本地标记队列gcBlackenPromptly = true
		*把所有本地标记队列中的对象都推到全局标记队列
		*允许启动新的后台标记任务
		*所有后台标记任务都完成, 并且没有更多的任务跳到函数顶部, 当做是第二次调用了
	*本地标记队列已禁用
		*停止所有运行中的G, 并禁止它们运行
		*世界已停止(STW)！！！！！
		*标记对根对象的扫描已完成, 会影响gcMarkRootPrepare中的处理work.markrootDone = true
		*禁止辅助GC和后台标记任务的运行gcBlackenEnabled = 0
		*调用gcWakeAllAssists唤醒所有因为辅助GC而休眠的G
		*调用gcMarkTermination进入完成标记阶段, 会重新启动世界

gcMarkTermination函数会进入完成标记阶段
gcMarkTermination
	*禁止辅助GC和后台标记任务的运行gcBlackenEnabled = 0
	*重新允许本地标记队列(下次GC使用)gcBlackenPromptly = false
	*设置当前GC阶段到完成标记阶段, 并启用写屏障
	*切换到g0运行
	*设置当前GC阶段到关闭, 并禁用写屏障
	*调用gcSweep唤醒后台清扫任务, 将在STW结束后开始运行
	*重置清扫状态sweep.nbgsweep = 0, sweep.npausesweep = 0
	*唤醒等待清扫的G
	*重新启动世界
	*世界已重新启动！！！！！
	
/******完成标记阶段*****/


/*********************************清扫任务*********************************/
	
	
gcSweep函数会唤醒后台清扫任务
	*所有span都会变为等待清扫的span, mheap_.sweepgen += 2
	*mheap_.sweepdone = 0
	*mheap_.sweepSpans[mheap_.sweepgen/2%2]应该为空的链表, 在上一次sweep阶段清空这个链表
	*调用ready唤醒bgsweep(sweep.g)
	
	

bgsweep函数用于后台清扫任务, 在程序runtime.main启动时会启动一个, 进入清扫阶段时被gcSweep唤醒
	*sweep.g = getg()用于gcSweep调用ready唤醒这个任务
	*调用goparkunlock睡眠并且等待唤醒
	*循环清扫
		*调用gosweepone清扫一个span, sweepone返回不等于^uintptr(0)则进入调度(一次只做少量工作)
			*切换到g0运行sweepone
		*如果mheap_.sweepdone == 0 清扫未完成则继续循环
		*否则调用goparkunlock睡眠并且等待唤醒


sweepone函数用于清扫单个span
	*如果mheap_.sweepdone != 0, 则返回^uintptr(0)
	*循环
		*从mheap_.sweepSpans[1-sweepgen/2%2]中取出一个span
		*如果mheap_.sweepSpans[1-sweepgen/2%2]链表为空则表示清扫完成, mheap_.sweepdone + 1
		*如果s.state != mSpanInUse, 其他M已经在清扫这个span时跳过
		*span.sweepgen != 全局sweepgen-2(表示span已经或者正在sweep), 则跳过
		*设置span.sweepgen == 全局sweepgen-1(表示span正在sweep)
		*调用sweep清扫单个span后break跳出循环




sweep函数用于清扫单个span
sweep
	*调用countAlloc通过gcmarkBits计算存活对象数量nalloc
	*设置新的allocCount=nalloc
	*重置freeindex=0, 下次分配从0开始搜索
	*allocBits变为新的gcmarkBits, 下次分配对象时可以根据allocBits得知哪些元素是未分配的
	*调用newMarkBits重新分配一块全部为0的gcmarkBits为下次GC准备
	*调用refillAllocCache更新freeindex(此时freeindex被之前初始化为0)开始的allocCache, 整数型的allocCache用于缓存freeindex开始的bitmap, 缓存的bit值与原值allocBits相反
	*如果span的类型是0(大对象)并且其中的对象已经不存活则释放到heap或者span中的所有对象都存活, 则更新sweepgen到最新
	*如果span的类型不是0(非大对象)并且有需要释放的对象, 则mheap_.central[spc].mcentral调用freeSpan把span加入到mcentral中
	*如果span的类型是0(大对象)并且其中的对象已经不存活则调用mheap_.freeSpan释放span到heap
	*如果span还在使用中, 则将span放入mheap_.sweepSpans[sweepgen/2%2]中
	
	
	

freeSpan函数用于更新mcentral中nonempty和empty链表, 返回值res代表该span是否归还heap
	*如果preserve为true, 则将span的sweepgen设置为最新一代, 返回false preserve代表保留, 不归还给heap (sweepone调用sweep传递preserve为false, 申请分配内存cacheSpan对等待sweep的span调用sweep传递preserve为true)
	*如果wasempty为true, 将span从empty移除并插入nonempty
	*将span的sweepgen设置为最新一代
	*如果allocCount不为0(该span有分配的对象), 则返回false
	*该span没有分配的对象, 则将span从nonempty移除并且调用mheap_.freeSpan(调用freeSpanLocked将span归还到heap的free/freelarge链表), 返回true
	
	
	
	
	