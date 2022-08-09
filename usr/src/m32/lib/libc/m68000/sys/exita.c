
#include <errno.h>

void _exit(ret)
int ret;
{
    int syscall, err;
    
    syscall = 1;
    0x4e4e; /*trap*/
}
