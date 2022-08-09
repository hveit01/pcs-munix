
#include <errno.h>

int unlink(path)
char* path;
{
    int syscall, err;
    
    syscall = 10;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
