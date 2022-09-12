/*PCS*/
#include <errno.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/dirent.h>

int getdents(fildes, buf, bufsz)
int fildes;
char* buf;
unsigned bufsz;
{
	int syscall, err;
	struct direct dummy;
	
	syscall = 0x61;
	0x4e4e;	/*trap*/
	if (err)
		errno = err;
	else
		return;
	
	if (err != EINVAL)
		return -1;

	return getS51Kdents(fildes, buf, bufsz);
}

#define DSZ (int)sizeof(struct direct)

static int getS51Kdents(fildes, buf, bufsz)
int fildes;
char *buf;
unsigned bufsz;
{
	struct direct tmp[64];
	int nread, ninc, nsz;
	int retc = 0;
	int bpend;
	int pos;
	register struct dirent *bp = (struct dirent*)buf;
	register struct direct *tbp;
	
	bpend = (int)&buf[bufsz];

	pos = lseek(fildes, 0, 1);
	if (pos == -1)
		return -1;

	for (;;) {
		nread = read(fildes, tmp, 64*DSZ);
		if (nread == -1)
			return -1;
		if (nread == 0)
			return retc;
		if ((nread % DSZ) != 0) {
			errno = ENOTDIR;
			return -1;
		}
		for (tbp = tmp; nread > 0;
				tbp++, nread -= DSZ, pos += DSZ) {
			if (tbp->d_ino == 0)
				continue;
			nsz = strlen(tbp->d_name);
			if (nsz > DIRSIZ) nsz = DIRSIZ;
			ninc = (nsz + DIRSIZ) & ~3;


			if (((int)bp+ninc) > bpend) {
				lseek(fildes, pos, 0);
				return retc;
			}
			bp->d_ino = tbp->d_ino;
			bp->d_off = pos;
			memcpy(bp->d_name, tbp->d_name, nsz);
			bp->d_name[nsz] = 0;
			bp->d_reclen = ninc;
			bp = (struct dirent*)((char*)bp+ninc);
			retc += ninc;
		}
	}
}
