#include <lib.h>

int pmod;
void usage(void) {
    printf("usage: mkdir [-p] [dir..]\n");
    exit(1);
}
int main(int argc, char **argv) {
    ARGBEGIN {
        case 'p':
            pmod = 1;
            break;
        default:
            usage();
            break;
    } ARGEND

    if (argc != 1) {
        usage();
    } else if (pmod) {
        char *path = argv[0];
        if (*path == '/') {
            path++;
        }
        int len = strlen(path);
        *(path + len) = '/';
        for (int i = 0; i <= len; ++i) {
            if (*(path + i) == '/') {
                *(path + i) = 0;
                create(path, FTYPE_DIR);
                *(path + i) = '/';
            }
        }
    } else {
        int r = create(argv[0], FTYPE_DIR);
        if (r < 0) {
            if (r == -E_FILE_EXISTS) {
                printf("mkdir: cannot create directory \'%s\': File exists\n", argv[0]);
            } else {
                printf("mkdir: cannot create directory \'%s\': No such file or directory\n", argv[0]);
            }
        }
    }

    return 0;
}