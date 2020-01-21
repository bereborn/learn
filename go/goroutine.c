G的状态
空闲中(_Gidle): 表示G刚刚新建, 仍未初始化
待运行(_Grunnable): 表示G在运行队列中, 等待M取出并运行
运行中(_Grunning): 表示M正在运行这个G, 这时候M会拥有一个P
系统调用中(_Gsyscall): 表示M正在运行这个G发起的系统调用, 这时候M并不拥有P
等待中(_Gwaiting): 表示G在等待某些条件完成, 这时候G不在运行也不在运行队列中(可能在channel的等待队列中)
已中止(_Gdead): 表示G未被使用, 可能已执行完毕(并在freelist中等待下次复用)
栈复制中(_Gcopystack): 表示G正在获取一个新的栈空间并把原来的内容复制过去(用于防止GC扫描)
M的状态
M并没有像G和P一样的状态标记, 但可以认为一个M有以下的状态:

自旋中(spinning): M正在从运行队列获取G, 这时候M会拥有一个P
执行go代码中: M正在执行go代码, 这时候M会拥有一个P
执行原生代码中: M正在执行原生代码或者阻塞的syscall, 这时M并不拥有P
休眠中: M发现无待运行的G时会进入休眠, 并添加到空闲M链表中, 这时M并不拥有P
自旋中(spinning)这个状态非常重要, 是否需要唤醒或者创建新的M取决于当前自旋中的M的数量.

P的状态
空闲中(_Pidle): 当M发现无待运行的G时会进入休眠, 这时M拥有的P会变为空闲并加到空闲P链表中
运行中(_Prunning): 当M拥有了一个P后, 这个P的状态就会变为运行中, M运行G会使用这个P中的资源
系统调用中(_Psyscall): 当go调用原生代码, 原生代码又反过来调用go代码时, 使用的P会变为此状态
GC停止中(_Pgcstop): 当gc停止了整个世界(STW)时, P会变为此状态
已中止(_Pdead): 当P的数量在运行时改变, 且数量减少时多余的P会变为此状态




G里面比较重要的成员如下:

stack: 当前g使用的栈空间, 有lo和hi两个成员, stack.lo为栈空间的低地址, stack.hi为栈空间的高地址
stackguard0: 检查栈空间是否足够的值, 低于这个值会扩张栈, 0是go代码使用的
stackguard1: 检查栈空间是否足够的值, 低于这个值会扩张栈, 1是原生代码使用的
m: 当前g对应的m
sched: g的调度数据, 当g中断时会保存当前的pc和rsp等值到这里, 恢复运行时会使用这里的值
atomicstatus: g的当前状态
schedlink: 下一个g, 当g在链表结构中会使用
preempt: g是否被抢占中
lockedm: g是否要求要回到这个M执行, 有的时候g中断了恢复会要求使用原来的M执行


M里面比较重要的成员如下:

g0: 提供系统栈空间, 用于调度的特殊g, 调度和执行系统调用时会切换到这个g
mstartfn: 启动函数
curg: 当前运行的g
p: 当前拥有的P
nextp: 唤醒M时, M会拥有这个P
spinning: 自旋状态
park: M休眠时使用的信号量, 唤醒M时会通过它唤醒
schedlink: 下一个m, 当m在链表结构中会使用
mcache: 分配内存时使用的本地分配器, 和p.mcache一样(拥有P时会复制过来)
lockedg: lockedm的对应值


P里面比较重要的成员如下:

status: p的当前状态
link: 下一个p, 当p在链表结构中会使用
m: 拥有这个P的M
mcache: 分配内存时使用的本地分配器
runqhead: 本地运行队列的出队序号
runqtail: 本地运行队列的入队序号
runq: 本地运行队列的数组, 可以保存256个G
runnext: 下一个优先运行的G
gfree: G的自由列表, 保存变为_Gdead后可以复用的G实例
gcBgMarkWorker: 后台GC的worker函数, 如果它存在M会优先执行它
gcw: GC的本地工作队列, 详细将在下一篇(GC篇)分析

P为线程提供执行资源, 比如对象分配内存mcache, 本地任务队列runq等, 线程独享所绑定的P资源, 可在无锁状态下执行高效的操作
G并非执行体, 仅仅保存并发任务状态, 为任务执行提供所需要的栈内空间, G任务创建后被放置在P本地队列或全局队列, 等待工作线程调度执行
实际执行的是系统线程m, 它和p绑定, 以循环调度的方式不停执行g并发任务, m通过修改寄存器将执行栈指向G自带的栈内存, 并在此空间分配堆栈, 执行任务函数, 当中途需要切换时, 只需将相关寄存器保存到g.sched即可维持状态, 任何m可以根据g.sched恢复执行
p一般和CPU核数绑定, m则根据需要按需创建, 比如当m因陷入系统调用长时间阻塞时, p被监控线程sysmon抢回, 去新建或者换行m执行其他任务
g的初始栈只有2kb, 且创建只在用户空间简单的对象分配, 远比进入内核分配线程简单多, 调度器让多个m进入调度循环, 不停获取并行任务

M执行G并发任务有两个起点: 线程启动mstart, 还有stopm休眠唤醒后再调度恢复调度循环
go需要保证有足够的M可以运行G, 是通过这样的机制实现的:
	*newproc入队待运行的G后, 如果当前无自旋的M但是有空闲的P, 就唤醒或者新建一个M
	*schedule成功获取一个G, 当M离开自旋状态并准备运行出队的G时, 如果当前无自旋的M但是有空闲的P, 就唤醒或者新建一个M
	*schedule调用findrunnable获取不到G, 当M离开自旋状态并准备休眠时, 会在离开自旋状态后再次检查所有运行队列, 如果有待运行的G则重新进入自旋状态
wakep	---	sched.nmspinning累计自旋数, 避免newproc1唤醒过多线程	startm(nil, true)设置自旋状态为true
findrunnable	---	2*sched.nmspinning >= procs-sched.npidle 如果旋转m的两倍数量 >= 忙碌的数量P, 则跳转到stop, 释放P, 先让M离开自旋状态(再次检查所有P的本地运行队列, 如果不为空则让M重新进入自旋状态), sched.nmspinning减少自旋数, 调用stopm
schedule	---	成功获取一个G调用resetspinning, 让M离开自旋状态, sched.nmspinning减少自旋数, 如果当前无自旋的M但是有空闲的P, 就唤醒或者新建一个M

findrunnable中stop先让M离开自旋状态再次检查所有P的本地运行队列, 如果反过来, 在检查所有P的本地运行队列之后M离开自旋状态之前, 有新提交的gogoroutine, 结果没人会休眠线程去执行goroutine

用户逻辑用的g的栈, g任务在暂停时, 防止多个g任务共享栈引发混乱
执行管理操作时, 如执行垃圾回收操作时, 收缩被线程持有的g栈空间, 需要切换到g0的栈执行, 与用户逻辑隔离
systemstack用于切换当前的g到g0, 并且使用g0的栈空间, 然后调用传入的函数, 再切换回原来的g和原来的栈空间
systemstack
	*如果当前g是g0则无需切换
	*切换用户逻辑g, 保存用户g状态到g.sched
	*从g0.sched获取g0的栈stack, 通过sp寄存器切换到g0的栈内存
	*执行系统管理函数
	*切换回用户g, 恢复执行现场
	


-- gfree ---------+
|			  	  |
--> _Gidle --> _Gdead ------> _Grunnable ---> _Grunning ---> _Gdead --- ... --> gfree -->
	 新建     初始化前		   初始化后			执行中		 执行完


rt0_go：

*设置g0到TLS中, 表示当前的g是g0
*设置m0.g0 = g0	
*设置g0.m = m0
*runtime.schedinit
	*设置最大m数量sched.maxmcount = 10000
	*调用stackinit初始化栈空间复用管理链表
	*调用mcommoninit初始当前m
	*生成P的处理在procresize
		*allp   [_MaxGomaxprocs + 1]*p	生成p
		*调用allocmcache为p.mcache创建cache
		*设置g0.m.p = allp[0]
		*设置allp[0].m = m0
		*allp数组中p.runq没有任务,将p添加sched中的空闲p链表sched.pidle

*runtime.newproc创建一个新的goroutine, 指向的是runtime.main, go func(...)会被编译器翻译成newproc
	*计算额外参数的地址argp
	*获取调用端的地址(返回地址)pc
	*用g0创建g对象
	*使用systemstack调用newproc1, systemstack会切换当前的g到g0, 并且使用g0的栈空间, 然后调用传入的函数, 再切换回原来的g和原来的栈空间
		*调用getg获取当前的g, 会编译为读取FS寄存器(TLS), 这里会获取到g0
		*设置g0对应的m的locks++, 禁止抢占
		*获取m0拥有的p(allp[0])
		*新建一个g
			*首先调用gfget从p.gfree获取g, 如果之前有g被回收在这里就可以复用
			*获取不到时调用malg分配一个g, 调用stackalloc初始的栈空间大小是2K
			*需要先设置g的状态为已中止(_Gdead), 这样gc不会去扫描这个g的未初始化的栈
			*将g添加到allgs数组, 这是垃圾回收遍历扫描需要, 以便获取指针引用, 收缩栈空间
		*设置g将执行参数拷贝入栈
		*设置调度数据g.sched初始化用于保存执行现场的区域
			*设置g.sched.sp等于参数+返回地址后的rsp地址
			*设置g.sched.pc等于目标函数的地址, 查看gostartcallfn和gostartcall, 返回地址是goexit, 表示调用完目标函数后会调用goexit
			*设置g.sched.g等于g
		*设置g的状态为待运行(_Grunnable)
		*设置g唯一id号
		*调用runqput把g放到运行队列(任务队列分为三级，按优先级从高到低分别是 P.runnext、P.runq、Sched.runq)
			*首先随机把g放到p.runnext, 如果放到runnext则入队原来在runnext的g
			*然后尝试把g放到P的本地运行队列runq(无锁操作)
			*如果本地运行队列满了则调用runqputslow把g放到全局运行队列sched.runq(加锁操作)
				*runqputslow会把本地运行队列中一半的g放到全局运行队列, 这样下次就可以继续用快速的本地运行队列了
				
		*如果当前有空闲的P, 但是无自旋的M(nmspinning等于0), 并且主函数已执行(rt0_go中主函数未执行mainStarted = false)则调用wakep唤醒或新建一个M
			*首先nmspinning累加, 成功再继续, 避免多个线程同时执行wakep唤醒过多线程
			*wakep调用startm函数(startm(nil, true))
				*调用pidleget从空闲P链表sched.pidle获取一个空闲的P
				*调用mget从空闲M链表sched.midle获取一个空闲的M
				*如果没有空闲的M, 则调用newm新建一个M
					*newm会新建一个m的实例, 所有新建的m添加到allm链表, 且不被释放, 设置m.nextp = _p_, 设置m启动函数mspinning(设置自旋状态spinning=true), m的实例包含一个新创建的g0, 然后调用newosproc创建一个系统线程(allm链表超出最大限制会导致进程崩溃, 默认10000)
				*设置自旋状态spinning=true
				*设置nextp = P
				*调用notewakeup(&mp.park)唤醒线程
				
*执行mstart
	*调用getg获取当前的g, 这里会获取到g0
	*如果g未分配栈则从当前的栈空间(系统栈空间)上分配, 也就是说g0会使用系统栈空间
	*调用mstart1函数
		*调用gosave函数保存当前的状态到g0的调度数据中, 以后每次调度都会从这个栈地址开始
		*调用asminit函数, 不做任何事情
		*调用minit函数, 设置当前线程可以接收的信号(signal)
		*如果m.mstartfn不为空, 则执行m.mstartfn
		*调用acquirep关联m.nextp和当前m	(wakep函数调用startm会设置m.nextp, newm函数会设置m.nextp)
		*调用schedule函数
		*top:
			*如果sched.gcwaiting != 0, 调用gcstopm	(在GC调用stopTheWorldWithSema停止整个世界时,会设置sched.gcwaiting = gomaxprocs)
				*设置m.spinning = false
				*调用releasep
				*设置_p_.status = _Pgcstop
				*sched.stopwait--
				*如果sched.stopwait = 0 则调用notewakeup(&sched.stopnote)	(在GC调用stopTheWorldWithSema停止整个世界时,会调用notetsleep(&sched.stopnote)等待所有m停止运行)
				*调用stopm休眠当前的M
				*从stopm唤醒后从schedule函数的顶部top开始执行
			
			*如果gcBlackenEnabled != 0 (gcStart会启用辅助GC gcBlackenEnabled = 1), 调用findRunnableGCWorker用于返回gcBgMarkWorker这个后台标记任务g
			
			*为了公平起见, 每61次调度从全局运行队列获取一次G
			*从P的本地运行队列中获取G, 调用runqget函数
			
			*快速获取失败时, 调用findrunnable函数获取待运行的G, 会阻塞到获取成功为止
			*top:
				*如果sched.gcwaiting != 0, 调用gcstopm
				*从本地队列获取
				*从全局队列获取
				*检查netpoll任务
				*设置工作线程m自旋状态, 随机挑选P, 调用runqsteal窃取任务
					*计算runq转移任务数量 n-n/2
					*如果没有尝试偷runnext
					*转移任务_p_.runq
					*修改被窃取P队列状态_p_.runqhead
			*stop:
				*一无所得开始休眠步骤
				*再次检查全局运行队列中是否有G, 有则获取并返回
				*调用releasep释放M拥有的P, P会变为空闲(_Pidle)状态
				*调用pidleput把P添加到空闲P链表中
				*让M离开自旋状态
				*首先减少表示当前自旋中的M的数量的全局变量nmspinning
				*再次检查所有P的本地运行队列, 如果不为空则让M重新进入自旋状态, 并跳到findrunnable的顶部重试
				*再次检查网络事件反应器是否有待运行的G, 这里对netpoll的调用会阻塞, 直到某个fd收到了事件
				*如果最终还是获取不到G, 调用stopm休眠当前的M
				*唤醒后跳到findrunnable的顶部top重试
			
			*成功获取到一个G
			*调用resetspinning, 让M离开自旋状态
				*如果当前有空闲的P(sched.npidle大于0, P创建时p.runq没有任务, 将p添加sched中的空闲p链表sched.pidle), 但是无自旋的M(nmspinning等于0), 则调用wakep唤醒或新建一个M
			*如果G要求回到指定的M(例如上面的runtime.main)
				*调用startlockedm函数把G和P交给该M, 自己进入休眠
				*从休眠唤醒后跳到schedule的顶部重试
				
			*调用execute函数执行G
				*把G的状态由待运行(_Grunnable)改为运行中(_Grunning)
				*设置G的stackguard, 栈空间不足时可以扩张
				*如果执行本地队列(inheritTime=true)则增加P中记录的调度次数(对应上面的每61次优先获取一次全局运行队列)
				*设置g.m.curg = g
				*设置g.m = m
				*调用gogo函数
					*这个函数会根据g.sched中保存的状态恢复各个寄存器的值并继续运行g(如果G被抢占或者等待资源而进入休眠, 在休眠前会保存状态到g.sched)
					*首先针对g.sched.ctxt调用写屏障(GC标记指针存活), ctxt中一般会保存指向[函数+参数]的指针
					*设置TLS中的g为g.sched.g, 也就是g自身
					*设置rsp寄存器为g.sched.rsp
					*设置rax寄存器为g.sched.ret
					*设置rdx寄存器为g.sched.ctxt (上下文)
					*设置rbp寄存器为g.sched.rbp
					*清空sched中保存的信息
					*跳转到g.sched.pc(g.sched.pc在G首次运行时会指向目标函数的第一条机器指令, g.sched.pc会变为唤醒后需要继续执行的地址)
					*函数运行完毕返回时将会调用goexit函数, (因为前面创建goroutine的newproc1函数把G栈顶端被压入了goexit地址, 所以函数返回地址为goexit)
						*goexit函数会调用goexit1函数
						*goexit1通过mcall调用goexit0函数, mcall这个函数保存当前的运行状态到g.sched, 然后切换到g0和g0的栈空间, 再调用指定的函数(回到g0的栈空间这个步骤非常重要, 因为这个时候g已经中断, 继续使用g的栈空间且其他M唤醒了这个g将会产生灾难性的后果)
							*设置g.sched.pc等于当前的返回地址
							*设置g.sched.sp等于寄存器rsp的值
							*设置g.sched.g等于当前的g
							*设置g.sched.bp等于寄存器rbp的值
							*切换TLS中当前的g等于m.g0
							*设置寄存器rsp等于g0.sched.sp, 使用g0的栈空间
							*设置第一个参数为原来的g
							*设置rdx寄存器为指向函数地址的指针(上下文)
							*调用指定的函数, 不会返回
						*goexit0函数调用时已经回到了g0的栈空间
							*把G的状态由运行中(_Grunning)改为已中止(_Gdead)
							*清空G的成员
							*调用dropg函数解除M和G之间的关联
							*调用gfput函数把G放到P的自由列表中, 下次创建G时可以复用
							*调用schedule函数继续调度

	
acquirep用于关联p和当前m
*acquirep
	*检查m.p = 0 且 m.mcache = nil
	*检查_p_.m = 0 且  _p_.status = _Pidle
	*设置m.p = _p_
	*设置_p_.m = m
	*设置_p_.status = _Prunning
	*设置m.mcache = _p_.mcache
	
releasep用于取消关联p和当前m
*releasep
	*检查m.p != 0 且 m.mcache != nil
	*检查_p_.m = 0 且  _p_.status = _Prunning
	*设置m.p = 0
	*设置_p_.m = 0
	*设置_p_.status = _Pidle
	*设置m.mcache = nil
	
	
stopm用于停止当前m的执行直到新的work来临
*stopm
	*检查m.p = 0
	*检查m.spinning = false
	*调用mput将m放入空闲M链表sched.midle
	*调用notesleep(&_g_.m.park)睡眠m
	*此处被唤醒, 调用noteclear(&_g_.m.park)清除m.park
	*调用acquirep用于关联m.nextp和当前m (?????????????????来源nextp)
	*设置m.nextp = 0


wakep用于唤醒m执行
wakep
	*调用startm(nil, true)
	
startm
	*如果没有指定p, 调用pidleget尝试获取空闲p, 获取失败则返回终止
	*调用mget从空闲M链表sched.midle获取一个空闲的M
	*如果没有闲置的m则调用newm新建
	*设置m.spinning
	*设置m.nextp = p
	*调用notewakeup唤醒m
	
newm
	*调用allocm创建一个m对象, 并创建m.g0, g0是默认的8kb栈内存, 栈内存地址被传到newosproc函数作为系统线程默认堆栈空间
	*调用newosproc创建系统线程, clone(cloneFlags, unsafe.Pointer(mp.g0.stack.hi), unsafe.Pointer(mp), unsafe.Pointer(mp.g0), unsafe.Pointer(funcPC(mstart)))
	
	
							
*第一个被调度的G会运行runtime.main
	*标记主函数已调用, 设置mainStarted = true
	*启动一个新的M执行sysmon函数, 这个函数会监控全局的状态并对运行时间过长的G进行抢占
	*要求G必须在当前M(系统主线程)上执行
	*调用runtime_init函数
	*调用gcenable函数创建bgsweep
	*调用main.init函数, 如果函数存在
	*不再要求G必须在当前M上运行
	*如果程序是作为c的类库编译的, 在这里返回
	*调用main.main函数
	*如果当前发生了panic, 则等待panic处理
	*调用exit(0)退出程序
		
		
		
		
runtime.main启动一个新的M执行sysmon函数, 这个函数会监控全局的状态并对运行时间过长的G进行抢占	
*sysmon		
	*retake函数负责处理抢占
		*枚举allp数组所有的P
		*如果P在系统调用中(_Psyscall), P有其他的任务, 且经过了一次sysmon循环(20us~10ms), 则调用handoffp解除M和P之间的关联来抢占这个P
			*更新syscall统计信息
			*P仍有任务则调用startm函数(_p_, false)
				*调用mget从空闲M链表sched.midle获取一个空闲的M
				*如果没有空闲的M, 则调用newm新建一个M
					*newm会新建一个m的实例, 所有新建的m添加到allm链表, 且不被释放, 设置m启动函数mspinning(设置自旋状态spinning=true), m的实例包含一个新创建的g0, 然后调用newosproc创建一个系统线程(allm链表超出最大限制会导致进程崩溃, 默认10000)
					*设置自旋状态spinning=true
					*设置nextp
				*调用notewakeup(&mp.park)唤醒线程
			*否则调用pidleput将P放入空闲P链表sched.pidle
		*如果P在运行中(_Prunning), 且经过了一次sysmon循环并且G运行时间超过forcePreemptNS(10ms), 则抢占这个P
			*更新运行g统计信息
			*调用preemptone函数
				*设置g.preempt = true
				*设置g.stackguard0 = stackPreempt
	
		
通过两个标志实现抢占, 实际起作用的是g.stackguard0, g.preempt只是后备, 以便再stackguard0做溢出检查标志时, 依然可以用preempt恢复抢占状态
栈扩张调用的是morestack_noctxt函数, morestack_noctxt函数清空rdx寄存器并调用morestack函数, morestack函数会保存G的状态到g.sched, 切换到g0和g0的栈空间, 然后调用newstack函数

newstack函数判断g.stackguard0等于stackPreempt, 就知道这是抢占触发的, 这时会再检查一遍是否要抢占:
*newstack
	*preempt = true需要抢占
		*如果M被锁定(函数的本地变量中有P), 则跳过这一次的抢占并调用gogo函数继续运行G
		*如果M正在分配内存, 则跳过这一次的抢占并调用gogo函数继续运行G
		*如果M设置了当前不能抢占, 则跳过这一次的抢占并调用gogo函数继续运行G
		*如果M的状态不是运行中, 则跳过这一次的抢占并调用gogo函数继续运行G
		*判断gp.preemptscan, 为true是GC对栈空间执行标记触发的抢占, 调用scanstack扫描g的栈空间 (在GC调用scang扫描G的栈时,如果G的状态为_Grunning则发起抢占并preemptscan = true)
		*调用gopreempt_m完成抢占
即使这一次抢占失败, 因为g.preempt等于true, runtime中的一些代码会重新设置stackPreempt以重试下一次的抢占

*gopreempt_m
	*调用goschedImpl
		*把G的状态由运行中(_Grunning)改为待运行(_Grunnable)
		*调用dropg函数解除M和G之间的关联
		*调用globrunqput把G放到全局运行队列
		*调用schedule函数继续调度
因为全局运行队列的优先度比较低, 各个M会经过一段时间再去重新获取这个G执行, 抢占机制保证了不会有一个G长时间的运行导致其他G无法运行的情况发生
		
		
		
		
		
为支持并发调度, 专门对 syscall cgo 进行了包装, 以便在长时间阻塞时能切换执行其他任务, 标准库 syscall 包里, 将相关系统调用函数分为 Syscall 和 RawSyscall 两类, Syscall 增加了 entrysyscall/exitsyscall
监控线程sysmon对syscall很重要, 因为它负责将因系统调用而长时间阻塞的P抢回, 用于执行任务, 否则整体性能严重下降
*entersyscall
	*reentersyscall
		*调用save保存执行状态, sp、pc寄存器等值保存到g.sched区域
		*设置G的状态由运行中(_Grunning)改为系统调用中(_Gsyscall)
		*设置p.status = _Psyscall
		
*exitsyscall
	*调用exitsyscallfast函数重新绑定原有或者空闲的P, 以继续执行当前G任务
		*g.m.p != 0 且 p.status = _Psyscall 则调用exitsyscallfast_reacquired尝试关联原来的P
			*设置_g_.m.p.m = _g_.m
		*如果原来的P关联失败则调用exitsyscallfast_pidle函数尝试获取其他空闲的P
			*设置_g_.m.p = _p_
			*设置_p_.m = _g_.m
	*g.m的P绑定成功, 设置G的状态由系统调用中(_Psyscall)改为运行中(_Grunning), 调用excute执行g
	*如果多次尝试绑定P失败, 只能调用exitsyscall0函数将任务放入待运行队列中
		*设置_g_.m.curg = nil
		*调用globrunqput将任务放入全局的待运行队列中
		*调用stopm停止当前m直到有新任务
		*从stopm唤醒后从schedule函数开始执行
	
	
	
notesleep既不让出m, 也不会让g重新回到任务队列
适合stopm这种获取不到g需要睡眠m的场景
futex快速用户区互斥, 是一种在用户空间实现锁互斥机制, 多进程或多线程通过共享一块内存(整数)实现等待和唤醒操作
notesleep
	*调用futexsleep睡眠m

notewakeup
	*调用futexwakeup唤醒m
	
	
	
	
	
mcall这个函数保存当前的运行状态到g, 包括sp、pc寄存器等值保存到g.sched区域, 用于实现g任务暂停后恢复
gopark并没有将g放回运行队列, 如果没有主动恢复, 任务会遗失, 适合channel、select这种会保存g并且会调用goready恢复g执行的场景
gopark
	*通过mcall函数保存当前的运行状态到g.sched
	*调用park_m函数
		*把G的状态从运行中(_Grunning)改为等待中(_Gwaiting)
		*然后调用dropg函数解除M和G之间的关联
		*再调用传入的解锁函数, 这里的解锁函数会对解除channel.lock的锁定
		*最后调用schedule函数继续调度
	
goready用于恢复g的执行, g优先放到P.runnext执行
goready
	*调用ready函数
		*把G的状态从运行中(_Gwaiting)改为等待中(_Grunnable)
		*调用runqput把g放到运行队列
		*如果当前有空闲的P, 但是无自旋的M(nmspinning等于0), 则调用wakep唤醒或新建一个M
	
	
	
	
	
	
	
	
	
	
	
	
	
	
		
	
	
type chantype struct {
	typ  _type
	elem *_type
	dir  uintptr
}

type _type struct {
	size       uintptr
	ptrdata    uintptr // size of memory prefix holding all pointers
	hash       uint32
	tflag      tflag
	align      uint8
	fieldalign uint8
	kind       uint8
	alg        *typeAlg
	// gcdata stores the GC type data for the garbage collector.
	// If the KindGCProg bit is set in kind, gcdata is a GC program.
	// Otherwise it is a ptrmask bitmap. See mbitmap.go for details.
	gcdata    *byte
	str       nameOff
	ptrToThis typeOff
}
	
	
qcount: 当前队列中的元素数量
dataqsiz: 队列可以容纳的元素数量, 如果为0表示这个channel无缓冲区
buf: 队列的缓冲区, 结构是环形队列
elemsize: 元素的大小
closed: 是否已关闭
elemtype: 元素的类型, 判断是否调用写屏障时使用
sendx: 发送元素的序号
recvx: 接收元素的序号
recvq: 当前等待从channel接收数据的G的链表(实际类型是sudog的链表)
sendq: 当前等待发送数据到channel的G的链表(实际类型是sudog的链表)
lock: 操作channel时使用的线程锁

type waitq struct {
	first *sudog
	last  *sudog
}

type sudog struct {
	// The following fields are protected by the hchan.lock of the
	// channel this sudog is blocking on. shrinkstack depends on
	// this for sudogs involved in channel ops.

	g          *g
	selectdone *uint32 // CAS to 1 to win select race (may point to stack)
	next       *sudog
	prev       *sudog
	elem       unsafe.Pointer // data element (may point to stack)

	// The following fields are never accessed concurrently.
	// For channels, waitlink is only accessed by g.
	// For semaphores, all fields (including the ones above)
	// are only accessed when holding a semaRoot lock.

	acquiretime int64
	releasetime int64
	ticket      uint32
	parent      *sudog // semaRoot binary tree
	waitlink    *sudog // g.waiting list or semaRoot
	waittail    *sudog // semaRoot
	c           *hchan // channel
}


makechan
	*数据项不能超过64kb
	*缓冲槽大小检查
	*受垃圾回收器限制, 指针类型缓冲槽或者size为0需要为buf单独分配内存
	*设置elemsize, elemtype, dataqsiz的值
	
chansend
	*对channel加锁防止数据竞争
	*通过 dequeue从recvq取出最先陷入等待的 Goroutine 并调用send直接向它发送数据, 获取到的G会从recvq出队
		*如果sudog.elem不等于nil, 调用sendDirect函数从发送者直接复制元素到接收者sg.elem
		*调用goready恢复接收者的G
			*把G的状态由等待中(_Gwaiting)改为待运行(_Grunnable)
			*把G放到P的本地运行队列
			*如果当前有空闲的P, 但是无自旋的M(nmspinning等于0), 则唤醒或新建一个M
	*判断是否可以把元素放到缓冲区中
		*如果缓冲区有空余的空间, 则把元素放到缓冲区并从chansend返回
		*chanbuf 计算出下一个可以放置待处理变量的位置
		*通过 typedmemmove 将发送的消息拷贝到缓冲区buf中并增加 sendx 索引和 qcount 计数器
	*无缓冲区或缓冲区已经写满, 发送者的G需要等待
		*获取当前的g
		*执行 acquireSudog 函数获取一个 sudog 结构体并设置这一次阻塞发送的相关信息
		*设置sudog.elem = 指向发送内存的指针
		*设置sudog.g = g
		*设置sudog.c = channel
		*设置g.waiting = sudog
		*把sudog放入channel.sendq
		*调用 goparkunlock 函数将当前的 Goroutine 更新成 Gwaiting 状态并解锁，该 Goroutine 可以被调用 goready 再次唤醒
			*调用gopark函数, gopark并没有将G放回运行队列, 需要主动恢复, 否则任务会遗失
				*通过mcall函数调用park_m函数
					*把G的状态从运行中(_Grunning)改为等待中(_Gwaiting)
					*然后调用dropg函数解除M和G之间的关联
					*再调用传入的解锁函数, 这里的解锁函数会对解除channel.lock的锁定
					*最后调用schedule函数继续调度
		*从这里是goready恢复执行, 表示已经成功发送或者channel已关闭
		*检查sudog.param为nil, 且channel已关闭, 抛出panic
		*释放sudog返回
			
		
chanrecv		
	*当我们从一个空 Channel 中接收数据时会直接调用 gopark 直接让出当前 Goroutine 处理器的使用权
	*如果当前的 Channel 已经被关闭并且缓冲区中不存在任何的数据, 那么就会直接解锁当前的 Channel 并清除 ep 指针的数据
	*当 Channel 的 sendq 队列中包含处于等待状态的 Goroutine 时, 如果有, 表示channel无缓冲区或者缓冲区已满, 我们其实就会直接取出队列头的 Goroutine
		*调用recv函数
			*如果无缓冲区, 调用recvDirect函数把元素直接复制给接收者
				*将 sendq 中 Goroutine 存储的 elem 数据拷贝到目标内存地址中
			*如果缓冲区已满
				*chanbuf 计算出下一个出队的位置
				*把channel缓冲区buf出队元素直接复制给接受者
				*把sendq的sudog中的elem复制到刚才出队的位置
				*这时候缓冲区仍然是满的, 但是发送序号和接收序号都会增加1
				*调用goready恢复接收者的G
					*调用ready
						*把G的状态从运行中(_Gwaiting)改为等待中(_Grunning)
						*调用runqput把g放到运行队列
						*如果当前有空闲的P(sched.npidle大于0), 但是无自旋的M(nmspinning等于0), 则调用wakep唤醒或新建一个M
	*判断是否可以从缓冲区获取元素
		*typedmemmove 将缓冲区中的数据拷贝到内存中, 之后会清除队列中的数据
		*递增 recvx 索引的数据, 当发现索引超过了当前队列的容量时, 由于这是一个循环队列所以就会将它归零, 减少 qcount 计数器并释放持有 Channel 的锁
	*无缓冲区或者缓冲区没有元素, 接收者G需要等待
		*获取当前的g
		*新建一个sudog
		*设置sudog.elem = 指向接收内存的指针
		*设置sudog.g = g
		*设置sudog.c = channel
		*设置g.waiting = sudog
		*把sudog放入channel.recvq
		*调用 goparkunlock 函数将当前的 Goroutine 更新成 Gwaiting 状态并解锁，该 Goroutine 可以被调用 goready 再次唤醒
			*调用gopark函数, gopark并没有将G放回运行队列, 需要主动恢复, 否则任务会遗失
				*通过mcall函数调用park_m函数
					*把G的状态从运行中(_Grunning)改为等待中(_Gwaiting)
					*然后调用dropg函数解除M和G之间的关联
					*再调用传入的解锁函数, 这里的解锁函数会对解除channel.lock的锁定
					*最后调用schedule函数继续调度
		*从这里是goready恢复执行, 表示已经成功接收或者channel已关闭
		*检查sudog.param为nil, 且channel已关闭, 抛出panic
		*释放sudog返回
		
		
closechan	
	*设置channel.closed = 1
	*枚举channel.recvq, 清零它们sudog.elem, 设置sudog.param = nil
	*枚举channel.sendq, 设置sudog.elem = nil, 设置sudog.param = nil
	*调用goready函数恢复所有接收者和发送者的G
		
		
select结构
type hselect struct {
	tcase uint16 			// ncase 总数
	ncase uint16 			// ncase 初始顺序
	pollorder *uint16 		// 乱序后 scase 序号
	lockorder **hchan 		// 按 scase channel 地址排序
	scase [1]scase 			// scase 数组
}
		
select 控制结构中 case 却使用了 scase 结构体来表示
type scase struct {
	c           *hchan					//Channel
	elem        unsafe.Pointer			//接收或者发送数据的变量地址
	kind        uint16					//表示当前 case 的种类	const (caseNil = iota caseRecv caseSend caseDefault)
	pc          uintptr
	releasetime int64
}


selectgo
	*初始化
		*将scase封装成slice, 生成scases
		*调用fastrandn对scase序号洗牌, 乱序, 生成pollorder
		*按照channel地址顺序生成顺序锁lockorder, 按照相同顺序锁channel避免死锁
	*loop
		*遍历所有的ncase
			*从乱序的pollorder中获取scase, select随机选择的原因
			*获取scase.kind
			*caseNil跳过, caseDefault尝试执行
			*caseRecv caseSend 尝试接收发送数据, 任意一个成功则返回
		*没有任何准备好的scase, 则执行default并返回, select 带default实现非阻塞
		*没有任何准备好的scase, 将当前select G打包成sudog, 放到channel等待队列中, 调用gopark等待唤醒
		*遍历找出那个scase唤醒select G, 并且利用循环清理所有的等待队列sudog
		
	
	
	
	
	