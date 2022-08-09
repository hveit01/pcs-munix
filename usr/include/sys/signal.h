#define NSIG    31
/*
 * No more than 32 signals (1-32) because they are
 * stored in bits in a long word.
 */

#define	SIGHUP	1	/* hangup */
#define	SIGINT	2	/* interrupt (rubout) */
#define	SIGQUIT	3	/* quit (FS) */
#define SIGILL  4       /* illegal instruction */
#define SIGTRAP 5       /* trace or breakpoint */
#define SIGIOT  6       /* ? */
#define SIGEMT  7       /* ? */
#define SIGFPE  8       /* floating exception */
#define SIGKILL 9       /* kill, uncatchable termination */
#define SIGBUS  10      /* address error: bus timeout error */
#define SIGSEGV 11      /* address error: mmu error */
#define SIGSYS  12      /* bad system call */
#define SIGPIPE 13      /* end of pipe */
#define SIGALRM 14      /* alarm clock */
#define SIGTERM 15      /* Catchable termination */
#define SIGADDR 16      /* address error: odd address */
#define SIGZERO 17      /* zero divide */
#define SIGCHK  18      /* check error */
#define SIGOVER 19      /* arithmetic overflow */
#define SIGPRIV 20      /* privilege violation */
#define SIGUSR1 21      /* user defined signal 1 */
#define SIGUSR2 22      /* user defined signal 2 */
#define SIGCLD  23      /* death of a child (old signal) */
#define SIGPWR  24      /* power-fail restart */
/* Job control support */
#define SIGSTOP	25	/* stop process */
#define SIGTSTP 26	/* stop generated from keyboard */
#define SIGCONT 27	/* continue after stop */
#define SIGCHLD 28	/* child status has changed */
#define SIGTTIN 29	/* BG read attempted from control terminal */
#define SIGTTOU 30	/* BG write attempted from control terminal */
#define SIGTINT 31	/* input record is available at control terminal */

/*
 * Signal vector "template" used in sigvec call.
 */
struct	sigvec {
	int	(*sv_handler)();	/* signal handler */
	long	sv_mask;		/* signal mask to apply */
	int	sv_onstack;		/* if non-zero, take on signal stack */
};

/*
 * Structure used in sigstack call.
 */
struct	sigstack {
	char	*ss_sp;			/* signal stack pointer */
	int	ss_onstack;		/* current status */
};

#define	SIG_DFL	(int (*)())0
#if lint
#define	SIG_IGN	(int (*)())0
#else
#define	SIG_IGN	(int (*)())1
#endif
/* For job control */
#define	SIG_CATCH (int (*)())2
#define	SIG_HOLD  (int (*)())3
