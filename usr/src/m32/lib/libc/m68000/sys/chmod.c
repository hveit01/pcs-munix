
#include <errno.h>

int chmod (path, mode)
char *path;
int mode;
{
    int syscall, err;
    
    syscall = 0x0f;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
