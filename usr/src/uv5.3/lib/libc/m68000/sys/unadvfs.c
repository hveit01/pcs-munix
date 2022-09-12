
#include <errno.h>

int unadvfs (svcnm)
char *svcnm;
{
	int syscall, err;
	
	syscall = 0x20;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
