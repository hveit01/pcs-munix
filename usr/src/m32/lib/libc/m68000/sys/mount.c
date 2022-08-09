
#include <errno.h>

int mount (spec, dir, rwflag)
char *spec, *dir;
int rwflag;
{
    int syscall, err;
    
    syscall = 0x15;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
