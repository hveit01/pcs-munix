
#include <errno.h>

int ptrace (request, pid, addr, data)
int request, pid;
int * addr;
long data;
{
	int syscall, err;
	
	syscall = 0x1a;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
