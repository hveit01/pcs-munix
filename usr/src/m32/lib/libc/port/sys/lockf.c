#include <errno.h>
#include <fcntl.h>

int lockf (fdes, mode, length)
int fdes;
int mode;
long length;
{
    int syscall, err;
    
    syscall = 0x51;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
