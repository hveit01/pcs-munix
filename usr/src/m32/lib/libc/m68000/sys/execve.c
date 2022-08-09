
#include <errno.h>

int execve (path, argv, envp)
char *path, *argv[], *envp[];
{
    int syscall, err;
    
    syscall = 0x3b;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
