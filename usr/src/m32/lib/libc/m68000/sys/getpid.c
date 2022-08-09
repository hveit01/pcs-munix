
#include <errno.h>

static int _getpid()
{
    int syscall, err;
    
    syscall = 0x14;
    0x4e4e; /*trap*/
}

int getpid()
{
    return _getpid() >> 16;
}

int getppid()
{
    return (short)_getpid();
}
