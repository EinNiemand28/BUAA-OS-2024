# lab0实验报告

## 思考题

### Thinking 0.1

> >- 在前述已初始化的 ~/learnGit 目录下，创建一个名为 README.txt 的文件。执行命令 git status > Untracked.txt。
> >
> >- 在 README.txt 文件中添加任意文件内容，然后使用 add 命令，再执行命令 git status > Stage.txt。
> >
> >- 提交 README.txt，并在提交说明里写入自己的学号。
> >
> >- 执行命令 cat Untracked.txt 和 cat Stage.txt，对比两次运行的结果，体会README.txt 两次所处位置的不同。
> >
> >- 修改 README.txt 文件，再执行命令 git status > Modified.txt。
> >
> >- 执行命令 cat Modified.txt，观察其结果和第一次执行 add 命令之前的 status 是
> >
> >    否一样，并思考原因。

​	两次`status`不一样. 第一次add之前`README.txt`是未跟踪的文件; 对`README.txt`进行修改后, 显示“尚未暂存以备提交的变更: 修改:	README.txt”.

​	原因: 第一次add之前只是新建了`README.txt`, 并未将其暂存, 故提示未跟踪的文件; 修改后, 这时`README.txt`在暂存区和工作区都存在, 故提示文件被修改.

### Thinking 0.2

> >仔细看看0.10，思考一下箭头中的 add the file 、stage the file 和 commit 分别对应的是 Git 里的哪些命令呢？ 

![image-20240313163006265](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240313163006265.png)

- add the file对应命令`git add`
- stage the file对应命令`git add`
- commit对应命令`git commit`

### Thinking 0.3

> >1. 代码文件 print.c 被错误删除时，应当使用什么命令将其恢复？
> >2. 代码文件 print.c 被错误删除后，执行了 git rm print.c 命令，此时应当 使用什么命令将其恢复？
> >3. 无关文件 hello.txt 已经被添加到暂存区时，如何在不删除此文件的前提下将其移出暂存区？

1. `git checkout -- print.c`
2. `git reset HEAD print.c && git checkout -- print.c`
3. `git rm --cached hello.txt`

### Thinking 0.4

> >- 找到在 /home/22xxxxxx/learnGit 下刚刚创建的 README.txt 文件，若不存 在则新建该文件。
> >- 在文件里加入 Testing 1，git add，git commit，提交说明记为 1。
> >- 模仿上述做法，把 1 分别改为 2 和 3，再提交两次。
> >- 使用 git log 命令查看提交日志，看是否已经有三次提交，记下提交说明为 3 的哈希值a。
> >- 进行版本回退。执行命令 git reset --hard HEAD^ 后，再执行 git log，观 察其变化。
> >- 找到提交说明为 1 的哈希值，执行命令 git reset --hard \<hash\> 后，再执 行 git log，观察其变化。
> >- 现在已经回到了旧版本，为了再次回到新版本，执行 git reset --hard \<hash\> ，再执行 git log，观察其变化。

- 第一次`git log`

    ![image-20240314182910688](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240314182910688.png)

- 第二次`git log`

    ![image-20240314183027575](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240314183027575.png)

- 第三次`git log`

    ![image-20240314183114548](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240314183114548.png)

- 第四次`git log`

    ![image-20240314183256702](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240314183256702.png)

### Thinking 0.5

> >执行如下命令, 并查看结果
> >
> >- `echo first`
> >- `echo second > output.txt`
> >- `echo third > output.txt`
> >- `echo forth >> output.txt`

![image-20240314183420152](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240314183420152.png)

### Thinking 0.6

> >使用你知道的方法（包括重定向）创建下图内容的文件（文件命名为 test）， 将创建该文件的命令序列保存在 command 文件中，并将 test 文件作为批处理文件运行，将 运行结果输出至 result 文件中。给出 command 文件和 result 文件的内容，并对最后的结 果进行解释说明（可以从 test 文件的内容入手）. 具体实现的过程中思考下列问题: echo echo Shell Start 与 echo \`echo Shell Start\` 效果是否有区别; echo echo \$c>file1 与 echo \`echo \$c>file1\` 效果是否有区别. 
> >
> >![image-20240314182238602](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240314182238602.png)

![image-20240314184455200](C:\Users\pengzhengyu\AppData\Roaming\Typora\typora-user-images\image-20240314184455200.png)

​	test中命令作用为: 分别将a和b赋值为1和2, 然后将c赋值为a+b的值; 接着将c,b,a的值分别写入file1,file2,file3, 然后将这三个文件的内容追加写入file4; 最后将file4的内容写入result.

​	`echo echo Shell Start`的效果为将"echo Shell Start"作为字符串输出

​	`echo (反单引号)echo Shell Start(反单引号)`的效果为将"echo Shell Start"的输出作为输出, 即会输出字符串"Shell Start"

​	`echo echo $c>file1`的效果为将"echo $c"重定向到file1

​	`echo (反单引号)echo $c>file1(反单引号)`的效果为将"$c"重定向到file1

## 难点分析

### Makefile

#### gcc用法

```makefile
gcc -c xxx.c -o DIR/xxx.o # -o指定生成目录 -c仅编译

gcc …… -I path # 编译时指定所需头文件目录
```

#### 跨目录

```makefile
make -C DIR # 进入DIR目录执行其中的Makefile文件 
```

### sed, grep, awk等命令

​	使用灵活, 配合重定向和管道使用.

### shell脚本 

​	是一群指令的集合.

​	`$n, $#, $*, $?`等用法.

​	流程控制语句,变量的使用和函数的书写 (注意语法和空格)

## 实验体会

​	lab0的主要任务还是学习掌握使用linux终端进行各种文件操作, 这些命令和用法虽不难懂, 但却繁杂而灵活. 而指导书上的内容只是一些简单的介绍, 这就需要我们去多查阅相关资料和教程, 并在实验平台上多多动手实践.

​	在lab0的学习过程中, 虽然通过不断查阅资料完成了习题, 但对各种命令的掌握还是不够熟练, 希望能在以后的不断练习中解决这个问题.