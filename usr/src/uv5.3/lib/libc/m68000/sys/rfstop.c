#include <errno.h>

rfstop()
{
	int syscall, err;
	
	syscall = 0x59;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
