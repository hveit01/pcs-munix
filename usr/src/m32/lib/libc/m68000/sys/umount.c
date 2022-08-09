
#include <errno.h>

int umount (spec)
char *spec;
{
    int syscall, err;
    
    syscall = 0x16;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
