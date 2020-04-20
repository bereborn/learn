# Docker

---
## Docker 组件
- **组件**
1. Docker Daemon（Dockerd）：Docker容器管理的守护进程，负责和Docker client交互，并管理Docker镜像、容器（镜像构建、卷管理、日志）
2. Driver：将仅与 Docker容器有关的管理从 Docker Daemon 的所有逻辑中区分开来设计了Driver层来抽象不同类别各自的功能范畴，graphdriver 主要用于完成容器镜像的管理，networkdriver 的作用是完成 Docker容器网络环境的配置，execdriver作为 Docker容器的执行驱动，负责创建容器运行时的命名空间，负责容 器资源使用的统计与限制，负责容器内部进程的真正运行等
3. Containerd：Dockerd 与 docker-containerd 之间是通过 grpc 协议通信的，一个工业级标准的容器运行时，它强调简单性、健壮性和可移植性，负责镜像管理（拉取/推送容器镜像、元信息等）、存储管理（管理镜像及容器数据的存储）、网络管理（管理容器网络接口及网络）、容器执行和生命周期（调用最终运行时组件执行）
4. libcontainer：libcontainer是直接访问内核中与容器相关的系统调用的库
5. docker-shim：一个真实运行的容器的真实垫片载体，每启动一个容器都会起一个新的docker-shim的一个进程，他直接通过指定的三个参数：容器id，boundle目录（containerd的对应某个容器生成的目，一般位于：/var/run/docker/libcontainerd/containerID），运行是二进制（默认为runc）来调用runc的api创建一个容器（比如创建容器：最后拼装的命令如下：runc create ...），docker-shim 允许 RunC 在启动容器之后退出，同时即使在 containerd 和 dockerd 都挂掉的情况下，容器的标准 IO 和其它的文件描述符也都是可用的，有了 docker-shim 就可以在不中断容器运行的情况下升级或重启 dockerd
6. RunC：实现了容器启停、资源隔离等功能

- **docker start**
1. dockerd：   
(1)dockerd首先准备rootfs，通过mount的方式将所有的layer合并起来，aufs只在运行的时候动态的将容器的 read-write layer 、init-layer和下面的read-only layer进行合并   
(2)dockerd接下来准备容器里面需要用到的配置文件<contain-id>-json.log、hostname、resolv.conf、resolv.conf.hash、shm   
(3)dockerd准备OCI（Open Containers Initiative）需要的bundle的config.json配置文件   
(4)dockerd准备IO文件，用来和容器之间进行通信，比如这里的init-stdin文件用来向容器的stdin中写数据，init-stdout用来接收容器的stdout输出

```
# 路径：/run/docker/libcontainerd/<contain-id>
# ls 841e5bfba8264f2a1fba8cc9af266b597f91573b95868e3fd0b1a86ce4a97d45
config.json  init-stderr  init-stdin   init-stdout
{
    "ociVersion":"1.0.0-rc2-dev",   # 对应的OCI（Open Containers Initiative）标准版本
    "platform":{    # 平台信息amd64 + Linux
        "os":"linux",
        "arch":"amd64"
    },
    "process":{ # 容器启动后执行什么命令
        "user":{
            "uid":0,
            "gid":0
        },
        "args":[
            "/bin/sh",
            "-c",
            "while true; do echo hello world; sleep 1;done"
        ],
        "env":[
            "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
            "HOSTNAME=841e5bfba826"
        ],
        "cwd":"/",
    },
    "root":{    # 根文件系统的位置
        "path":"/var/lib/docker/overlay2/f53b4402bfca2b8b71bbdf12b85ce5364474b13a6664efd187e16aa5a467452d/merged"
    },
    "hostname":"841e5bfba826",  # 容器的主机名
    "mounts":[  # 需要挂载哪些目录到容器里面
        {
            "destination":"/proc",
            "type":"proc",
            "source":"proc",
            "options":[
                "nosuid",
                "noexec",
                "nodev"
            ]
        },
        {
            "destination":"/dev",
            "type":"tmpfs",
            "source":"tmpfs",
            "options":[
                "nosuid",
                "strictatime",
                "mode=755"
            ]
        },
        {
            "destination":"/dev/pts",
            "type":"devpts",
            "source":"devpts",
            "options":[
                "nosuid",
                "noexec",
                "newinstance",
                "ptmxmode=0666",
                "mode=0620",
                "gid=5"
            ]
        },
        {
            "destination":"/sys",
            "type":"sysfs",
            "source":"sysfs",
            "options":[
                "nosuid",
                "noexec",
                "nodev",
                "ro"
            ]
        },
        {
            "destination":"/sys/fs/cgroup",
            "type":"cgroup",
            "source":"cgroup",
            "options":[
                "ro",
                "nosuid",
                "noexec",
                "nodev"
            ]
        },
        {
            "destination":"/dev/mqueue",
            "type":"mqueue",
            "source":"mqueue",
            "options":[
                "nosuid",
                "noexec",
                "nodev"
            ]
        },
        {
            "destination":"/etc/resolv.conf",
            "type":"bind",
            "source":"/var/lib/docker/containers/841e5bfba8264f2a1fba8cc9af266b597f91573b95868e3fd0b1a86ce4a97d45/resolv.conf",
            "options":[
                "rbind",
                "rprivate"
            ]
        },
        {
            "destination":"/etc/hostname",
            "type":"bind",
            "source":"/var/lib/docker/containers/841e5bfba8264f2a1fba8cc9af266b597f91573b95868e3fd0b1a86ce4a97d45/hostname",
            "options":[
                "rbind",
                "rprivate"
            ]
        },
        {
            "destination":"/etc/hosts",
            "type":"bind",
            "source":"/var/lib/docker/containers/841e5bfba8264f2a1fba8cc9af266b597f91573b95868e3fd0b1a86ce4a97d45/hosts",
            "options":[
                "rbind",
                "rprivate"
            ]
        },
        {
            "destination":"/dev/shm",
            "type":"bind",
            "source":"/var/lib/docker/containers/841e5bfba8264f2a1fba8cc9af266b597f91573b95868e3fd0b1a86ce4a97d45/shm",
            "options":[
                "rbind",
                "rprivate"
            ]
        }
    ],
    "hooks":{   # 配置容器运行生命周期中会调用的hooks，包括prestart、poststart和poststop
        "prestart":[
            {
                "path":"/usr/bin/dockerd",
                "args":[
                    "libnetwork-setkey",
                    "841e5bfba8264f2a1fba8cc9af266b597f91573b95868e3fd0b1a86ce4a97d45",
                    "dece008273ed62d59111645b342a211dcef57d350f6910fa0350e50365ee99e7"
                ]
            }
        ]
    },
    "linux":{
        "resources":{
            "devices":[ # 设置哪些设备可以在容器内被访问到，/dev/null、/dev/zero、/dev/full、/dev/random、/dev/urandom、/dev/tty、/dev/console、/dev/ptmx
                {
                    "allow":false,
                    "access":"rwm"
                },
                {
                    "allow":true,
                    "type":"c",
                    "major":1,
                    "minor":5,
                    "access":"rwm"
                },
                {
                    "allow":true,
                    "type":"c",
                    "major":1,
                    "minor":3,
                    "access":"rwm"
                },
                {
                    "allow":true,
                    "type":"c",
                    "major":1,
                    "minor":9,
                    "access":"rwm"
                },
                {
                    "allow":true,
                    "type":"c",
                    "major":1,
                    "minor":8,
                    "access":"rwm"
                },
                {
                    "allow":true,
                    "type":"c",
                    "major":5,
                    "minor":0,
                    "access":"rwm"
                },
                {
                    "allow":true,
                    "type":"c",
                    "major":5,
                    "minor":1,
                    "access":"rwm"
                },
                {
                    "allow":false,
                    "type":"c",
                    "major":10,
                    "minor":229,
                    "access":"rwm"
                }
            ],
            "disableOOMKiller":false,
            "oomScoreAdj":0,
            "memory":{
                "swappiness":18446744073709551615
            },
            "cpu":{

            },
            "pids":{
                "limit":0
            },
            "blockIO":{
                "blkioWeight":0
            }
        },
        "cgroupsPath":"/docker/841e5bfba8264f2a1fba8cc9af266b597f91573b95868e3fd0b1a86ce4a97d45",   # cgroup的路径
        "namespaces":[  # namespace相关的配置
            {
                "type":"mount"
            },
            {
                "type":"network"
            },
            {
                "type":"uts"
            },
            {
                "type":"pid"
            },
            {
                "type":"ipc"
            }
        ],
        "seccomp":{ # 和安全相关的配置
            "defaultAction":"SCMP_ACT_ERRNO",
            "architectures":[
                "SCMP_ARCH_X86_64",
                "SCMP_ARCH_X86",
                "SCMP_ARCH_X32"
            ],
            "syscalls":[    # 调整容器运行时的kernel参数，主要是一些网络参数，因为每个network namespace都有自己的协议栈，所以可以修改自己协议栈的参数而不影响别人
                {
                    "name":"accept",
                    "action":"SCMP_ACT_ALLOW"
                },
                {
                    "name":"bind",
                    "action":"SCMP_ACT_ALLOW"
                },
                {
                    "name":"chmod",
                    "action":"SCMP_ACT_ALLOW"
                },
                {
                    "name":"chown",
                    "action":"SCMP_ACT_ALLOW"
                },
                {
                    "name":"close",
                    "action":"SCMP_ACT_ALLOW"
                },
                {
                    "name":"connect",
                    "action":"SCMP_ACT_ALLOW"
                },
                {
                    "name":"exit",
                    "action":"SCMP_ACT_ALLOW"
                },
                {
                    "name":"fork",
                    "action":"SCMP_ACT_ALLOW"
                },
                {
                    "name":"listen",
                    "action":"SCMP_ACT_ALLOW"
                },
                {
                    "name":"recv",
                    "action":"SCMP_ACT_ALLOW"
                },
                {
                    "name":"socket",
                    "action":"SCMP_ACT_ALLOW",
                    "args":[
                        {
                            "index":0,
                            "value":1,
                            "valueTwo":0,
                            "op":"SCMP_CMP_EQ"
                        }
                    ]
                },
                {
                    "name":"clone",
                    "action":"SCMP_ACT_ALLOW",
                    "args":[
                        {
                            "index":0,
                            "value":2080505856,
                            "valueTwo":0,
                            "op":"SCMP_CMP_MASKED_EQ"
                        }
                    ]
                },
            ]
        },
        "maskedPaths":[ # 设置容器内的哪些目录对用户不可见
            "/proc/kcore",
            "/proc/latency_stats",
            "/proc/timer_list",
            "/proc/timer_stats",
            "/proc/sched_debug",
            "/sys/firmware"
        ],
        "readonlyPaths":[   # 设置容器内的哪些目录是只读的
            "/proc/asound",
            "/proc/bus",
            "/proc/fs",
            "/proc/irq",
            "/proc/sys",
            "/proc/sysrq-trigger"
        ]
    }
}
```
2. docker-containerd：   
(1)dockerd通过grpc给containerd发送请求，通知containerd启动容器   
(2)docker-containerd创建control、exit、process.json这三个文件   
(3)docker-containerd通过control文件启动shim进程，等着runc创建容器并将容器里第一个进程的pid写入pid文件   
(4)如果containerd读取pid文件失败，则读取shim-log.json和log.json，看出了什么异常   
(5)如果读取pid文件成功，说明容器创建成功，则将当前时间作为容器的启动时间写入starttime文件   
(6)调用runc的start命令启动容器   
(7)监听容器的OOM事件和容器退出事件以便作出响应，OOM事件通过group.event_control（fd）进行监听，容器退出事件通过exit管道（pipe）来实现（在shim进程退出时，close 掉 exit 文件）

```
# 路径：/run/docker/libcontainerd/containerd/<contain-id>/init
# ls 841e5bfba8264f2a1fba8cc9af266b597f91573b95868e3fd0b1a86ce4a97d45/init
control：用来往shim发送控制命令，包括关闭stdin和调整终端的窗口大小
log.json：runc如果运行失败的话，会写日志到这个文件
process.json：包含容器中进程相关的一些属性信息，后续在这个容器上执行docker exec命令时会用到这个文件 
starttime：记录容器的启动时间
exit：shim进程退出的时候，会关闭该管道，然后containerd就会收到通知，做一些清理工作
pid：容器启动后，runc会将容器中第一个进程的pid写到这个文件中
shim-log.json：shim进程执行失败的话，会写日志到这个文件
```
3. docker-containerd-shim：   
(1)shim进程被containerd启动之后，第一步是设置子孙进程成为孤儿进程后由shim进程接管，即shim将变成孤儿进程的父进程，这样就保证容器里的第一个进程不会因为runc进程的退出而被init进程接管（）   
(2)调用runc create命令创建容器，shim直接通过指定的三个参数：容器id，boundle目录（containerd的对应某个容器生成的目，一般位于：/var/run/docker/libcontainerd/containerID），运行是二进制（默认为runc）来调用runc的api创建一个容器（比如创建容器：最后拼装的命令如下：runc create ...），docker-shim 允许 RunC 在启动容器之后退出，同时即使在 containerd 和 dockerd 都挂掉的情况下，容器的标准 IO 和其它的文件描述符也都是可用的，有了 docker-shim 就可以在不中断容器运行的情况下升级或重启 dockerd，容器创建成功后，runc会将容器的第一个进程的pid写入上面containerd目录下的pid文件中，这样containerd进程就知道容器创建成功后会调用runc start启动容器
4. runc：   
(1)runc会被调用两次，第一次是shim调用runc create创建容器，第二次是containerd调用runc start启动容器   
(2)创建容器runc init：runc会根据参数中传入的bundle目录名称以及容器ID创建容器   
(3)启动容器runc start：启动配置文件中指定的相应进程

```
# 路径：/run/docker/libcontainerd/containerd/<contain-id>/state.json
# cat 841e5bfba8264f2a1fba8cc9af266b597f91573b95868e3fd0b1a86ce4a97d45/state.json
{
    "bundle":"/var/run/docker/libcontainerd/841e5bfba8264f2a1fba8cc9af266b597f91573b95868e3fd0b1a86ce4a97d45",
    "labels":null,
    "stdin":"",
    "stdout":"",
    "stderr":"",
    "runtime":"docker-runc",
    "runtimeArgs":null,
    "shim":"docker-containerd-shim",
    "noPivotRoot":false
}
```


- **从 docker 到 RunC **
1. dockerd <--> "docker hub"：dockerd拿到image后，就会在本地创建相应的容器，然后再做一些初始化工作后，最后通过grpc的方式通知docker-containerd进程启动指定的容器
2. dockerd <--> docker-containerd：dockerd拿到image后，就会在本地创建相应的容器，然后再做一些初始化工作后，最后通过grpc的方式通知docker-containerd进程启动指定的容器，docker-containerd是和dockerd一起启动的后台进程，之间的通信协议是grpc
2. docker-containerd <--> docker-containerd-shim：当docker-containerd收到dockerd的启动容器请求之后，会做一些初始化工作，然后启动docker-containerd-shim进程，并将相关配置所在的目录作为参数传给它，docker-containerd和docker-containerd-shim都属于containerd项目，docker-containerd管理所有本机正在运行的容器，而docker-containerd-shim只负责管理一个运行的容器，相当于是对runc的一个包装，充当containerd和runc之间的桥梁
3. docker-containerd-shim <--> docker-runc：docker-containerd-shim进程启动后，就会按照runtime的标准准备好相关运行时环境，然后启动docker-runc进程
4. docker-runc <--> hello：runc进程打开容器的配置文件，找到rootfs的位置，并启动配置文件中指定的相应进程，在hello-world的这个例子中，runc会启动容器中的hello程序
5. 进程间关系：runc将容器启动起来后，runc进程就退出了，于是容器里面的第一个进程（hello）的父进程就变成了docker-containerd-shim，进程树的关系如下：
```
systemd───dockerd───docker-containerd───docker-containerd-shim───hello
```

- **Docker Daemon**
1. Engine 是 Docker架构中的运行引擎，Engine 扮演着 Docker Container存储仓库的角色，并且通过 Job 的形式管理 Docker运行中涉及的所有任务
2. graphdriver 用于完成 Docker 镜像的管理，包括获取、存储以及容器 rootfs 的构建等
3. volumesdriver 采用 vfs驱动实现数据卷的管理，通过一个数据卷可以被多个 Docker 容器挂载，从而使 Docker 容器可以实现互相共享数据等
4. execdriver 是 Docker 中用来执行 Docker容器任务的驱动


- **Containerd 组件和子系统**
1. distribute子系统：该服务实现拿去镜像功能
2. bundle子系统：该服务允许用户从磁盘映像中提取镜像和打包成bundle
3. runtime子系统：该服务实现bundles的执行，包括运行时容器的创建
4. Executor组件：实际容器运行时的执行器
5. Supervisor组件：监视和报告容器状态
6. Metadata组件：将元数据存储在图形数据库中，用于存储对镜像和bundle的任何持久性引用，输入到数据库的数据将具有在组件之间协调的模式，以提供对任意数据的访问，其他功能包括定义了用于磁盘资源的垃圾回收的钩子
7. Content组件：提供对content addressable storage （镜像的层文件）的访问，所有不可变的内容将存储在这里，通过内容的hash索引
8. Snapshot组件：管理容器映像的文件系统快照
9. Events组件：支持事件的收集和使用，以提供一致的，事件驱动的行为和审计
10. Metrics组件：每个组件将导出几个指标，可通过指标API访问

- **RunC**
1. RunC 是容器的运行runtime的实现，它负责利用符合标准的文件等资源运行容器，但是它不包含 docker 那样的镜像管理功能，OCI bundle 就是指容器的文件系统、config.json文件和 runtime.json文件
2. 容器标准包（bundle）和配置：   
(1)config.json：基本配置文件，包括与宿主机独立的和应用相关的特定信息，如安全权限、环境变量和参数等   
&emsp;a)ociVersion：对应的OCI标准版本   
&emsp;b)root：rootfs路径位置   
&emsp;c)mounts：各类文件挂载点及相应容器内挂载目录，Linux平台必须包含/proc、/sys、/dev/pts、/dev/shm这四个目录（此配置信息必须与runtime.json 配置中保持一致）   
&emsp;d)process：容器启动后执行什么命令   
&emsp;e)hostname：容器的主机名   
&emsp;f)platform：平台信息，amd64 + Linux   
&emsp;g)linux：Linux平台的特殊配置Linux Container Configuration      
&emsp;h)hooks：配置容器运行生命周期Runtime and Lifecycle中会调用的hooks，包括prestart、poststart和poststop   
&emsp;i)annotations：容器的注释，相当于容器标签，key:value格式   
(2)runtime.json：运行时配置文件，包含运行时与主机相关的信息，如内存限制、本地设备访问权限、挂载点等，除了上述配置信息以外，运行时配置文件还提供了"钩子(hooks)"的特性，这样可以在容器运行前和停止后各执行一些自定义脚本，hooks的配置包含执行脚本路径、参数、环境变量等   
(3)rootfs/：根文件系统目录，包含了容器执行所需的必要环境依赖，如/bin、/var、/lib、/dev、/usr等目录及相应文件，rootfs目录必须与包含配置信息的config.json文件同时存在容器目录最顶层
3. Linux平台拓展Linux Container Configuration   
(1)namespaces: namespace相关的配置   
(2)uidMappings，gidMappings：配置主机和容器用户/组之间的对应关系   
(3)devices：设置哪些设备可以在容器内被访问到   
(4)cgroupsPath：cgroup的路径   
(5)resources：Cgroup中具体子项的配置   
4. 容器的状态state.json文件信息：   
(1)ociVersion：存放OCI标准的具体版本号   
(2)id：容器ID，通常是一个哈希值   
(3)status：容器的运行时状态   
(4)pid：容器内第一个进程在系统初始pid namespace中的pid，即在容器外面看到的pid   
(5)bundle：bundle所在位置的绝对路径   
(6)annotations：容器的注释，相当于容器标签，来自于容器的配置文件，key：value格式   
5. 容器状态和操作：   
(1)creating：使用 create 命令创建容器，这个过程称为创建中   
(2)created：容器已经创建出来，但是还没有运行，表示镜像文件和配置没有错误，容器能够在当前平台上运行   
(3)running：容器里面的进程处于运行状态，正在执行用户设定的任务   
(4)stopped：容器运行完成，或者运行出错，或者 stop 命令之后，容器处于暂停状态，这个状态，容器还有很多信息保存在平台中，并没有完全被删除   
(5)paused：暂停容器中的所有进程，可以使用 resume 命令恢复这些进程的执行
6. 容器的生命周期Runtime and Lifecycle    
(1)runc create创建容器，参数中指定bundle的位置以及容器的ID，容器的状态变为creating   
(2)runc根据bundle中的config.json，准备好容器运行时需要的环境和资源，但不运行process中指定的进程，这步执行完成之后，表示容器创建成功，修改config.json将不再对创建的容器产生影响，这时容器的状态变成created   
(3)runc start启动容器   
(4)runc执行config.json中配置的prestart钩子   
(5)runc执行config.json中process指定的程序，这时容器状态变成了running   
(6)runc执行poststart钩子   
(7)容器由于某些原因退出，比如容器中的第一个进程主动退出，挂掉或者被kill掉等，这时容器状态变成了stoped   
(8)runc delete删除容器，这时runc就会删除掉上面第2步所做的所有工作   
(9)runc执行poststop钩子

- **runC 原理**   
runC工作原理：Docker通过调用libcontainer包对namespaces、cgroups、capabilities以及文件系统的管理和分配来"隔离"出一个与宿主机系统共享内核但与系统中的其它进程资源相隔离的执行环境

- **runC start 源码剖析**
1. main()函数开始：程序通过cli包对runC的各个子命令、参数、版本号以及帮助信息进行规定，然后程序会通过用户输入的子命令来调用对应的处理函数，这里则调用start.go中的startContainer()函数
2. 创建逻辑容器Container与逻辑进程process：逻辑容器container和逻辑进程process是libcontainer中所定义的结构体，逻辑容器container中包含了namespace、cgroups、device和mountpoint等各种配置信息，逻辑进程process中则包含了容器中所要运行的指令以其参数和环境变量等，创建过程：   
(1)将*.json装入可以被libcontainer使用的结构体config中，然后使用config作为参数来调用   
(2)libcontainer.New()生成用来产生container的工厂factory，再调用factory.Create(config)生成一个将config包含其中的逻辑容器container    
(3)接下来调用newProcess(config)来将config中关于容器内所要运行命令的相关信息填充到process结构体中即逻辑进程process（进程的命令、参数、环境变量、用户、标准输入输出等），使用container.Start(process)来启动逻辑容器   
3. 启动逻辑容器container：   
(1)runC会调用Start()，主要工作就是调用newParentProcess()来生成parentprocess实例（结构体）和用于runC与容器内init进程相互通信的管道   
(2)在parentprocess实例中，除了有记录了将来与容器内进程进行通信的管道与各种基本配置等，还有一个极为重要的字段就是其中的cmd，cmd字段是定义在os/exec包中的一个结构体，os/exec包主要用于创建一个新的进程，并在这个进程中执行指定的命令，开发者可以在工程中导入os/exec包，然后将cmd结构体进行填充，即将所需运行程序的路径和程序名，程序所需参数，环境变量，各种操作系统特有的属性和拓展的文件描述符等   
(3)在runC中程序将cmd的应用路径字段Path填充为/proc/self/exe（即为应用程序本身，runC），参数字段Args填充为启动的容器子进程init程序，表示对容器进行初始化，SysProcAttr字段中则填充了各种runC所需启用的namespace等属性   
(4)然后调用parentprocess.cmd.Start()启动物理容器中的init进程，接下来将物理容器中init进程的进程号加入到Cgroup控制组中，对容器内的进程实施资源控制，再把配置参数通过管道传送给init进程，最后通过管道等待init进程根据上述配置完成所有的初始化工作，或者出错退出   
4. 物理容器的配置和创建，容器中的init进程首先会调用StartInitialization()函数，通过管道从父进程接收各种配置参数，然后对容器进行如下配置：   
(1)如果用户指定，则将init进程加入其指定的namespace   
(2)设置进程的会话ID   
(3)初始化网络设备，配置网络和路由规则   
(4)对指定目录下的文件系统进行挂载，并切换根目录到新挂载的文件系统下，设置hostname，加载profile信息   
(5)最后使用exec系统调用来执行用户所指定的在容器中运行的程序


> [docker、containerd、runc、docker-shim](https://www.jianshu.com/p/52c0f12b0294)   
> [docker核心组件--containerd和runC](https://my.oschina.net/u/2306127/blog/1600270)

> [从 docker 到 runC](https://www.cnblogs.com/sparkdev/p/9129334.html)   
> [走进docker(01)：hello-world的背后发生了什么？](https://segmentfault.com/a/1190000009309297)   
> [走进docker(04)：什么是容器的runtime?](https://segmentfault.com/a/1190000009583199)   
> [走进docker(07)：docker start命令背后发生了什么？](https://segmentfault.com/a/1190000010057763)   
> [dockerd、contaierd、containerd-shim、runC通信机制分析](https://blog.csdn.net/Jinhua_Wei/article/details/79874592)

> [Containerd 简介](https://www.cnblogs.com/sparkdev/p/9063042.html)

> [RunC 是什么？](https://www.cnblogs.com/sparkdev/p/9032209.html)   
> [DockOne技术分享（二十八）： OCI标准和runC原理解读](https://blog.csdn.net/weixin_34413065/article/details/92525442)   
> [OCI 和 runc：容器标准化和 docker](https://cizixs.com/2017/11/05/oci-and-runc/)

---
## Docker 使用 Linux namespace 隔离容器的运行环境

- **Linux namespace 的概念**
1. namespace 的概念：将某个特定的全局系统资源通过抽象方法使得namespace 中的进程看起来拥有它们自己的隔离的全局系统资源实例
2. Linux 内核中实现了六种 namespace：

namespace | 被隔离的全局系统资源 | 在容器语境下的隔离效果 |
----------|----------------------|------------------------|
Mount namespaces | 文件系统挂接点 | 每个容器能看到不同的文件系统层次结构，每个mount namespace都拥有一份自己的挂载点列表
UTS namespaces | nodename 和 domainname | 每个容器可以有自己的 hostname 和 domainame
IPC namespaces | 特定的进程间通信资源，包括System V IPC 和  POSIX message queues | 每个容器有其自己的 System V IPC 和 POSIX 消息队列文件系统，因此，只有在同一个 IPC namespace 的进程之间才能互相通信
PID namespaces | 进程 ID 数字空间 （PID） | 每个 PID namespace 中的进程可以有其独立的 PID，每个容器可以有其 PID 为 1 的root 进程，也使得容器可以在不同的 host 之间迁移，因为 namespace 中的进程 ID 和 host 无关了，这也使得容器中的每个进程有两个PID：容器中的 PID 和 host 上的 PID
Network namespaces | 网络相关的系统资源 | 每个容器用有其独立的网络设备，IP 地址，IP 路由表，/proc/net 目录，端口号等等，这也使得一个 host 上多个容器内的同一个应用都绑定到各自容器的 80 端口上
User namespaces | 用户和组 ID 空间 | 在 user namespace 中的进程的用户和组 ID 可以和在 host 上不同，每个 container 可以有不同的 user 和 group id，一个 host 上的非特权用户可以成为 user namespace 中的特权用户

- **Linux 系统调用**

```
/* CLONE_NEWUTS UTS namespaces */
/* CLONE_NEWIPC IPC namespaces */
/* CLONE_NEWPID PID namespaces */
/* CLONE_NEWNS Mount namespaces */
    
/* 定义一个给 clone 用的栈，栈大小1M */
#define STACK_SIZE (1024 * 1024)
static char container_stack[STACK_SIZE];
 
char* const container_args[] = {
    "/bin/bash",
    NULL
};
 
int container_main(void* arg)
{
    /* 查看子进程的PID，输出子进程的 pid 为 1 */
    printf("Container [%5d] - inside the container!\n", getpid());
    /* 设置 hostname */
    sethostname("container",10);
    
    //remount "/proc" to make sure the "top" and "ps" show container's information
    if (mount("proc", "rootfs/proc", "proc", 0, NULL) !=0 ) {
        perror("proc");
    }
    if (mount("sysfs", "rootfs/sys", "sysfs", 0, NULL)!=0) {
        perror("sys");
    }
    if (mount("none", "rootfs/tmp", "tmpfs", 0, NULL)!=0) {
        perror("tmp");
    }
    if (mount("udev", "rootfs/dev", "devtmpfs", 0, NULL)!=0) {
        perror("dev");
    }
    if (mount("devpts", "rootfs/dev/pts", "devpts", 0, NULL)!=0) {
        perror("dev/pts");
    }
    if (mount("shm", "rootfs/dev/shm", "tmpfs", 0, NULL)!=0) {
        perror("dev/shm");
    }
    if (mount("tmpfs", "rootfs/run", "tmpfs", 0, NULL)!=0) {
        perror("run");
    }
    /* 
     * 模仿Docker的从外向容器里mount相关的配置文件 
     * 你可以查看：/var/lib/docker/containers/<container_id>/目录，
     * 你会看到docker的这些文件的。
     */
    if (mount("conf/hosts", "rootfs/etc/hosts", "none", MS_BIND, NULL)!=0 ||
          mount("conf/hostname", "rootfs/etc/hostname", "none", MS_BIND, NULL)!=0 ||
          mount("conf/resolv.conf", "rootfs/etc/resolv.conf", "none", MS_BIND, NULL)!=0 ) {
        perror("conf");
    }
    /* 模仿docker run命令中的 -v, --volume=[] 参数干的事 */
    if (mount("/tmp/t1", "rootfs/mnt", "none", MS_BIND, NULL)!=0) {
        perror("mnt");
    }
 
    /* chroot 隔离目录 */
    if ( chdir("./rootfs") != 0 || chroot("./") != 0 ){
        perror("chdir/chroot");
    }
    
    /* 直接执行一个shell，以便我们观察这个进程空间里的资源是否被隔离了 */
    execv(container_args[0], container_args); 
    printf("Something's wrong!\n");
    return 1;
}
 
int main()
{
    printf("Parent - start a container!\n");
    /* 调用clone函数，其中传出一个函数，还有一个栈空间的（为什么传尾指针，因为栈是反着的） */
    
    int container_pid = clone(container_main, container_stack+STACK_SIZE, 
    SIGCHLD | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWPID | CLONE_NEWNS, NULL);
    /* 等待子进程结束 */
    waitpid(container_pid, NULL, 0);
    printf("Parent - container stopped!\n");
    return 0;
}

# IPC需要有一个全局的ID，我们的Namespace需要对这个ID隔离，不能让别的Namespace的进程看到
# 要做到进程空间的隔离，首先要创建出 PID 为1的进程，把子进程的PID在容器内变成1
# 模仿了Docker的Mount Namespace，构建 rootfs，父进程就可以动态地设置容器需要的这些文件的配置， 然后再把他们mount进容器，容器的镜像中的配置就比较灵活了
# hchen@ubuntu:~/rootfs$ ls
# bin  dev  etc  home  lib  lib64  mnt  opt  proc  root  run  sbin  sys  tmp  usr  var

```

```
/* CLONE_NEWUSER User namespaces */

#define STACK_SIZE (1024 * 1024)
 
static char container_stack[STACK_SIZE];
char* const container_args[] = {
    "/bin/bash",
    NULL
};
 
int pipefd[2];
 
void set_map(char* file, int inside_id, int outside_id, int len) {
    FILE* mapfd = fopen(file, "w");
    if (NULL == mapfd) {
        perror("open file error");
        return;
    }
    fprintf(mapfd, "%d %d %d", inside_id, outside_id, len);
    fclose(mapfd);
}
 
void set_uid_map(pid_t pid, int inside_id, int outside_id, int len) {
    char file[256];
    sprintf(file, "/proc/%d/uid_map", pid);
    set_map(file, inside_id, outside_id, len);
}
 
void set_gid_map(pid_t pid, int inside_id, int outside_id, int len) {
    char file[256];
    sprintf(file, "/proc/%d/gid_map", pid);
    set_map(file, inside_id, outside_id, len);
}
 
int container_main(void* arg)
{
 
    printf("Container [%5d] - inside the container!\n", getpid());
 
    printf("Container: eUID = %ld;  eGID = %ld, UID=%ld, GID=%ld\n",
            (long) geteuid(), (long) getegid(), (long) getuid(), (long) getgid());
 
    /* 等待父进程通知后再往下执行（进程间的同步） */
    char ch;
    close(pipefd[1]);
    read(pipefd[0], &ch, 1);
 
    printf("Container [%5d] - setup hostname!\n", getpid());
    //set hostname
    sethostname("container",10);
 
    //remount "/proc" to make sure the "top" and "ps" show container's information
    mount("proc", "/proc", "proc", 0, NULL);
 
    execv(container_args[0], container_args);
    printf("Something's wrong!\n");
    return 1;
}
 
int main()
{
    const int gid=getgid(), uid=getuid();
 
    printf("Parent: eUID = %ld;  eGID = %ld, UID=%ld, GID=%ld\n",
            (long) geteuid(), (long) getegid(), (long) getuid(), (long) getgid());
 
    pipe(pipefd);
  
    printf("Parent [%5d] - start a container!\n", getpid());
 
    int container_pid = clone(container_main, container_stack+STACK_SIZE, 
            CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUSER | SIGCHLD, NULL);
 
     
    printf("Parent [%5d] - Container [%5d]!\n", getpid(), container_pid);
 
    //To map the uid/gid, 
    //   we need edit the /proc/PID/uid_map (or /proc/PID/gid_map) in parent
    //The file format is
    //   ID-inside-ns   ID-outside-ns   length
    //if no mapping, 
    //   the uid will be taken from /proc/sys/kernel/overflowuid
    //   the gid will be taken from /proc/sys/kernel/overflowgid
    set_uid_map(container_pid, 0, uid, 1);
    set_gid_map(container_pid, 0, gid, 1);
 
    printf("Parent [%5d] - user/group mapping done!\n", getpid());
 
    /* 通知子进程 */
    close(pipefd[1]);
 
    waitpid(container_pid, NULL, 0);
    printf("Parent - container stopped!\n");
    return 0;
}

输出：
hchen@ubuntu:~$ id
uid=1000(hchen) gid=1000(hchen) groups=1000(hchen)
 
hchen@ubuntu:~$ ./user # 以hchen用户运行
Parent: eUID = 1000;  eGID = 1000, UID=1000, GID=1000 
Parent [ 3262] - start a container!
Parent [ 3262] - Container [ 3263]!
Parent [ 3262] - user/group mapping done!
Container [    1] - inside the container!
Container: eUID = 0;  eGID = 0, UID=0, GID=0 #<---Container里的UID/GID都为0了
Container [    1] - setup hostname!
 
root@container:~# id # 我们可以看到容器里的用户和命令行提示符是root用户了
# 虽然容器里是root，但其实这个容器的/bin/bash进程是以一个普通用户hchen来运行的，这样一来容器的安全性会得到提高
uid=0(root) gid=0(root) groups=0(root),65534(nogroup)

# 要把容器中的uid和真实系统的uid给映射在一起（容器内是 root，被映射到容器外一个非 root 用户），需要修改 /proc/<pid>/uid_map 和 /proc/<pid>/gid_map 这两个文件
# 这两个文件的格式为： ID-inside-ns ID-outside-ns length
# ID-inside-ns 表示在容器显示的UID或GID
# ID-outside-ns 表示容器外映射的真实的UID或GID
# length 表示映射的范围，一般填1，表示一一对应
```

```
/* Docker Network Namespace */

## 首先，我们先增加一个网桥lxcbr0，模仿docker0
brctl addbr lxcbr0
brctl stp lxcbr0 off
ifconfig lxcbr0 192.168.10.1/24 up #为网桥设置IP地址
 
## 接下来，我们要创建一个network namespace - ns1
 
# 增加一个namesapce 命令为 ns1 （使用ip netns add命令）
ip netns add ns1 
 
# 激活namespace中的loopback，即127.0.0.1（使用ip netns exec ns1来操作ns1中的命令）
ip netns exec ns1   ip link set dev lo up 
 
## 然后，我们需要增加一对虚拟网卡
 
# 增加一个pair虚拟网卡，注意其中的veth类型，其中一个网卡要按进容器中
ip link add veth-ns1 type veth peer name lxcbr0.1
 
# 把 veth-ns1 按到namespace ns1中，这样容器中就会有一个新的网卡了
ip link set veth-ns1 netns ns1
 
# 把容器里的 veth-ns1改名为 eth0 （容器外会冲突，容器内就不会了）
ip netns exec ns1  ip link set dev veth-ns1 name eth0 
 
# 为容器中的网卡分配一个IP地址，并激活它
ip netns exec ns1 ifconfig eth0 192.168.10.11/24 up
 
 
# 上面我们把veth-ns1这个网卡按到了容器中，然后我们要把lxcbr0.1添加上网桥上
brctl addif lxcbr0 lxcbr0.1
 
# 为容器增加一个路由规则，让容器可以访问外面的网络
ip netns exec ns1     ip route add default via 192.168.10.1
 
# 在/etc/netns下创建network namespce名称为ns1的目录，
# 然后为这个namespace设置resolv.conf，这样，容器内就可以访问域名了
mkdir -p /etc/netns/ns1
echo "nameserver 8.8.8.8" > /etc/netns/ns1/resolv.conf
```

- **Namespace 文件**
```
int container_main(void* arg)
{
    /* 查看子进程的PID，我们可以看到其输出子进程的 pid 为 1 */
    printf("Container [%5d] - inside the container!\n", getpid());
    sethostname("container",10);
    execv(container_args[0], container_args);
    printf("Something's wrong!\n");
    return 1;
}
 
int main()
{
    printf("Parent [%5d] - start a container!\n", getpid());
    /*启用PID namespace - CLONE_NEWPID*/
    int container_pid = clone(container_main, container_stack+STACK_SIZE, 
            CLONE_NEWUTS | CLONE_NEWPID | SIGCHLD, NULL); 
    waitpid(container_pid, NULL, 0);
    printf("Parent - container stopped!\n");
    return 0;
}

# shell中查看一下父子进程的PID：
# hchen@ubuntu:~$ pstree -p 4599
# pid.mnt(4599)───bash(4600)

# 父进程的：
# hchen@ubuntu:~$ sudo ls -l /proc/4599/ns
# total 0
# lrwxrwxrwx 1 root root 0  4月  7 22:01 ipc -> ipc:[4026531839]
# lrwxrwxrwx 1 root root 0  4月  7 22:01 mnt -> mnt:[4026531840]
# lrwxrwxrwx 1 root root 0  4月  7 22:01 net -> net:[4026531956]
# lrwxrwxrwx 1 root root 0  4月  7 22:01 pid -> pid:[4026531836]
# lrwxrwxrwx 1 root root 0  4月  7 22:01 user -> user:[4026531837]
# lrwxrwxrwx 1 root root 0  4月  7 22:01 uts -> uts:[4026531838]

# 子进程的：
# hchen@ubuntu:~$ sudo ls -l /proc/4600/ns
# total 0
# lrwxrwxrwx 1 root root 0  4月  7 22:01 ipc -> ipc:[4026531839]
# lrwxrwxrwx 1 root root 0  4月  7 22:01 mnt -> mnt:[4026532520]
# lrwxrwxrwx 1 root root 0  4月  7 22:01 net -> net:[4026531956]
# lrwxrwxrwx 1 root root 0  4月  7 22:01 pid -> pid:[4026532522]
# lrwxrwxrwx 1 root root 0  4月  7 22:01 user -> user:[4026531837]
# lrwxrwxrwx 1 root root 0  4月  7 22:01 uts -> uts:[4026532521]

# 其中的ipc，net，user是同一个ID，而mnt,pid,uts都是不一样的，如果两个进程指向的namespace编号相同，就说明他们在同一个namespace下，否则则在不同namespace里面
# 这些文件另一个作用：一旦这些文件被打开，只要其fd被占用着，那么就算PID所属的所有进程都已经结束，创建的namespace也会一直存在
# int setns(int fd, int nstype) // 把某进程加入到某个namespace
# 其中第一个参数fd就是一个open()系统调用打开了上述文件后返回的fd
# fd = open("/proc/4600/ns/nts", O_RDONLY)  // 获取namespace文件描述符
# setns(fd, 0); // 加入新的namespace
```

> [DOCKER基础技术：LINUX NAMESPACE（上）](https://coolshell.cn/articles/17010.html)    
> [DOCKER基础技术：LINUX NAMESPACE（下）](https://coolshell.cn/articles/17029.html)   
> [Docker 使用 Linux namespace 隔离容器的运行环境](https://www.cnblogs.com/sammyliu/p/5878973.html)

---
## Docker 网络

- **虚拟网络设备之tun/tap**
- **虚拟网络设备之veth**
- **虚拟网络设备之bridge**

- **Docker 网络 bridge 模式**
1. 使用一个 linux bridge，默认为 docker0
2. 使用 veth 对，一头在容器的网络 namespace 中，一头在 docker0 上
3. Docker采用 NAT 方式，将容器内部的服务监听的端口与宿主机的某一个端口port进行"绑定"，使得宿主机以外的世界可以主动将网络报文发送至容器内部
4. 外界访问容器内的服务时，需要访问宿主机的 IP 以及宿主机的端口 port
5. NAT 模式由于是在三层网络上的实现手段，故肯定会影响网络的传输效率
6. 容器拥有独立、隔离的网络栈，让容器和宿主机以外的世界通过NAT建立通信
7. iptables 的 SNTA 规则，使得从容器离开去外界的网络包的源 IP 地址被转换为 Docker 主机的IP地址

- **Docker 网络 Host 模式**
1. Docker 容器会和 host 宿主机共享同一个网络 namespace
2. 容器的 IP 地址同 Docker host 的 IP 地址

- **Docker 网络 container 模式**
1. 处于这个模式下的 Docker 容器会共享其他容器的网络环境，因此，至少这两个容器之间不存在网络隔离，而这两个容器又与宿主机以及除此之外其他的容器存在网络隔离

- **Docker 网络 none 模式**
1. 容器内部就只能使用loopback网络设备，不会再有其他的网络资源

- **iptables 规则**

```
# iptables 的 SNTA 规则，使得从容器离开去外界的网络包的源 IP 地址被转换为 Docker 主机的 IP 地址
# nat 表上 POSTROUTING 链上的有这么一条规则
-A POSTROUTING -s 172.17.0.0/16 ! -o docker0 -j MASQUERADE

# iptables 的 DNTA 规则，使得外界可以主动将网络报文发送至容器内部
*nat
-A DOCKER ! -i docker0 -p tcp -m tcp --dport 3000 -j DNAT --to-destination 172.17.0.3:3000
*filter
-A DOCKER -d 172.17.0.3/32 ! -i docker0 -o docker0 -p tcp -m tcp --dport 3000 -j ACCEPT
```

- **容器的 DNS 和主机名hostname**
1. 实际上容器中的 /etc 目录下有 3 个文件是在容器启动后被虚拟文件覆盖掉的，分别是 /etc/hostname、/etc/hosts 和 /etc/resolv.conf

> [Linux虚拟网络设备之tun/tap](https://segmentfault.com/a/1190000009249039)   
> [Linux虚拟网络设备之veth](https://segmentfault.com/a/1190000009251098)   
> [Linux虚拟网络设备之bridge(桥)](https://segmentfault.com/a/1190000009491002)   
> [Docker 网络](https://www.cnblogs.com/sammyliu/p/5894191.html)   
> [Docker 网络之理解 bridge 驱动](https://www.cnblogs.com/sparkdev/p/9217310.html)

---
## Docker 计算机资源隔离

- **Linux Control Group**
1. Linux CGroup 作用：可以为系统进程的用户定义组群分配资源：比如CPU时间、系统内存、网络带宽，可以监控配置的cgroup，拒绝cgroup访问某些资源，甚至在运行的系统中动态配置cgroup

- **CPU 限制**
```
# 创建 cpu cgroup
# mkdir /sys/fs/cgroup/cpu/test

# 设置CPU利用率为50%
# echo 50000 > /sys/fs/cgroup/cpu/test/cpu.cfs_quota_us
# 限制CPU只能使用2核和3核
# echo \"2,3\" > /sys/fs/cgroup/cpuset/test/cpuset.cpus

# 把进程pid加入cgroup中
# echo [pid] >> /sys/fs/cgroup/cpu/test/tasks
```
- **内存限制**

```
# 创建 memory cgroup
# mkdir /sys/fs/cgroup/memory/test

# 分配内存64k
# echo 64k > /sys/fs/cgroup/memory/test/memory.limit_in_bytes

# 把进程pid加入cgroup中
# echo [pid] > /sys/fs/cgroup/memory/test/tasks
```
- **磁盘I/O限制**

```
# 创建 blkio cgroup
# mkdir /sys/fs/cgroup/blkio/test

# 把读IO限制到1MB/s
# echo [设备号] 1048576  > /sys/fs/cgroup/blkio/test/blkio.throttle.read_bps_device
# echo [pid] > /sys/fs/cgroup/blkio/test/tasks
```
- **CGroup 的子系统**
1. blkio：为块设备设定输入/输出限制，比如物理设备（磁盘，固态硬盘，USB等等）
2. cpu：提供cpu限制
3. cpuacct：自动生成cgroup中的任务所使用的cpu报告
4. cpuset：为cgroup中的任务分配独立的cpu和内存节点
5. devices：允许或拒绝cgroup中的任务访问设备
6. memory：为cgroup中的任务设定内存限制
7. net_cls：使用等级标识符（classid）标记网络数据包，可允许Linux流量控制程序iptables 或者 traffic controller识别从具体cgroup中生成的数据包
8. net_prio：设计网络流量的优先级

- **Docker 对 cgroups 的使用**

```
# Docker 启动一个容器后，会在 /sys/fs/cgroup 目录下的各个资源目录下生成以容器 ID 为名字的目录（group），在容器被 stopped 后，该目录被删除
/sys/fs/cgroup/cpu/docker/03dd196f415276375f754d51ce29b418b170bd92d88c5e420d6901c32f93dc14

# 运行命令 docker run -d --name web41 --cpu-quota 25000 --cpu-period 100 --cpu-shares 30 training/webapp python app.py 启动一个新的容器
# Docker 会将容器中的进程的 ID 加入到各个资源对应的 tasks 文件中
root@devstack:/sys/fs/cgroup/cpu/docker/06bd180cd340f8288c18e8f0e01ade66d066058dd053ef46161eb682ab69ec24# cat cpu.cfs_quota_us
25000
root@devstack:/sys/fs/cgroup/cpu/docker/06bd180cd340f8288c18e8f0e01ade66d066058dd053ef46161eb682ab69ec24# cat tasks
3704
root@devstack:/sys/fs/cgroup/cpu/docker/06bd180cd340f8288c18e8f0e01ade66d066058dd053ef46161eb682ab69ec24# cat cpu.cfs_period_us
2000
```


> [DOCKER基础技术：LINUX CGROUP](https://coolshell.cn/articles/17049.html)   
> [Docker 容器使用 cgroups 限制资源使用](https://www.cnblogs.com/sammyliu/p/5886833.html)



---
## Docker 镜像

- **AUFS**
1. AUFS：一种Union File System，所谓UnionFS就是把不同物理位置的目录合并mount到同一个目录中
2. 命令：mount -t aufs -o dirs=./fruits:./vegetables none ./mnt
 
```
$ tree
.
├── fruits
│   ├── apple
│   └── tomato
└── vegetables
    ├── carrots
    └── tomato
    
# 创建一个mount目录
$ mkdir mnt
 
# 把水果目录和蔬菜目录union mount到 ./mnt目录中
$ sudo mount -t aufs -o dirs=./fruits:./vegetables none ./mnt
 
#  查看./mnt目录
$ tree ./mnt
./mnt
├── apple
├── carrots
└── tomato

# 修改内容
$ echo mnt > ./mnt/apple
$ cat ./mnt/apple
mnt
$ cat ./fruits/apple
mnt

# 添加文件
$ echo mnt_carrots > ./mnt/carrots
$ cat ./vegetables/carrots
 
$ cat ./fruits/carrots
mnt_carrots

# 在mount aufs命令中，我们没有指它vegetables和fruits的目录权限，默认上来说，命令行上第一个（最左边）的目录是可读可写的，后面的全都是只读的

# 指定权限来mount aufs
$ sudo mount -t aufs -o dirs=./fruits=rw:./vegetables=rw none ./mnt
$ echo "mnt_carrots" > ./mnt/carrots
$ cat ./vegetables/carrots
mnt_carrots
$ cat ./fruits/carrots
cat: ./fruits/carrots: No such file or directory

$ echo "mnt_tomato" > ./mnt/tomato
$ cat ./fruits/tomato
mnt_tomato
$ cat ./vegetables/tomato
I am a vegetable

# 如果有重复的文件名，在mount命令行上，越往前的就优先级越高
```
3. AUFS目录权限：   
(1)rw表示可写可读read-write   
(2)ro表示read-only，如果你不指权限，那么除了第一个外ro是默认值   
(3)whiteout，如果在union中删除的某个文件，实际上是位于一个readonly的分支（目录）上，那么，在mount的union这个目录中你将看不到这个文件，但是read-only这个层上我们无法做任何的修改，所以，我们就需要对这个readonly目录里的文件作whiteout隐藏文件

```
# AUFS的whiteout的实现是通过在上层的可写的目录下建立对应的whiteout隐藏文件来实现的
# tree
.
├── fruits
│   ├── apple
│   └── tomato
├── test
└── vegetables
    ├── carrots
    └── tomato
    
# mkdir mnt
# mount -t aufs -o dirs=./test=rw:./fruits=ro:./vegetables=ro none ./mnt
# ls ./mnt/
apple  carrots  tomato

# touch ./test/.wh.apple
# ls ./mnt
carrots  tomato
```
4. 相关术语：   
(1)Branch是各个要被union起来的目录（上面使用的dirs的命令行参数），Branch根据被union的顺序形成一个stack，一般来说最上面的是可写的，下面的都是只读的，同时Branch的stack可以在被mount后进行修改   
(2)Opaque的意思就是不允许任何下层的某个目录显示出来
5. 相关问题：   
(1)文件在原来的地方被修改，可以指定一个叫udba（User’s Direct Branch Access）的参数决定是否同步修改   
(2)如果有多个rw的branch（目录）被union起来了，那么，当我创建文件的时候，aufs会创建在哪里呢？aufs提供了一个create的参数可以供你来配置相当的创建策略   
&emsp;a)create=rr，新创建的文件轮流写到多个rw的branch   
&emsp;b)create=mfs[:second] | most−free−space[:second] 选一个可用空间最好的分支，可以指定一个检查可用磁盘空间的时间    
&emsp;c)create=mfsrr:low[:second]选一个空间大于low的branch，如果空间小于low了，那么aufs会使用 round-robin 方式   

- **Linux 的 rootfs 和 bootfs**
1. boot file system （bootfs）：包含 boot loader 和 kernel，用户不会修改这个文件系统，实际上，在启动（boot）过程完成后，整个内核都会被加载进内存，此时 bootfs 会被卸载掉从而释放出所占用的内存，同时也可以看出，对于同样内核版本的不同的 Linux 发行版的 bootfs 都是一致的
2. root file system （rootfs）：包含典型的目录结构，包括 /dev, /proc, /bin, /etc, /lib, /usr, and /tmp 等再加上要运行用户应用所需要的所有配置文件，二进制文件和库文件

- **Docker 镜像的 rootfs**
1. 在一个Linux 系统之中，所有 Docker 容器都共享主机系统的 bootfs 即 Linux 内核
2. 每个容器有自己的 rootfs，它来自不同的 Linux 发行版的基础镜像，包括 Ubuntu，Debian 和 SUSE 等
3. rootfs有两种典型的文件：bin 目录中的文件会被直接使用，proc 目录中的文件是后来生成的

- **Docker 使用的 AUFS 文件系统**
1. 容器的文件系统是从 14 个只读镜像层和1个可写容器层通过 AUFS mount 出来的
2. 在容器中创建一个文件，该文件会被创建在可写的容器层中
3. 修改一个镜像层中的文件，文件被拷贝到了可写容器层然后被修改了，另外两个层中的文件保持了不变，这说明了 AUFS 的 CoW 特性
4. 删除容器层中的文件，容器层中出现了一个 .wh 文件用于隐藏文件，镜像层中的文件保持不变

- **镜像的分层结构**
1. 新镜像是从 base 镜像一层一层叠加生成的，每安装一个软件，就在现有镜像的基础上增加一层
2. 采用分层结构可以共享资源，有多个镜像都从相同的 base 镜像构建而来，那么 Docker Host 只需在磁盘上保存一份 base 镜像，同时内存中也只需加载一份 base 镜像，就可以为所有容器服务了，而且镜像的每一层都可以被共享

- **容器可写层**
1. 当容器启动时，一个新的可写层被加载到镜像的顶部，这一层通常被称作"容器层"，"容器层"之下的都叫"镜像层"，所有对容器的改动 - 无论添加、删除、还是修改文件都只会发生在容器层中，只有容器层是可写的，容器层下面的所有镜像层都是只读的
2. 文件覆盖：镜像层数量可能会很多，所有镜像层会联合在一起组成一个统一的文件系统，如果不同层中有一个相同路径的文件，比如 /a，上层的 /a 会覆盖下层的 /a，也就是说用户只能访问到上层中的文件 /a，在容器层中，用户看到的是一个叠加之后的文件系统
3. 添加文件：在容器中创建文件时，新文件被添加到容器层中
4. 读取文件：Docker 会从上往下依次在各镜像层中查找此文件，一旦找到，打开并读入内存
5. 修改文件：Docker 会从上往下依次在各镜像层中查找此文件，一旦找到，立即将其复制到容器层，然后修改之（Copy-on-Write）
6. 删除文件：Docker 也是从上往下依次在镜像层中查找此文件，找到后，会在容器层中记录了一个 .wh 文件用于隐藏文件

- **Docker 镜像包含内容**
1. Filesystem Layers：可读写部分（read-write layer 以及 volumes）、init-layer（/etc/hosts，/etc/hostname，/etc/resolv.conf 这些文件与这个容器内的环境息息相关但不适合被打包作为镜像的文件内容，同时这些内容又不应该直接修改在宿主机文件上）、只读层（read-only layer）这 3 部分结构共同组成了一个容器所需的下层文件系统，它们通过联合挂载的方式巧妙地表现为一层，使得容器进程对这些层的存在一无所知，同时 docker 将 container 的 read-write layer、init-layer 和 image 的 layer 的元数据放在了不同的两个目录中
2. Image Config：基本配置文件，如CPU架构类型（architecture）、操作系统（os）、运行container时的默认参数（config）、image所包含的filesystem layers（rootfs）
3. manifest：manifest(描述文件)主要存在于 registry 中作为 docker 镜像的元数据文件，在 pull、push、save 和 load 过程中作为镜像结构和基础信息的描述文件（manifest里面包含的内容就是对 config 和 layer 的 sha256 + media type描述，目的就是为了下载 config 和 layer）

- **Docker 镜像存储管理**
1. registry：registry 用以保存 docker 镜像，其中还包括镜像层次结构和关于镜像的元数据
2. repository：repository是具有某个功能的docker镜像的所有迭代版本构成的镜像组，持久化文件 repositories.json 存储了所有本地镜像的 repository 的名字，比如docker .io/nginx、docker .io/mysql、docker .io/ubuntu，还有每个 repository 下的镜像的名字、标签及其对应的镜像 ID（当前 docker 默认采用 SHA256 算法根据镜像元数据配置文件计算出镜像 ID，即镜像的摘要 Digest）
3. manifest：manifest(描述文件)主要存在于 registry 中作为 docker 镜像的元数据文件，在 pull、push、save 和 load 过程中作为镜像结构和基础信息的描述文件，在镜像被 pull 或者 load 到 docker 宿主机时，manifest 被转化为本地的镜像配置文件 config
4. Digest：拉取镜像时显示的摘要（Digest）就是对镜像的 manifest 内容计算 sha256sum 得到的
5. image：用来存储一组镜像相关的元数据信息，包括了镜像架构(如 amd64)、操作系统(如 linux)、镜像默认配置、构建该镜像的容器 ID 和配置、创建时间、创建该镜像的 docker 版本、构建镜像的历史信息以及 rootfs 组成，docker 利用 rootfs 中的 diff_id 计算出内容寻址的索引（chainID）来获取 layer 相关信息，进而获取每一个镜像层的文件内容（注意，每个 diff_id 对应一个镜像层）
6. layer：单个镜像层可能被多个镜像共享，镜像层只包含一个具体的镜像层文件包，用户在 docker 宿主机上下载了某个镜像层之后，docker 会在宿主机上基于镜像层文件包和 image 元数据 manifest 构建本地的 layer 元数据，包括：   
(1)diff_id：基于镜像层文件包的内容sha256计算得   
(2)parent：父镜像层的chainID(最底层不含该文件)   
(3)目录名称 chainID：根据父镜像层的 chainID 加上一个空格和当前层的 diffID进行sha256计算得到（如果镜像是底层没有父镜像层，该层的 diffID 便是 chainID）   
(4)size：当前layer的大小   
(5)cache-id：由宿主机随即生成的一个uuid，根镜像层文件一一对应，用于宿主机标志和索引镜像层文件，指向真正存放layer文件的地方   
而当 docker 将在宿主机上产生的新的镜像层上传到 registry 时，与新镜像层相关的宿主机上的元数据不会与镜像层一块打包上传，layer 主要存放了镜像层的 diff_id、size、cache-id 和 parent 等内容，实际的文件内容则是由存储驱动来管理，并可以通过 cache-id 在本地索引到

```
# repository
# 路径：/var/lib/docker/image/overlay2
# cat repositories.json

{
    "Repositories":{
        "centos":{
            "centos:centos7":"sha256:5e35e350aded98340bc8fcb0ba392d809c807bc3eb5c618d4a0674d98d88bccd",
            "centos@sha256:4a701376d03f6b39b8c2a8f4a8e499441b0d567f9ab9d58e4991de4472fb813c":"sha256:5e35e350aded98340bc8fcb0ba392d809c807bc3eb5c618d4a0674d98d88bccd"
        },
        "test":{
            "test:v1":"sha256:b1ad2a43c99a3cca7344c126379d286738c9d01156e801c4ff0cb9ae418011e6"
        }
    }
}
sh-3.2# docker images --digests
REPOSITORY          TAG                 DIGEST                                                                    IMAGE ID            CREATED             SIZE
test                v1                  <none>                                                                    b1ad2a43c99a        5 days ago          203 MB
centos              centos7             sha256:4a701376d03f6b39b8c2a8f4a8e499441b0d567f9ab9d58e4991de4472fb813c   5e35e350aded        4 months ago        203 MB
```

```
# manifest 和 digest
# 路径：/var/lib/docker/image/overlay2/imagedb/content/sha256/<Digest>
# cat 5e35e350aded98340bc8fcb0ba392d809c807bc3eb5c618d4a0674d98d88bccd

{
    "architecture":"amd64",
    "config":{
        "Hostname":"",
        "Domainname":"",
        "User":"",
        "AttachStdin":false,
        "AttachStdout":false,
        "AttachStderr":false,
        "Tty":false,
        "OpenStdin":false,
        "StdinOnce":false,
        "Env":[
            "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
        ],
        "Cmd":[
            "/bin/bash"
        ],
        "ArgsEscaped":true,
        "Image":"sha256:a0232440f519e6fde80830ff51d9c6bf6ef628b844a826986f995c635baa4ff9",
        "Volumes":null,
        "WorkingDir":"",
        "Entrypoint":null,
        "OnBuild":null,
        "Labels":{
            "org.label-schema.build-date":"20191001",
            "org.label-schema.license":"GPLv2",
            "org.label-schema.name":"CentOS Base Image",
            "org.label-schema.schema-version":"1.0",
            "org.label-schema.vendor":"CentOS"
        }
    },
    "container":"66db091183f2f98b5f52eace5d6d4d6662af823bd85640b2afa4d42ffbe9b03e",
    "container_config":{
        "Hostname":"66db091183f2",
        "Domainname":"",
        "User":"",
        "AttachStdin":false,
        "AttachStdout":false,
        "AttachStderr":false,
        "Tty":false,
        "OpenStdin":false,
        "StdinOnce":false,
        "Env":[
            "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
        ],
        "Cmd":[
            "/bin/sh",
            "-c",
            "#(nop) ",
            "CMD ["/bin/bash"]"
        ],
        "ArgsEscaped":true,
        "Image":"sha256:a0232440f519e6fde80830ff51d9c6bf6ef628b844a826986f995c635baa4ff9",
        "Volumes":null,
        "WorkingDir":"",
        "Entrypoint":null,
        "OnBuild":null,
        "Labels":{
            "org.label-schema.build-date":"20191001",
            "org.label-schema.license":"GPLv2",
            "org.label-schema.name":"CentOS Base Image",
            "org.label-schema.schema-version":"1.0",
            "org.label-schema.vendor":"CentOS"
        }
    },
    "created":"2019-11-12T00:20:33.943019377Z",
    "docker_version":"18.06.1-ce",
    "history":[
        {
            "created":"2019-11-12T00:20:33.422734627Z",
            "created_by":"/bin/sh -c #(nop) ADD file:45a381049c52b5664e5e911dead277b25fadbae689c0bb35be3c42dff0f2dffe in / "
        },
        {
            "created":"2019-11-12T00:20:33.793796025Z",
            "created_by":"/bin/sh -c #(nop)  LABEL org.label-schema.schema-version=1.0 org.label-schema.name=CentOS Base Image org.label-schema.vendor=CentOS org.label-schema.license=GPLv2 org.label-schema.build-date=20191001",
            "empty_layer":true
        },
        {
            "created":"2019-11-12T00:20:33.943019377Z",
            "created_by":"/bin/sh -c #(nop)  CMD ["/bin/bash"]",
            "empty_layer":true
        }
    ],
    "os":"linux",
    "rootfs":{
        "type":"layers",
        "diff_ids":[
            "sha256:77b174a6a187b610e4699546bd973a8d1e77663796e3724318a2a4b24cb07ea0"
        ]
    }
}
```

```
# layer层
# 路径：/var/lib/docker/image/overlay2/layerdb/sha256/<chainID>
# ls 77b174a6a187b610e4699546bd973a8d1e77663796e3724318a2a4b24cb07ea0
cache-id           diff               size               tar-split.json.gz
```

```
# docker 镜像可读写层
# 路径：/var/lib/docker/image/overlay2/layerdb/mounts/<container_id>
# ls 841e5bfba8264f2a1fba8cc9af266b597f91573b95868e3fd0b1a86ce4a97d45
init-id   mount-id  parent
# cat init-id 
# init-id 文件包含了 init layer 的 cache-id
f53b4402bfca2b8b71bbdf12b85ce5364474b13a6664efd187e16aa5a467452d-init
# cat mount-id 
# mount-id 文件包含了 mount layer 的 cache-id
f53b4402bfca2b8b71bbdf12b85ce5364474b13a6664efd187e16aa5a467452d/
# cat parent 
# image 的最上一层 layer 的 chain-id
sha256:77b174a6a187b610e4699546bd973a8d1e77663796e3724318a2a4b24cb07ea0
```

```
# docker 容器运行时配置文件
# 路径：/var/lib/docker/containers/<container_id>
# ls 841e5bfba8264f2a1fba8cc9af266b597f91573b95868e3fd0b1a86ce4a97d45
841e5bfba8264f2a1fba8cc9af266b597f91573b95868e3fd0b1a86ce4a97d45-json.log        # 容器的日志文件，后续容器的stdout和stderr都会输出到这个目录
hostname            # 容器的主机名
resolv.conf         # DNS服务器的IP
resolv.conf.hash    # resolv.conf文件的校验码
shm                 # 为容器分配的一个内存文件系统，后面会绑定到容器中的/dev/shm目录
checkpoints         # 通用的配置，如容器名称，要执行的命令等
config.v2.json      # 主机相关的配置，跟操作系统平台有关，如cgroup的配置
hostconfig.json     # 容器的checkpoint这个功能在当前版本还是experimental状态
```


> [DOCKER基础技术：AUFS](https://coolshell.cn/articles/17061.html)   
> [Docker 存储 - AUFS](https://www.cnblogs.com/sammyliu/p/5931383.html)   
> [Docker镜像结构原理](https://blog.51cto.com/liuleis/2070461)   
> [Docker 镜像之进阶篇](https://www.cnblogs.com/sparkdev/p/9092082.html)   
> [Docker 镜像之存储管理](https://www.cnblogs.com/sparkdev/p/9121188.html)

> [走进docker(02)：image(镜像)是什么？](https://segmentfault.com/a/1190000009309347)   
> [走进docker(05)：docker在本地如何管理image（镜像）?](https://segmentfault.com/a/1190000009730986)   
> [走进docker(06)：docker create命令背后发生了什么？](https://segmentfault.com/a/1190000009769352)


---
### Docker 存储之卷（Volume）

- **Docker volume 的几种形态**
1. 不使用 Docker volume：容器的数据被保存在容器之内，它只在容器的生命周期内存在，会随着容器的被删除而被删除
2. Data volume：一个 data volume 是容器中绕过 Union 文件系统的一个特定的目录，用来保存数据，而不管容器的生命周期

- **Volume 删除和孤单 volume 清理**
1. 在删除容器时删除该容器的卷：docker rm -v
2. 批量删除孤单 volumes：自动化地清理孤单卷

> [Docker 存储之卷（Volume）](https://www.cnblogs.com/sammyliu/p/5932996.html)

---
## Docker 核心技术与实现原理
1. Namespaces
2. CGroups
3. UnionFS

> [Docker 核心技术与实现原理](https://draveness.me/docker)

---
## 在 docker 容器中捕获信号
- **容器中的信号**
1. Docker 的 stop 和 kill 命令都是用来向容器发送信号的，注意，==只有容器中的 1 号进程能够收到信号==，这一点非常关键！
2. stop 命令会首先发送 SIGTERM 信号，并等待应用优雅的结束，kill 命令默认发送的是 SIGKILL 信号
3. 容器中的 1 号进程是非常重要的，如果它不能正确的处理相关的信号，那么应用程序退出的方式几乎总是被强制杀死而不是优雅的退出，1 号进程主要由 EntryPoint, CMD, RUN 等指令的写法决定

> [在 docker 容器中捕获信号](https://www.cnblogs.com/sparkdev/p/7598590.html)

---
## Docker 命令
1. 构建：docker build -t test:v1 .
2. 运行：docker run -d --name test test:v1 /bin/sh -c "while true; do echo hello world; sleep 1;done"
3. 运行：docker run --rm --name test -it test:v1 /bin/bash
4. 运行：docker run --rm --name docker_go_build -it -v /home/root1/lizexin/xl_dcdn_data_server:/home/root1/lizexin/xl_dcdn_data_server -w /home/root1/lizexin/xl_dcdn_data_server docker_go_build:v1
5. 查看信息：docker inspect 

- **Mac 访问 docker 宿主主机**
1. screen /Users/reborn/Library/Containers/com.docker.docker/Data/com.docker.driver.amd64-linux/tty

---


https://blog.csdn.net/cbl709/article/details/42570161   
https://yeasy.gitbooks.io/docker_practice/image/internal.html   




https://www.cnblogs.com/ityouknow/p/8520296.html

Linux云计算网络   
https://www.cnblogs.com/bakari/default.html?page=4

docker   
https://cizixs.com/categories/blog/