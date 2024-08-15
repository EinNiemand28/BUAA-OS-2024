# Shell挑战性任务

## 任务要求

本次挑战性任务是在 lab6 实现的 MOS Shell 基础上继续实现新的功能。

### 实现功能

具体内容参考[任务说明](https://github.com/EinNiemand28/EinNiemand28.github.io/blob/main/file/Shell%20%E6%8C%91%E6%88%98%E6%80%A7%E4%BB%BB%E5%8A%A1%20-%20%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F%E5%AE%9E%E9%AA%8C%E6%8C%87%E5%AF%BC%E4%B9%A6.pdf)。（各测试点间无依赖关系）

### 关于shell

在 MOS 中，进程是一个很重要的概念，我们需要清楚 MOS 在运行的过程中存在哪些进程、这些进程之间的关系是什么，这有助于我们完成 shell 的相关任务。

比如，当我们启动 shell 时，它的 main 函数构成了一个新的进程。这个主进程主要负责完成命令的读取（`readline`）等工作（这个进程从 mosh 被打开到结束也都是不会被 free 的），不断有新的进程被 fork 出来通过调用`runcmd`去执行命令。在`runcmd`中，则首先调用`parsecmd`解析命令的参数。

`parsecmd`通过返回`argc`表明完成对命令和参数的解析，或者递归调用`parsecmd`进行更复杂的解析工作。在这个过程中，主要通过`gettoken`和`_gettoken`对字符串命令进行拆解。

解析完成后，又会调用`spawn`函数加载对应的可执行文件、fork 新的进程并设置跳转入口，以执行对应的命令。`spawn`会返回这个进程的 id ，使父进程得以**等待**（或根据要实现的功能进程不同的处理）子进程完成可执行文件的运行（所有会结束的进程最后会调用`exit()`），然后父进程也就将结束它的使命。

以上是 mosh 运行的大致逻辑，对这些有了基本的了解后，让我们着手开始增强 mosh 的功能吧。

## 具体实现

### 不带`.b`后缀指令

只需在`spawn`加载可执行文件时进行额外判断即可，若可执行文件不存在，则添加`.b`后缀再次尝试

```c
int fd;
char cmd[1024] = {0};
if ((fd = open(prog, O_RDONLY)) < 0) {
    strcpy(cmd, prog);
    strcat(cmd, ".b\0"); // lib/string.c 中实现
    //debugf("cmd: %s\n", cmd);
    if ((fd = open(cmd, O_RDONLY)) < 0) {
        return fd;
    }
}
```

### 指令条件执行

显然，首先这需要我们修改 MOS 中对用户进程`exit`的实现，使其能够返回值。

```c
void exit(int r) {
	// After fs is ready (lab5), all our open files should be closed before dying.
#if !defined(LAB) || LAB >= 5
	close_all();
#endif
	env = &envs[ENVX(syscall_getenvid())];
	if (envs[ENVX(env->env_parent_id)].env_ipc_recving != 0) {
		//debugf("%d should send: %d\n", env->env_id, r);
		ipc_send(env->env_parent_id, r, 0, 0);
	}
	syscall_env_destroy(0);
	user_panic("unreachable code");
}
```

这里我采用了 `ipc` 来实现返回值的传递，当父进程需要返回值时，让子进程调用`ipc_send`

以条件`||`为例

```c
case -2: // Or
    if ((*rightpipe = fork()) == 0) {
        return argc; // 子进程执行左指令
    } else {
        u_int who;
        r = ipc_recv(&who, 0, 0);
        //debugf("%d %d %d\n", *rightpipe, who, r);
        if (r == 0) { // 若左为真，则跳过之后的指令，直到遇到注释'#'，或者遇到条件'&&'能让它停下来
            do {
                c = gettoken(0, &t);
            } while (c != -1 && c != '#' && c);
            if (c != -1) {
                return 0;
            }
        }
        return parsecmd(argv, rightpipe, background); // 可以继续解析
    }
    break;
```

另外由于可执行文件执行之后的返回值不能直接传给上述父进程，所以可以修改`wait`函数，让其将值先返回给子进程，子进程再将此值返回

```c
// wait.c
int wait(u_int envid) {
    // 注意我们这里再 wait 中使用了 ipc，之后在其他地方再想使用这个 wait 时需要注意 ipc 是否会产生影响
	const volatile struct Env *e;
	int r;
	u_int who;
	e = &envs[ENVX(envid)];
	// debugf("%08x waiting %08x\n", syscall_getenvid(), envid);
	while (e->env_id == envid && e->env_status != ENV_FREE) {
		r = ipc_recv(&who, 0, 0);
		syscall_yield();
	}
	return r;
}

// sh.c
void runcmd(char *s) {
    // ...
    int child = spawn(argv[0], argv);
    if (child >= 0) {
        r = wait(child);
    }
    // ...
    exit(r);
}
```

### 更多指令

>需要实现`touch`、`mkdir`、`rm`，并需要考虑给出的情形

首先我们需要实现 MOS 的创建文件的用户接口（文件系统的接口已给出），新增以下内容

```c
// fsreq.h
struct Fsreq_create {
	char req_path[MAXPATHLEN];
	u_int f_type;
};

// lib.h
int fsipc_create(const char *, int);
int create(const char *path, u_int f_type);

// serv.h
int file_create(char *path, struct File **file);

// file.c
int create(const char *path, u_int f_type) {
	return fsipc_create(path, f_type);
}

// fsipc.c
int fsipc_create(const char *path, int f_type) {
	int len = strlen(path);
	if (len == 0 || len > MAXPATHLEN) {
		return -E_BAD_PATH;
	}

	struct Fsreq_create *req = (struct Fsreq_create *) fsipcbuf;
	req->f_type = f_type;
	strcpy((char *) req->req_path, path);

	return fsipc(FSREQ_CREATE, req, 0, 0);
}

// serv.c
void *serve_table[MAX_FSREQNO] = {
    ..., [FSREQ_CREATE] = serve_create,
};

void serve_create(u_int envid, struct Fsreq_create *rq) {
	struct File *f;
	int r;

	if ((r = file_create(rq->req_path, &f)) < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}

	f->f_type = rq->f_type;
	ipc_send(envid, 0, 0, 0);
}
```

然后，对于普通的`touch`和`mkdir`命令，我们只需要在对应`*.c`文件中进行以下操作即可

```c
int r = create(argv[1], FTYPE_REG); // 或修改类型为 FTYPE_DIR 以创建文件目录
```

-------

对于`mkdir`，我们需要额外实现`-p`参数：当使用 `-p` 选项时忽略错误，若目录已存在则直接退出，若创建目录的父目录不存在则递归创建目录。

首先我们可以借助 ARG 宏来解析参数

```c
ARGBEGIN {
    // 以此类推
    case 'p':
        pmod = 1;
        break;
    default:
        usage();
        break;
} ARGEND
// 要注意解析完成后从 argv[0] 开始就是命令的执行参数
```

对于`-p`参数，我的处理办法为，对路径进行解析，忽视错误对路径中每一个子目录进行创建

```c
char *path = argv[0];
if (*path == '/') {
    path++;
}
int len = strlen(path);
*(path + len) = '/'; // 方便创建最后一个目录
for (int i = 0; i <= len; ++i) {
    if (*(path + i) == '/') {
        *(path + i) = 0;
        create(path, FTYPE_DIR);
        *(path + i) = '/';
    }
}
```

-----

对于`rm`指令，用户接口`remove`已给出，直接使用即可

不过我们需要处理不同的情况

>- `rm <file>`：若文件存在则删除 `<file>`，否则输出错误信息。
>- `rm <dir>`：输出错误信息。
>- `rm -r <dir>|<file>`：若文件或文件夹存在则删除，否则输出错误信息。
>- `rm -rf <dir>|<file>`：如果对应文件或文件夹存在则删除，否则直接退出。

因此我们需要先获取路径对应的文件描述符，然后针对不同情况进行不同的处理

```c
int fd = open(argv[0], O_RDONLY); // 只读
if (rmod && fmod) {
    remove(argv[0]);
} else {
    if (fd < 0) {
        printf("rm: cannot remove \'%s\': No such file or directory\n", argv[0]);
    } else {
        struct Filefd *filefd = (struct Filefd *) INDEX2FD(fd);
        // 获取文件描述符
        if (!rmod && filefd->f_file.f_type == FTYPE_DIR) {
            printf("rm: cannot remove \'%s\': Is a directory\n", argv[0]);
        } else {
            remove(argv[0]);
        }
    }
}
```

### 反引号

>本次任务只需考虑`echo`进行的输出且数据较弱，故忽略`echo`指令和反引号，直接执行反引号中的指令貌似也能过。不过这里还是对反引号原有的要求进行了实现。

首先我们在`SYMBOLS`里面添加反引号以完成识别

```c
#define SYMBOLS "<|>&;()`#"
```

这里我采用了如下的方式区分前后的反引号

```c
int backquote;
int _gettoken(char *s, char **p1, char **p2) {
	// ...
    if (strchr(SYMBOLS, *s)) {
		int t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		if (t == '`') {
			backquote ^= 1;
		}
		return t;
	}
    // ...
}
```

然后使用管道将反引号中指令的所有输出传给父进程，对其解析以作为使用反引号的指令的参数

```c
case '`':
    if (backquote) {
        if ((r = pipe(p)) < 0) {
            debugf("failed to allocate a pipe: %d\n", r);
            exit(0);
        }
        if ((*rightpipe = fork()) == 0) {
            // fork 子进程指令解析反引号中内容
            // 并将标准输入端与管道写端相连
            dup(p[1], 1);
            close(p[1]);
            close(p[0]);
            return parsecmd(argv, rightpipe, background);
        } else {
            // 将标准输入端与管道读端相连，使用 read 读取信息
            dup(p[0], 0);
            close(p[0]);
            close(p[1]);
            char *tmp = (char *) bf;
            for (int i = 0, count = 0; i < MAXLEN; i++) {
                count += read(0, bf + i, 1);
                if (count == i) break;
                syscall_yield();
            }
            while (strchr("\t\r\n", *tmp)) {
                *tmp++ = 0;
            }
            // 以换行符进行分割
            while (*tmp) {
                char *s1 = tmp;
                while (!strchr("\t\r\n", *tmp) && *tmp) {
                    tmp++;
                }
                *tmp++ = 0;
                argv[argc++] = s1;
                while (strchr("\t\r\n", *tmp) && *tmp) {
                    tmp++;
                }
            }
            do {
                c = gettoken(0, &t);
                // 这里父进程需要跳过直到右引号的所有内容
            } while (c != '`');
        }
    } else {
        return argc;
        // 子进程完成了对反引号中内容的解析，返回执行
    }
    break;
```

这里最开始我使用原始的`wait`但是没有成功，因此使用了类似于轮询的方式完成了对数据的读取

不过后来我需要在`wait.c`中添加新的功能，所以上面比较丑陋的轮询方式可以简化为下面的函数调用

```c
// wait.c
void check2(u_int envid) {
	const volatile struct Env *e;
	e = &envs[ENVX(envid)];
	while (e->env_id == envid && e->env_status != ENV_FREE) {
		syscall_yield();
	}
}
```

### 注释功能

这个比较简单，完成对`#`的识别后进行如下处理即可

```c
case 0:
case '#':
    return argc;
```

### 一行多指令

常规的进程 fork

需要注意的点是可能之前会有重定向，将输出指向了其他地方，这里需要将其重新指回控制台

```c
case ';':
    if ((*rightpipe = fork()) == 0) {
        return argc;
    } else {
        wait(*rightpipe);
        if (redirect) {
            close(0);
            close(1);
            dup(opencons(), 1);
            dup(1, 0);
            redirect = 0;
        }
        return parsecmd(argv, rightpipe, background);
    }
    break;
```

### 追加重定向

首先我们需要实现文件操作的`O_APPEND`模式，即追加写功能

```c
// lib.h
#define O_APPEND 0x1000

// serv.c
void serve_open(u_int envid, struct Fsreq_open *rq) {
    // ...
	o->o_mode = rq->req_omode;
	ff->f_fd.fd_omode = o->o_mode;
    
    if (ff->f_fd.fd_omode & O_APPEND) {
		ff->f_fd.fd_offset = f->f_size;
	}
    // ...
}
```

然后在`parsecmd`中进行相应的修改

```c
case '>':;
    int cc = gettoken(0, &t);
    int mode = O_WRONLY | O_CREAT; // 没有则创建
    if (cc == '>') { // 判断是否是追加重定向
        mode |= O_APPEND;
        if ((cc = gettoken(0, &t)) != 'w') {
            debugf("syntax error: > not followed by word\n");
            exit(0);
        }
    } else if (cc == 'w') {
        mode |= O_TRUNC; // 普通重定向需先清空文件内容
    } else {
        debugf("syntax error: > not followed by word\n");
        exit(0);
    }
    if ((r = open(t, mode)) < 0) {
        debugf("failed to open \'%s\': %d\n", t, r);
        exit(0);
    }
    fd = r;
    dup(fd, 1); // 标准输出端与对应文件符相连
    close(fd);
    break;
```

### 引号支持

由于不用考虑引号和反引号的嵌套处理，所以也就相对简单，只需在`_gettoken`时将引号内所用内容标记为一个`word`（`w`）

```c
int _gettoken(char *s, char **p1, char **p2) {
    // ...
    if (*s == '\"') { // 这个判断需要在其他所有情况的前面
		*p1 = ++s;
		while (*s != '\"' && *s++);
		*s++ = 0;
		*p2 = s;
		return 'w';
	}
    // ...
}
```

### 历史指令

~~重量级~~

首先本着要做就要做好的原则，我先实现了对键入命令任意位置的修改的支持（使用`←`,`→`,`Backspace`,`Del`）

查阅资料可知，我们需要判断的 ascii 为

| 键        | ascii码    |
| --------- | ---------- |
| ←         | \033[D     |
| →         | \\033[C    |
| ↑         | \033[A     |
| ↓         | \033[B     |
| Backspace | \b or 0x7f |
| Del       | ~          |

通过以下宏我们可以实现光标的移动

```c
#define MoveLeft(x) debugf("\033[%dD", x)
#define MoveRight(x) debugf("\033[%dC", x)
#define MoveUp(x) debugf("\033[%dA", x)
#define Movedown(x) debugf("\033[%dB", x)
```

然后修改`readline`的逻辑

```c
void readline(char *buf, u_int n) {
	int r, len = 0, i = 0;
	char tmp;
	while (i < n) {
		if ((r = read(0, &tmp, 1)) != 1) {
			if (r < 0) {
				debugf("read error: %d\n", r);
			}
			exit(0);
		}
		if (tmp == '\b' || tmp == 0x7f) {
		//  backspace         delete
			if (i > 0) {
				if (i == len) {
					buf[--i] = 0;
					MoveLeft(1);
					debugf(" ");
					MoveLeft(1);
				} else {
					for (int j = i - 1; j < len - 1; ++j) {
						buf[j] = buf[j + 1];
					}
					buf[len - 1] = 0;
					MoveLeft(i--);
					debugf("%s ", buf);
					MoveLeft(len - i);
				}
				len -= 1;
			}
		} else if (tmp == '~') {
			if (i < len) {
				for (int j = i; j < len; ++j) {
					buf[j] = buf[j + 1];
				}
				buf[--len] = 0;
				if (i != 0) {
					MoveLeft(i);
				}
				debugf("%s ", buf);
				MoveLeft(len - i + 1);
			}
		} else if (tmp == '\r' || tmp == '\n') {
			buf[len] = 0;
			return;
		} else if (方向键) {
         	// ...
         } else { // 普通字符
			if (i == len) {
				buf[i++] = tmp;
			} else {
				for (int j = len; j > i; j--) {
					buf[j] = buf[j - 1];
				}
				buf[i] = tmp;
				buf[len + 1] = 0;
				MoveLeft(++i);
				debugf("%s", buf);
				MoveLeft(len - i + 1);
			}
			len += 1;
		}
    }
    // ...
}
```

对于方向键的处理，逻辑如下（包括对历史指令的读取）

```c
} else if (tmp == '\033') {
    read(0, &tmp, 1);
    if (tmp == '[') {
        read(0, &tmp, 1);
        switch (tmp) {
            case 'A':
                Movedown(1);
                if (hisline == hislen) {
                    // hisline == hislen 即意味要从当前指令改为历史指令，故保存
                    strcpy(histmp, buf);
                    *(histmp + len) = 0;
                }
                if (hisline > 0) {
                    hisline--;
                    readHistory(hisline, buf);
                    deleteLine(i, len);
                    debugf("%s", buf);
                    len = strlen(buf);
                    i = len;
                }
                break;
            case 'B':
                //MoveUp(1); 下键不需要
                if (hisline < hislen - 1) {
                    hisline++;
                    readHistory(hisline, buf);
                } else if (hisline + 1 == hislen) {
                    // 回到最开始的指令
                    hisline++;
                    strcpy(buf, histmp);
                }
                deleteLine(i, len);
                debugf("%s", buf);
                len = strlen(buf);
                *(buf + len) = 0;
                i = len;
                break;
            case 'C':
                if (i < len) {
                    i += 1;
                } else {
                    MoveLeft(1);
                }
                break;
            case 'D':
                if (i > 0) {
                    i -= 1;
                } else {
                    MoveRight(1);
                }
                break;
            default:
                break;
        }
    }
}
```

其中`deleteline`的作用是**清除**整行的内容

```c
void deleteLine(int pos, int len) {
	if (pos != 0) {
		MoveLeft(pos);
	}
	for (int k = 0; k < len; ++k) {
		debugf(" ");
	}
	if (len != 0) {
		MoveLeft(len);
	}
}
```

-----

对于历史指令的保存和读取，首先，每读取到一条新指令还未执行时，将其存入`.mosh_history`（上限为 20 条）

需要注意的是如果存到了上限，需要删除最开始的一条指令，然后再写入

这里我直接将后十九条指令读出暂存，清空`.mosh_history`，然后再重新存入

```c
int hislen, hisoffset[HISTFILESIZE + 5]; // 记录每条指令的偏移
void saveHistory(char *buf) {
	int r, fd;
	if (hislen < HISTFILESIZE) { // 还未存满
		if ((fd = open("/.mosh_history", O_WRONLY | O_APPEND)) < 0) {
			user_panic("open failed");
		}
		int len = strlen(buf);
		*(buf + len) = '\n'; // 换行
		len += 1;
		*(buf + len) = 0;
		if ((r = write(fd, buf, len)) != len) {
			user_panic("write error");
		}
		hisoffset[hislen++] = len + (hislen > 0 ? hisoffset[hislen - 1] : 0);
		close(fd);
    } else {
		if ((fd = open("/.mosh_history", O_RDONLY)) < 0) {
			user_panic("open failed");
		}
		if ((r = seek(fd, hisoffset[0])) < 0) {
			user_panic("seek failed");
		}
		int len = hisoffset[HISTFILESIZE - 1] - hisoffset[0];
		if ((r = read(fd, buftmp, len)) != len) {
			user_panic("read error");
		}
		if ((fd = open("/.mosh_history", O_TRUNC | O_WRONLY)) < 0) {
			user_panic("trunc failed");
		}
		if ((r = write(fd, buftmp, len)) != len) {
			debugf("%s\n", buftmp);
			debugf("%d %d\n", len, r);
			user_panic("rewrite error");
		}
		if ((fd = open("/.mosh_history", O_WRONLY | O_APPEND)) < 0) {
			user_panic("rewrite not correctly");
		}
		len = strlen(buf);
		*(buf + len) = '\n';
		len += 1;
		*(buf + len) = 0;
		if ((r = write(fd, buf, len)) != len) {
			user_panic("write error");
		}
		int ttmp = hisoffset[0];
		for (int i = 0; i < HISTFILESIZE; i++) {
			hisoffset[i] = hisoffset[i + 1] - ttmp;
		}
		hisoffset[hislen - 1] = hisoffset[hislen - 2] + len;
		close(fd);
	}
}
```

读取指令中，先调用`seek`处理偏移

```c
void readHistory(int line, char *buf) {
	int r, fd;
	if ((fd = open("/.mosh_history", O_RDONLY)) < 0) {
		user_panic("open failed");
	}
	if (line >= hislen) {
		*buf = 0;
		return;
	}
	int offset = (line > 0 ? hisoffset[line - 1] : 0);
	int len = (line > 0 ? hisoffset[line] - hisoffset[line - 1] : hisoffset[line]);
	if ((r = seek(fd, offset)) < 0) {
		user_panic("seek failed");
	}
	if ((r = read(fd, buf, len)) != len) {
		user_panic("read failed");
	}
	close(fd);
	buf[len - 1] = 0;
}
```

对于`history`指令，直接将其变为`cat /.mosh_history`即可

```c
int runcmd(char *s) {
    // ...
    if (!strcmp(argv[0], "history") || !strcmp(argv[0], "history.b")) {
		argv[0] = "cat";
		argv[1] = ".mosh_history";
		argc = 2;
	}
    // ...
}
```

### 前后台任务管理

实现得比较匆忙和混乱，相比于历史指令，这部分更是在 bug 里找 bug

总之这里收回前文对进程之间关系描述的伏笔，需要注意的是一定要在`mosh`的**主进程**中读写 jobs 的相关信息，然后对于 ipc 的使用也要多加考虑，以防产生错误行为

不过写完这一部分，~~感觉对进程和 ipc 的理解又深了许多~~