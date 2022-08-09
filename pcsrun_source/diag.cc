
#include "pcsrun.h"
#include "m68k.h"

static FILE *logfd = 0;

/* Exit with an error message.  Use printf syntax. */
void exit_error(const char* fmt, ...)
{
	static int guard_val = 0;
	char buff[100];
	unsigned int pc;
	va_list args;

	if(guard_val)
		return;
	else
		guard_val = 1;

	fprintf(stderr, "\n\nFatal error:\n");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
	pc = m68k_get_reg(NULL, M68K_REG_PPC);
	m68k_disassemble(buff, pc, M68K_CPU_TYPE_68020);
	monitor("Fatal");

	exit(EXIT_FAILURE);
}

static void _debug(const char *fmt, va_list ap) 
{
	va_list ap1 = ap;
	vprintf(fmt, ap); fflush(stdout);
	if (logfd) {
		vfprintf(logfd, fmt, ap1); fflush(logfd);
	}
}

void debug(const char* fmt, ...)
{
	va_list ap;
	if (debugflg & DEBFLG_D) {
		va_start(ap, fmt);
		printf("%02d: ", PROC::cur->Pid());
		if (logfd) fprintf(logfd, "%02d: ", PROC::cur->Pid());
		_debug(fmt, ap);
	}
}

void debug2(const char* fmt, ...)
{
	va_list ap;
	if (debugflg & DEBFLG_D) {
		va_start(ap, fmt);
		_debug(fmt, ap);
	}
}

void debug_coffhdr(struct ehd* hdr)
{
	debug("f.magic        0%o\n", swapw(hdr->ef.magic));
	debug("f.nscns        0x%x\n", swapw(hdr->ef.nscns));
	debug("f.timdat       0x%x\n", swapl(hdr->ef.timdat));
	debug("f.symptr       0x%x\n", swapl(hdr->ef.symptr));
	debug("f.nsyms        0x%x\n", swapl(hdr->ef.nsyms));
	debug("f.opthdr       0x%x\n", swapw(hdr->ef.opthdr));
	debug("f.flags        0x%x\n\n", swapw(hdr->ef.flags));
	
	debug("magic        0%o\n", swapw(hdr->af.magic));
	debug("vstamp       0x%x\n", swapw(hdr->af.vstamp));
	debug("tsize        0x%x\n", swapl(hdr->af.tsize));
	debug("dsize        0x%x\n", swapl(hdr->af.dsize));
	debug("bsize        0x%x\n", swapl(hdr->af.bsize));
	debug("entry        0x%x\n", swapl(hdr->af.entry));
	debug("text_start   0x%x\n", swapl(hdr->af.ts));
	debug("data_start   0x%x\n\n", swapl(hdr->af.ds));
}

void debug_memarea(struct scnhdr* sf, const char* name, unsigned int base, unsigned int size, unsigned int pos)
{
	debug("name    %s\n", name);
	debug("paddr   0x%x\n", swapl(sf->s_paddr));
	debug("vaddr   0x%x\n", base);
	debug("size    0x%x\n", size);
	debug("scnptr  0x%x\n", pos);
	debug("relptr  0x%x\n", swapl(sf->s_relptr));
	debug("lnnoptr 0x%x\n", swapl(sf->s_lnnoptr));
	debug("nreloc  0x%x\n", swapl(sf->s_nreloc));
	debug("nlnno   0x%x\n", swapl(sf->s_nlnno));
	debug("flags   0x%x\n\n", swapl(sf->s_flags));
}

void trace_regs()
{
	unsigned int sp, pc;
	int i,k;
	char buf[256];

	pc = m68k_get_reg(0, M68K_REG_PC);
	debug("PC=%08x SR=%04x (total instrs=%d)\n", pc, m68k_get_reg(0, M68K_REG_SR), instsum);
	debug("D0=%08x D1=%08x D2=%08x D3=%08x\n",
		m68k_get_reg(0, M68K_REG_D0), m68k_get_reg(0, M68K_REG_D1),
		m68k_get_reg(0, M68K_REG_D2), m68k_get_reg(0, M68K_REG_D3));
	debug("D4=%08x D5=%08x D6=%08x D7=%08x\n",
		m68k_get_reg(0, M68K_REG_D4), m68k_get_reg(0, M68K_REG_D5),
		m68k_get_reg(0, M68K_REG_D6), m68k_get_reg(0, M68K_REG_D7));
	debug("A0=%08x A1=%08x A2=%08x A3=%08x\n",
		m68k_get_reg(0, M68K_REG_A0), m68k_get_reg(0, M68K_REG_A1),
		m68k_get_reg(0, M68K_REG_A2), m68k_get_reg(0, M68K_REG_A3));
	debug("A4=%08x A5=%08x A6=%08x A7=%08x\n",
		m68k_get_reg(0, M68K_REG_A4), m68k_get_reg(0, M68K_REG_A5),
		m68k_get_reg(0, M68K_REG_A6), m68k_get_reg(0, M68K_REG_A7));

	/* dump stack */
	if (debugflg & DEBFLG_F) {
		debug("Stack:");
		sp = m68k_get_reg(0, M68K_REG_A7);

		for (k=0, i=0; i < 128; i+= 4, k++) {
			if ((k%4) == 0) {
				debug2("\n");
				debug("%08x:", sp+i);
			}
			unsigned addr = cpu_read_long(sp+i);
			const char* sym = PROC::cur->FindName(addr);
			if (sym[0]=='$')
				debug2(" %08x", addr);
			else
				debug2(" %8s", sym);
		}

		/* show current instr at PC */
		m68k_disassemble(buf, pc, M68K_CPU_TYPE_68020);
		debug2("\n");
		const char* sym = PROC::cur->FindName(pc);
		if (sym[0]=='$')
			debug("At %08X: %s\n\n", pc, buf);
		else
			debug("At %08X %s: %s\n\n", pc, sym, buf);
	}
}

int instcnt = 1;
int instsum = 0;

#define MAXBPS 100 
struct BP {
	int pid;
	unsigned int pc;
} breakpoints[MAXBPS] = {
	0, 0,
};

static void addbp(int pid, int addr)
{
	int i;
	for (i=0; i<MAXBPS; i++) {
		if (breakpoints[i].pid == 0 && breakpoints[i].pc == 0) {
			breakpoints[i].pid = pid;
			breakpoints[i].pc = addr;
			debug2("Added breakpoint %d:%08x\n", pid, addr);
			return;
		}
	}
	debug2("Too many breakpoints\n");
};

static void delbp(int pid, int addr)
{
	int i;
	addr = -addr;
	
	for (i=0; i<MAXBPS; i++) {
		if (breakpoints[i].pid == pid && breakpoints[i].pc == addr) {
			breakpoints[i].pid = 0;
			breakpoints[i].pc = 0;
			debug2("Deleted breakpoint %d:%08x\n", pid, addr);
			return;
		}
	}
};

static void listbps()
{
	int i;
	debug2("Breakpoints:\n");
	debug2("-pid- ---addr---\n");
	for (i=0; i<MAXBPS; i++) {
		if (breakpoints[i].pid != 0 && breakpoints[i].pc != 0)
			debug2(" %3d    %08x\n", breakpoints[i].pid, breakpoints[i].pc);
	}
}

static void clearbps()
{
	int i;
	for(i=0; i<MAXBPS; i++) {
		breakpoints[i].pid = 0;
		breakpoints[i].pc = 0;
	}
}

int checkbp(unsigned int addr)
{
	int i;
	int pid = PROC::cur->pid;
	for (i=0; i<MAXBPS; i++)
		if (breakpoints[i].pid == pid && breakpoints[i].pc == addr)
			return 1;
	return 0;
}

void mon_help(bool dflg)
{
	if (dflg) {
		printf("*** Debug flags:\n"
			"    D - enable debug output (implicit)\n\n"
			"    A - trace all instrs executed\n"
			"    K - trace kernel syscalls\n"
			"    T - trace jsr calls\n\n"
			"    C - break on jsr calls\n"
			"    E - break on exec syscall\n"
			"    M - break into monitor at start (cmd line only)\n"
			"    O - break on open/close\n"
			"    S - break on any syscall\n"
			"    X - break on exit syscall\n\n"
			"    B - dump buffer on syscalls\n"
			"    F - print stack frame with regs\n"
			"    L - log to file debug.log\n"
			"    U - don't unlink files on exit\n"
			"    W - warn if read on NULL\n"
			"    Z - clear all debug flags\n\n");
		return;
	}
	
	printf("*** Monitor commands:\n"
		"h,?    - this help\n"
		"d[...] - show/set debugflags\n"
		"o pid  - enforce context switch to PID\n"
		"r      - show regs\n"
		"p      - list processes\n"
		"x,q    - exit simulator\n"
		"c      - continue after break (except when fatal error)\n"
		"s nnn  - singlestep NNN instrs\n"
		"e addr - examine 64 bytes at ADDR\n"
		"a addr - list C string at ADDR\n"
		"m      - list memory regions\n"
		"f      - list open files by current process\n"
		"bl     - list current breakpoints\n"
		"bc     - clear all breakpoints\n"
		"b addr - set breakpoint for current process at ADDR\n"
		"b-addr - clear breakpoint for current process at ADDR\n"
		"b pid:addr - set breakpoint for process PID at ADDR\n\n");
}

void monitor(const char* prompt)
{
	int i;
	char line[80], *end;
	unsigned int addr, data;
	int bpad, bppid;
	PROC *p;
	MEM *m;
	PROC *savedctx = PROC::cur;

	//	debug2("Instr-Hook: ");
	trace_regs();
	for(;;) {
		fflush(stdin);
		printf("%s> ", prompt); fflush(stdout);
		switch (getchar()) {
		default:
		case 'h':
		case 'H':
		case '?':
			mon_help(false);
			break;
		case 'd':
			gets(line);
			if (!*line) {
				printf("*** Debugflags are ");
				print_debugflg();
				mon_help(true);
			} else {
				parse_debugflg(line);
				printf("*** Debugflags set to ");
				print_debugflg();
			}
			break;
		case '\n': case '\r':
			break;
		case 'o':
			gets(line);
			if (!*line) break;
			i = strtol(line, 0, 10);
			PROC::cur->SaveCpuContext(); /* save current context */
			p = PROC::GetProc(i);
			if (p)
				p->SwitchTo();
			break;			
		case 'r':
			trace_regs();
			break;
		case 'p':
			PROC::DebugList();
			break;
			break;
		case 'x':
		case 'X':
		case 'q':
		case 'Q':
			debug2("*** Exit program\n");
			exit(0);
		case 'R':
		case 'c':
			instcnt = 10000000;
			debug2("*** Continue until break (max %d instrs)\n",instcnt);
			savedctx->SwitchTo();
			return;
		case 's':
			gets(line);
			instcnt = strtol(line, 0, 10);
			if (instcnt <= 0) instcnt = 1;
			debug2("*** Execute %d step(s)\n", instcnt);
			savedctx->SwitchTo();
			return;
		case 'e':
			gets(line);
			addr = strtol(line, 0, 16);
			for (i=0; i<64; i++) {
				if ((i%16)==0) debug2("\n0x%08x:", addr+i);
				data = cpu_read_byte(addr+i);
				debug2(" %02x",data);
			}
			debug2("\n\n");
			break;
		case 'a':
			gets(line);
			addr = strtol(line, 0, 16);
			debug2("0x%08x: \"", addr);
			for (i=0; ; i++) {
				data = cpu_read_byte(addr+i);
				if (data == 0) break;
				debug2("%c", data);
			}
			debug2("\"\n\n");
			break;
		case 'm':
			debug2("*** Memory Regions:\n");
			debug2("--name-- --base-- --size--\n");
			for (m = PROC::cur->memlist; m; m = m->next) {
				debug2("%8s %08x %6d\n", m->name, m->base, m->size);
			}
			break;
		case 'f':
			debug2("*** Open files:\n");
			debug2("-fd- -intfd- -mode- --------path---------\n");
			p = PROC::cur;
			for (i = 0; i < PCSNFILES; i++) {
				FILEENT* f = &p->files[i];
				if (f->RealFd() != -1) {
					debug2(" %2d   %2d      %04o   %s\n", i, f->RealFd(), f->Mode(), f->Name());
				}
			}
			break;
		case 'b':
			gets(line);
			if (line[0] == 'L' || line[0] == 'l')
				listbps();
			else if (line[0] == 'C' || line[0] == 'c')
				clearbps();
			else {
				if ((end=strchr(line, ':'))) {
					bppid = strtol(line, 0, 10);
					bpad = strtol(end+1, 0, 16);
					if (bppid < 0) {
						bppid = -bppid;
						bpad = -bpad;
					}
				} else {
					bppid = PROC::cur->Pid();
					bpad = strtol(line, 0, 16);
				}
				if (bpad < 0)
					delbp(bppid, bpad);
				else
					addbp(bppid, bpad);
				break;
			}
		}
	}
	instcnt = 1;
}

/* user friendly print of PCS errno */
const char *errstr(int err) 
{
	static char errbuf[256];
	
	switch (err) {
	case 0:	return "---";
	case 1: return "EPERM";
	case 2: return "ENOENT";
	case 3: return "ESRCH";
	case 4: return "EINTR";
	case 5: return "EIO";
	case 6: return "ENXIO";
	case 7: return "E2BIG";
	case 8: return "ENOEXEC";
	case 9: return "EBADF";
	case 10: return "ECHILD";
	case 11: return "EAGAIN";
	case 12: return "ENOMEM";
	case 13: return "EACCES";
	case 14: return "EFAULT";
	case 15: return "ENOTBLK";
	case 16: return "EBUSY";
	case 17: return "EEXIST";
	case 18: return "EXDEV";
	case 19: return "ENODEV";
	case 20: return "ENOTDIR";
	case 21: return "EISDIR";
	case 22: return "EINVAL";
	case 23: return "ENFILE";
	case 24: return "EMFILE";
	case 25: return "ENOTTY";
	case 26: return "ETXTBSY";
	case 27: return "EFBIG";
	case 28: return "ENOSPC";
	case 29: return "ESPIPE";
	case 30: return "EROFS";
	case 31: return "EMLINK";
	case 32: return "EPIPE";
	case 33: return "EDOM";
	case 34: return "ERANGE";
	case 35: return "ENOMSG";
	case 36: return "EIDRM";
	case 37: return "ECHRNG";
	case 38: return "EL2NSYNC";
	case 39: return "EL3HLT";
	case 40: return "EL3RST";
	case 41: return "ELNRNG";
	case 42: return "EUNATCH";
	case 43: return "ENOCSI";
	case 44: return "EL2HLT";
	case 45: return "ENOSWP";
	case 46: return "EXPATH";
	case 47: return "EXREDO";
	case 48: return "EDEADLK";
	case 49: return "ENOUARP";
	case 50: return "ENOUGW";
	case 51: return "ELOOP";
	case 99: return "EXSYMPATH";
	case 135: return "EWOULDBLOCK";
	case 136: return "EINPROGRESS";
	case 137: return "EALREADY";
	case 138: return "ENOTSOCK";
	case 139: return "EDESTADDRREQ";
	case 140: return "EMSGSIZE";
	case 141: return "EPROTOTYPE";
	case 142: return "ENOPROTOOPT";
	case 143: return "EPROTONOSUPPORT";
	case 144: return "ESOCKTNOSUPPORT";
	case 145: return "EOPNOTSUPP";
	case 146: return "EPFNOSUPPORT";
	case 147: return "EAFNOSUPPORT";
	case 148: return "EADDRINUSE";
	case 149: return "EADDRNOTAVAIL";
	case 150: return "ENETDOWN";
	case 151: return "ENETUNREACH";
	case 152: return "ENETRESET";
	case 153: return "ECONNABORTED";
	case 154: return "ECONNRESET";
	case 155: return "ENOBUFS";
	case 156: return "EISCONN";
	case 157: return "ENOTCONN";
	case 158: return "ESHUTDOWN";
	case 159: return "ETOOMANYREFS";
	case 160: return "ETIMEDOUT";
	case 161: return "ECONNREFUSED";
	case 164: return "EHOSTDOWN";
	case 165: return "EHOSTUNREACH";
	case 166: return "ENOTEMPTY";
	default:
		sprintf(errbuf, "Unknown err %d", err);
		return errbuf;
	}
}

void parse_debugflg(const char* flags) {
	debugflg = 0;
	for (int i=0; flags[i]; i++) {
		switch(tolower(flags[i])) {
		default:
		case '-': break;
		case 'a': 
			debugflg |= DEBFLG_A; break;
		case 'b': 
			debugflg |= DEBFLG_B; break;
		case 'c': 
			debugflg |= DEBFLG_C; break;
		case 'd': 
			debugflg |= DEBFLG_D; break;
		case 'e': 
			debugflg |= DEBFLG_E; break;
		case 'f': 
			debugflg |= DEBFLG_F; break;
		case 'k': 
			debugflg |= DEBFLG_K; break;
		case 'l': 
			debugflg |= DEBFLG_L; break;
		case 'm': 
			debugflg |= DEBFLG_M; break;
		case 'o': 
			debugflg |= DEBFLG_O; break;
		case 's': 
			debugflg |= DEBFLG_S; break;
		case 't': 
			debugflg |= DEBFLG_T; break;
		case 'u': 
			debugflg |= DEBFLG_U; break;
		case 'w': 
			debugflg |= DEBFLG_W; break;
		case 'x': 
			debugflg |= DEBFLG_X; break;
		}
	}
	if (debugflg & ~(DEBFLG_W|DEBGFLG_U)) debugflg |= DEBFLG_D;
	if (debugflg & DEBFLG_L) {
		if (!logfd)
			logfd = fopen("debug.log", "a+");
	} else if (logfd) {
		fclose(logfd);
		logfd = 0;
	}
}

void print_debugflg() 
{
	if (debugflg == 0) {
		printf("unset\n"); return;
	}
	if (debugflg & DEBFLG_A) putchar('A');
	if (debugflg & DEBFLG_B) putchar('B');
	if (debugflg & DEBFLG_C) putchar('C');
	if (debugflg & DEBFLG_E) putchar('E');
	if (debugflg & DEBFLG_F) putchar('F');
	if (debugflg & DEBFLG_K) putchar('K');
	if (debugflg & DEBFLG_L) putchar('L');
	if (debugflg & DEBFLG_M) putchar('M');
	if (debugflg & DEBFLG_O) putchar('O');
	if (debugflg & DEBFLG_S) putchar('S');
	if (debugflg & DEBFLG_T) putchar('T');
	if (debugflg & DEBFLG_U) putchar('U');
	if (debugflg & DEBFLG_W) putchar('W');
	if (debugflg & DEBFLG_X) putchar('X');
	putchar('\n');
}
