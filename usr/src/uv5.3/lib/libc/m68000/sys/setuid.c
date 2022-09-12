
#include <errno.h>

int setuid(uid)
int uid;
{
	int syscall, err;
	
	syscall = 0x17;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
