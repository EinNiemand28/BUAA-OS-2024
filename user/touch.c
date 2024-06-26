#include <lib.h>

void usage(void) {
    printf("usage: touch [file..]\n");
    exit(0);
}
int main(int argc, char **argv) {
    if (argc != 2) {
        usage();
    }

    int r = create(argv[1], FTYPE_REG);
    if (r < 0 && r != -E_FILE_EXISTS) {
        printf("touch: cannot touch \'%s\': No such file or directory\n", argv[1]);
    }
    return 0;
}