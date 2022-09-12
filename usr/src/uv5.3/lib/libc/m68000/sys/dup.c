#include <errno.h>

int dup(filedes, filedes2)
int filedes, filedes2;
{
	int syscall, err;
	
	syscall = 0x29;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}