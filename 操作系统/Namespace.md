# Linux Namespace

---
## UTS Namespace

- **进程task结构**

```
/* 每个进程对应的task结构体struct task_struct中，增加了一个叫nsproxy的字段，类型是struct nsproxy */
struct task_struct {
  ...
  /* namespaces */
  struct nsproxy *nsproxy;
  ...
}

struct nsproxy {
  atomic_t count;
  struct uts_namespace *uts_ns;
  struct ipc_namespace *ipc_ns;
  struct mnt_namespace *mnt_ns;
  struct pid_namespace *pid_ns_for_children;
  struct net       *net_ns;
  struct cgroup_namespace *cgroup_ns;
};
```
- **gethostname**

```
/* task结构体里面的nsproxy->uts_ns所指向的结构体是不一样的，于是达到了隔离UTS的目的 */
static inline struct new_utsname *utsname(void)
{
  //current指向当前进程的task结构体
  return &current->nsproxy->uts_ns->name;
}

SYSCALL_DEFINE2(gethostname, char __user *, name, int, len)
{
  struct new_utsname *u;
  ...
  u = utsname();
  if (copy_to_user(name, u->nodename, i)){
    errno = -EFAULT;
  }
  ...
}
```

---
## Pid Namespace

- **Linux Namespace 和 Pid Namespace**
1. PID namespaces用来隔离进程的ID空间，使得不同Pid namespace里的进程ID可以重复且相互之间不影响
2. PID namespace可以嵌套，也就是说有父子关系，在当前namespace里面创建的所有新的namespace都是当前namespace的子namespace，父namespace里面可以看到所有子孙后代namespace里的进程信息，而子namespace里看不到祖先或者兄弟namespace里的进程信息，目前PID namespace最多可以嵌套32层，由内核中的宏MAX_PID_NS_LEVEL来定义
3. Linux下的每个进程都有一个对应的/proc/PID目录，该目录包含了大量的有关当前进程的信息，对一个PID namespace而言，/proc目录只包含当前namespace和它所有子孙后代namespace里的进程的信息
4. 在Linux系统中，进程ID从1开始往后不断增加，并且不能重复（当然进程退出后，ID会被回收再利用），进程ID为1的进程是内核启动的第一个应用层进程，一般是init进程，具有特殊意义，当系统中一个进程的父进程退出时，内核会指定init进程成为这个进程的新父进程，而当init进程退出时，系统也将退出
5. 除了在init进程里指定了handler的信号外，内核会帮init进程屏蔽掉其他任何信号，这样可以防止其他进程不小心kill掉init进程导致系统挂掉，不过有了PID namespace后，可以通过在父namespace中发送SIGKILL或者SIGSTOP信号来终止子namespace中的ID为1的进程
6. 由于ID为1的进程的特殊性，所以每个PID namespace的第一个进程的ID都是1，当这个进程运行停止后，内核将会给这个namespace里的所有其他进程发送SIGKILL信号，致使其他所有进程都停止，于是namespace被销毁掉

---
## Network Namespace

- **Network Namespace**
1. network namespace用来隔离网络设备, IP地址, 端口等. 每个namespace将会有自己独立的网络栈，路由表，防火墙规则，socket等
2. 每个新的network namespace默认有一个本地环回接口，除了lo接口外，所有的其他网络设备（物理/虚拟网络接口，网桥等）只能属于一个network namespace，每个socket也只能属于一个network namespace
3. 当新的network namespace被创建时，lo接口默认是关闭的，需要自己手动启动起


---
## User Namespace 

- **User Namespace**
1. user namespace可以嵌套（目前内核控制最多32层），除了系统默认的user namespace外，所有的user namespace都有一个父user namespace，每个user namespace都可以有零到多个子user namespace
2. 在不同的user namespace中，同样一个用户的user ID 和group ID可以不一样，即一个用户可以在父user namespace中是普通用户，在子user namespace中是超级用户（超级用户只相对于子user namespace所拥有的资源，无法访问其他user namespace中需要超级用户才能访问资源）

- **创建user namespace**
1. 如果没有映射的话，当在新的user namespace中用getuid()和getgid()获取user id和group id时，系统将返回文件/proc/sys/kernel/overflowuid中定义的user ID以及proc/sys/kernel/overflowgid中定义的group ID，它们的默认值都是65534
2. Linux将root的一些权限分解了，变成了各种capability，只要拥有了相应的capability，就能做相应的操作，不需要root账户的权限


> [lLinux Namespace](https://segmentfault.com/a/1190000009732550)