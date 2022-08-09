/* THIS IS OUTDATED AND UNUSED! */

#include "m68k.h"
#include "pcsrun.h"
#include <fcntl.h>
#include <errno.h>
#include <time.h>

static void copyin(unsigned int* buf, unsigned int addr, int n)
{
	int i;
//	debug2("Copyin: %08x:", addr);
	for (i = 0; i<n; i++) {
		unsigned int val = cpu_read_long(addr+4*i);
//		debug2(" %02x", val);
		buf[i] = val;
	}
//	putchar('\n');
}

static char* copystr(char* buf, unsigned addr)
{
	int i, c;
	for (i=0; (c=cpu_read_byte(addr+i)) != 0; i++)
		buf[i] = c;
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

extern int instsum;

int t14_getpid(unsigned int pc, unsigned int argptr, int* errval)
{
	return PROC::cur->Pid() << 16 | (PROC::cur->Ppid() & 0xffff);
}

int t14_access(unsigned int pc, unsigned int argptr, int* errval)
{ /* 0=exist, 1= read, 2=write, 4=execute */
	char* path; 
	copystr(path1, args.access33.fname);
	debug("access: Path=%s, mode=%o\n", path1, args.access33.accmode);
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

int t14_signal(unsigned int pc, unsigned int argptr, int* errval)
{
	int retval;
	int num = args.signal48.num;
	debug("  signal: num=%d func=%08x\n", num, args.signal48.func);
	if (num < 0 || num >= PCSNSIGS) { /* SIGKILL, SIGSTOP */
		*errval = 22; /* EINVAL */
		return -1;
	} else {
		retval = PROC::cur->signals[num];
		PROC::cur->signals[num] = args.signal48.func;
	}
	return retval;
}

int t14_fork(unsigned int pc, unsigned int argptr, int* errval)
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

int t14_exece(unsigned int pc, unsigned int argptr, int* errval)
{
	int i, argc, envc;
	char **argv, **envp;
	
	copystr(path1, args.exece59.fname);
	debug("exece: fname=%s\n", path1);
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

int t14_wait(unsigned int pc, unsigned int argptr, int* errval)
{
	if (PROC::cur->waitptr)
		debug("Process already waiting!");
	PROC::cur->waitptr = args.wait7.pstatus;
	debug("wait: &status = 0x%08x\n", args.wait7.pstatus);
	PROC::cur->SetStat('W');
	return 0;
}

int t14_write(unsigned int pc, unsigned int argptr, int* errval)
{
	int fd = args.write4.fd;
	unsigned int buf = args.write4.buf;
	int cnt = args.write4.cnt;
	debug("  write: fd=%d buf=%08x n=%d\n", fd, buf, cnt);

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
	debug_rwbuf(buf, i);
	return *errval ? -1 : i;
}

int t14_open(unsigned int pc, unsigned int argptr, int* errval)
{
	copystr(path1, args.open5.fname);
	
	char* path = xlatpath(path1);
	int pcsflags = args.open5.flags;
//	int mode = args.open5.flags;
	int mode = 0777;
	debug("  open: path=%s, flags=0%o, mode=0%o\n", path1, pcsflags, mode);

	int flags = 0;
	if (pcsflags == 0)
		flags = _O_RDONLY;
	else {
		if (pcsflags & 00001) flags |= _O_WRONLY;
		if (pcsflags & 00002) flags |= _O_RDWR;
		if (pcsflags & 00010) flags |= _O_APPEND;
		if (pcsflags & 00400) flags |= _O_CREAT;
		if (pcsflags & 01000) flags |= _O_TRUNC;
		if (pcsflags & 02000) flags |= _O_EXCL;
	}
	
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
	case 0:
		break;
	default:
		exit_error("open: path=%s, mode=0%o, errno=%s", path1, mode, errstr(errno));
	}
	
	int extfd;
	FILEENT* fh = PROC::cur->GetEmptySlot(&extfd, errval);
	if (!fh) return -1;
	
	fh->Set(path1, intfd, mode);
	return extfd;
}

int t14_close(unsigned int pc, unsigned int argptr, int* errval)
{
	int intfd;
	int extfd = args.close6.fd;
	debug("  close: fd=%d\n", extfd);
	
	FILEENT* fh = PROC::cur->GetFile(extfd, errval);
	if (!fh) return -1;
	
	*errval = fh->Close();
	return *errval ? -1 : 0;
}

int t14_ioctl(unsigned int pc, unsigned int argptr, int* errval)
{
	int extfd = args.ioctl54.fd;
	FILEENT* fh = PROC::cur->GetFile(extfd, errval);
	if (!fh) return -1;

	/* this is for function 0x5401: isatty */
	debug("  ioctl: fd=%d, func=%x\n", args.ioctl54.fd, args.ioctl54.func);
	switch (args.ioctl54.func) {
	case 0x5401:
		if (extfd >= 3 || strncmp(fh->Name(), "(std", 4) != 0)
			break;
		*errval = 25; /* ENOTTY */
		return -1;
	default:
		exit_error("ioctl: func=%x - fix me!", args.ioctl54.func);
	}
	return 0;
}

int t14_brk(unsigned int pc, unsigned int argptr, int* errval)
{
	unsigned int size;
	MEM* mem = PROC::cur->FindMem(".bss");
	if (!mem)
		exit_error("Internal error: t14_brk");
	size = args.brk17.newbrk - (mem->base + mem->size);
	if (size < 0) {
		*errval = 12;
		return -1;
	}	
	mem->Grow(size);
	return 0;
}

int t14_read(unsigned int pc, unsigned int argptr, int* errval)
{
	int extfd = args.read3.fd;
	unsigned int buf = args.read3.buf;
	int cnt = args.read3.cnt;
	debug("  read: fd=%d buf=%08x n=%d\n", extfd, buf, cnt);
	
	FILEENT* fh = PROC::cur->GetFile(extfd, errval);
	if (!fh) return -1;

	errno = 0;
	*errval = 0;

	int i, rc, actcnt;
//	debug("Reading:\n");
	for (actcnt=i=0; i<cnt; i++,actcnt++) {
		unsigned int c;
		if (read(fh->RealFd(), &c, 1) != 1) {
			if (errno != 0) {
				*errval = 5; /* EIO */
			}
			break;
		}
		cpu_write_byte(buf + i, c);
	}
	debug_rwbuf(buf, i);
	return *errval ? -1 : i;
}

int t14_exit(unsigned int pc, unsigned int argptr, int* errval)
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

int t14_m881(unsigned int pc, unsigned int argptr, int* errval)
{
	PROC::cur->Use881();
	return 0;
}

int t14_unlink(unsigned int pc, unsigned int argptr, int* errval)
{
	/* for debug, no unlink is done, rather the file is renamed */
	copystr(path1, args.unlink10.fname);
	PROC::cur->Unlink(path1);
	return 0;
}

int t14_creat(unsigned int pc, unsigned int argptr, int* errval)
{
	copystr(path1, args.creat8.fname);
	char* path = xlatpath(path1);
//	int mode = args.creat8.mode;
	int mode = 0777;
	debug("creat: path=%s, mode=0%o\n", path1, mode);

	errno = 0;
	int intfd = open(path, _O_CREAT|_O_TRUNC|_O_WRONLY, mode);
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
	
	fh->Set(path1, intfd, mode);
	return extfd;
}

int t14_gtime(unsigned int pc, unsigned int argptr, int* errval)
{
	return time(0);
}

int t14_seek(unsigned int pc, unsigned int argptr, int* errval) {
	PROC* ctx = PROC::cur;
	int ret;
	
	int extfd = args.seek19.fd;
	int pos = args.seek19.pos;
	int flag = args.seek19.flag;
	debug("  seek: fd=%d pos=%d flag=%d\n", extfd, pos, flag);
	
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
	int nargs;
} traplist[] = {
	"nosys0", 		0,				0,
	"exit",			t14_exit,		1,
	"fork",			t14_fork,		0,
	"read",			t14_read,		3,
	"write",		t14_write,		3,
	"open",			t14_open,		3,
	"close",		t14_close,		1,
	"wait",			t14_wait,		1,
	"creat",		t14_creat,		2,
	"link",			0,				0,
	"unlink",		t14_unlink,		1,
	"uipacket",		0,				0,
	"chdir",		0,				0,
	"gtime",		t14_gtime,		0,
	"mknod",		0,				0,
	"chmod",		0,				0,
	"chown",		0,				0,
	"brk",			t14_brk,		1,
	"stat",			0,				0,
	"seek",			t14_seek,		3,
	"getpid",		t14_getpid,		0,
	"mount",		0,				0,
	"umount",		0,				0,
	"setuid",		0,				0,
	"getuid",		0,				0,
	"stime",		0,				0,
	"ptrace",		0,				0,
	"alarm",		0,				0,
	"fstat",		0,				0,
	"pause",		0,				0,
	"utime",		0,				0,
	"nosys31",		0,				0,
	"nosys32",		0,				0,
	"access",		t14_access,		2,
	"nice",			0,				0,
	"ftime",		0,				0,
	"sync",			0,				0,
	"kill",			0,				0,
	"nosys38",		0,				0,
	"setpgrp",		0,				0,
	"nosys40",		0,				0,
	"dup",			0,				0,
	"pipe",			0,				0,
	"times",		0,				0,
	"profil",		0,				0,
	"fcntl",		0,				0,
	"setgid",		0,				0,
	"getgid",		0,				0,
	"signal",		t14_signal,	2,
	"nosys49",		0,				0,
	"nosys50",		0,				0,
	"acct",			0,				0,
	"nosys52",		0,				0,
	"lock",			0,				0,
	"ioctl",		t14_ioctl,		3,
	"nosys55",		0,				0,
	"forkexec",		0,				0,
	"utssys",		0,				0,
	"swapfunc",		0,				0,
	"exece",		t14_exece,		3,
	"umask",		0,				0,
	"chroot",		0,				0,
	"fork62",		t14_fork,		0,
	"read63",		t14_read,		3,
	"write64",		t14_write,		3,
	"ulimit",		0,				0,
	"hertz",		0,				0,
	"m881used",		t14_m881,		0,
	"ubsigvec",		0,				0,
	"ubsigblock",	0,				0,
	"ubsigsetmask",	0,				0,
	"ubsigpause",	0,				0,
	"ubsigstack",	0,				0,
	"ubkillpg",		0,				0,
	"ubwait",		0,				0,
	"ubsetpgrp",	0,				0,
	"ubgetpgrp",	0,				0,
	"msgsys",		0,				0,
	"semsys",		0,				0,
	"shmsys",		0,				0,
	"nullsys",		0,				0,
	"lockf",		0,				0,
	"nosys82",		0,				0,
	"symlink",		0,				0,
	"readlink",		0,				0,
	"lstat",		0,				0,
	"uadmin",		0,				0,
	"nosys87",		0,				0,
	"nosys88",		0,				0,
	"nosys89",		0,				0,
	"nosys90",		0,				0,
	"rmdir",		0,				0,
	"mkdir",		0,				0,
	"nosys93",		0,				0,
	"nosys94",		0,				0,
	"nosys95",		0,				0,
	"nosys96",		0,				0,
	"getdents",		0,				0,
	"nap",			0,				0,
	"select",		0,				0,
	0,				0,				0
};

void cpu_trap14()
{
	unsigned int pc, fp;
	int funcno, retval, errval = 0;
	unsigned int argptr;

	PROC::cur->SaveCpuContext();
	pc = m68k_get_reg(0, M68K_REG_PC);
	fp = m68k_get_reg(0, M68K_REG_A6);
	funcno = cpu_read_long(fp - 0x20); /* errno is at fp-0x24, funcno is at fp-0x20 */
	cpu_write_long(fp - 0x24, 0); /* enforce errno location being 0 even through forks */
	errno = 0;
	argptr = fp + 8;

	if (traplist[funcno].func) {
		debug("Trap14: pc=0x%x func=%d, syscall=\"%s\" @%d\n", pc, funcno, traplist[funcno].name, instsum);
		if (traplist[funcno].nargs)
			copyin(args.buf, argptr, traplist[funcno].nargs);
		retval = (*traplist[funcno].func)(pc, argptr, &errval);
		debug("Trap14: func=%d, syscall=\"%s\" returns 0x%x, errno=%s\n", 
			funcno, traplist[funcno].name, retval, errstr(errval));
		if (funcno==1) monitor("Exit");
	} else {
		debug("Trap14: func=%d, syscall=\"%s\" @ %d - not yet implemented\n", 
			funcno, traplist[funcno].name, instsum);
		monitor("Trap");
		exit(1);
	}
	
	cpu_write_long(fp-0x24, errval);	/* set errno */
	m68k_set_reg(M68K_REG_D0, retval);
	PROC::Switch();
}
