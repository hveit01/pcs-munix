
#include <errno.h>

symlink (name1, name2)
char *name1, *name2;
{
    int syscall, err;
    
    syscall = 0x53;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
