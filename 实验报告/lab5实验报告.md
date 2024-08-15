# lab5实验报告

## 思考题

### Thinking 5.1

>如果通过 kseg0 读写设备，那么对于设备的写入会缓存到 Cache 中。这是一种**错误**的行为，在实际编写代码的时候这么做会引发不可预知的问题。请思考：这么做这会引发什么问题？对于不同种类的设备（如我们提到的串口设备和 IDE 磁盘）的操作会有差异吗？可以从缓存的性质和缓存更新的策略来考虑。

当程序需要读取设备的数据时，若数据已经被写入了Cache，则会从Cache中直接读取，而此时若外部设备的数据发生变化，则会产生错误的行为。

### Thinking 5.2

>查找代码中的相关定义，试回答一个磁盘块中最多能存储多少个文件控制块？一个目录下最多能有多少个文件？我们的文件系统支持的单个文件最大为多大？ 

```c
// Bytes per file system block - same as page size
#define BLOCK_SIZE PAGE_SIZE

//...

#define FILE_STRUCT_SIZE 256

struct File {
	char f_name[MAXNAMELEN]; // filename
	uint32_t f_size;	 // file size in bytes
	uint32_t f_type;	 // file type
	uint32_t f_direct[NDIRECT];
	uint32_t f_indirect;

	struct File *f_dir; // the pointer to the dir where this file is in, valid only in memory.
	char f_pad[FILE_STRUCT_SIZE - MAXNAMELEN - (3 + NDIRECT) * 4 - sizeof(void *)];
} __attribute__((aligned(4), packed));

#define FILE2BLK (BLOCK_SIZE / sizeof(struct File))

// File types
#define FTYPE_REG 0 // Regular file
#define FTYPE_DIR 1 // Directory
```

由代码可知，一个磁盘块最多能存储$\frac{4096B}{256B}=16$个文件控制块。

若`f_type == FTYPE_DIR`，其中的指针最多可以记录$1024$个磁盘块，故一个目录下最多有$1024 \times 16 = 16384$个文件。

若`f_type == FTYPE_REG`，则同理可知单个文件最大为$1024 \times 4KB = 4MB$。

### Thinking 5.3

>请思考，在满足磁盘块缓存的设计的前提下，我们实验使用的内核支持的最大磁盘大小是多少？ 

```c
/* Disk block n, when in memory, is mapped into the file system
 * server's address space at DISKMAP+(n*BLOCK_SIZE). */
#define DISKMAP 0x10000000

/* Maximum disk size we can handle (1GB) */
#define DISKMAX 0x40000000
```

显然，支持的最大磁盘大小为$0{\rm x}4000\_0000B=1GB$

### Thinking 5.4

>在本实验中，fs/serv.h、user/include/fs.h 等文件中出现了许多宏定义，试列举你认为较为重要的宏定义，同时进行解释，并描述其主要应用之处。 

代码中的注释写得很详细，这些宏大部分指代不同数据结构的大小，如

```c
#define FILE2BLK (BLOCK_SIZE / sizeof(struct File))
//指一个磁盘块最大文件块数，可用来遍历目录项

u_int nblock;
nblock = ROUND(dir->f_size, BLOCK_SIZE) / BLOCK_SIZE;
for (int i = 0; i < nblock; i++) {
    void *blk;
    try(file_get_block(dir, i, &blk));
	struct File *files = (struct File *)blk;
    for (struct File *f = files; f < files + FILE2BLK; ++f) {
        //do something...
    }
}
```

### Thinking 5.5

>在 Lab4“系统调用与 fork”的实验中我们实现了极为重要的 fork 函数。那么 fork 前后的父子进程是否会共享文件描述符和定位指针呢？请在完成上述练习的基础上编写一个程序进行验证。

```c
//serv_check.c
#include <lib.h>

static char *msg = "This is the NEW message of the day!\n";
static char *diff_msg = "This is a different message of the day!\n";

int main() {
	int r;
	int fdnum;
	char buf[512];
	int n;

	if ((r = open("/newmotd", O_RDWR)) < 0) {
		user_panic("cannot open /newmotd: %d", r);
	}
	fdnum = r;
	debugf("open is good\n");

	if (r = fork()) {
		n = read(fdnum, buf, 10);
		debugf("father: \'%s\'\n", buf);
	} else {
		n = read(fdnum, buf, 10);
		debugf("child: \'%s\'\n", buf);
	}
}
```

![](https://cdn.jsdelivr.net/gh/EinNiemand28/my-img@master/images/image-20240530213141501.png)

### Thinking 5.6

>请解释 File, Fd, Filefd 结构体及其各个域的作用。比如各个结构体会在哪些过程中被使用，是否对应磁盘上的物理实体还是单纯的内存数据等。说明形式自定，要求简洁明了，可大致勾勒出文件系统数据结构与物理实体的对应关系与设计框架。 

```c
#define MAXNAMELEN 128
#define NDIRECT 10
#define FILE_STRUCT_SIZE 256
// file control block
struct File {
	char f_name[MAXNAMELEN]; // filename
	uint32_t f_size;	 // file size in bytes
	uint32_t f_type;	 // file type
	uint32_t f_direct[NDIRECT];
	uint32_t f_indirect;
    // 指向存放文件数据的对应磁盘块，不适用指向间接磁盘块（(uint32_t *) f_indirect）的前十个指针
    /* 使用示范
    	int bno; // the block number
    	if (i < NDIRECT) {
    		bno = dirf->f_direct[i];
    	} else {
    		bno = ((uint32_t *) disk[dirf->f_indirect].data)[i];
    	}
    */

	struct File *f_dir; // the pointer to the dir where this file is in, valid only in memory.
	char f_pad[FILE_STRUCT_SIZE - MAXNAMELEN - (3 + NDIRECT) * 4 - sizeof(void *)]; // 占位
} __attribute__((aligned(4), packed));

// file descriptor
struct Fd {
	u_int fd_dev_id; // 该文件对应的设备
	u_int fd_offset; // 读写的偏移量
	u_int fd_omode; // 读写方式，包括只读、只写、读写等
};

// 引入文件操作符，是为了给用户程序提供操作文件的同一接口，使用户进程能知晓文件的信息和状态

// file descriptor + file
struct Filefd {
	struct Fd f_fd; // 文件描述符
	u_int f_fileid; // 文件id
	struct File f_file; // 对应的文件控制块
};
```

### Thinking 5.7

![](https://cdn.jsdelivr.net/gh/EinNiemand28/my-img@master/images/image-20240528225331405.png)

>图 5.9 中有多种不同形式的箭头，请解释这些不同箭头的差别，并思考我们的操作系统是如何实现对应类型的进程间通信的。 

图中实线箭头表示调用另一个程序中的对应函数，虚线箭头表示使用IPC获取调用结果。

- 文件系统服务进程在完成初始化（`serv_init,fs_init`）后，调用`serve`函数（一个死循环），然后反复调用`ipc_recv`等待用户进程发出操作请求，并根据请求的不同类型执行相应的文件操作，完成服务后使用`ipc_send`返回结果。
- 用户进程针对不同的请求类型，通过`ipc_send`发送必要的参数（将请求的内容放在结构体中），然后等待被文件系统反馈的结果（`ipc_send`）唤醒。

## 难点分析

### 外设控制

CPU 通过读写设备控制器上的寄存器实现对设备的控制和通信。而在MIPS体系结构下，我们使用 MMIO机制访问设备寄存器。

**MMIO**（内存映射IO）：使用不同的物理内存地址 为设备寄存器编址，将一部分对物理内存的访问 “重定向” 到设备地址空间中。

| 偏移 | 寄存器功能                                                   | 数据位宽 |
| ---- | :----------------------------------------------------------- | -------- |
| 0x0  | 读/写：向磁盘中读/写数据，从 0 字节开始逐个读出/写入         | 4B       |
| 0x1  | 读：设备错误信息；写：设置 IDE 命令的特定参数                | 1B       |
| 0x2  | 写：设置一次需要操作的扇区数量                               | 1B       |
| 0x3  | 写：设置目标扇区号的 [7:0] 位（LBAL）                        | 1B       |
| 0x4  | 写：设置目标扇区号的 [15:8] 位（LBAM）                       | 1B       |
| 0x5  | 写：设置目标扇区号的 [23:16] 位（LBAH）                      | 1B       |
| 0x6  | 写：设置目标扇区号的 [27:24] 位，配置扇区寻址模式 （CHS/LBA），设置要操作的磁盘编号 | 1B       |
| 0x7  | 读：获取设备状态；写：配置设备工作状态                       | 1 字节   |

![](https://cdn.jsdelivr.net/gh/EinNiemand28/my-img@master/images/image-20240530222324917.png)

使用系统调用`sys_write_dev`和`sys_read_dev`读写`kseg1`段内核数据，来实现设备读写操作。

### 文件系统结构

#### 磁盘结构

```c
struct Block {
	uint8_t data[BLOCK_SIZE];
	uint32_t type;
} disk[NBLOCK];
// 操作系统与磁盘交互的最小逻辑单元
// Block0作为引导扇区和分区表使用
// Block1作为超级块（Super Block），文件系统的基本信息
// Block2作为位图块，用来管理空闲的磁盘资源

struct Super {
	uint32_t s_magic;   // Magic number: FS_MAGIC
	uint32_t s_nblocks; // Total number of blocks on disk （1024）
	struct File s_root; // Root directory node（f_type: FTYPE_DIR, f_name: "/"）
};
/*---------------------------------------*/
// Initial the disk. Do some work with bitmap and super block.
void init_disk() {
	int i, diff;

	// Step 1: Mark boot sector block.
	disk[0].type = BLOCK_BOOT;

	// Step 2: Initialize boundary.
	nbitblock = (NBLOCK + BLOCK_SIZE_BIT - 1) / BLOCK_SIZE_BIT;
    // 为了使用Bitmap标识整个磁盘上所有块的使用情况所需要的磁盘块的数量
	nextbno = 2 + nbitblock;

	// Step 2: Initialize bitmap blocks.
	for (i = 0; i < nbitblock; ++i) {
		disk[2 + i].type = BLOCK_BMAP;
	}
	for (i = 0; i < nbitblock; ++i) {
		memset(disk[2 + i].data, 0xff, BLOCK_SIZE);
	}
	if (NBLOCK != nbitblock * BLOCK_SIZE_BIT) {
        // 不存在的部分置为0
		diff = NBLOCK % BLOCK_SIZE_BIT / 8;
		memset(disk[2 + (nbitblock - 1)].data + diff, 0x00, BLOCK_SIZE - diff);
	}

	// Step 3: Initialize super block.
	disk[1].type = BLOCK_SUPER;
	super.s_magic = FS_MAGIC;
	super.s_nblocks = NBLOCK;
	super.s_root.f_type = FTYPE_DIR;
	strcpy(super.s_root.f_name, "/");
}
```

#### 文件系统

见`Thinking 5.6`

#### 用户接口

引入文件描述符（file descriptor）作为用户程序管理、操作文件的基础，隔离底层的文件系统实现，抽象地表示对一个文件进行的操作。

关于`Fd`和`Filefd`可看`Thinking 5.6`

我们将一个进程所有的文件描述符存储在`[FDTABLE, FILEBASE)`这一地址空间中（使用`fd_alloc`分配），在`FILEBASE`之上存储对应的`Filefd`。

- Filefd 结构体的第一个成员就是 Fd，因此指向 Filefd 的指针同样指向这个 Fd 的起始位置，故可以进行强制转换。

- 可使用`fa2data`获取文件描述符对应的文件的数据，使用`fd2num`返回描述符编号

```c
void *fd2data(struct Fd *fd) {
	return (void *)INDEX2DATA(fd2num(fd));
}

int fd2num(struct Fd *fd) {
	return ((u_int)fd - FDTABLE) / PTMAP;
}
```

### 文件操作如何进行

![](https://cdn.jsdelivr.net/gh/EinNiemand28/my-img@master/images/image-20240530231846036.png)

随后在函数`fsipc`中使用IPC向文件服务系统发送请求，在`serve_*`中调用相应的`fs_*`对文件和磁盘数据进行操作

## 实验体会

lab5总体看虽然代码量大很吓人，但实际上通过阅读指导书了解了MOS文件系统的精巧设计思路，仿照lab4的思路去理解各个函数之间的调用关系，还是可以比较轻松地完成任务的。只不过确实这次课下没有付出相应的充足时间，导致课上很不熟练、bug频出，最后也没时间做extra了。

然后关于课上debug，一点惨痛教训：指针的类型强制转换，是改变了该指针对一段内存的解释方式，一定要搞清楚要用这个指针去读取什么类型的数据！
