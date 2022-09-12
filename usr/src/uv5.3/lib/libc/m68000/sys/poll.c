#include <stropts.h>
#include <poll.h>
#include <errno.h>

int poll(fds, nfds, timeout)
struct pollfd fds[];
unsigned long nfds;
int timeout;
{
	int syscall, err;
	
	syscall = 0x60;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
