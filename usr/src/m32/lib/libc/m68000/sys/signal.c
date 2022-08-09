
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/errno.h>

typedef int (*sigfunc)();

static sigfunc sigtab[NSIG];

static int _psig(sig)
int sig;
{
    short sigsave = ((short*)&sig)[-2];
    0x48e7; 0xfffc;         /* movem d0-a5, sp@- */
    (*sigtab[sigsave])(sigsave);
    0x4cdf; 0x3fff;         /* movem sp@+, d0-a5 */
    0x4e5e;                 /* unlk a6 */
    0x548f;                 /* addql #2, sp */
    0x44df;                 /* movw sp@+, cc */
    0x4e75;                 /* rts */
}

static sigfunc _signal(/*varargs*/)
{
    int syscall, err;
    
    syscall = 0x30;
    0x4e4e; /*trap*/
    if (err) {
        errno = err;
        return (sigfunc)-1;
    }
}

int signal(sig, func)
int sig;
sigfunc func;
{
    sigfunc oldsig, ret;

    if (sig < 0 || sig >= NSIG) {
        errno = EINVAL;
        return -1;
    }

    oldsig = sigtab[sig];
    sigtab[sig] = func;
    
    if (func != SIG_IGN && func != SIG_DFL)
        func = (sigfunc)_psig;

    ret = _signal(sig, func);
    if (ret == SIG_DFL || ret == SIG_IGN)
        return (int)ret;
    else 
        return (int)oldsig;
}
