# lab4实验报告

## 思考题

### Thinking 4.1

>思考并回答下面的问题：
>
>- 内核在保存现场的时候是如何避免破坏通用寄存器的？
>- 系统陷入内核调用后可以直接从当时的 \$a0-\$a3 参数寄存器中得到用户调用 msyscall 留下的信息吗？
>- 我们是怎么做到让 sys 开头的函数“认为”我们提供了和用户调用 msyscall 时同样的参数的？
>- 内核处理系统调用的过程对 Trapframe 做了哪些更改？这种修改对应的用户态的变化是什么？

1. 通过`include/stackframe.h`中的`SAVE_ALL`宏，用`$k0,$k1`寄存器将其他通用寄存器保存到内核栈上。

2. 可以，不过在陷入内核态时已经将`$a0-$a3`寄存器中的值保存到了内核栈中，一般来说直接从内核栈中获取这些参数的值。

3. 根据$\cal{MIPS}$调用规范，调用`msyscall`时，`syscall_*`已经将调用参数存入了`$a0-$a3`寄存器和用户栈中；进一步，陷入内核调用后，将用户进程上下文环境保存进内核栈中。因此，函数`sys_*`可以使用到所需要的参数。

4. ```c
    void do_syscall(struct Trapframe *tf) {
    	//...
        tf->cp0_epc = tf->cp0_epc + 4;
        //完成系统调用后直接返回下一条指令（规定了syscall不会在延迟槽中被使用）
        //...
        tf->regs[2] = (*func) (arg1, arg2, arg3, arg4, arg5);
        //将系统调用返回值传回用户态
    }
    ```

### Thinking 4.2

> 思考 envid2env 函数: 为什么 envid2env 中需要判断 e->env_id != envid 的情况？如果没有这步判断会发生什么情况？

```c
#define LOG2NENV 10
#define NENV (1 << LOG2NENV)
#define ENVX(envid) ((envid) & (NENV - 1))
```

当`envid!=0`时，通过宏`ENVX`获取当前进程`e`，可以看到，该宏取envid的**低10位**。

```c
u_int mkenvid(struct Env *e) {
	static u_int i = 0;
	return ((++i) << (1 + LOG2NENV)) | (e - envs);
    //高位不全为0
}
```

如果传入的是无效（没被分配）的envid，则不判断`e->env_id!=envid`就会产生错误。

### Thinking 4.3

> 思考下面的问题，并对这个问题谈谈你的理解：请回顾 kern/env.c 文件中 mkenvid() 函数的实现，该函数不会返回 0，请结合系统调用和 IPC 部分的实现与 envid2env() 函数的行为进行解释。

由上可知，envid不会为0。故将0作为保留值，可以很方便地获取当前$\cal{PCB}$的指针。

### Thinking 4.4

> 关于 fork 函数的两个返回值，下面说法正确的是：
>
> A、fork 在父进程中被调用两次，产生两个返回值
>
> B、fork 在两个进程中分别被调用一次，产生两个不同的返回值
>
> C、fork 只在父进程中被调用了一次，在两个进程中各产生一个返回值
>
> D、fork 只在子进程中被调用了一次，在两个进程中各产生一个返回值

```c
int fork(void) {
    //...
	child = syscall_exofork();
	if (child == 0) {
		env = envs + ENVX(syscall_getenvid());
		return 0;
	}
    //...
    return child;
}
```

$\cal{C}$，`fork`只在当前进程（父进程）中被调用了一次，通过进一步调用`sys_exofork`创建了新的进程，复制了此时的现场并将子进程上下文环境中的`e->env_tf.regs[2]($v0)`设为**0**，而父进程则在最后返回子进程id。

### Thinking 4.5

> 我们并不应该对所有的用户空间页都使用 duppage 进行映射。那么究竟哪些用户空间页应该映射，哪些不应该呢？请结合 kern/env.c 中 env_init 函数进行的页面映射、include/mmu.h 里的内存布局图以及本章的后续描述进行思考。 

首先`ULIM`和`UTOP`之间的内容为内核的页表数据，也是所有进程共享的只读空间，同时在`env_alloc`已从`base_pgdir`中拷贝至进程页表，故不需要映射。

然后`UTOP`和`USTACKTOP`之间的内容是用来处理用户异常的，也不需要进行映射。

故最终需要映射的内容为`USTACKTOP`之下所有的有效页（`PTE_V`）。

### Thinking 4.6

> 在遍历地址空间存取页表项时你需要使用到 vpd 和 vpt 这两个指针，请参考 user/include/lib.h 中的相关定义，思考并回答这几个问题：
>
> - vpt 和 vpd 的作用是什么？怎样使用它们？
> - 从实现的角度谈一下为什么进程能够通过这种方式来存取自身的页表？
> - 它们是如何体现自映射设计的？
> - 进程能够通过这种方式来修改自己的页表项吗？

1. `vpt`和`vpd`分别是指向用户页表和页目录的指针，以`vpt`为基地址，加上页表偏移量即可得到指向`va`对应的页表项的指针，即`vpt[va >> 12]`；以`vpd`为基地址，加上页目录偏移量即可得到指向`va`对应的页目录项的指针。

2. ```c
    #define vpt ((const volatile Pte *)UVPT)
    #define vpd ((const volatile Pde *)(UVPT + (PDX(UVPT) << PGSHIFT)))
    //这两个值分别为用户地址空间中页表的首地址和页目录的首地址
    ```

3. ```c
    static int env_setup_vm(struct Env *e) {
        //...
        e->env_pgdir[PDX(UVPT)] = PADDR(e->env_pgdir) | PTE_V;
        //将进程页目录表中 UVPT 对应的页目录项设为了进程页目录表对应的地址
    	return 0;
    }
    ```

4. 不能，页表由内核维护，用户只能进行访问。

### Thinking 4.7

> 在 do_tlb_mod 函数中，你可能注意到了一个向异常处理栈复制 Trapframe 运行现场的过程，请思考并回答这几个问题：
>
> -  这里实现了一个支持类似于“异常重入”的机制，而在什么时候会出现这种“异常重 入”？
> - 内核为什么需要将异常的现场 Trapframe 复制到用户空间？

1. 异常重入是指在处理异常的过程中，又触发了新的异常。如在处理写时复制的页写入异常时，又发生了缺页等异常。
2. 因为页写入异常的处理是在用户态进行的，所以需要保存现场到用户空间用以恢复现场。

### Thinking 4.8

> 在用户态处理页写入异常，相比于在内核态处理有什么优势？

提高系统的稳定性，即使处理异常时进程崩溃，也不会影响到整个系统。

### Thinking 4.9

> 请思考并回答以下几个问题：
>
> - 为什么需要将 syscall_set_tlb_mod_entry 的调用放置在 syscall_exofork 之前？
> - 如果放置在写时复制保护机制完成之后会有怎样的效果？

1. 只需要在写时复制保护机制完成之前调用`syscall_set_tlb_mod_entry`即可。
2. 无法正常处理页写入相关异常。

## 难点分析

### 系统调用完整过程

![syscall](https://cdn.jsdelivr.net/gh/EinNiemand28/my-img@master/images/image-20240522161536474.png)

1. 用户程序调用`syscall_*`函数（定义在`user/lib/syscall_lib.h`中）。

2. `syscall_*`函数将系统调用号和系统调用参数存入寄存器（`$a0-$a3`）和用户栈中（这一过程由编译器自动编译），然后调用`msyscall。`

3. 在`msyscall`函数中执行`syscall`指令使CPU陷入内核态。

    ```assembly
    LEAF(msyscall)
    	syscall
    	jr		ra
    	//记得返回syscall_*
    END(msyscall)
    ```

4. 陷入内核态后，跳转到`.exc_gen_entry`处执行指令。

    ```assembly
    .section .text.exc_gen_entry
    exc_gen_entry:
    	SAVE_ALL
    	//将用户进程上下文环境保存在内核栈中
    	mfc0    t0, CP0_STATUS
    	and     t0, t0, ~(STATUS_UM | STATUS_EXL | STATUS_IE)
    	mtc0   	t0, CP0_STATUS
    	mfc0	t0, CP0_CAUSE
    	andi	t0, 0x7c
    	lw		t0, exception_handlers(t0)
    	//跳转到对应的异常处理函数
    	jr		t0
    ```

5. `kern/traps.c`中定义了异常处理函数，系统调用对应的异常码为$8$。

    ```c
    void (*exception_handlers[32])(void) = {
        [0 ... 31] = handle_reserved,
        [0] = handle_int,
        [2 ... 3] = handle_tlb,
        [1] = handle_mod,
        [8] = handle_sys,
    };
    ```

    `kern/genex.S`中使用`BUILD_HANDLER`宏对`handle_*`函数进行了封装。

    ```assembly
    .macro BUILD_HANDLER exception handler
    NESTED(handle_\exception, TF_SIZE + 8, zero)
    	move    a0, sp
    	addiu   sp, sp, -8
    	jal     \handler
    	addiu   sp, sp, 8
    	j       ret_from_exception
    END(handle_\exception)
    .endm
    BUILD_HANDLER mod do_tlb_mod
    BUILD_HANDLER sys do_syscall
    ```

6. `do_syscall`根据系统调用号调用对应的函数，并将返回值存入内核栈中保存的用户进程（`tf.reg[2]`）、修改epc使得能够返回到`msyscall`函数中的`jr ra`指令。

7. 调用`ret_from_exception`，执行`RESTORE_ALL`宏还原现场并返回用户程序，系统调用结束。

### ipc

![ipc](https://cdn.jsdelivr.net/gh/EinNiemand28/my-img@master/images/image-20240522164558501.png)

### fork的实现

![fork](https://cdn.jsdelivr.net/gh/EinNiemand28/my-img@master/images/image-20240522165234868.png)

#### 写时复制机制

使用`fork`创建子进程时，让父子进程暂时共享内存，当页面需要写入数据时才将内存分离，以避免同时消耗大量物理内存。

具体实现为，将虚拟页权限位设置为`PTE_D = 0 && PTE_COW = 1`，当需要写入时触发`TLB_Mod`异常。

#### 页写入异常处理机制

当用户程序写入一个在TLB中`PTE_D`位为$0$的页面时，系统会陷入`TLB Mod`异常，之后会跳转到`kern/tlbex.c`中的`do_tlb_mod`函数。

该函数会将进程上下文保存在**用户态的异常处理栈**中，并设置`$a0`和`$epc`的值，使得异常恢复后能跳转到`env_user_tlb_mod_entry`所设置的用户异常处理函数的地址。也就是说，`TLB Mod`异常的主要处理过程是在用户态中。

------

对于写时复制机制，我们将父子进程的`TLB Mod`异常处理函数设为`cow_entry`

```c
static void __attribute__((noreturn)) cow_entry(struct Trapframe *tf) {
    //获取发生写入异常的地址
	u_int va = tf->cp0_badvaddr;
	u_int perm;
	perm = vpt[VPN(va)] & 0xfff;
    //检查标志位
	if (!(perm & PTE_COW)) {
		user_panic("doesn't have PTE_COW");
	}
    //分配一个新的物理页面到临时地址UCOW，修改权限
	perm = (perm & (~PTE_COW)) | PTE_D;
	syscall_mem_alloc(0, (void *) UCOW, perm);
    //复制页面内容至新页面
	memcpy((void *) UCOW, (void *) ROUNDDOWN(va, PAGE_SIZE), PAGE_SIZE);
	//映射写入异常的地址到新页面上
	syscall_mem_map(0, (void *) UCOW, 0, (void *) va, perm);
    //解除临时地址UCOW的内存映射
	syscall_mem_unmap(0, (void *) UCOW);
    //设置返回地址
	int r = syscall_set_trapframe(0, tf);
	user_panic("syscall_set_trapframe returned %d", r);
}
```

----

值得一提的是，如何在用户异常处理函数中返回呢？通过上面的内容可以看到，采用的方法是陷入一个新的异常，替换内核栈的内容，从而在返回时根据替换的`Trapframe`恢复现场。这就是系统调用`sys_set_trapframe`的基本原理。

```c
int sys_set_trapframe(u_int envid, struct Trapframe *tf) {
	//...
	if (env == curenv) {
		*((struct Trapframe *)KSTACKTOP - 1) = *tf;
		//注意返回值
		return tf->regs[2];
	} else {
		//...
	}
}
```

#### 页面映射

由于写时复制机制，我们只需调用`syscall_mem_map`将父进程的页面映射到子进程中。

```c
//fork()
for (i = 0; i < VPN(USTACKTOP); i++) {
    if ((vpd[i >> 10] & PTE_V) && (vpt[i] & PTE_V)) {
        //vpd和vpt的使用见思考题4.6
        duppage(child, i);
    }
}
```

权限设置规则如下：

- 原本便不可写、共享或就是写时复制的页面不需要更改其权限
- 对可写、非共享且不是写时复制的页面，需要取消其可写位，设置写时复制位。然后将其映射给子进程， 并更新父进程这一页的权限

```c
//duppage()
if ((perm & PTE_D) && !(perm & PTE_LIBRARY) && !(perm & PTE_COW)) {
    perm = (perm & (~PTE_D)) | PTE_COW;
    syscall_mem_map(0, (void *) addr, envid, (void *) addr, perm);
    syscall_mem_map(0, (void *) addr, 0, (void *) addr, perm);
} else {
    syscall_mem_map(0, (void *) addr, envid, (void *) addr, perm);
}
```

## 实验体会

