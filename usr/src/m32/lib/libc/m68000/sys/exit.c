
#include <errno.h>

void exit(status)
int status;
{
    int syscall, err;
    
    _cleanup();
    syscall = 1;
    0x4e4e; /*trap*/
}
