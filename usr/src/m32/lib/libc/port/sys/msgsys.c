
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

static int _msgsys(op /*, varargs*/)
{
    int syscall, err;
    
    syscall = 0x4d;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return -1;
    }
}

int msgget (key, msgflg)
key_t key;
int msgflg;
{
    return _msgsys(0, key, msgflg);
}

int msgctl (msqid, cmd, buf)
int msqid, cmd;
struct msqid_ds *buf;
{
    return _msgsys(1, msqid, cmd, buf);
}

int msgrcv (msqid, msgp, msgsz, msgtyp, msgflg)
int msqid;
struct msgbuf *msgp;
int msgsz;
long msgtyp;
int msgflg;
{
    return _msgsys(2, msqid, msgp, msgsz, msgtyp, msgflg);
}

int msgsnd (msqid, msgp, msgsz, msgflg)
int msqid;
struct msgbuf *msgp;
int msgsz, msgflg;
{
    return _msgsys(3, msqid, msgp, msgsz, msgflg);
}
