
#include <sys/types.h>
#include <errno.h>

int select(nfds, readfds, writefds, exceptfds, timeout)
int nfds;
fd_set *readfds, *writefds, *exceptfds;
struct timeval *timeout;
{
	int syscall, err;
	
	syscall = 0x63;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
