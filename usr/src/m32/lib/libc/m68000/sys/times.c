
#include <errno.h>
#include <sys/types.h>
#include <sys/times.h>

long times (buffer)
struct tms *buffer;
{
    int syscall, err;
    
    syscall = 0x2b;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
