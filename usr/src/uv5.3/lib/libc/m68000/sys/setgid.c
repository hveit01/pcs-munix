
#include <errno.h>

int setgid(gid)
int gid;
{
	int syscall, err;
	
	syscall = 0x2e;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
