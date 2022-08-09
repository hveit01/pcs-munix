
#include <errno.h>

int plock (op)
int op;
{
    int syscall, err;
    
    syscall = 0x35;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
