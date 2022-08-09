/*
 * PCS Munix system call
 */
#include <errno.h>

int uidoit(arg, cmd)
char *arg;
int cmd;
{
    int syscall, err;
    
    syscall = 0x0b; /*uipacket */
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}
