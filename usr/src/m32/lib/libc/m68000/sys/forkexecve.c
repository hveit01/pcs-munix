
#include <errno.h>

int forkexecve(path, argp, envp)
char *path;
char *argp[];
char *envp[];
{
    int syscall, err;
    
    syscall = 0x38;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
