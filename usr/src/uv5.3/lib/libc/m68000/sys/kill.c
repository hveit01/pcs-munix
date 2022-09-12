
#include <errno.h>

int kill (pid, sig)
int pid, sig;
{
	int syscall, err;
	
	syscall = 0x25;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
