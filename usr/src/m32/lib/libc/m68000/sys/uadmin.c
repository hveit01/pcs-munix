
#include <errno.h>
#include <sys/uadmin.h>

int uadmin(cmd, fcn)
int cmd, fcn;
{
    int syscall, err;
    
    syscall = 0x56;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
