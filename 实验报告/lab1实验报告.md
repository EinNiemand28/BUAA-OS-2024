# lab1实验报告

## 思考题

### Thinking1.1

> >请阅读 附录中的编译链接详解，尝试分别使用实验环境中的原生 x86 工具 链（gcc、ld、readelf、objdump 等）和 MIPS 交叉编译工具链（带有 mips-linux-gnu前缀），重复其中的编译和解析过程，观察相应的结果，并解释其中向 objdump 传入的参数 的含义。

![image-20240327151506934](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240327151506934.png)

`objdump`反汇编得到内容为：

![image-20240327151610588](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240327151610588.png)

![image-20240327151634334](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240327151634334.png)

`objdump`中传入参数含义如下：

```bash
objdump [-D|--disassemble-all]    # 显示所有节的汇编程序内容
        [-S|--source]             # 混合源代码与反汇编
        [--source-comment[=text]]
```

### Thinking 1.2

> >思考下述问题：
> >
> >- 尝试使用我们编写的 readelf 程序，解析之前在 target 目录下生成的内核 ELF 文件。
> >- 也许你会发现我们编写的 readelf 程序是不能解析 readelf 文件本身的，而我们刚 才介绍的系统工具 readelf 则可以解析，这是为什么呢？（提示：尝试使用 readelf -h，并阅读 tools/readelf 目录下的 Makefile，观察 readelf 与 hello 的不同）

使用编写的`readelf`程序解析内核ELF文件，得到：

![image-20240327152550820](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240327152550820.png)

使用`readelf -h`分别解析`readelf`和`hello`程序，得到：

![image-20240327153755908](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240327153755908.png)

![image-20240327153810226](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240327153810226.png)

由此猜测我们编写的`readelf`程序不能解析64位架构程序

另外通过`readelf -h ../../target/mos`得到其系统架构为`MIPS R3000 32位`，也许也能作为例证

### Thinking 1.3

> >在理论课上我们了解到，MIPS 体系结构上电时，启动入口地址为 0xBFC00000 （其实启动入口地址是根据具体型号而定的，由硬件逻辑确定，也有可能不是这个地址，但 一定是一个确定的地址），但实验操作系统的内核入口并没有放在上电启动地址，而是按照内存布局图放置。思考为什么这样放置内核还能保证内核入口被正确跳转到？
> >
> >（提示：思考实验中启动过程的两阶段分别由谁执行。）

系统启动分为两个阶段，在stage1时，`bootloader`程序直接从ROM或FLASH上加载，它会初始化基本的硬件设备，同时为加载stage2准备RAM空间，然后将stage2的代码复制到RAM空间，并且设置堆栈，最后跳转到stage2的入口函数。

stage2时将内核镜像从存储器读到RAM中，并为内核设置启动参数，然后将CPU指令寄存器的内容设置为内核入口函数的地址，即可将保证内核入口能被正确跳转到。

启动的基本步骤如图：

![image-20240328170913579](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240328170913579.png)

## 难点分析

### 加载内核到正确位置

需要仔细阅读指导书相关部分和`mmmu.h`中的内存布局图

![image-20240328180000743](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240328180000743.png)

### ELF文件解析

首先要搞清楚ELF文件的结构，然后以此掌握文件头和节头表表项中的信息

```c
typedef struct {
    // 存放魔数以及其他信息，用于验证ELF文件的有效性
    unsigned char e_ident[EI_NIDENT]; /* Magic number and other info */
    ...
    // 程序头表所在处与此文件头的偏移
    Elf32_Off e_phoff; /* Program header table file offset */
    // 节头表所在处与此文件头的偏移
    Elf32_Off e_shoff; /* Section header table file offset */
    ...
    // 程序头表表项大小
    Elf32_Half e_phentsize; /* Program header table entry size */
    // 程序头表表项数
    Elf32_Half e_phnum; /* Program header table entry count */
    // 节头表表项大小
    Elf32_Half e_shentsize; /* Section header table entry size */
    // 节头表表项数
    Elf32_Half e_shnum; /* Section header table entry count */
    ...
} Elf32_Ehdr;
/*----------------------*/
typedef struct {
    Elf32_Word sh_name; // 节的名称
    Elf32_Word sh_type; // 节的类型
    Elf32_Word sh_flags; // 节的标志位
    Elf32_Addr sh_addr; // 节的地址(指导链接过程)
    Elf32_Off sh_offset; // 节的文件内偏移
    Elf32_Word sh_size; // 节的大小(以字节计算)
    Elf32_Word sh_link; // 节头表索引链接
    Elf32_Word sh_info; // 额外信息
    Elf32_Word sh_addralign; // 地址对齐
    Elf32_Word sh_entsize; // 此节头表表项的大小
} Elf32_Shdr;
```

### 变长参数

变长参数`va_list`的用法

通过`va_arg(va_list, type)`去访问参数的宏

限时测试的时候在对它和指针的使用上出了问题

## 实验体会

lab1的主要收获是学习了操作系统启动的基本流程、了解掌握ELF文件以及最后实现的一个`printk`函数。本次实验主要难点是要读懂指导书和思考源码的逻辑，由于源码中有很多宏语句以及文件之间依赖关系复杂，很难迅速地搞清楚系统是怎样的结构、各部分有什么作用，这一点在没有vscode远程连接跳板机时显得尤为突出。但是限时测试中不能使用vscode，所以还是需要去多读源码，做到心中有数才行。

其实读源码的时候也能看到很多有用的注释，在填补空缺和理解代码时给了我很大帮助。相比之下，指导书中的内容就没有那么直观。总之，需要我们多花时间，自主学习，查找课外资料。
