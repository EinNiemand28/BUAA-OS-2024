# lab6实验报告

## 思考题

### Thinking 6.1

>示例代码中，父进程操作管道的写端，子进程操作管道的读端。如果现在想让父进程作为“读者”，代码应当如何修改？

```c
#include <stdio.h>
#include <unistd.h>

int fildes[2];
// fildes[0]读端 fildes[1]写端
char buf[100];
int status;

int main() {
    status = pipe(fildes);
    
    if (status == -1) {
        printf("error in pipe!\n");
    }
    
    switch(fork()) {
        case -1:
            break;
        case 0: /* 子进程 - 作为管道的写者 */
            close(fildes[0]);						
            write(fildes[1], "Hello world\n", 12);
            close(fildes[1]);
            exit(EXIT_SUCCESS);
        default: /* 父进程 - 作为管道的读者 */
            close(fildes[1]);
            read(fildes[0], buf, 100);
            printf("father-process read:%s", buf);
            close(fildes[0]);
            exit(EXIT_SUCCESS);
    }
}
```

### Thinking 6.2

>上面这种不同步修改 pp_ref 而导致的进程竞争问题在 user/lib/fd.c 中的 dup 函数中也存在。请结合代码模仿上述情景，分析一下我们的 dup 函数中为什么会出现预想之外的情况？ 

阅读代码可以知道，dup 函数的流程为：

1. 将`newfd`对应的页映射到`oldfd`对应的物理页面
1. 将`newfd`的数据所在页映射到`oldfd`的数据所在物理页面

如果某个进程在执行 dup 时，已经完成了 1，但还未完成 2，此时发生中断，与之通信的进程会根据`ref(fd)==ref(pipe)`认为写者进程已关闭，从而执行读操作，但实际上写者还未完成写操作，导致读者读到了错误的数据。

所以应该先执行 2，再执行 1，这样就不会出现上述问题。

### Thinking 6.3

>阅读上述材料并思考：为什么系统调用一定是原子操作呢？如果你觉得不是所有的系统调用都是原子操作，请给出反例。希望能结合相关代码进行分析说明。

```assembly
#include <asm/asm.h>
#include <stackframe.h>

.section .text.tlb_miss_entry
tlb_miss_entry:
	j       exc_gen_entry

.section .text.exc_gen_entry
exc_gen_entry:
	SAVE_ALL
	/*
	* Note: When EXL is set or UM is unset, the processor is in kernel mode.
	* When EXL is set, the value of EPC is not updated when a new exception occurs.
	* To keep the processor in kernel mode and enable exception reentrancy,
	* we unset UM and EXL, and unset IE to globally disable interrupts.
	*/
	mfc0    t0, CP0_STATUS
	and     t0, t0, ~(STATUS_UM | STATUS_EXL | STATUS_IE)
	mtc0    t0, CP0_STATUS
/* Exercise 3.9: Your code here. */
	mfc0	t0, CP0_CAUSE
	andi	t0, 0x7c
	lw		t0, exception_handlers(t0)
	jr		t0
```

通过阅读指导书和注释可知，我们使用的 MIPS 4Kc 中的 CP0_Status 寄存器的 EXL 位和 UM 位表示了处理器当前的运行状态（当且仅当 EXL 被设置为 0 且 UM 被设置为 1 时，处理器处于用户模式），IE 位表示中断是否开启。而我们在处理系统调用时，将 EXL 、 UM 和 IE 位都设置为 0，使系统处于内核模式且不响应中断，故系统调用一定是原子操作。

### Thinking 6.4

>仔细阅读上面这段话，并思考下列问题
>
>- 按照上述说法控制 pipe_close 中 fd 和 pipe unmap 的顺序，是否可以解决上述场景的进程竞争问题？给出你的分析过程。
>- 我们只分析了 close 时的情形，在 fd.c 中有一个 dup 函数，用于复制文件描述符。试想，如果要复制的文件描述符指向一个管道，那么是否会出现与 close 类似的问题？请模仿上述材料写写你的理解。

- 可以解决。因为在任何时刻都有`ref(fd) <= ref(pipe)`，所以在`pipe_close`中先执行 unmap fd，就不会出现上述进程竞争的问题。
- 会出现类似的问题。因为在执行 dup 时，若先 map fd 再 map pipe ，就会使得`ref(fd)`的 +1 先于 pipe ，导致在两个 unmap 的间隙，会出现`ref(pipe) == ref(fd)`的情况。

### Thinking 6.5

>思考以下三个问题。
>
>- 认真回看 Lab5 文件系统相关代码，弄清打开文件的过程。
>- 回顾 Lab1 与 Lab3，思考如何读取并加载 ELF 文件。
>- 在 Lab1 中我们介绍了 data text bss 段及它们的含义，data 段存放初始化过的全局变量，bss 段存放未初始化的全局变量。关于 memsize 和 filesize ，我们在 Note 1.3.4中也解释了它们的含义与特点。关于 Note 1.3.4，注意其中关于“bss 段并不在文件中占数据”表述的含义。回顾 Lab3 并思考：elf_load_seg() 和 load_icode_mapper() 函数是如何确保加载 ELF 文件时，bss 段数据被正确加载进虚拟内存空间。bss 段在 ELF 中并不占空间，但 ELF 加载进内存后，bss 段的数据占据了空间，并且初始值都是 0。请回顾 elf_load_seg() 和 load_icode_mapper() 的实现，思考这一点是如何实现的？
>
>下面给出一些对于上述问题的提示，以便大家更好地把握加载内核进程和加载用户进程的区别与联系，类比完成 spawn 函数。
>
>​	关于第一个问题，在 Lab3 中我们创建进程，并且通过 ENV_CREATE(...) 在内核态加载了初始进程，而我们的 spawn 函数则是通过和文件系统交互，取得文件描述块，进而找到 ELF 在“硬盘”中的位置，进而读取。
>
>​	关于第二个问题，各位已经在 Lab3 中填写了 load_icode 函数，实现了 ELF 可执行文件中读取数据并加载到内存空间，其中通过调用 elf_load_seg 函数来加载各个程序段。在 Lab3 中我们要填写 load_icode_mapper 回调函数，在内核态下加载 ELF 数据到内存空间；相应地，在 Lab6 中 spawn 函数也需要在用户态下使用系统调用为 ELF 数据分配空间。 

- 打开文件的过程为：
    1. 调用`file.c`中的`open`函数指定文件路径`path`和读取模式`mode`
    2. 调用`fd_alloc`分配文件描述符，再调用`fsipc_open`向文件服务系统发送打开文件的请求
    3. 调用`fsipc`向文件服务系统进程发送 IPC 请求，并等待返回
    4. 文件服务系统进程收到请求后调用`serve_open`处理请求，并调用`file_open`打开文件，返回文件描述符
- 读取并加载 ELF 文件的过程为：
    1. 调用`load_icode`函数将可执行文件（文件头地址为 binary ）加载到进程 e 的内存空间
    2. 调用`elf_load_seg`函数加载各个程序段，对于其中需要加载的页面，使用回调函数`load_icode_mapper`完成单个页面的加载
- 在分配页面时，将text 段和 data 段占据的页面中没有占满的空间置为 0 给 bss 段，且另外再给 bss 分配时，仅使用`syscall_mem_alloc`而不映射任何内容

### Thinking 6.6

>通过阅读代码空白段的注释我们知道，将标准输入或输出定向到文件，需要我们将其 dup 到 0 或 1 号文件描述符（fd）。那么问题来了：在哪步，0 和 1 被“安排”为标准输入和标准输出？请分析代码执行流程，给出答案。 

```c
// user/init.c
// stdin should be 0, because no file descriptors are open yet
if ((r = opencons()) != 0) {
    user_panic("opencons: %d", r);
}
// stdout
if ((r = dup(0, 1)) < 0) {
    user_panic("dup: %d", r);
}
```

### Thinking 6.7

>在 shell 中执行的命令分为内置命令和外部命令。在执行内置命令时 shell 不需要 fork 一个子 shell，如 Linux 系统中的 cd 命令。在执行外部命令时 shell 需要 fork 一个子 shell，然后子 shell 去执行这条命令。
>
>据此判断，在 MOS 中我们用到的 shell 命令是内置命令还是外部命令？请思考为什么 Linux 的 cd 命令是内部命令而不是外部命令？ 

是外部命令。

cd 命令使用频繁，若设置为外部指令则每次使用时都会多次 fork。故设置为内部指令可以提高系统运行效率。

### Thinking 6.8

>在你的 shell 中输入命令 ls.b | cat.b > motd。
>
>- 请问你可以在你的 shell 中观察到几次 spawn ？分别对应哪个进程？
>- 请问你可以在你的 shell 中观察到几次进程销毁？分别对应哪个进程？

```shell
$ ls.b | cat.b > motd
[00002803] pipecreate 
spawn: 14341
spawn: 16390
[00003805] destroying 00003805
[00003805] free env 00003805
i am killed ... 
[00004006] destroying 00004006
[00004006] free env 00004006
i am killed ... 
[00003004] destroying 00003004
[00003004] free env 00003004
i am killed ... 
[00002803] destroying 00002803
[00002803] free env 00002803
i am killed ... 
```

- 共有 2 次 spawn，分别打开了 ls.b， cat.b 进程
- 共有 4 次进程销毁，分别销毁了：执行管道左边命令的进程、执行管道右边命令的进程、解析并执行管道右边命令的进程、解析并执行当前命令的进程

## 难点分析

### 管道

#### 管道的原理

管道是一种进程间通信的方式，分为有名管道和匿名管道，其中匿名管道只能在具有公共祖先的进程之间使用，且通常使用在父子进程之间（ MOS 实验中仅要求实现匿名管道）。

```c
/* Overview:
 *   Create a pipe.
 *
 * Post-Condition:
 *   Return 0 and set 'pfd[0]' to the read end and 'pfd[1]' to the
 *   write end of the pipe on success.
 *   Return an corresponding error code on error.
 */
int pipe(int pfd[2]);
```

在 UNIX 以及 MOS 中，父进程调用 pipe 函数后，会打开两个新的文件描述符（**设置 PTE_LIBRARY 权限位**），它们被映射到同一内存空间（其中 fd[0] 表示读端、 fd[1] 表示写端）。在 fork 函数的配合下，子进程复制父进程的两个文件描述符，从而在父子进程间形成了四个（父子各拥有一读一写）指向同一片内存区域的文件描述符，父子进程可根据需 要关掉自己不用的一个，从而实现父子进程间的单向通信管道。

![](https://cdn.jsdelivr.net/gh/EinNiemand28/my-img@master/images/image-20240611223809577.png)

#### 管道的使用

```c
// user/lib/pipe.c
#define PIPE_SIZE 32 // small to provoke races

struct Pipe {
	u_int p_rpos;		 // read position
	u_int p_wpos;		 // write position
	u_char p_buf[PIPE_SIZE]; // data buffer
};
```

一个管道有 PIPE_SIZE(32 Byte) 大小的**环形**缓冲区，读写的位置 i 实际上是 i%PIPE_SIZE 。

- 写者可写条件：`p_wpos - p_rpos < PIPE_SIZE`（这里有个坑点，要注意这些变量均为**无符号数**，相减会“溢出”）
- 读者可读条件：` p_rpos < p_wpos`

判断管道是否关闭的等式：`pageref(rfd) + pageref(wfd) = pageref(pipe)`

>每个匿名管道分配了三页空间：一页是读数据的文件描述符 rfd，一页是写数据的文件描述符 wfd，剩下一页是被两个文件描述符共享的管道数据缓冲区 pipe
>
>故在有 1 个读者、1 个写者的前提下，管道将被引用两次

这里进程竞争问题对判断的影响和对应处理可见上文 Thinking 。

### Shell

![](https://cdn.jsdelivr.net/gh/EinNiemand28/my-img@master/images/image-20240611225349797.png)

#### 加载可执行文件

```c
int spawn(char *prog, char **argv);
int spawnl(char *prog, char *args, ...) {
	// Thanks to MIPS calling convention, the layout of arguments on the stack
	// are straightforward.
	return spawn(prog, &args);
}
```

- 从文件系统读取指定的文件（二进制 ELF 文件，在 MOS 中为 *.b ）
- 调用`syscall_exofork`申请新的进程控制块
- 调用`init_stack`为子进程初始化地址空间。对于栈空间，由于 spawn 需要将命令行参数传递给用户程序，所以要将参数也写入用户栈中
- 将目标程序对应的 ELF 加载到子进程的地址空间中
- 调用`syscall_set_trapframe`设置子进程的寄存器
- 调用`syscall_mem_map`将父进程的共享页面映射到子进程的地址空间中
- 调用`syscall_set_env_status`设置子进程为可执行状态

#### Shell命令的执行

- 使用 readline 函数循环读入 buf 并进行特殊字符的处理
- fork 子进程执行 runcmd
- 使用 gettoken 和 parsecmd 来解析指令，并使用 spawn 加载运行对应的程序

## 实验体会

本次实验要求我们使用管道实现一个简单的 Shell ，总体来说没什么难点；不过对于 Shell 的具体实现指导书没有过多的讲解，如果想丰富 Shell 的功能，还需要多读源码、对整个 MOS 的运行过程有一个非常清晰的认识。