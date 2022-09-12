
#include <errno.h>

int uidoit(/*varargs*/)
{
	int syscall, err;
	
	syscall = 0x0b;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
