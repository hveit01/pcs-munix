
#include <sys/types.h>
#include <sys/ipc.h>
/*include <sys/sem.h>*/
#include <errno.h>

/*PCS: this appears to be carelessly copied from the man page.
 * object file unnecessarily defines this var */
union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
} arg;

static int _semsys(op /*, varargs*/)
{
    int syscall, err;
    
    syscall = 0x4e;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}

int semctl (semid, semnum, cmd, arg)
int semid, cmd;
int semnum;
union semun arg;
{
    return _semsys(0, semid, semnum, cmd, arg);
}

int semget (key, nsems, semflg)
key_t key;
int nsems, semflg;
{
    return _semsys(1, key, nsems, semflg);
}

int semop (semid, sops, nsops)
int semid;
struct sembuf *sops;
int nsops;
{
    return _semsys(2, semid, sops, nsops);
}
