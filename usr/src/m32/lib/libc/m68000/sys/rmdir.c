
#include <errno.h>

int rmdir(path)
char* path;
{
    int syscall, err;
    
    syscall = 0x5b;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
