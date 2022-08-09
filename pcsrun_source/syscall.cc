#include "m68k.h"
#include "pcsrun.h"
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#ifdef __unix__
#define BINMODE 0
#else
#define BINMODE O_BINARY
#endif

static char* copystr(char* buf, unsigned addr)
{
	int i, c;
	for (i=0; (c=cpu_read_byte(addr+i)) != 0; i++) {
		buf[i] = c;
	}
	buf[i] = 0;
	return buf;
}

char** copysvector(unsigned int argp, int* argc)
{
	int i;
	unsigned int a;
	char **argv;
	char buf[1024];
	
	for (i = 0; ; i++) { /* count args */
		a = cpu_read_long(argp + i*sizeof(unsigned int));
		if (a == 0) break;
	}
	*argc = i;
	argv = (char**)malloc((*argc+1)*sizeof(char*));
	for (i=0; i< *argc; i++) {
		a = cpu_read_long(argp + i*sizeof(unsigned int));
		copystr(buf, a);
		argv[i] = strdup(buf);
	}
	argv[i] = 0;
	return argv;
}

union {
	unsigned int buf[1];
	struct {
		unsigned int ret;
	} exit1;
	struct {
		unsigned int fd;
		unsigned int buf;
		int cnt;
	} read3;
	struct {
		unsigned int fd;
		unsigned int buf;
		int cnt;
	} write4;
	struct {
		unsigned int fname;
		unsigned int flags;
		unsigned int mode;
	} open5;
	struct {
		unsigned int fd;
	} close6;
	struct {
		unsigned int pstatus;
	} wait7;
	struct {
		unsigned int fname;
		unsigned int mode;
	} creat8;
	struct {
		unsigned int fname;
	} unlink10;
	struct {
		unsigned int newbrk;
	} brk17;
	struct {
		unsigned int fd;
		unsigned int pos;
		unsigned int flag;
	} seek19;
	struct {
		unsigned int fname;
		unsigned int accmode;
	} access33;
	struct {
		unsigned int num;
		unsigned int func;
	} signal48;
	struct {
		unsigned int fd;
		unsigned int func;
		unsigned int retval;
	} ioctl54;
	struct {
		unsigned int fname;
		unsigned int argp;
		unsigned int envp;
	} exece59;
} args;

char path1[256];
char path2[256];

extern int instsum;

int trap_getpid(unsigned int pc, unsigned int argptr, int* errval)
{
	return PROC::cur->Pid() << 16 | (PROC::cur->Ppid() & 0xffff);
}

int trap_access(unsigned int pc, unsigned int argptr, int* errval)
{ /* 0=exist, 1= read, 2=write, 4=execute */
	char* path; 
//	debug("  access: Path=%s, mode=%o\n", path1, args.access33.accmode);
	path = xlatpath(path1);
	
	access(path, args.access33.accmode);
	switch (errno) {
	case 0:
		break;
	case ENOENT:
		*errval = 2; /* ENOENT */
		return -1;
	default:
		*errval = 13; /*EACCES*/
		return -1;
	}
	return 0;
}

int trap_signal(unsigned int pc, unsigned int argptr, int* errval)
{
	int retval;
	int num = args.signal48.num;
//	debug("  signal: num=%d func=%08x\n", num, args.signal48.func);
	if (num < 0 || num >= PCSNSIGS) { /* SIGKILL, SIGSTOP */
		*errval = 22; /* EINVAL */
		return -1;
	} else {
		retval = PROC::cur->signals[num];
		PROC::cur->signals[num] = args.signal48.func;
	}
	return retval;
}

int trap_fork(unsigned int pc, unsigned int argptr, int* errval)
{ /* child returns 0, parent gets child pid */

	PROC* parent = PROC::cur;
	PROC* child = parent->Clone();

	/* we are, after cloneproc in child context */
	m68k_set_reg(M68K_REG_PC, pc);		/* ensure fork returns 0 */
	child->SaveCpuContext();
//	debug("in Child context\n");
//	trace_regs();

	/* switch to parent context */
	parent->RestoreCpuContext();
	m68k_set_reg(M68K_REG_PC, pc+2);	/* skip clear d0 in fork syscall */
	parent->SaveCpuContext();
//	debug("in Parent context\n");
//	trace_regs();

	PROC::cur = parent;
	PROC::cur->RestoreCpuContext();

	debug("  fork: parent pc=%x child pc=%x errval=%d\n", pc+2, pc, *errval);	
	
	return child->Pid();
}

int trap_exece(unsigned int pc, unsigned int argptr, int* errval)
{
	int i, argc, envc;
	char **argv, **envp;
	
	debug("  exece: fname=%s\n", path1);
	argv = copysvector(args.exece59.argp, &argc);
	for (i=0; argv[i]; i++)
		debug("  argv[%d] = %s\n", i, argv[i]);

	envp = copysvector(args.exece59.envp, &envc);
	PROC::cur->CloneEnv((const char**)envp);
	for (i=0; envp[i]; i++)
		debug("  envp[%d] = %s\n", i, envp[i]);

	PROC::cur->Load(path1, argc, argv);
	
	return 0;
}

int trap_wait(unsigned int pc, unsigned int argptr, int* errval)
{
	if (PROC::cur->waitptr)
		debug("Process already waiting!");
	PROC::cur->waitptr = args.wait7.pstatus;
//	debug("wait: &status = 0x%08x\n", args.wait7.pstatus);
	PROC::cur->SetStat('W');
	return 0;
}

int trap_write(unsigned int pc, unsigned int argptr, int* errval)
{
	int fd = args.write4.fd;
	unsigned int buf = args.write4.buf;
	int cnt = args.write4.cnt;
//	debug("  write: fd=%d buf=%08x n=%d\n", fd, buf, cnt);

	FILEENT* fh = PROC::cur->GetFile(fd, errval);
	if (!fh) return -1;
	
	errno = 0;
	*errval = 0;

	int i;
	for (i=0; i<cnt; i++) {
		unsigned int c = cpu_read_byte(buf + i);
		if (write(fh->RealFd(), &c, 1) != 1) {
			if (errno != 0) {
				debug("  Write to fd=%d returns errno=%s\n", fd, errstr(errno));
				*errval = 9; /* EBADF */
			}
			break;
		}
	}
	fh->DebugRWbuf(false, buf, i);
	return *errval ? -1 : i;
}

int trap_open(unsigned int pc, unsigned int argptr, int* errval)
{
	char* path = xlatpath(path1);
	int pcsflags = args.open5.flags;
//	int mode = args.open5.flags;
	int mode = 0777;
//	debug("  open: path=%s, flags=0%o, mode=0%o\n", path1, pcsflags, mode);

	int flags = 0;
	if (pcsflags == 0)
		flags = O_RDONLY;
	else {
		if (pcsflags & 00001) flags |= O_WRONLY;
		if (pcsflags & 00002) flags |= O_RDWR;
		if (pcsflags & 00010) flags |= O_APPEND;
		if (pcsflags & 00400) flags |= O_CREAT;
		if (pcsflags & 01000) flags |= O_TRUNC;
		if (pcsflags & 02000) flags |= O_EXCL;
	}
	flags |= BINMODE;
	
	errno = 0;
	int intfd = open(path, flags, mode);
	free(path);
	switch (errno) {
	case ENOENT:
		*errval = 2; return -1;
	case EACCES:
		*errval = 13; return -1;
	case EEXIST:
		*errval = 17; return -1;
	case ENFILE:
		*errval = 23; return -1;
	case EINVAL: 
		*errval = 22; return -1;	
	case 0:
		break;
	default:
		exit_error("open: path=%s, mode=0%o, errno=%s", path1, mode, errstr(errno));
	}
	
	int extfd;
	FILEENT* fh = PROC::cur->GetEmptySlot(&extfd, errval);
	if (!fh) return -1;
	
	fh->Set(path1, intfd, mode, false);
	return extfd;
}

int trap_umask(unsigned int pc, unsigned int argptr, int* errval)
{
	/* this is a dummy routine */
	*errval = 0;
	return 0;
}

int trap_chmod(unsigned int pc, unsigned int argptr, int* errval)
{
	/* this is a dummy routine */
	*errval = 0;
	return 0;
}


int trap_close(unsigned int pc, unsigned int argptr, int* errval)
{
	int intfd;
	int extfd = args.close6.fd;
//	debug("  close: fd=%d\n", extfd);
	
	FILEENT* fh = PROC::cur->GetFile(extfd, errval);
	if (!fh) return -1;
	
	*errval = fh->Close();
	return *errval ? -1 : 0;
}

int trap_ioctl(unsigned int pc, unsigned int argptr, int* errval)
{
	int extfd = args.ioctl54.fd;
	FILEENT* fh = PROC::cur->GetFile(extfd, errval);
	if (!fh) return -1;

	/* this is for function 0x5401: isatty */
//	debug("  ioctl: fd=%d, func=%x\n", args.ioctl54.fd, args.ioctl54.func);
	switch (args.ioctl54.func) {
	case 0x5401:
		if (fh->IsTty())
			break;
		*errval = 25; /* ENOTTY */
		return -1;
	default:
		exit_error("ioctl: func=%x - fix me!", args.ioctl54.func);
	}
	return 0;
}

int trap_brk(unsigned int pc, unsigned int argptr, int* errval)
{
	unsigned int size;
	MEM* mem = PROC::cur->FindMem(".bss");
	if (!mem)
		exit_error("Internal error: trap_brk");
	size = args.brk17.newbrk - (mem->base + mem->size);
	if (size < 0) {
		*errval = 12;
		return -1;
	}	
	mem->Grow(size);
	return 0;
}

int trap_read(unsigned int pc, unsigned int argptr, int* errval)
{
	int extfd = args.read3.fd;
	unsigned int buf = args.read3.buf;
	int cnt = args.read3.cnt;
//	debug("  read: fd=%d buf=%08x n=%d\n", extfd, buf, cnt);
	
	FILEENT* fh = PROC::cur->GetFile(extfd, errval);
	if (!fh) return -1;

	errno = 0;
	*errval = 0;

	int i, rc, actcnt;
//	debug("Reading:\n");
	for (actcnt=i=0; i<cnt; i++,actcnt++) {
		unsigned int c;
		rc = read(fh->RealFd(), &c, 1);
		if (rc == 0) break;
		else if (rc == -1) {
			*errval = 5; /* EIO */
			break;
		}
		cpu_write_byte(buf + i, c);
	}
	fh->DebugRWbuf(true, buf, i);
	if (rc == 0) return i;
	return *errval != 0 ? -1 : i;
}

int trap_exit(unsigned int pc, unsigned int argptr, int* errval)
{
	PROC *child = PROC::cur;
	int ppid = child->Ppid();
	int pid = child->Pid();
	int retval = args.exit1.ret;
	
	debug("  exit: retval=%x\n", retval);
	if (ppid == 1) {
		debug2("*** Program terminated with retval=%d\n", retval);
		exit(retval);
	}

	PROC *parent = PROC::GetProc(ppid);
	if (!parent)
		exit_error("Parent of pid=%d is dead; terminating", ppid);
	if (parent->waitptr == 0)
		exit_error("Parent of pid=%d is not waiting -> SIGCLD", ppid);
	
	if (debugflg & DEBFLG_X) monitor("Exit");
	
	child->SaveCpuContext(); /*save current context */
	child->SetStat('E');	/* mark process as exited */
	child->DeleteMem();     /* free memory resources */
	
	PROC::cur = parent; /* make this the current context to change mem value */
	parent->RestoreCpuContext(); /* restore registers */
	parent->SetStat('R'); /* mark process as runnable */
	cpu_write_long(parent->waitptr, retval); /* copy status into the defined memory cell */
	parent->waitptr = 0; /* process no longer waiting */
	
//	debug("parent: pid=%d stat=%c ctx=%x waitptr=%x\n", parent->Pid(), parent->stat, parent, parent->waitptr);
//	debug("child: pid=%d stat=%c ctx=%x waitptr=%x\n", child->Pid(), child->stat, child, child->waitptr);
	return pid; /* wait returnes pid of exited child */
}

int trap_m881(unsigned int pc, unsigned int argptr, int* errval)
{
	PROC::cur->Use881();
	return 0;
}

int trap_unlink(unsigned int pc, unsigned int argptr, int* errval)
{
	/* for debug, no unlink is done, rather the file is renamed */
//	copystr(path1, args.unlink10.fname);
	PROC::cur->Unlink(path1);
	return 0;
}

int trap_creat(unsigned int pc, unsigned int argptr, int* errval)
{
//	copystr(path1, args.creat8.fname);
	char* path = xlatpath(path1);
//	int mode = args.creat8.mode;
	int mode = 0777;
//	debug("creat: path=%s, mode=0%o\n", path1, mode);

	errno = 0;
	int intfd = open(path, O_CREAT|O_TRUNC|O_WRONLY|BINMODE, mode);
	free(path);
	switch (errno) {
	case ENOENT:
		*errval = 2; return -1;
	case EACCES:
		*errval = 13; return -1;
	case EEXIST:
		*errval = 17; return -1;
	case EINVAL:
		*errval = 22; return -1;
	case ENFILE:
		*errval = 23; return -1;
	case 0:
		break;
	default:
		exit_error("creat: path=%s, mode=0%o, errno=%s", path1, mode, errstr(errno));
	}
	
	int extfd;
	FILEENT* fh = PROC::cur->GetEmptySlot(&extfd, errval);
	if (!fh) return -1;
	
	fh->Set(path1, intfd, mode, false);
	return extfd;
}

int trap_gtime(unsigned int pc, unsigned int argptr, int* errval)
{
	return time(0);
}

int trap_seek(unsigned int pc, unsigned int argptr, int* errval) {
	PROC* ctx = PROC::cur;
	int ret;
	
	int extfd = args.seek19.fd;
	int pos = args.seek19.pos;
	int flag = args.seek19.flag;
//	debug("  seek: fd=%d pos=%d flag=%d\n", extfd, pos, flag);
	
	FILEENT* fh = PROC::cur->GetFile(extfd, errval);
	if (!fh) return -1;
	
	errno = 0;
	ret = lseek(fh->RealFd(), pos, flag);
	if (errno) {
		*errval = 22; /* EINVAL */
		return -1;
	}
	return ret;
}

struct traplist {
	const char* name;
	int (*func)(unsigned int pc, unsigned int argptr, int* errval);
	const char* sig;
	bool seterrno;
} traplist[] = {
	"nosys0", 		0,				"",			false,
	"exit",			trap_exit,		"s",		false,
	"fork",			trap_fork,		"",			true,
	"read",			trap_read,		"slu",		true,
	"write",		trap_write,		"slu",		true,
	"open",			trap_open,		"pss",		true,
	"close",		trap_close,		"s",		true,
	"wait",			trap_wait,		"l",		true,
	"creat",		trap_creat,		"ps",		true,
	"link",			0,				"ll",	false,
	"unlink",		trap_unlink,	"p",		true,
	"uipacket",		0,				"ls",	false,
	"chdir",		0,				"l",	false,
	"gtime",		trap_gtime,		"",		false,
	"mknod",		0,				"lsse",	false,
	"chmod",		trap_chmod,		"ls",	false,
	"chown",		0,				"lss",	false,
	"brk",			trap_brk,		"l",		true,
	"stat",			0,				"pll",	false,
	"seek",			trap_seek,		"sls",		true,
	"getpid",		trap_getpid,	"",			false,
	"mount",		0,				"pqs",	false,
	"umount",		0,				"p",	false,
	"setuid",		0,				"s",	false,
	"getuid",		0,				"",		false,
	"stime",		0,				"l",	false,
	"ptrace",		0,				"ssll",	false,
	"alarm",		0,				"u",	false,
	"fstat",		0,				"sll",	false,
	"pause",		0,				"",		false,
	"utime",		0,				"ll",	false,
	"nosys31",		0,				"",			false,
	"nosys32",		0,				"",			false,
	"access",		trap_access,	"ps",		true,
	"nice",			0,				"s",	false,
	"ftime",		0,				"l",	false,
	"sync",			0,				"",		false,
	"kill",			0,				"ss",		true,
	"nosys38",		0,				"",			false,
	"setpgrp",		0,				"s",	false,
	"nosys40",		0,				"",			false,
	"dup",			0,				"ss",	false,
	"pipe",			0,				"l",	false,
	"times",		0,				"l",	false,
	"profil",		0,				"llls",	false,
	"fcntl",		0,				"ss?",	false,
	"setgid",		0,				"s",	false,
	"getgid",		0,				"",		false,
	"signal",		trap_signal,	"sl",		true,
	"nosys49",		0,				"",			false,
	"nosys50",		0,				"",			false,
	"acct",			0,				"l",	false,
	"nosys52",		0,				"",			false,
	"lock",			0,				"s",	false,
	"ioctl",		trap_ioctl,		"ss?",		true,
	"nosys55",		0,				"",			false,
	"forkexec",		0,				"lll",	false,
	"utssys",		0,				"ssl",	false,
	"swapfunc",		0,				"l",	false,
	"exece",		trap_exece,		"pll",		true,
	"umask",		trap_umask,		"s",	false,
	"chroot",		0,				"p",	false,
	"fork62",		trap_fork,		"",		false,
	"lread",		trap_read,		"lll",		true,
	"lwrite",		trap_write,		"lll",		true,
	"ulimit",		0,				"sl",	false,
	"hertz",		0,				"",		false,
	"m881used",		trap_m881,		"",			false,
	"ubsigvec",		0,				"sll",	false,
	"ubsigblock",	0,				"l",	false,
	"ubsigsetmask",	0,				"l",	false,
	"ubsigpause",	0,				"l",	false,
	"ubsigstack",	0,				"ll",	false,
	"ubkillpg",		0,				"ss",	false,
	"ubwait",		0,				"ls",	false,
	"ubsetpgrp",	0,				"ss",	false,
	"ubgetpgrp",	0,				"s",	false,
	"msgsys",		0,				"s?",	false,
	"semsys",		0,				"s?",	false,
	"shmsys",		0,				"s?",	false,
	"nullsys",		0,				"",			false,
	"lockf",		0,				"ssl",	false,
	"nosys82",		0,				"",			false,
	"symlink",		0,				"pq",	false,
	"readlink",		0,				"pls",	false,
	"lstat",		0,				"pll",	false,
	"uadmin",		0,				"ss",	false,
	"nosys87",		0,				"",			false,
	"nosys88",		0,				"",			false,
	"nosys89",		0,				"",			false,
	"nosys90",		0,				"",			false,
	"rmdir",		0,				"l",	false,
	"mkdir",		0,				"ls",	false,
	"nosys93",		0,				"",			false,
	"nosys94",		0,				"",			false,
	"nosys95",		0,				"",			false,
	"nosys96",		0,				"",			false,
	"getdents",		0,				"slu",	false,
	"nap",			0,				"s",	false,
	"select",		0,				"sllll",false,
	0,				0,				"",			false
};

unsigned int copyin32(unsigned int *buf, int funcno, unsigned int argptr)
{
	const char* sig = traplist[funcno].sig;
	int len = strlen(sig);
	int i;
	unsigned int s;
	for (int i=0; i<len; i++) {
		switch (sig[i]) {
		case 'p':
			buf[i] = cpu_read_long(argptr);
			copystr(path1, buf[i]);
			argptr += 4;
			break;
		case 'q':
			buf[i] = cpu_read_long(argptr);
			copystr(path2, buf[i]);
			argptr += 4;
			break;
		case 'u':
		case 's':
		case 'l':
			buf[i] = cpu_read_long(argptr);
			argptr += 4;
			break;
		case '?':
			return argptr;
		default:
			exit_error("Error in syscall sig: '%c' - fix me!", sig[i]);
		}
	}
	return argptr;
}

unsigned int copyin16(unsigned int *buf, int funcno, unsigned int argptr)
{
	const char* sig = traplist[funcno].sig;
	int len = strlen(sig);
	int i;
	unsigned int s;
	for (int i=0; i<len; i++) {
		switch (sig[i]) {
		case 'p':
			buf[i] = cpu_read_long(argptr);
			copystr(path1, buf[i]);
			argptr += 4;
			break;
		case 'q':
			buf[i] = cpu_read_long(argptr);
			copystr(path2, buf[i]);
			argptr += 4;
			break;
		case 'l':
			buf[i] = cpu_read_long(argptr);
			argptr += 4;
			break;
		case 's':
			s = cpu_read_word(argptr);
			if (s & 0x8000) s |= 0xffff0000;
			buf[i] = s;
			argptr += 2;
			break;
		case 'u':
			buf[i] = cpu_read_word(argptr);
			argptr += 2;
			break;
		case '?':
			return argptr;
		default:
			exit_error("Error in syscall sig: '%c' - fix me!", sig[i]);
		}
	}
	return argptr;
}

char* debug_printsyscall(int funcno, unsigned int callpc)
{
    static char debug_scbuf[2048];
	struct traplist *t = &traplist[funcno];
	const char* sig = t->sig;
	int len = strlen(sig);
	
	sprintf(debug_scbuf, "SYSCALL from caller=%x at step=%d: %s(", callpc, instsum, t->name);
	for (int i=0; i<len; i++) {
		if (i>0) strcat(debug_scbuf, ", ");
		char* p = debug_scbuf + strlen(debug_scbuf);
		switch(sig[i]) {
		case 'p':
			copystr(path1, args.buf[i]);
			sprintf(p, "\"%s\"", path1);
			break;
		case 'q':
			copystr(path2, args.buf[i]);
			sprintf(p, "\"%s\"", path2);
			break;
		case 's':
		case 'l':
		case 'u':
			sprintf(p, "0x%x", args.buf[i]);
			break;
		case '?':
		case 'e':
			sprintf(p, "...");
			break;
		default:
			break;
		}
	}
	strcat(debug_scbuf, ")");
	
	if (debugflg & DEBFLG_S)
		monitor(t->name);
	else if ((debugflg & DEBFLG_O) && (funcno==5 || funcno==6))
		monitor(t->name);
	
	return debug_scbuf;
}

void seterrno16(int funcno, unsigned int addr, int errval)
{
	if (traplist[funcno].seterrno)
		cpu_write_word(addr, errval);
	errno = 0;
}

void cpu_trap13()
{
	PROC::cur->SaveCpuContext();
	unsigned int pc = m68k_get_reg(0, M68K_REG_PC);
	unsigned int fp = m68k_get_reg(0, M68K_REG_A6);
	int funcno = cpu_read_word(fp - 0x1c); /* errno is at fp-0x1e, funcno is at fp-0x1c */
	unsigned int argptr = fp + 8;
	
	if (traplist[funcno].func) {
		int errval = 0;
		seterrno16(funcno, fp - 0x1e, errval); /* enforce errno location being 0 even through forks */
		argptr = copyin16(args.buf, funcno, argptr);
		unsigned int callpc = cpu_read_long(fp+4);
		char* scp = debug_printsyscall(funcno, callpc);
		int retval = (*traplist[funcno].func)(pc, argptr, &errval);
		debug2(retval==-1 ? "%s = %d" : "%s = 0x%x", scp, retval);
		if (errval) debug2(", errno=%s", errstr(errval));
		debug2("\n");
		seterrno16(funcno, fp - 0x1e, errval);
		m68k_set_reg(M68K_REG_D0, retval);
		PROC::Switch();
	} else {
		debug("Trap13: func=%d, syscall=\"%s\" @ %d - not yet implemented\n", funcno, traplist[funcno].name, instsum);
		monitor("Trap13");
		exit(1);
	}
}

void seterrno32(int funcno, unsigned int addr, int errval)
{
	if (traplist[funcno].seterrno)
		cpu_write_long(addr, errval);
	errno = 0;
}

void cpu_trap14()
{
	PROC::cur->SaveCpuContext();
	unsigned int pc = m68k_get_reg(0, M68K_REG_PC);
	unsigned int fp = m68k_get_reg(0, M68K_REG_A6);

	int funcno = cpu_read_long(fp - 0x20); /* errno is at fp-0x24, funcno is at fp-0x20 */
	unsigned int argptr = fp + 8;

	if (traplist[funcno].func) {
		int errval = 0;
		seterrno32(funcno, fp - 0x24, errval); /* enforce errno location being 0 even through forks */
		argptr = copyin32(args.buf, funcno, argptr);
		unsigned int callpc = cpu_read_long(fp+4);
		char* scp = debug_printsyscall(funcno, callpc);
		int retval = (*traplist[funcno].func)(pc, argptr, &errval);
		debug(retval==-1 ? "%s = %d" : "%s = 0x%x", scp, retval);
		if (errval) debug2(", errno=%s", errstr(errval));
		debug2("\n");	

		seterrno32(funcno, fp - 0x24, errval); 
		m68k_set_reg(M68K_REG_D0, retval);
		PROC::Switch();
	} else {
		debug("Trap14: func=%d, syscall=\"%s\" @ %d - not yet implemented\n", funcno, traplist[funcno].name, instsum);
		monitor("Trap14");
		exit(1);
	}
}
