
#include <errno.h>

int hertz()
{
    int syscall, err;
    
    syscall = 0x42;
    0x4e4e; /*trap*/
}
