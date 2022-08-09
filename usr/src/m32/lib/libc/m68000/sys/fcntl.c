
#include <errno.h>
#include <fcntl.h>

int fcntl(fildes, cmd, arg)
{
    int syscall, err;
    
    syscall = 0x2d;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
