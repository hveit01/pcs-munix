
#include <errno.h>

int chroot (path)
char *path;
{
    int syscall, err;
    
    syscall = 0x3d;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
