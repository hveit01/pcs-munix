#include <errno.h>
#include <stropts.h>

int getmsg(fd, ctlptr, dataptr, flags)
int fd;
struct strbuf *ctlptr;
struct strbuf *dataptr;
int *flags;
{
	int syscall, err;
	
	syscall = 0x5e;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
