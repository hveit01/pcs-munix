
#include <errno.h>

int nice (incr)
int incr;
{
	int syscall, err;
	
	syscall = 0x22;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
