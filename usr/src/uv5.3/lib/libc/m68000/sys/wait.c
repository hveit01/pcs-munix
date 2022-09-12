#include <errno.h>

int wait (stat_loc)
int *stat_loc;
{
	int syscall, err;
	
	syscall = 7;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
