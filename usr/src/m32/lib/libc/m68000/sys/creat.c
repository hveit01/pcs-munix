
#include <errno.h>

int creat (path, mode)
char *path;
int mode;
{
    int syscall, err;
    
    syscall = 8;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
