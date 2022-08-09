
#include <errno.h>

int stime (tp)
long *tp;
{
    int syscall, err;
    
    syscall = 0x19;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
