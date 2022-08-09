
#include <errno.h>

int phys(/*varargs*/)
{
    int syscall, err;
    
    syscall = 0;
    0x4e4e; /*trap*/
}

int physalloc(/*varargs*/)
{
    int syscall, err;
    
    syscall = 0;
    0x4e4e; /*trap*/
}

int physfree(/*varargs*/)
{
    int syscall, err;
    
    syscall = 0;
    0x4e4e; /*trap*/
}
