
#include <errno.h>

int _getuids()
{
    int syscall, err;
    
    syscall = 0x18;
    0x4e4e; /*trap*/
}

int getuid()
{
    return _getuids() >> 16;
}

int geteuid()
{
    return (short)_getuids();
}
