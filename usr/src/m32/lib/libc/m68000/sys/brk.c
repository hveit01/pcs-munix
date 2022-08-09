#include <errno.h>

extern int end;
char *_currbrk = (char*)&end;

int brk(endds)
char *endds;
{
    int syscall, err;
    char *currbrk = _currbrk;
    
    syscall = 0x11;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
    _currbrk = endds;
}
