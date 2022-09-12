
#include <errno.h>

int fork()
{
	int syscall, err;
	
	syscall = 2;
	0x4e4e;	/*trap*/
	0x4280; /*clrl d0*/
	if (err) {
		errno = err;
		return -1;
	}
}
