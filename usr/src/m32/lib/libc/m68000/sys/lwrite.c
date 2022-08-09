
#include <errno.h>

long lwrite(filedes, buf, nbyte)
int filedes;
char* buf;
long nbyte;
{
    int syscall, err;
    
    syscall = 4;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
