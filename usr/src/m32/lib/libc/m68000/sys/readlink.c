
#include <errno.h>

int readlink(path, buf, bufsiz)
char *path, *buf;
int bufsiz;
{
    int syscall, err;
    
    syscall = 0x54;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
