# lab3实验报告

## 思考题

### Thinking 3.1
>请结合 MOS 中的页目录自映射应用解释代码中 `e->env_pgdir[PDX(UVPT)] = PADDR(e->env_pgdir) | PTE_V` 的含义。

这句代码的含义为：将进程页目录中虚拟地址`UVPT`对应的页目录项设置为该进程页目录的物理地址，并将权限设为只读。	

### Thinking 3.2
>elf_load_seg 以函数指针的形式，接受外部自定义的回调函数 map_page。请你找到与之相关的 data 这一参数在此处的来源，并思考它的作用。没有这个参数可不可以？为什么？

`data`参数的来源为该进程的进程控制块的指针。不能没有这个参数，因为`load_icode_mapper`插入页表时需要知道进程的页目录基地址和`asid`。

### Thinking 3.3
>结合 elf_load_seg 的参数和实现，考虑该函数需要处理哪些页面加载的情况。

![image-20240424172525402](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240424172525402.png)

由图和`elf_load_seg`实现可知，进行页面加载的过程如下：

1. 若`va`未对齐，则从第一个页面`offset`处加载大小为`MIN(bin_size, PAGE_SIZE - offset)`的数据
2. 加载段内已页对齐的部分
3. 若在内存中的大小大于在文件中的大小，则多余的部分用$0$进行填充

### Thinking 3.4
>思考上面这一段话，并根据自己在 Lab2 中的理解，回答：
>- 你认为这里的 env_tf.cp0_epc 存储的是物理地址还是虚拟地址?

存储的是虚拟地址。`epc`中存储的是异常结束后程序恢复执行的位置，而执行指令的$\cal{CPU}$只会发出虚拟地址。

### Thinking 3.5
>试找出 0、1、2、3 号异常处理函数的具体实现位置。8 号异常（系统调用）涉及的 do_syscall() 函数将在 Lab4 中实现。 

具体实现位置在`genex.S`中

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

NESTED(handle_int, TF_SIZE, zero)
	mfc0    t0, CP0_CAUSE
	mfc0    t2, CP0_STATUS
	and     t0, t2
	andi    t1, t0, STATUS_IM7
	bnez    t1, timer_irq
timer_irq:
	li      a0, 0
	j       schedule
END(handle_int)

BUILD_HANDLER tlb do_tlb_refill

#if !defined(LAB) || LAB >= 4
BUILD_HANDLER mod do_tlb_mod
BUILD_HANDLER sys do_syscall
#endif

BUILD_HANDLER reserved do_reserved
```

### Thinking 3.6
>阅读 entry.S、genex.S 和 env_asm.S 这几个文件，并尝试说出时钟中断在哪些时候开启，在哪些时候关闭。

时钟中断在处理异常时关闭：

```assembly
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
	//...
```

在调度执行每个进程之前开启，由`genex.S`中宏`RESTORE_ALL`恢复`cp0_status`：

```assembly
FEXPORT(ret_from_exception)
	RESTORE_ALL
	eret

.macro RESTORE_ALL
.set noreorder
.set noat
	lw      v0, TF_STATUS(sp)
	mtc0    v0, CP0_STATUS
	//...
```



### Thinking 3.7
>阅读相关代码，思考操作系统是怎么根据时钟中断切换进程的。

首先$\cal{MOS}$在每次`env_run`之后使用宏`RESET_KCLOCK`完成时钟中断的初始化，然后当时钟中断发生时，调用`schedule`函数进行调度，具体操作如下：

1. 若满足以下条件则切换进程
    - 尚未调度过任何进程（curenv 为空指针）
    - 当前进程已经用完了时间片
    - 当前进程不再就绪（如被阻塞或退出）
    - `yield` 参数指定必须发生切换
2. 将当前进程的可用时间片数`count`减一

## 难点分析

### 进程控制块

每个进程都是一个实体，有其自己的地址空间，通常包括代码段、数据段和堆栈。进程控制块（$\cal{PCB}$）是系统感知进程存在的唯一标志，进程与进程控制块一一对应。

```c
// Control block of an environment (process).
struct Env {
	struct Trapframe env_tf;	 // saved context (registers) before switching
	LIST_ENTRY(Env) env_link;	 // intrusive entry in 'env_free_list'
	u_int env_id;			 // unique environment identifier
	u_int env_asid;			 // ASID of this env
	u_int env_parent_id;		 // env_id of this env's parent
	u_int env_status;		 // status of this env
	Pde *env_pgdir;			 // page directory
	TAILQ_ENTRY(Env) env_sched_link; // intrusive entry in 'env_sched_list'
	u_int env_pri;			 // schedule priority
};
```

### 模板页表和进程创建

预先创建一个页表作为“模板”，建立对`pages`和`envs`数组的映射关系，使得创建一个新进程时，可以很方便地将这两个数组分别映射到用户地址空间（`kuseg`）中的`UPAGES`和`UENVS`处。

![image-20240425203421515](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240425203421515.png)

进程是资源分配的最小单位。一个进程对应一个虚拟地址空间，而建立一个虚拟地址空间就是建立一个页表。

```c
//建立进程虚拟地址空间
memcpy(e->env_pgdir + PDX(UTOP), base_pgdir + PDX(UTOP), 
       sizeof(Pde) * (PDX(UVPT) - PDX(UTOP)));
//复制模板页表
e->env_pgdir[PDX(UVPT)] = PADDR(e->env_pgdir) | PTE_V;
/*
	建立页表自映射
	将进程页目录中虚拟地址`UVPT`对应的页目录项设置为该进程页目录的物理地址，并将权限设为只读
*/
```

### 宏拼接

`ENV_CREATE`宏的实现：

```c
#define ENV_CREATE(x) 												\
	({ 															\
		extern u_char binary_##x##_start[]; 						\
		extern u_int binary_##x##_size; 							\
		env_create(binary_##x##_start, (u_int)binary_##x##_size, 1); \
	})
```

### 进程切换

时间片轮转算法，由时钟中断实现

### 异常的分发与处理

对不同异常选择特定的异常处理函数

```assembly
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
	mfc0	t0, CP0_CAUSE
	andi	t0, 0x7c
	lw		t0, exception_handlers(t0)
	jr		t0
```

处理异常时取对应指令

```c
void handle(struct Trapframe *tf) {
    //把kuseg转换为kseg0然后直接访存
	u_int epc = tf->cp0_epc;
	Pte *pte;
	page_lookup(curenv->env_pgdir, epc, &pte);
	u_int *inst = (u_int *) (KADDR(PTE_ADDR(*pte)) | (epc & 0xfff));
    //...
}
```

## 实验体会

首先是学会了命令行$\cal{GDB}$调试（~~虽然新主楼的机子用tmux的时候很卡~~），确实很有用，加上$print$大法感觉自己~~debug能力强的可怕~~

但是之前lab2的知识还不是特别熟，用的时候就出问题了，内存管理机制、`mmu.h`里面的内存分布图和各种宏还要多看多记