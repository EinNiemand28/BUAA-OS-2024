#include <lib.h>
#define ITER 1000
int atoi(char *s) {
    int ret = 0;
    while (*s) {
        ret = ret * (*s++ - '0');
    }
    return ret;
}
int main(int argc, char **argv) {
    int n = atoi(argv[1]);
    //debugf("%d\n", n);
    for (int t = 0; t < n * ITER; t++) {
        //debugf("%d\n", t);
        syscall_yield();
    }
    //debugf("finished\n");
    return 0;
}
