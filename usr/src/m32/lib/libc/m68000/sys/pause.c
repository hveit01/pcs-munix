
#include <errno.h>

int pause()
{
    int syscall, err;
    
    syscall = 0x1d;
    0x4e4e; /*trap*/
}
