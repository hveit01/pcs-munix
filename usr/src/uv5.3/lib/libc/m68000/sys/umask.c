
#include <errno.h>

int umask(cmask)
int cmask;
{
	int syscall, err;
	
	syscall = 0x3c;
	0x4e4e;	/*trap*/
}
