
#include <errno.h>

int acct (path)
char *path;
{
    int syscall, err;
    
    syscall = 0x33;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
