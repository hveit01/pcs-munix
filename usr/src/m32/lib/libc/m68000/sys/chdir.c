
#include <errno.h>

int chdir (path)
char *path;
{
    int syscall, err;
    
    syscall = 0x0c;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
