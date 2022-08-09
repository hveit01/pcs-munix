
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

int lstat (path, buf)
char *path;
struct stat *buf;
{
    int syscall, err;
    
    syscall = 0x55;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
