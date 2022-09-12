
#include <errno.h>
#include <sys/types.h>
#include <sys/timeb.h>

int ftime(timeb)
struct timeb *timeb;
{
	int syscall, err;
	
	syscall = 0x23;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
