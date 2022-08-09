
#include <errno.h>

int _getgids()
{
    int syscall, err;
    
    syscall = 0x2f;
    0x4e4e; /*trap*/
}

int getgid()
{
    return _getgids() >> 16;
}

int getegid()
{
    return (short)_getgids();
}
