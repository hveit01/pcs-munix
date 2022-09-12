
#include <errno.h>

long _time()
{
	int syscall, err;
	
	syscall = 0x0d;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}

long time(tloc)
long *tloc;
{
	long t = _time();
	if (tloc)
		*tloc = t;
	return t;
}