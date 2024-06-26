#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()`#"

/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes ('\0'), so that the
 *   returned token is a null-terminated string.
 */

int backquote;

int _gettoken(char *s, char **p1, char **p2) {
	*p1 = 0;
	*p2 = 0;
	if (s == 0) {
		return 0;
	}

	while (strchr(WHITESPACE, *s)) {
		*s++ = 0;
	}
	if (*s == 0) {
		return 0;
	}

	if (*s == '\"') {
		*p1 = ++s;
		while (*s != '\"' && *s++);
		*s++ = 0;
		*p2 = s;
		return 'w';
	}

	if (strchr(SYMBOLS, *s)) {
		int t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		if (t == '`') {
			backquote ^= 1;
		}
		if (t == '&' && *s == '&') {
			*s++ = 0;
			*p2 = s;
			return -1;
		}
		if (t == '|' && *s == '|') {
			*s++ = 0;
			*p2 = s;
			return -2;
		}
		return t;
	}

	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s)) {
		s++;
	}
	*p2 = s;
	return 'w';
}

int gettoken(char *s, char **p1) {
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 128
#define MAXLEN 2048
char bf[MAXLEN];
int redirect;

int jobscount;
struct Job {
	int status;
	int env_id;
	char cmd[1024];
} jobs[20];

int parsecmd(char **argv, int *rightpipe) {
	int argc = 0;
	while (1) {
		char *t;
		int fd, r;
		int c = gettoken(0, &t);
		int p[2];
		// debugf("token: %s\n", t);
		switch (c) {
		case 0:
		case '#':
			return argc;
		case -1: // And
			// if ((*rightpipe = fork()) == 0) {
			// 	return argc;
			// } else {
			// 	u_int who;
			// 	r = ipc_recv(&who, 0, 0);
			// 	if (r) {
			// 		do {
			// 			c = gettoken(0, &t);
			// 		} while (c != -2 && c != '#' && c);
			// 		if (c != -2) {
			// 			return 0;
			// 		}
			// 	}
			// 	return parsecmd(argv, rightpipe);
			// }
			break;
		case -2: // Or
			// if ((*rightpipe = fork()) == 0) {
			// 	return argc;
			// } else {
			// 	u_int who;
			// 	r = ipc_recv(&who, 0, 0);
			// 	//debugf("%d %d %d\n", *rightpipe, who, r);
			// 	if (r == 0) {
			// 		do {
			// 			c = gettoken(0, &t);
			// 		} while (c != -1 && c != '#' && c);
			// 		if (c != -1) {
			// 			return 0;
			// 		}
			// 	}
			// 	return parsecmd(argv, rightpipe);
			// }
			break;
		case '`':
			if (backquote) {
				if ((r = pipe(p)) < 0) {
					debugf("failed to allocate a pipe: %d\n", r);
					exit(0);
				}
				if ((*rightpipe = fork()) == 0) {
					dup(p[1], 1);
					close(p[1]);
					close(p[0]);
					return parsecmd(argv, rightpipe);
				} else {
					//debugf("fork %08x to solve `\n", *rightpipe);
					dup(p[0], 0);
					close(p[0]);
					close(p[1]);
					// debugf("wait %08x\n", *rightpipe);
					// wait(*rightpipe);
					char *tmp = (char *) bf;
					for (int i = 0, count = 0; i < MAXLEN; i++) {
						count += read(0, bf + i, 1);
						if (count == i) break;
						syscall_yield();
					}
					//debugf("hhh2\n");
					// read(0, tmp, MAXLEN);
					// debugf("arg: %s\n", tmp);
					while (strchr("\t\r\n", *tmp)) {
						*tmp++ = 0;
					}
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
						//debugf("token: %c %s\n", c, t);
					} while (c != '`');
				}
			} else {
				// for (int i = 0; i < argc; i++) {
				// 	debugf("%s\n", argv[i]);
				// }
				return argc;
			}
			break;
		case ';':
			if ((*rightpipe = fork()) == 0) {
				return argc;
			} else {
				//debugf("fork %08x to solve ;\n", *rightpipe);
				//debugf("wait %08x\n", *rightpipe);
				wait(*rightpipe);
				if (redirect) {
					close(0);
					close(1);
					dup(opencons(), 1);
					dup(1, 0);
					redirect = 0;
				}
				return parsecmd(argv, rightpipe);
			}
			break;
		case 'w':
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				exit(0);
			}
			argv[argc++] = t;
			break;
		case '<':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: < not followed by word\n");
				exit(0);
			}
			// Open 't' for reading, dup it onto fd 0, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (1/3) */

			//user_panic("< redirection not implemented");
			if ((r = open(t, O_RDONLY)) < 0) {
				debugf("failed to open \'%s\': %d\n", t, r);
				exit(0);
			}
			fd = r;
			dup(fd, 0);
			close(fd);
			break;
		case '>':;
			int cc = gettoken(0, &t);
			int mode = O_WRONLY | O_CREAT;
			if (cc == '>') {
				mode |= O_APPEND;
				if ((cc = gettoken(0, &t)) != 'w') {
					debugf("syntax error: > not followed by word\n");
					exit(0);
				}
			} else if (cc == 'w') {
				mode |= O_TRUNC;
			} else {
				debugf("syntax error: > not followed by word\n");
				exit(0);
			}
			//debugf("%d\n", mode & O_APPEND);

			// Open 't' for writing, create it if not exist and trunc it if exist, dup
			// it onto fd 1, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (2/3) */

			//user_panic("> redirection not implemented");
			if ((r = open(t, mode)) < 0) {
				debugf("failed to open \'%s\': %d\n", t, r);
				exit(0);
			}
			fd = r;
			dup(fd, 1);
			close(fd);
			break;
		case '|':
			/*
			 * First, allocate a pipe.
			 * Then fork, set '*rightpipe' to the returned child envid or zero.
			 * The child runs the right side of the pipe:
			 * - dup the read end of the pipe onto 0
			 * - close the read end of the pipe
			 * - close the write end of the pipe
			 * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of the
			 *   command line.
			 * The parent runs the left side of the pipe:
			 * - dup the write end of the pipe onto 1
			 * - close the write end of the pipe
			 * - close the read end of the pipe
			 * - and 'return argc', to execute the left of the pipeline.
			 */
			/* Exercise 6.5: Your code here. (3/3) */

			//user_panic("| not implemented");
			if ((r = pipe(p)) < 0) {
				debugf("failed to allocate a pipe: %d\n", r);
				exit(0);
			}
			redirect = 1;
			if ((*rightpipe = fork()) == 0) {
				dup(p[0], 0);
				close(p[0]);
				close(p[1]);
				return parsecmd(argv, rightpipe);
			} else {
				dup(p[1], 1);
				close(p[1]);
				close(p[0]);
				return argc;
			}
			break;
		case '&':
			break;
		}
	}

	return argc;
}

#define HISTFILESIZE 20

#define MoveLeft(x) debugf("\033[%dD", x)
#define MoveRight(x) debugf("\033[%dC", x)
#define MoveUp(x) debugf("\033[%dA", x)
#define Movedown(x) debugf("\033[%dB", x)
char histmp[1024], buftmp[1024 * HISTFILESIZE];
int hislen, hisoffset[HISTFILESIZE + 5];

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

void saveHistory(char *buf) {
	int r, fd;
	if (hislen < HISTFILESIZE) {
		if ((fd = open("/.mosh_history", O_WRONLY | O_APPEND)) < 0) {
			user_panic("open failed");
		}
		int len = strlen(buf);
		*(buf + len) = '\n';
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

void readline(char *buf, u_int n) {
	int r, len = 0, i = 0, hisline = hislen;
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
		} else if (tmp == '\033') {
			read(0, &tmp, 1);
			if (tmp == '[') {
				read(0, &tmp, 1);
				switch (tmp) {
					case 'A':
						Movedown(1);
						if (hisline == hislen) {
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
						//MoveUp(1);
						if (hisline < hislen - 1) {
							hisline++;
							readHistory(hisline, buf);
						} else if (hisline + 1 == hislen) {
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
		} else if (tmp == '\r' || tmp == '\n') {
			buf[len] = 0;
			return;
		} else {
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

		if (len >= n) break;
	}
	debugf("line too long\n");
	while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n') {
		;
	}
	buf[0] = 0;
}

void runcmd(char *s) {
	gettoken(s, 0);

	char *argv[MAXARGS];
	int rightpipe = 0, r;
	u_int who;
	int argc = parsecmd(argv, &rightpipe);
	if (argc == 0) {
		return;
	}
	argv[argc] = 0;

	if (!strcmp(argv[0], "history") || !strcmp(argv[0], "history.b")) {
		argv[0] = "cat";
		argv[1] = ".mosh_history";
		argc = 2;
	}

	int child = spawn(argv[0], argv);
	// debugf("child: %08x\nrightpipe: %08x\n", child, rightpipe);
	close_all();
	// debugf("executed\n");
	if (child >= 0) {
		r = ipc_recv(&who, 0, 0);
	} else {
		debugf("spawn %s: %d\n", argv[0], child);
	}

	if (rightpipe) {
		wait(rightpipe);
	}
	// debugf("exit\n");
	exit(r);
}

char buf[1024];

void usage(void) {
	printf("usage: sh [-ix] [script-file]\n");
	exit(0);
}

int main(int argc, char **argv) {
	int r;
	int interactive = iscons(0);
	int echocmds = 0;
	printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	printf("::                                                         ::\n");
	printf("::                     MOS Shell 2024                      ::\n");
	printf("::                                                         ::\n");
	printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	ARGBEGIN {
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}
	ARGEND

	if (argc > 1) {
		usage();
	}
	if (argc == 1) {
		close(0);
		if ((r = open(argv[0], O_RDONLY)) < 0) {
			user_panic("open %s: %d", argv[0], r);
		}
		user_assert(r == 0);
	}
	if ((r = open("/.mosh_history", O_RDONLY)) < 0) {
		if (create("/.mosh_history", FTYPE_REG) < 0) {
			user_panic("create failed");
		}
	} else {
		close(r);
	}
	for (;;) {
		if (interactive) {
			printf("\n$ ");
		}
		readline(buf, sizeof buf);
		saveHistory(buf);
		if (buf[0] == '#') {
			continue;
		}
		if (echocmds) {
			printf("# %s\n", buf);
		}
		if ((r = fork()) < 0) {
			user_panic("fork: %d", r);
		}
		if (r == 0) {
			runcmd(buf);
			exit(0);
		} else {
			wait(r);
		}
		// debugf("hislen: %d\n", hislen);
		// for (int i = 0; i < hislen; i++) {
		// 	debugf("%2d: %d\n", i, hisoffset[i]);
		// }
	}
	return 0;
}
