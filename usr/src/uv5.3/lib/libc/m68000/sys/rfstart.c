#include <errno.h>

rfstart()
{
	int syscall, err;
	
	syscall = 0x57;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
