
#include <errno.h>

int nap(dely)
int dely;
{
	int syscall, err;
	
	syscall = 0x62;
	0x4e4e;	/*trap*/
}
