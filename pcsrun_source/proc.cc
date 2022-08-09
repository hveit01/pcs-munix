#include "pcsrun.h"
#include "m68k.h"

#include <errno.h>

/*static attributes of PROC */
PROC* PROC::cur = 0;
PROC* PROC::procs = 0;
int PROC::thepid = 10;

void PROC::CloneFiles(PROC* parent)
{
	for (int i = 0; i < PCSNFILES; i++)
		files[i].Clone(parent->files[i]);
}

void PROC::deleteenv()
{
	for (int i=0; i < envc; i++) 
		delete envp[i];
	delete envp;
	envp = 0;
	envc = 0;
}

void PROC::CloneEnv(const char** env)
{
	int i;
	if (!env) return;

	if (envc) /* delete old envp */
		deleteenv();

	for (i = 0; env[i]; i++); /* count elements of env */
	envc = i;
	
	envp = new char*[i+1];	
	for (i = 0; i < envc; i++) /* copy env strings */
		envp[i] = strdup(env[i]);
	envp[i] = 0; /* last element must be 0 */
}

void PROC::initsigs()
{
	for (int i=0; i< PCSNSIGS; i++)
		signals[i] = 0; /* SIG_DFL */
}

void PROC::initfiles()
{
	for (int i = 0; i<PCSNFILES; i++)
		files[i].Set(0, -1, 0, false);
}

void PROC::initstdio()
{
	if (ppid == 1) { /* initial process */
		files[0].Set("(stdin)",  dup(fileno(stdin)),  0, true);
		files[1].Set("(stdout)", dup(fileno(stdout)), 1, true);
		files[2].Set("(stderr)", dup(fileno(stderr)), 1, true);
	}
}

void PROC::initcpucontext()
{
	/* initialize CPU context */
	cpuctx = new char[m68k_context_size()]; 
}

void PROC::SaveCpuContext()
{
	m68k_get_context((void*)cpuctx);
}

void PROC::RestoreCpuContext()
{
	m68k_set_context((void*)cpuctx);
}

void PROC::clonemem(PROC* parent)
{
	for (MEM *m = parent->memlist; m; m = m->next)
		memlist = m->Clone(memlist);
}

PROC::PROC(int ppi, const char** env)
	: next(procs),
	  memlist(0), stat('R'), m881(0), ppid(ppi),
	  waitptr(0), envp(0), envc(0),
	  tentry(0), tstart(0), tsize(0),
	  dstart(0), dsize(0), bstart(0), bsize(0),
	  sstart(0), ssize(0)

{
	
	procs = this;	/* link into list of procs */
	cur = this; /* make current proc */
	pid = thepid++; /* create a new PID */
	strcpy(name, "(fork)"); /* mark as new born */
	names = new NMAP();	/* new name map */
	
	initsigs(); /* initialize everything */
	initfiles();
	initstdio();
	CloneEnv(env);
	initcpucontext();
	SaveCpuContext(); /* store the current emulator state */

//	debug("Create process, pid=%d, context=%08x cpu=%x\n", pid, this, cpuctx);
	debug("Create process pid=%d\n", pid);
}

PROC::~PROC()
{
	delete names;
	delete cpuctx; cpuctx = 0;
	deleteenv();
	DeleteMem();
}

PROC* PROC::Clone()
{
	procs = new PROC(pid, (const char**)envp);
	procs->CloneFiles(this);
	procs->clonemem(this);
	return procs;
}

PROC* PROC::nextrunnable()
{
	PROC* last = cur;
	PROC* p = last;
	do {
		p = p->next;
		if (p == 0) p = procs;
		if (p->IsRunnable())
			return p;
	} while (p != last);
	if (p->IsRunnable())
		return p;

	debug2("*** No runnable process - system stalled\n");
	monitor("Stalled");
	exit(1);
}

void PROC::DebugList()
{
	debug2("Processes:\n");
	debug2("-pid-- -ppid- st 881 ---ctx--- --name-------------\n"); 
	for (PROC *p = procs; p; p = p->next) {
		debug2("%4d   %4d    %c  %1d  %8x  %s\n", 
			p->pid, p->ppid, p->stat, 
			p->m881, p, p->name);
	}
}

void PROC::DeleteMem()
{
	MEM *m = memlist;
	while (m) {
		m = MEM::Delete(m);
	}
	memlist = 0;
}

PROC* PROC::GetProc(int pid)
{
	PROC* p = procs;
	while (p && !p->IsPid(pid))
		p = p->next;
	return p;
}

void PROC::SwitchTo()
{
	cur->SaveCpuContext(); /* save current CPU state */
	cur = this;
	cur->RestoreCpuContext(); /* set new process context */
//	debug2("*** Switch to pid=%d\n\n", cur->pid);
//	trace_regs();
}

void PROC::Switch()
{
	int curpid = cur->pid;
	if (procs->next == 0)
		return;	/* only one process*/
//	debug("  Switch: curproc = %08x, next = %08x\n", cur, cur->next ? cur->next : procs);
//	trace_regs();

	PROC *nextctx = nextrunnable();
	if (nextctx->IsPid(curpid)) { /* no switch */
//		debug("*** Switch: no other runnable proc\n\n");
//		debug2("\n");
		return;
	}
	
	nextctx->SwitchTo();
}

MEM* PROC::FindMem(const char* name) const
{
	for (MEM *m = memlist; m; m = m->next) {
		if (!strcmp(m->name, name))
			return m;
	}
	return 0;
}

static MEM ZEROREGION("ZERO",0, 4096, MEM_RO, 0);

MEM* PROC::FindMem(unsigned int addr, int sz, bool fail)
{
	for (MEM *mem = memlist; mem; mem = mem->next) {
		if (addr >= mem->base && (addr+sz) <= (mem->base+mem->size))
			return mem;
		else if ((mem->flags & MEM_STACK) &&
			(addr+4096) >= mem->base && (addr+4096+sz) <= (mem->base+mem->size)) {
				mem->Grow(-4096);
				return mem;
		}
	}
	
	if (fail) {
		exit_error("Fatal: Process pid=%d, no mem at addr=%x, sz=%d\n", 
			pid, addr, sz);
		monitor("Fatal");
		exit(1);
	} else {
		if (debugflg & DEBFLG_W)
			printf("Warning: read %d bytes from NULL pointer %08x\n",
				sz, addr);
		return &ZEROREGION;
	}
}									

MEM* PROC::AllocMem(const char* name, unsigned int addr, int size, int flags)
{
	MEM* m = FindMem(name);
	if (m)
		m->Replace(addr, size, flags);
	else
		m = memlist = new MEM(name, addr, size, flags, memlist);
	return m;
}

FILEENT* PROC::GetFile(int extfd, int* errval)
{
	FILEENT* fh;

	if (extfd < 0 || extfd >= PCSNFILES || 
		(fh = &files[extfd])->IsClosed()) {
		*errval = 9; /*EBADF */
		return 0;
	}
	return fh;
}

FILEENT* PROC::GetEmptySlot(int* extfd, int* errval)
{
	for (int i=0; i<PCSNFILES; i++) {
		FILEENT* fh = &files[i];
		if (fh->IsClosed()) {
			*extfd = i;
			return fh;
		}
	}
	*errval = 24; /* EMFILE */
	return 0;
}

char* PROC::copystr8(const char* s)
{
	static char buf[10];
	for (int i=0; i< 10; i++) buf[i] = 0;
	strncpy(buf, s, 8);
	return buf;	
}

void PROC::loadseg(FILE* fd, int offset, const char* name, int base, int size, int flag)
{
	MEM* seg = AllocMem(name, base, size, flag);
	if (fd) {
		fseek(fd, offset, 0);
		int k = fread(seg->mem, 1, size, fd);
		if (k != size || ferror(fd)) {
			exit_error("Failed to read %s section from disk", name);
		}
	}
}

void PROC::loadmap()
{
	char line[300], *p;
	sprintf(line, "%s.map", p = xlatpath(name));
	delete p;
	FILE *mapfd = fopen(line,"r");
	if (!mapfd) return;
	debug("  mapfile at %s\n", line);

	names->clear();

	/* skip over header */
	while (fgets(line, 300, mapfd)) {
		if (strstr(line,"Address") > 0) break;
	}
	while (fgets(line, 300, mapfd)) {
		int seg, addr;
		char sym[100];
		sscanf(line,"%04x:%08x        %s",&seg,&addr,sym);
		switch (seg) {
		case 1: seg = tstart; break;
		case 2: seg = dstart; break;
		case 3: seg = bstart; break;
		default: continue; /* ignore debugging etc segs */
		}
		names->enter(seg+addr, sym);
	}
	fclose(mapfd);
//	names->dump();
}

const char *PROC::FindName(unsigned addr)
{
	return cur->names->find(addr);
}

int PROC::Load(char* prog, int argc, char* argv[])
{
	struct ehd	hdr;
	int i, k;
	FILE* fd;
	
	for (i = 0; i< argc; i++)
		debug("  loader: argv[%d] = %s\n", i, argv[i]);
	for (i=0; envp[i]; i++)
		debug("  loader: envp[%d] = %s\n", i, envp[i]);

	strcpy(name, prog);	/* set correct name */
	char* exename = xlatpath(prog); /* translate name into filesystem name */
	debug("  loader: Path to load: %s\n", exename);

	/* load the binary */
	fd = fopen(exename, "rb");
	if (!fd)
		exit_error("Cannot load %s (fopen)\n", exename);
	delete exename;

	/* read exe header */
	if (fread(&hdr, sizeof(struct ehd), 1, fd) != 1) {
		fclose(fd);
		exit_error("Invalid file header %s\n", prog);
		return 0;
	}
	
	/* make text readonly for certain exe types */
	int ro = swapw(hdr.ef.magic) == 0410 ? MEM_RO : 0;

#if 0
	debug_coffhdr(&hdr);
#endif
/*
	 * read in first few bytes of file for segment sizes
	 * magic = 407/410/413
	 *  407 is plain executable
	 *  410 is RO text
	 *  413 is page-aligned text and data
	 *  570 Common object
	 *  575 "
	 *  set tstart to start of text portion
	 */
	int magic = swapw(hdr.ef.magic);
	debug("  executable magic=%x (0%o)\n", magic, magic);
	if (magic == 0x178 || magic == 0x17d) {
		/* allocate memory for segments */

		for (int i = 0; i<swapw(hdr.ef.nscns); i++) {
			char* name = copystr8(hdr.sf[i].s_name);
			unsigned int base = swapl(hdr.sf[i].s_vaddr);
			unsigned int size = swapl(hdr.sf[i].s_size);
			unsigned int pos = swapl(hdr.sf[i].s_scnptr);
		
			if (!strcmp(name, ".text")) {
				tstart = base;
				tsize = size;
				tentry = swapl(hdr.af.entry);
				loadseg(fd, pos, name, tstart, tsize, ro);
			} else if (!strcmp(name, ".data")) {
				dstart = base;
				dsize = size;
				loadseg(fd, pos, name, dstart, dsize, 0);
			} else if (!strcmp(name, ".bss")) {
				bstart = base;
				bsize = size;
				loadseg(0, 0, name, bstart, bsize, 0);
			}
#if 0
			debug_memarea(&hdr.sf[i], name, base, size, pos);
#endif
		}
		sstart = 0x3f7ff000;
	} else if (magic == 0411) { /* old PCS 1.X format */
			struct oexec *ohdr = (struct oexec*)&hdr;
			const char* name = ".text";
			tstart = 0x600000;
			tsize = swapl(ohdr->a_text);
			tentry = tstart;
			loadseg(fd, sizeof(struct oexec), name, tstart, tsize, 0);
			
			name = ".data";
			dstart = 0x800000;
			dsize = swapl(ohdr->a_data);
			printf("%08x %08x %d\n", ohdr->a_data, dsize, dsize);
			
			if (dsize >= 0x200000)
				exit_error("Magic=%x: large datasize 0x%x - not yet supported\n", magic, dsize);
			loadseg(fd, sizeof(struct oexec)+tsize, name, dstart, dsize, 0);

			bstart = dstart + dsize;
			bsize = swapl(ohdr->a_bss);
			loadseg(0, 0, ".bss", bstart, bsize, 0);
			
			sstart = 0xf00000 + 10240;

	} else {
		exit_error("Executable format magic=%x not yet supported", magic);
	}

	/* now that we have the segs, try to get a symbol map */
	loadmap();
	
	/* the __entry of a PROC is called with a pointer to a data structure:
	 *    struct argcvdata {
	 *		short argc;
	 *		char* argv[]; // terminated with NULL
	 *		char* envp[];
	 *    }
	 */
	
	/* allocate space for arglist (max 10K) */
	/* this will be located above USRSTACK */
	int USRSTACK = sstart - 10240;

	/*allocate stack */
	MEM* stack = AllocMem(".stack", USRSTACK, 10240, MEM_STACK);
	unsigned char* argarea = stack->GetMem();

	unsigned char* cp;
	unsigned int *ap, envcnt;
//	argv++; argc--;
	
	cp = argarea;
	*(unsigned short*)cp = swapw(argc);
	ap = (unsigned int*)(cp+sizeof(unsigned short));

	/* the already set environment in PROC */
	cp = (unsigned char*)(ap + argc + 1 + envc + 1);

	for (i = 0; i < argc; i++) {
		strcpy((char*)cp, argv[i]);
		*ap++ = swapl((cp - argarea) + USRSTACK);
		cp += strlen(argv[i])+1;
	}
	*ap++ = 0;
	debug("  argv copied: %d items at %x\n", argc, USRSTACK);
	
	unsigned char* envarea = (unsigned char*)ap;
	for (i = 0; i < envc; i++) {
		strcpy((char*)cp, envp[i]);
		*ap++ = swapl((cp - argarea) + USRSTACK);
		cp += strlen(envp[i])+1;
	}
	debug("  envp copied: %d items at %x\n", envc, USRSTACK+envarea-argarea);
	*ap++ = 0;
	
	if ( ((unsigned char*)ap - argarea) >= 10240) {
		exit_error("ARGV/ENVP Overflow\n");
	}
	
	stack->Grow(-4096);
	sstart = USRSTACK-4;

	/* set registers */
	debug("  start prog: stack=%08x pc=%08x\n", sstart, tentry);

	m68k_set_reg(M68K_REG_A7, sstart);
	m68k_set_reg(M68K_REG_PC, tentry);
	m68k_set_reg(M68K_REG_SR, 0x0700);

	SaveCpuContext(); /* set new process context */
	PROC::cur = this;

	if (debugflg & DEBFLG_E)
		monitor("Exec");

	fclose(fd);
	return 1;
}

void PROC::Unlink(const char* path)
{
	for (int i=0; i<PCSNFILES; i++) {
		FILEENT* fh = 0;
		if ((fh = &files[i])->IsNamed(path)) {
			fh->DeleteOnClose();
			return;
		}
	}
	FILEENT::Delete(path);
}

static int tindent = 0;
static void trace_jsr(unsigned pc, int sz)
{
	long addr;
	debug("%08x: ", pc);
	for (int i=0; i < tindent; i++) debug2(" ");
	if (sz == 4)
		addr = cpu_read_long(pc+2); /* absolute addr */
	else
		addr = pc + (short)cpu_read_word(pc+2) + 2;

	const char *sym = PROC::FindName(addr);
	debug2("jsr\t%s(", sym);
	
	int nargs = 0;
	switch(cpu_read_word(pc+sz+2)) {
	case 0xdefc: /*adda.w*/
		nargs = cpu_read_word(pc+sz+4) / 4; break;
	case 0x508f:
		nargs = 2; break;
	case 0x588f:
		nargs = 1; break;
	default: 
		break;
	}
	
	/* we are about to do JSR, so A7 points to the 1st arg pushed, if any */
	unsigned sp = m68k_get_reg(0, M68K_REG_A7);
	for (int i=0; i<nargs; i++) {
		if (i>0) debug2(",");
		const char *sym = PROC::FindName(cpu_read_long(sp + 4*i));
		debug2("%s",sym);
	}
	debug2(")\n");
	tindent++;
}

static void trace_rts(unsigned pc)
{
	tindent--;
//	debug("%08x: ", pc);
//	for (int i=0; i < tindent; i++) debug2(" ");
//	debug2("rts\n");
}

void cpu_instr_hook()
{
	unsigned int pc = m68k_get_reg(0, M68K_REG_PC);

	instcnt--;	/* instrs to execute before stop */
	instsum++;	/* elapsed instrs */

	/* current instruction at PC */
	int op = cpu_read_word(pc);

	/* breakpoint reached? */
	if (checkbp(pc)) {
		monitor("Break");

	/* # instrs to execute expired? */
	} else if (instcnt <= 0 && (debugflg & DEBFLG_M)) {
		monitor("Monitor");
		return;

	/* trace all instructions */
	} else if (debugflg & DEBFLG_A) {
		char buff[1024];
		m68k_disassemble(buff, pc, M68K_CPU_TYPE_68020);
		printf("%08x: %s\n", pc, buff);
		if ((instsum % 100) == 0)
			trace_regs();
	
	/* trace jsr instructions */
	} else if (debugflg & DEBFLG_T) {
		int op = cpu_read_word(pc);
		switch (op) {
		case 0x4eb9:	/*jsr*/
			trace_jsr(pc, 4); 
			break;
		case 0x4eba:	/* jsr short */
			trace_jsr(pc, 2);
			break;
		case 0x4e75:	/*rts*/
			trace_rts(pc); 
			break;
		default: 
			break;
		}
	}

	if (debugflg & DEBFLG_C) {
		switch (op) {
		case 0x4eb9:	/*jsr*/
		case 0x4eba:	/* jsr short */
			monitor("Call");
			break;
		default:
			break;
		}
	}
}
