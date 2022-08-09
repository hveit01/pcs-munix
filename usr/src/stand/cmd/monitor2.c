#include "data.h"


/* enters here from locore when some trap occurred */
trap(usp)
{
    extern except_start(), except_end(); /* marked region */
    
    int pc;
    short esr;

    register struct exvec *regs = (struct exvec*)&usp;
    regs->exa7 = ((int)regs) + getexvecsize(regs);
    esr = WORD(ESR);

    switch (regs->exno) {
    case 2: /*buserror*/
        pc = regs->exu.ex2.expc;        /* save exception PC */
        
        /* was this the result of a memory probe?
         * then return -1 as failed attempt */
        if (pc >= (int)except_start && pc < (int)except_end) {
            regs->exu.ex2.expc = (int)invalid_trap;  /* enforce calling a function returning -1 */
            regs->exu.ex2.exfmtvec = 0; /* clear format vector */
            return;
        }
        /* no wasn't, this is a real unexpected bus error */
        trap_buserror(regs, esr);
        break;

    case 9:  /*trace*/
    case 33: /*trap1*/
        setsr(0x2700);                  /* reset to IPL 7, supervisor mode, disable trace */
        trap_trace(regs);               /* handle trace elsewhere */
        break;

    case 30: /*autoint 6*/
        printf("\nhalt");               /* halt system */
        while (WORD(PCR) & 0x100);      /* wait until bit 8 becomes 0 */
        /*FALLTHRU*/

    default:
        trap_buserror(regs, esr);       /* handle all exceptions like bus error */
        break;

    case 27: /*autoint 3 */
        break;                          /* ignore it */
    }
}

/* this is an unexpected trap, not a trace, so cleanup the mess */
trap_buserror(regs, esr)
register struct exvec *regs;
int esr;
{
    static int berrcnt;                 /* buserror protector semaphore - prevent double fault */
    
    if (berrcnt++ > 2)
        printf("error in pmd\n");
    else {
        cons_detect();                  /* detect consoles */
        save_exframe(regs);             /* stores registers in minitor register frame */
        restore_brkpt();                /* restore all breakpoints */
        mon_printregs(regs, esr);       /* dump registers and ESR error code */
        mon_printproc(regs);            /* dump stacktrace */
    }
    
    berrcnt = 0;                        /* clear protector semaphore */
    longjmp(monreturn);                 /* fall back into monitor loop */
    /*NOTREACHED*/
}

/*****************************************************************************/
/* here memory probe region starts, a bus error with PC in this region will
 * fake a regular -1 result; this is returned by memory probing routines
 * when there is no memory */
except_start() {}

unsigned int mem_readbyte(addr)
char *addr;
{
    char tmp = *addr;                   /* try reading addr */
    tmp = *addr;                        /* to be sure, twice */

    /* no bus error occurred here, so return regular result */
    return tmp;
}

unsigned int mem_readword(addr)
short *addr;
{
    short tmp = *addr;                  /* try reading addr */
    0x4e71; 0x4e71; 0x4e71; 0x4e71;     /* 8 * NOP instruction for delay */
    0x4e71; 0x4e71; 0x4e71; 0x4e71;

    /* no bus error occurred here, so return regular result */
    return tmp;
}

unsigned int mem_readlong(addr)
unsigned long *addr;
{
    unsigned long tmp = *addr;          /* try reading addr */
    tmp = *addr;                        /* twice */

    /* no bus error occurred here, so return regular result */
    return tmp;
}

int mem_writebyte(addr, data)
char* addr;
int data;
{
    *addr = data;                       /* try write byte */
    
    /* no bus error occurred here, so return success */
    return 0;
}
    
int mem_writeword(addr, data)
short* addr;
int data;
{
    *addr = data;                       /* try write word */
    
    /* no bus error occurred here, so return success */
    return 0;
}

int mem_writelong(addr, data)
int* addr;
int data;
{
    *addr = data;                       /* try write long */
    
    /* no bus error occurred here, so return success */
    return 0;
}

/* end of memory probing region */
except_end() {}
/*****************************************************************************/


/* print procedure stack trace, follow the A6 frame pointer chain */
mon_printproc(regs)
struct exvec *regs;
{
    register long *framep;
    long retaddr;
    register int i, level;

    printf("\nreturn address:procedure address\n");
    framep = (long*)regs->exa6;
    
    i = 0;
    retaddr = 0;
    level = 20;                         /* maximum 20 levels deep */

    while (level > 0) {
        if (framep == 0) break;
        if (((int)framep) & 1) {        /* odd address? */
            printf("\nFrame pointer error:#%X\n", framep);
            break;
        }

        retaddr = mem_readlong(framep+1);   /* return address in frame */
        if (retaddr == 0) break;        /* end of chain? */
        if ((funcptr)retaddr == warmstart)
            break;                      /* reached minitor? */
        if ((retaddr & 1) || mem_readlong(retaddr) == -1) { /* odd address or no mem? */
            printf("\nreturn addres error:#%lx\n", retaddr);
            break;
        }

        printf("#%8lx:", retaddr);
        retaddr = mem_readlong(retaddr - sizeof(long)); /* caller */
        printf("#%8lx  ", retaddr);
        if (retaddr == -1) break;       /* was invalid */

        if (i++ == 3) {                 /* newline after 4 levels */
            putchar('\n');
            i = 0;
        }
        framep = (long*)mem_readlong(framep);   /* next frame */
        level--;
    }

    if (level)
        printf("\npmd aborted after 20 calls");
    printf("\n");
}

mon_printregs(regs, esr)
register struct exvec *regs;
int esr;
{
    char* emsg;

    printf("\nregisters:\n");
    printf("A0 #%8x  A1 #%8x  A2 #%8x  A3 #%8x\n",
        regs->exa0, regs->exa1, regs->exa2, regs->exa3);
    printf("A4 #%8x  A5 #%8x  A6 #%8x  A7 #%8x\n",
        regs->exa4, regs->exa5, regs->exa6, regs->exa7);
    printf("D0 #%8x  D1 #%8x  D2 #%8x  D3 #%8x\n",
        regs->exd0, regs->exd1, regs->exd2, regs->exd3);
    printf("D4 #%8x  D5 #%8x  D6 #%8x  D7 #%8x\n",
        regs->exd4, regs->exd5, regs->exd6, regs->exd7);
    switch (regs->exno) {
    case 2:
    case 3:
        printf("%s error, access address = #%8lx, pc = #%8lx, status reg = #%4x\nesr=%4x ",
            regs->exno==2 ? "bus" : "address", regs->exu.ex2.exaccessaddr,
            regs->exu.ex2.expc, regs->exu.ex2.exstatreg, esr);
        mon_printesr(esr);
        return;
    case 4: 
        emsg = "illegal opcode";
        break;
    case 10:
        emsg = "illegal 1010 opcode";
        break;
    case 11:
        emsg = "illegal 1111 opcode";
        break;
    case 5:
        emsg = "zero divide";
        break;
    case 6:
        emsg = "check error";
        break;
    case 7:
        emsg = "trapv error";
        break;
    case 8:
        emsg = "privileged opcode";
        break;
    case 9:
        emsg = "trace exception";
        break;
    case 33:
        emsg = "breakpoint exception";
        break;
    case 45:
        emsg = "trap instr";
        break;
    case 0:
        printf("illegal vector interrupt:exception %d, vector at #%x",
            (regs->exu.ex2.exfmtvec & 0xfff)>>2, 
            (regs->exu.ex2.exfmtvec & 0xfff)>>2);
        emsg = "";
        break;
    default:
        printf("exception %d", regs->exno);
        if (regs->exno == 31) {
            printf(",esr = #%4x ", esr);
            printesr(esr);
        }
        emsg = "";
        break;
    }

    printf("%s, status reg =#%4x, pc = #%lx\n",
        emsg, regs->exu.ex2.exstatreg, regs->exu.ex2.expc);
}

/* This is junk */
__unusedfunction(fmt, a1, a2, a3)
char *fmt;
int a1, a2, a3;
{
    struct exvec except;
    int *ap = (int*)&fmt;

    ap -= 2;
    except.exa7 = (int)ap;
    printf(fmt, a1, a2, a3);
    mon_printfproc(&except);
}

printesr(esr)
register int esr;
{
    printf("(");
    if (esr & 1)
        printf(" NOT_VALID");
    if (esr & 2)
        printf(" NOT_EXEC");
    if (esr & 4)
        printf(" NOT_WRITE");
    if (esr & 8)
        printf(" INVALID_ADDRESS");
    if (esr & 16)
        printf(" NOT_USER");
    if (esr & 32)
        printf(" MEGAPAGEFAULT");
    if (esr & 64)
        printf(" CPU_TIMEOUT");
    if (esr & 256)
        printf(" P_BUS_PARITY");
    if (esr & 4096)
        printf(" DMA_TIMEOUT");
    if (esr & 8192)
        printf(" DMA_MMU_ERROR");
    if (esr & 16384)
        printf(" Q_BUS_PARITY");
    printf(" )\n");
}

mon_colon(cmd)
int cmd;
{
    monrepcnt = monrep;
    switch (cmd) {
    case 'b':
        mon_setbreak(monaddr);
        break;
    case 'd':
        mon_clrbreak(monaddr);
        break;
    case 'r':
        write_virtlong(SYSVA+0x24, tracevec);
        write_virtlong(SYSVA+0x84, trap1vec);
        /*FALLTHRU*/
    case 'c':
        monrunflg = 'r';
        mon_colonrun();
        break;
    case 's':
        monrunflg = 's';
        mon_colonrun();
        break;
    case 'S':
        monrunflg = 'S';
        mon_colonrun();
        break;
    default:
        printf("Unknown command :%c\n", cmd);
        break;
    }
}

minitor_init()
{
    register struct bkpt *p;
    for (p = breakpoints; p < &breakpoints[16]; p++) {
        p->addr = 0;
        p->instr = 0;
    }
    
    regframe.exu.ex2.exstatreg = 0x2000;
    regframe.exu.ex2.expc = SYSVA;
    regframe.exa7 = physbase;
    regframe.exa4 = 0;
    regframe.exa5 = 0;
    regframe.exa6 = 0;
    regframe.exa0 = 0;
    regframe.exa1 = 0;
    regframe.exa2 = 0;
    regframe.exa3 = 0;
    regframe.exd0 = 0;
    regframe.exd1 = 0;
    regframe.exd2 = 0;
    regframe.exd3 = 0;
    vbrsave = getvbr();
}

mon_setbreak(addr)
int addr;
{
    register struct bkpt *p;
    for (p = breakpoints; p < &breakpoints[10]; p++)
        if (p->addr == 0) break;

    if (p == &breakpoints[10])
        printf("Too many breakpoints\n");
    else
        p->addr = addr;
}

mon_clrbreak(addr)
int addr;
{
    register struct bkpt *p;
    for (p = breakpoints; p < &breakpoints[10]; p++) {
        if (addr == p->addr) {
            p->addr = 0;
            p->instr = 0;
        }
    }
}

restore_brkpt()
{
    register struct bkpt *p;
    for (p = breakpoints; p < &breakpoints[16]; p++) {
        if (p->addr)
            write_virtword(p->addr, p->instr);
    }
}

mon_printbrk()
{
    register struct bkpt *p;
    printf("Breakpoints at:\n");
    for (p = breakpoints; p < &breakpoints[16]; p++) {
        if (p->addr)
            printf("        #%8lx\n", p->addr);
    }
}

mon_dollar(cmd)
int cmd;
{
    switch(cmd) {
    case 'r':
        mon_printregs(&regframe, ESR);
        break;
    case 'b':
        mon_printbrk();
        break;
    case 'c':
        mon_printproc(&regframe);
        break;
    default:
        printf("Unknown command %c\n", cmd);
        break;
    }
}

mon_colonrun()
{
    register struct bkpt *p;
    
    if (regframe.exno == 33) {              /* trap1 = single step */
        montraceflg++;
        regframe.exu.ex2.exstatreg |= 0x8000;   /* set T0 trace bit */
        setvbr(vbrsave);
        retexcept(&regframe);
    }

    for (p = breakpoints; p < &breakpoints[16]; p++) {
        if (p->addr) {
            p->instr = mem_readword(p->addr);
            write_virtword(p->addr, 0x4e41);    /* TRAP1 instruction */
            if (mem_readword(p->addr) != 0x4e41)
                printf("Cannot set breakpoint at #%8lx\n", p->addr);
        }
    }
    
    if (mem_readword(regframe.exa7)== -1 || (regframe.exa7 & 1)) {
        printf("invalid a7 = #%8lx, cannot run program\n", regframe.exa7);
        return;
    }

    if (monrunflg == 's')
        regframe.exu.ex2.exstatreg |= 0x8000;   /* set T0 trace bit */
    else if (monrunflg == 'S')
        regframe.exu.ex2.exstatreg |= 0x4000;   /* set T1 trace bit */

    setvbr(vbrsave);
    retexcept(&regframe);
}

trap_trace(exvec)
struct exvec *exvec;
{
    vbrsave = getvbr();                         
    setvbr(MINITOR);
    save_exframe(exvec);

    if (montraceflg) {
        montraceflg = 0;
        mon_colonrun();
    }

    restore_brkpt();
    if (exvec->exno == 9) {                 /* TRACE exception */
        if (((int*)vbrsave)[33] == exvec->exu.ex2.expc) {
            exvec->exu.ex2.exstatreg &= 0x3fff; /* clear Trace bits in SR */
            return;
        }
        printf("Trace at ");
        moncur = exvec->exu.ex2.expc;
        monaddr = exvec->exu.ex2.expc;
        mon_disasemble(1);
    } else {
        printf("Breakpoint at ");
        monaddr = moncur = exvec->exu.ex2.expc-2; /* fix PC address, points past TRAP1 */
        mon_disasemble(1);
    }

    if (--monrepcnt > 0)
        mon_colonrun();

    longjmp(monreturn);
}

save_exframe(exvec)
register struct exvec *exvec;
{
    register int *src = (int*)exvec;
    register int *tgt = (int*)&regframe;
    register int cnt = getexvecsize(exvec) / sizeof(int);
    while (--cnt >= 0)                  /* copy exception frame into monitor RAM */
        *tgt++ = *src++;
    if (exvec->exno == 33)          
        regframe.exu.ex2.expc -= 2;     /* fix PC address, poits past TRAP1 */
    regframe.exu.ex2.exstatreg &= 0x3fff; /* clear Trace bits T0 and T1 */
}

int getexvecsize(exvec)
register struct exvec *exvec;
{
    if ((exvec->exu.ex2.exfmtvec & 0xe000)==0)
        return 0x50;                    /* exception frame types 0 or 1 */
    else if ((exvec->exu.ex2.exfmtvec & 0xf000)==0x2000)    /* type 2 */
        return 0x54;
    else if ((exvec->exu.ex2.exfmtvec & 0xf000)==0x9000)    /* type 9 */
        return 0x5c;
    else if ((exvec->exu.ex2.exfmtvec & 0xf000)==0xA000)    /* type 10 */
        return 0x68;
    else if ((exvec->exu.ex2.exfmtvec & 0xf000)==0xB000)    /* type 11 */
        return 0xa4;
    
    printf("veclen fmtvec=%x ??\n", exvec->exu.ex2.exfmtvec);   /* invalid */
    /* BUG? what the heck is returned in D0 ? */
}
