
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

static int _shmsys(op /*, varargs*/)
{
	int syscall, err;
	
	syscall = 0x4f;
	0x4e4e;	/*trap*/
	if (err) {
		errno = err;
		return -1;
	}
}

char *shmat (shmid, shmaddr, shmflg)
int shmid;
char *shmaddr;
int shmflg;
{
	return (char*)_shmsys(0, shmid, shmaddr, shmflg);
}

int shmctl (shmid, cmd, buf)
int shmid, cmd;
struct shmid_ds *buf;
{
	return _shmsys(1, shmid, cmd, buf);
}

int shmdt (shmaddr)
char *shmaddr;
{
	return _shmsys(2, shmaddr);
}

int shmget (key, size, shmflg)
key_t key;
long size;
int shmflg;
{
	return _shmsys(3, key, size, shmflg);
}
