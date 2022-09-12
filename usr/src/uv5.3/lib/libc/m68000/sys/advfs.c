#include <errno.h>

int advfs (fs, svcnm, rwflag, clist)
char *fs;		/* root of file system */
char *svcnm;	/* global name given to name server */
int	 rwflag;	/* readonly/read write flag	*/
char **clist;	/* client list			*/
{
	int syscall, err;
	
	syscall = 0x1f;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
