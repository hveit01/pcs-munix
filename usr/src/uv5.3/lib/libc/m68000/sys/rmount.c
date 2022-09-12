#include <errno.h>
#include <sys/nserve.h>
#include <sys/cirmgr.h>

int rmount(rmtfs, mntpt, token, rwflag)
char *rmtfs;		/* name of service (fs) */
char *mntpt;		/* directory mount point */
struct token *token;/* identifier of remote mach */
int	rwflag;			/* readonly/read write flag */
{
	int syscall, err;
	
	syscall = 0x31;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}
