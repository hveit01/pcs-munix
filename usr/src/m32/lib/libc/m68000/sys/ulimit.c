
#include <errno.h>

long ulimit (cmd, newlimit)
int cmd;
long newlimit;
{
    int syscall, err;
    
    syscall = 0x41;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
