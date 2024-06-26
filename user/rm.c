#include <lib.h>

int rmod, fmod;
void usage(void) {
    printf("usage: remove [-rf] [file..]\n");
    exit(0);
}
int main(int argc, char **argv) {
    ARGBEGIN {
        case 'r':
            rmod = 1;
            break;
        case 'f':
            fmod = 1;
            break;
        default:
            usage();
            break;
    } ARGEND

    if (argc != 1) {
        usage();
    } else {
        int fd = open(argv[0], O_RDONLY);
        if (rmod && fmod) {
            remove(argv[0]);
        } else {
            if (fd < 0) {
                printf("rm: cannot remove \'%s\': No such file or directory\n", argv[0]);
            } else {
                struct Filefd *filefd = (struct Filefd *) INDEX2FD(fd);
                if (!rmod && filefd->f_file.f_type == FTYPE_DIR) {
                    printf("rm: cannot remove \'%s\': Is a directory\n", argv[0]);
                } else {
                    remove(argv[0]);
                }
            }
        }
    }
    return 0;
}