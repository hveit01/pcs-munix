/*PCS*/
#include <sys/types.h>
#define INKERNEL
#include <sys/signal.h>
#include <errno.h>

typedef void (*sigfunc)();

static sigfunc sigtab[MAXSIG] = { 0, };
static char setflg[MAXSIG] = { 0, };

static void _psig(sig)
int sig;
{
	short sigsave = ((short*)&sig)[-2];
	0x48e7; 0xfffc;			/* movem d0-a5, sp@- */
	(*sigtab[sigsave])(sigsave);
	if (setflg[sigsave])
		sigrelse(sigsave);
	0x4cdf; 0x3fff;			/* movem sp@+, d0-a5 */
	0x4e5e;					/* unlk a6 */
	0x548f;					/* addql #2, sp */
	0x44df;					/* movw sp@+, cc */
	0x4e75;					/* rts */
}

static sigfunc _signal(/*varargs*/)
{
	int syscall, err;
	
	syscall = 0x30;
	0x4e4e;	/*trap*/
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
	setflg[sig] = 0;
	
	if ((int)func > (int)SIG_HOLD)
		func = (sigfunc)_psig;

	ret = _signal(sig, func);
	if ((int)ret > (int)SIG_HOLD) return (int)oldsig;
	/*implicit return ret */
}

int sighold(sig)
int sig;
{
	return (int)_signal(sig | SIGHOLD);
}

int sigrelse(sig)
int sig;
{
	return (int)_signal(sig | SIGRELSE);
}

int sigignore(sig)
int sig;
{
	return (int)_signal(sig | SIGIGNORE);
}

int sigpause(sig)
int sig;
{
	return (int)_signal(sig | SIGPAUSE);
}

sigfunc sigset(sig, func)
int sig;
sigfunc func;
{
	sigfunc oldsig, ret;
	
	if (sig < 0 || sig >= NSIG) {
		errno = EINVAL;
		return SIG_ERR;
	}
	oldsig = sigtab[sig];
	sigtab[sig] = func;
	setflg[sig] = 1;

	if ((int)func > (int)SIG_HOLD)
		func = _psig;
	ret = _signal(sig | SIGDEFER, func);
	if ((int)ret > (int)SIG_HOLD) return oldsig;
	/*implicit return ret */
}
