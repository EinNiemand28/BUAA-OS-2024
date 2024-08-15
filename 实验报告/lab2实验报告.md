# lab2实验报告

## 思考题

### Thinking2.1

> >请根据上述说明，回答问题：在编写的 C 程序中，指针变量中存储的地址被视为虚拟地址，还是物理地址？MIPS 汇编程序中`lw`和`sw`指令使用的地址被视为虚拟地址，还是物理地址？

C程序中指针变量存储的地址和MIPS汇编程序中访存指令使用的地址均为虚拟地址。

### Thinking2.2

> >请思考下述两个问题：
> >
> >- 从可重用性的角度，阐述用宏来实现链表的好处。
> >- 查看实验环境中的 `/usr/include/sys/queue.h`，了解其中单向链表与循环链表的实现，比较它们与本实验中使用的双向链表，分析三者在插入与删除操作上的性能差异。

首先使用宏定义，将链表的各种操作进行了封装，提高了代码的重用性和可读性；并且还实现了C不支持的泛型，由此可以实现自定义数据类型的链表。

|                      | 插入                                                         | 删除                                                   |
| -------------------- | ------------------------------------------------------------ | ------------------------------------------------------ |
| *Singly-linked List* | 头部插入$\mathcal{O}(1)$，尾部插入$\mathcal{O}(n)$，<br>指定节点前插入$\mathcal{O}(n)$，节点后插入$\mathcal{O}(1)$ | 删除头部$\mathcal{O}(1)$，删除指定节点$\mathcal{O}(n)$ |
| *List*               | 头部插入$\mathcal{O}(1)$，尾部插入$\mathcal{O}(n)$，<br>指定节点前插入$\mathcal{O}(1)$，节点后插入$\mathcal{O}(1)$ | 删除头部$\mathcal{O}(1)$，删除指定节点$\mathcal{O}(1)$ |
| *Circular queue*     | 头部插入$\mathcal{O}(1)$，尾部插入$\mathcal{O}(1)$，<br>指定节点前插入$\mathcal{O}(1)$，节点后插入$\mathcal{O}(1)$ | 删除头部$\mathcal{O}(1)$，删除指定节点$\mathcal{O}(1)$ |

### Thinking2.3

> >请阅读 `include/queue.h` 以及 `include/pmap.h`，将 `Page_list` 的结构梳 理清楚，选择正确的展开结构。

`Page_list`的展开结构为

```c
struct Page_list{
    struct {
        struct {
            struct Page *le_next;
            struct Page **le_prev;
        } pp_link;
        u_short pp_ref;
    }* lh_first;
}
```

### Thinking2.4

> >请思考下面两个问题：
> >
> >- 请阅读上面有关 TLB 的描述，从虚拟内存和多进程操作系统的实现角度，阐述 ASID 的必要性。
> >- 请阅读 MIPS 4Kc 文档《MIPS32® 4K™ Processor Core Family Software User’s Manual》的 Section 3.3.1 与 Section 3.4，结合 ASID 段的位数，说明 4Kc 中可容纳不同的地址空间的最大数量。

对不同进程来说，同一虚拟地址可能映射到不同的物理地址，所以使用`ASID`来判断是哪个进程的虚拟地址。

`ASID`段占8位，故可容纳256个地址空间。

| Field Name | Description                                                  |
| ---------- | ------------------------------------------------------------ |
| ASID[7:0]  | Address Space Identifier. Identifies which process or thread this TLB entry is associated with. |

### Thinking2.5

> >请回答下述三个问题：
> >
> >- `tlb_invalidate` 和 `tlb_out` 的调用关系？
> >- 请用一句话概括 `tlb_invalidate` 的作用。
> >- 逐行解释 `tlb_out` 中的汇编代码。

·`tlb_invalidate`调用`tlb_out`。

将指定虚拟地址在TLB中的表项删除。

```assembly
LEAF(tlb_out)
.set noreorder
	mfc0    t0, CP0_ENTRYHI
	/* 用于恢复 EntryHi */
	mtc0    a0, CP0_ENTRYHI
	/* 将调用函数的参数传入 */
	nop
	/* Step 1: Use 'tlbp' to probe TLB entry */
	/* Exercise 2.8: Your code here. (1/2) */
	tlbp
	/* 根据 EntryHi 中的 Key（包含VPN与ASID），查找 TLB 中与之对应的表项，并将
表项的索引存入 Index 寄存器 */
	nop
	/* Step 2: Fetch the probe result from CP0.Index */
	mfc0    t1, CP0_INDEX
	/* 读取查询结果 */
.set reorder
	bltz    t1, NO_SUCH_ENTRY
	/* 判断是否存在表项 */
.set noreorder
	mtc0    zero, CP0_ENTRYHI
	mtc0    zero, CP0_ENTRYLO0
	mtc0    zero, CP0_ENTRYLO1
	nop
	/* Step 3: Use 'tlbwi' to write CP0.EntryHi/Lo into TLB at CP0.Index  */
	/* Exercise 2.8: Your code here. (2/2) */
	tlbwi
	/* 向 EntryHi 和 EntryLo 写入指定的表项 */

.set reorder

NO_SUCH_ENTRY:
	mtc0    t0, CP0_ENTRYHI
	/* 恢复EntryHi */
	j       ra
END(tlb_out)
```

### Thinking2.6

> >从下述三个问题中任选其一回答：
> >
> >- 简单了解并叙述 X86 体系结构中的内存管理机制，比较 X86 和 MIPS 在内存管理上 的区别。
> >- 简单了解并叙述 RISC-V 中的内存管理机制，比较 RISC-V 与 MIPS 在内存管理上 的区别。
> >- 简单了解并叙述 LoongArch 中的内存管理机制，比较 LoongArch 与 MIPS 在内存 管理上的区别。

X86体系结构中使用了段页式内存管理机制，更有利于内存保护和共享。而MIPS内存管理更为简单，仅有页机制。

同时X86的分页是可选的，可通过CR0寄存器的PG位进行设置。

## 难点分析

### 常用宏

- `PDX(va)`：页目录偏移量（查找遍历页表时常用）
- `PTX(va)`：页表偏移量（查找遍历页表时常用）
- `PTE_ADDR(pte)`：获取页表项中的物理地址（读取 pte 时常用）
- `PADDR(kva)`：kseg0 处虚地址 $\to$ 物理地址
- `KADDR(va)`：物理地址 $\to$ kseg0 处虚地址（读取 pte 后可进行转换）
- `va2pa(Pde *pgdir, u_long va)`：查页表，虚地址 → 物理地址
- `pa2page(u_long pa)`：物理地址 $\to$ 页控制块（读取 pte 后可进行转换）
- `page2pa(struct Page *pp)`：页控制块 $\to$ 物理地址（填充 pte 时常用）
- ……

### 页控制块

```c
struct Page {
    struct {
        struct Page *le_next;	// 指向后一个Page的指针
        struct Page **le_prev;	// 指向前一个Page的le_next的指针
    } pp_link;
    u_short pp_ref;
}
```

数据结构实现

![](https://cdn.jsdelivr.net/gh/EinNiemand28/my-img@master/images/image-20240411162255197.png)

理解链表宏的实现并使用……

### 两级页表结构

![](https://cdn.jsdelivr.net/gh/EinNiemand28/my-img@master/images/image-20240411163138281.png)

#### 一些函数

`int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte)`

$\to$ 将一级页 表基地址 pgdir 对应的两级页表结构中 va 虚拟地址所在的二级页表项的指针存储在 ppte 指 向的空间上。如果 create 不为 0 且对应的二级页表不存在，则会使用 page_alloc 函数分配一 页物理内存用于存放二级页表，如果分配失败则返回错误码。

`int page_insert(Pde *pgdir, u_int asid, struct Page *pp, u_long va, u_int perm)`

$\to$ 将一级页表基地址 pgdir 对应的两级页表结构中虚拟地址 va 映射到页控制块 pp 对应的 物理页面，并将页表项权限为设置为 perm。

`struct Page * page_lookup(Pde *pgdir, u_long va, Pte **ppte)`

$\to$ 返回一级 页表基地址 pgdir 对应的两级页表结构中虚拟地址 va 映射的物理页面的页控制块，同时将 ppte 指向的空间设为对应的二级页表项地址。

### 访存与TLB重填

每个 TLB 表项都有两个组成部分，包括一组 Key 和两组 Data。

使用 VPN 中的高 19 位与 ASID 作为 Key，一次查找到两个 Data（EntryLo0存储偶页，EntryLo1存储奇页）

![](https://cdn.jsdelivr.net/gh/EinNiemand28/my-img@master/images/image-20240411163816488.png)

`do_tlb_refill`

1. 从 BadVAddr 中取出引发 TLB 缺失的虚拟地址。
2. 从 EntryHi 的 0 – 7 位取出当前进程的 ASID。
3. 先在栈上为返回地址、待填入 TLB 的页表项以及函数参数传递预留空间，并存入返回地址。 以存储奇偶页表项的地址、触发异常的虚拟地址和 ASID 为参数，调用 _do_tlb_refill函数，根据虚拟地址和 ASID 查找页表，将对 应的奇偶页表项写回其第一个参数所指定的地址。
4. 将页表项存入 EntryLo0、EntryLo1 ，并执行 tlbwr 将此时的 EntryHi 与 EntryLo0、 EntryLo1 写入到 TLB 中。

## 实验体会

lab2的主要任务是掌握MOS的内存管理机制，和lab1相比难度增大了不少。首先一定要学好理论课上的知识和指导书的内容；其次还是要多读代码多思考，不要畏惧眼花缭乱的宏命令和函数调用，利用好源码中的大量注释可以做到事倍功半。
