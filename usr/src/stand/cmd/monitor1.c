#include "data.h"

/* for exechdr */
#include <nlist.h>
#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>


static char *_Version = "@(#) RELEASE:  1.0  Sep 11 1986 /usr/src/stand/cmd/boot.c ";

/* monitor work space */
int curcmd;                 /* current command to be processed */
int curchar;                /* current char read from buffer */

struct regtable {
    char *name;
    int size;
    int offset;
} regtable[] = {
    { "a7", 4, 0 },
    { "d0", 4, 4 },
    { "d1", 4, 8 },
    { "d2", 4, 12 },
    { "d3", 4, 16 },
    { "d4", 4, 20 },
    { "d5", 4, 24 },
    { "d6", 4, 28 },
    { "D7", 4, 32 },
    { "a0", 4, 36 },
    { "a1", 4, 40 },
    { "a2", 4, 44 },
    { "a3", 4, 48 },
    { "a4", 4, 52 },
    { "a5", 4, 56 },
    { "a6", 4, 60 },
    { "sr", 2, 72 },
    { "ps", 2, 72 },
    { "pc", 4, 74 },
    { "usp", 4, 1000 },
    { "vbr", 4, 1001 },
    { "sfc", 4, 1002 },
    { "dfc", 4, 1003 },
    { "cacr", 4, 1004 },
    { "caar", 4, 1005 },
    { "ptbr", 0, PTBR },
    { "pcr", 2,  PCR },
    { "esr", 2,  ESR },
    { "ptec", 0, PTEC },
    { "spt", 0,  SPT },
    { "ccr", 2,  CCR },
    { "mfp", 0,  MFP },
    { "pbc", 2,  PBC },
    { "dpt", 0,  DPT },
    { "lru", 0,  LRU }
};
int regtable_sz = 35;

/* public vars are in data.c */

/* some externals */

/****************************************************************************/

/* get a char from current buffer (chptr), and store it into curchar; skip spaces */
nextchar()
{
    curchar = *chptr++;
    while (curchar == ' ')
        curchar = *chptr++;
}

/* check first char of a number for base: 0 = octal, # = hex, 1-9 = dec, other = invalid */
int beginnum(ch)
{
    if (ch == '0')
        return 8;
    if (ch > '0' && ch <= '9')
        return 10;
    if (ch == '#')
        return 16;
    if (ch == '-')
        return 1;
    return 0;
}

/* return 1 if digit is valid for given base */
int validdigit(ch, base)
{
    if (ch >= '0' && ch <= '7')
        return 1;
    if (ch >= '8' && ch <= '9' && base > 8)
        return 1;
    if (ch >= 'A' && ch <= 'F' && base > 10)
        return 1;
    if (ch >= 'a' && ch <= 'f' && base > 10)
        return 1;
    return 0;
}

/* read a number from buffer stream, returns value */
int getnumber()
{
    char numbuf[20];
    register char *np = numbuf;
    register int base;

    if (curchar == '-') {
        *np++ = curchar;
        curchar = *chptr++;
    }

    if ((base = beginnum(curchar)) != 0) {
        *np++ = curchar;
        for (curchar = *chptr++; validdigit(curchar, base) && np < &numbuf[16]; ) {
            *np++ = curchar;
            curchar = *chptr++;
        }
    }
    *np = '\0';

    while (curchar == ' ') curchar = *chptr++;

    return atoi(numbuf);
}

/* minitor main loop */
main() 
{
    funcptr target;                     /* target address */
    int option;                         /* cmd option letter */
    int offset;                         /* cmd offset */
    register int i;

    /* initialize some stuff */
    icc_noprint = echo = monrep = 1;    /* no ICC out, enable echo, set repeat count */
    monaddr = monlast = moncur = SYSVA; /* default address */
    curcmd = 'X';                       /* hex dump */

    cons_detect();                      /* detect consoles in system */
    clear_mem(1);                       /* clear memory, 1=also verify first SYSVA 8K */
    minitor_init();                     /* initialize minitor memory area */
    icc_detect();                       /* check for ICC0 in system */

    printf("\n\nMinitor-32 1.2 (%d)\n", mon_date);
    printf("Serial Number %4x, Ethernet address %4x%4x%4x\n",
        macaddress.hi, macaddress.mi, macaddress.lo);

    if (!coldinit)                      /* unless system is started from switchon, try autoboot */
        autoboot(1);

    setjmp(monreturn);                  /* return here in case of error */
    echo = 1;                           /* enable echo */
    noopenerr = 0;                      /* report device errors from now */

    for (;;) {                          /* endless loop */

        printf("<- ");                  /* prompt*/
        gets(monline);                  /* read line */
        chptr = monline;                /* set ptr to line */
        curchar = ' ';                  /* init curchar */
        nextchar();                     /* get next char */
        
        if (monregflg || monaddrsz)
            monaddr = SYSVA;            /* reset address from previous cmd */

        monaddrsz = monregflg = 0;

        if (beginnum(curchar))          /* found a number? */
            monaddr = monlast = getnumber();
        else if (curchar == '^') {      /* use address entered last? */
            monaddr = monlast;
            nextchar();
        } else if (curchar == '.') {    /* use current address? */
            monaddr = moncur;
            nextchar();
        } else if (curchar >= 'a' && curchar <= 'z') {  /* possible register */
            option = 0;
            for (i=0; (curchar >= 'a' && curchar <='z') || 
                      (curchar >= '0' && curchar <= '9'); curchar = *chptr++)
                monreg[i++] = curchar;  /* BUG: warning this buffer might overflow! */

            if (i==1) {                 /* single char */
                curchar = monreg[0];
                chptr--;
                goto iscommand;         /* check for command */
            }
            monreg[i] = '\0';
            while (curchar == ' ')      /* skip trailing spaces */
                curchar = *chptr++;
            if (curchar == '@') {
                option++;
                nextchar();
            }
            
            if (!parsereg())            /* not a register - ignore line */
                continue;

            curcmd = 'X';               /* is a reg, enforce hex mode */
            monrep = 1;                 /* and repeat = 1 */
            
            if (option) {               /* was '@' seen? */
                if (monaddrsz != 4) {   /* not a 4 byte register? report and ignore line */
                    printf("register %s is not a pointer\n", monreg);
                    continue;
                }
                monaddr = mem_readlong(monaddr);    /* dereference */
                monregflg = 0;          /* clear register flag */
                if (monaddr == -1) {
                    printf("\nInvalid address\n");  /* error, ignore line */
                    continue;
                }
                monaddrsz = 0;          /* allow further deref: this is now a real address */
            }
        }

        while (curchar == '@') {
            monaddr = mem_readlong(monaddr);    /* multiple dereference: addr@@@ */
            nextchar();
        }
        if (curchar == '+' || curchar == '-') { /* optional offset +/-nnnnnn */
            option = curchar=='+';
            nextchar();
            offset = getnumber();
            if (!option)
                offset = -offset;
            if (!monaddrsz)
                monaddr += offset;
        }

        if (curchar == ',') {           /* optional repeat */
            nextchar();
            monrep = getnumber();
            if (monaddrsz)              /* cannot repeat a register */
                monrep = 1;
        } else if (curchar=='\0')       /* end of line, no repeat was given */
            monrep = 1;

        if (curchar == '/' || curchar == '?')   /* compatibility: / or ? are "dump" */
            nextchar();

iscommand:
        if (curchar) {                  /* has a new command? */
            curcmd = curchar;
            nextchar();                 /* set it */
        } else {
            switch (curcmd) {           /* no, repeat last command */
            case 'D': case 'd':
            case 'F': case 'f':
            case 'O': case 'o':
            case 'X': case 'x':
            case 'c':
            case 'i':
            case 's':
                break;          
            case ':':
                curchar = coloncmd;     /* was colon? repeat colon command */
                break;
            default:
                curcmd = 0;             /* other? don't run this */
                break;
            }
        }

        if (monaddrsz) {                /* operation on register? */
            switch (curcmd) {
            case 'd': case 'o': case 'w': case 'x':
                if (monaddrsz == 4)     /* operate on 32 bit reg? */
                    curcmd -= 0x20;     /* make implicitly 32 bit */
                break;
            case 'D': case 'O': case 'W': case 'X':
                if (monaddrsz == 2)     /* operate on 16 bit reg? */
                    curcmd -= -0x20;    /* make implicitly 16 bit */
                break;
            case 'b':
                curcmd = monaddrsz == 2 ? 'w' : 'W';    /* b on register: make implicitly 16/32 */
                break;
            }
        }

        moncur = monaddr;               /* store current address for later */
        switch (curcmd) {
        case 'D': case 'O': case 'X': case 'c':
        case 'd': case 'o': case 'x': case 's':
            mon_dump(curcmd);
            break;
        case 'i':
            mon_disassemble(monrep);
            break;
        case 'W': case 'b': case 'w':
            mon_write(curcmd);
            break;
        case '=':
            mon_write( monaddrsz == 2 ? 'w' : 'W');
            break;
        case 'F': case 'f':
            mon_find(curcmd);
            break;
        case 'j':
            target = (funcptr)monaddr;
            (*target)();
            break;
        case 'r':
            target = (funcptr)MINITOR;
            (*target)();
            break;
        case 'l':
            minitor_init();
            mon_loadgo(1);
            break;
        case 'L':
            minitor_init();
            mon_loadgo(0);
            break;
        case 'I':
            mon_iccinit();
            break;
        case 'z':
            echo = 0;                   /* suppress echo */ 
            minitor_init();
            mon_loadserial();           /* load from DH11 */
            echo = 1;                   /* enable echo */
            curchar = 0;                /* clear curchar */
            break;
        case '$':
            mon_dollar(curchar);
            nextchar();
            break;
        case ':':
            coloncmd = curchar;
            moncolon(coloncmd);
            nextchar();
            break;
        case 'm':
            mem_test(getnumber());
            break;
        case 'a':
            autoboot(0);                /* autoboot, no icc load */
            break;
        case 'A':
            autoboot(2);                /* autoboot with icc load */
            break;
        case 'e':
            clear_mem(0);               /* clear low 1MB - 32k */
            break;
        case 'M':
            mon_cmdm();                 /* unimplemented command */
            break;
        case 'k':
            mon_cmdk();                 /* unimplemented command */
            break;
        case '\0':                      /* empty line */
            break;
        default:
            mon_error();                /* unknown command */
            continue;
        }

        if (curchar)                    /* further chars following? */
            mon_error();                /* yes error */
    }
}

mon_error()
{
    static int nocmdlist;               /* suppress verbose explanation after first error */
    register struct devtbl *devp;
    
    if (nocmdlist) {
        printf("Error\n");
        return;
    }

    nocmdlist = 1;
    printf("Commands: [addr [+/- offset]] [,rep-count] [cmd] [values], where cmd is\n\
\td,o,x   print word dec, oct, hex\n\
\tD,O,X   print long word dec, oct, hex\n\
\tc,s,i   print char, string (null terminated), instruction\n\
\tb,w,W   write following (up to 10) values as bytes, words long words\n\
\tf,F     find following value as short (f) or long (F) value\n\
\tj       jsr to Address\n\
\tl       followed by file_name: load file_name and start\n\
\tL       followed by file_name: load file_name, do not start\n\
\tI       I file_name: load file_name to ICC, I -s: start ICC\n\
\t:b,:d   set or delete breakpoint at addr\n\
\t:r,:c   continue program up to next breakpoint\n\
\t:s,:S   single step program (:S trace on change of flow)\n\
\t$r,$b,$c show registers, breakpoints, stack trace\n\
\ta,A,e,r,m a(utoboot), e(rase mem), r(einitialize), m(emtst)\n\
\tz       load program over serial line from DLV11-J Channel 1\n");

    printf("Address is number or register name: d0-d7,a0-a7,sr,ps,pc,\n\
usp,vbr,sfc,dfc,cacr,caar,ptbr,pcr,esr,spt,ccr,mfp,pbc,dpt,lru.\n\
Address may be followed by @\'s for indirection, e.g. pc@, a6@@\n\
Numbers: 123 (dec) 0307 (oct) #3F80 (hex)\n\
\'^\' is last explicitly entered Address, \'.\' is current address\n\
File_name is e.g. sw/unix or is(1) or sw/sa/mkfs or sw(0,0)/unix\n\
Valid boot device names are: ");

    for (devp = dev_table; devp->d_name; devp++)
        printf("%s ", devp->d_name);
    putchar('\n');
}

mon_dump(cmd)
int cmd;
{
    register unsigned char *cp = (unsigned char*)monaddr;
    register unsigned short *wp = (unsigned short*)monaddr;
    register unsigned int *ip = (unsigned int*)monaddr;
    int count, icnt, perline;
    register int i;

    switch(cmd) {
    case 'd': case 'o': case 'x':
        count = 16;
        perline = 8;
        break;
    case 'D': case 'O': case 'X':
        count = 16;
        perline = 4;
        break;
    case 'c':
        count = 64;
        perline = 64;
        break;
    case 's':
        count = 1;
        perline = 1;
        break;
    }

    if (monregflg)
        printf("%s: ", monreg);
    else
        printf("#%8x: ", monaddr);

    for (icnt=0, i=0; i <monrep; i++) {
        if (i != 0 && perline < monrep && (monaddr%count)==0) {
            if ((icnt % perline) != 0)
                putchar('\n');
            printf("#%8x:", monaddr);
            icnt = 0;
        }
        switch(cmd) {
        case 'd':
            printf("%5d ", *wp++);
            break;
        case 'D':
            printf("%10d ", *ip++);
            break;
        case 'o':
            printf("0%6o ", *wp++);
            break;
        case 'O':
            printf("0%11o ", *ip++);
            break;
        case 'x':
            printf("#%4x ", *wp++);
            break;
        case 'X':
            printf("#%8x ", *ip++);
            break;
        case 'c':
            putchar(*cp++);
            break;
        case 's':
            printf("%s", cp);
            while (*cp++);
            break;
        }

        switch (cmd) {
        case 'd': case 'o': case 'x':
            monaddr = (int)wp;
            break;
        case 'D': case 'O': case 'X':
            monaddr = (int)ip;
            break;
        case 'c': case 's':
            monaddr = (int)cp;
            break;
        }

        icnt++;
        if ((icnt % perline)==0)
            putchar('\n');
    }
    
    if ((icnt % perline) != 0)
        putchar('\n');
}

mon_write(cmd)
int cmd;
{
    int numbuf[10];
    register unsigned char *cp = (unsigned char*)monaddr;
    register unsigned short *wp = (unsigned short*)monaddr;
    register unsigned int *ip = (unsigned int*)monaddr;
    register int k, curnum, i;

    for (i=0; i < 10 && beginnum(curchar); )
        numbuf[i++] = getnumber();

    if (i==1) {
        curnum = numbuf[0];
        i = monrep;
        switch (cmd) {
        case 'b':
            for (k=0; k < i; k++)
                *cp++ = curnum;
            monaddr = (int)cp;
            break;
        case 'w':
            for (k=0; k < i; k++)
                *wp++ = curnum;
            monaddr = (int)wp;
            break;
        case 'W':
            for (k=0; k < i; k++)
                *ip++ = curnum;
            monaddr = (int)ip;
            break;
        }
    } else {
        switch (cmd) {
        case 'b':
            for (k=0; k < monrep; k++) {
                for (curnum = 0; curnum < i; curnum++)
                    *cp++ = numbuf[curnum];
            }
            monaddr = (int)cp;
            break;
        case 'w':
            for (k=0; k < monrep; k++) {
                for (curnum = 0; curnum < i; curnum++)
                    *wp++ = numbuf[curnum];
            }
            monaddr = (int)wp;
            break;
        case 'W':
            for (k=0; k < monrep; k++) {
                for (curnum = 0; curnum < i; curnum++)
                    *ip++ = numbuf[curnum];
            }
            monaddr = (int)ip;
            break;
        }
    }
    if (monaddr >= (int)&specregs && 
        monaddr <= (int)(((char*)&specregs)+sizeof(struct specregs)))
        setspecregs(&specregs);
}

mon_find(cmd)
int cmd;
{
    static unsigned int lastnum;        /* saved number from last find cmd */
    register unsigned int tofind;
    register int i;
    register unsigned short *wp;
    register unsigned int *ip;

    tofind = beginnum(curchar) ? getnumber() : lastnum;
    lastnum = tofind;

    if (cmd == 'f')
        tofind &= 0xffff;               /* mask 16 bits */

    for (i=0; i < monrep; i++) {
        ip = (unsigned int*)monaddr;
        wp = (unsigned short*)ip;
        if (cmd == 'f') {
            while (*wp != tofind) wp++;
            moncur = (int)wp;
            monaddr = (int)++wp;
        } else {
            while (*ip != tofind) ip = (unsigned int*)(((unsigned short*)ip)+1);
            monaddr = (int)ip;
            ip = (unsigned int*)(((unsigned short*)ip)+1);
        }
        /* found one */
        printf("%08x: ", moncur);
        if (cmd == 'f')
            printf("%4x\n", tofind);
        else
            printf("%8x\n", tofind);
    }
}

/***************************load and execute binaries*************************/

static struct exehdr {
    struct filehdr e_fhdr;
    struct aouthdr e_aout;
    struct scnhdr  e_text;
    struct scnhdr  e_data;
    struct scnhdr  e_bss;
} exechdr;                  /* header of loadable binaries e.g. aunix or iccunix */

int mon_loadgo(goflg)
int goflg;
{
    int i;
    funcptr start;

    for (i=0; curchar != ' ' && curchar != '\0'; ) {
        bootfile[i++] = curchar;
        nextchar();
    }
    bootfile[i++] = '\0';
    i = open(bootfile, 0);          /* open for read */
    if (i < 0) {
        printf("cannot open %s\n", bootfile);
        return -1;
    }

    lseek(i, 0, 0);                 /* seek to begin */
    
    read(i, &exechdr, sizeof(struct exehdr));   /* read header */
    if (exechdr.e_fhdr.f_magic != VAXROMAGIC || exechdr.e_aout.magic != 0410) {
        printf("%s not in COFF format\n", bootfile);
        close(i);
        return -1;
    }

    load_textdata(i);               /* load .text and .data segment */
    close(i);

    write_virtlong(con_current, con_type); /* save console in last word of RAM */
    
    if (!goflg)                     /* goflag = 0, do not start code */
        return 0;

    if (iccloaded) {                /* is ICC kernel loaded? */
        printf("I -s\n");
        icc_start();                /* yes, start ICC */
    } else if (hasicc)              /* does it actually have an ICC? */
        icc_idle();                 /* yes, wait until it is up */
    
    start = (funcptr)SYSVA;         /* start at virtual address 0 */
    return (*start)();
}

/* read from file */
int read(fd, buf, nread)
int fd;
register char* buf;
register int nread;
{
    register int ch;

    while (nread-- != 0) {
        ch = readb(fd);
        *buf++ = ch;
        if (ch == -1)               /* EOF? */
            return;
    }
}

int load_textdata(fd)
register int fd;
{
    register int *addr;

    textsize = exechdr.e_aout.tsize;
    datasize = exechdr.e_aout.dsize;

    /* when loading header, we've read ahead to startpos of .text in file */
    
    /* load text segment */
    addr = (int*)SYSVA;
    load_bytes(addr, textsize, fd); /* load textsegment from disk */
    addr = (int*)(textsize + SYSVA);
    *addr++ = 0;                    /* reserve 3 null words */
    *addr++ = 0;
    *addr++ = 0;

    /* load data segment */
    addr = (int*)(ptob(btop(textsize)) + SYSVA);    /* align to next page */
    load_bytes(addr, datasize, fd);
}

load_bytes(addr, sz, fd)
register char *addr;
int sz, fd;
{
    register struct iobuf *bp = &fdbuf[fd];
    register int cnt;

    for (; sz > 0; sz--) {
        
        /* read remaining data from fdbuffer */
        cnt = sz > bp->b_count ? bp->b_count : sz;
        memcpy(addr, bp->b_secbuf, cnt);
        bp->b_count -= cnt;
        bp->b_foffset += cnt;
        bp->b_secbuf += cnt;
        addr += cnt;
        sz -= cnt;
        /* still bytes to read left?  fdbuffer is empty */
        if (sz > 0) {
            *addr = readb(fd);  /* enforce buf fill by reading a single byte */
            sz--;
        }
    }
}

memcpy(tgt, src, sz)
register char *tgt, *src;
register int sz;
{
    while (--sz >= 0)
        *tgt++ = *src++;
}

/* read data from a DLV11 serial line.
 * This is likely to initially bootstrap a system if there is no
 * boot medium available yet - function for developers
 *
 * This uses a special loader protocol:
 * a        switch to address entry
 * d        switch to data entry
 * 0-9A-F   add a digit to current entry field (a or d)
 * :;<=>?   also allows these as hex digits (broken hex)
 * i        add 1 to addr
 * I        add 2 to addr
 * w        write a byte
 * W        write a word
 * L        write a long and increment addr by 4
 * q        report current addr and data
 * r        read byte at addr into data
 * R        read long at add, report current addr and data and incr addr by 4
 * j        call code at addr/data address depending on a/d mode 
 * z        set addr and data = 0
 * Q        exit back to monitor
 * CR/LF    print a dot
 */
mon_loadserial()
{
    register unsigned int addr, data;
    funcptr jumpaddr;
    int adflag = 0;
    
    addr = data = 0;
    for (;;) {
        switch(curchar = ser_read()) {
        case 'a':
            adflag = 0;
            break;
        case 'd':
            adflag = 1;
            break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            if (adflag)
                data = (data << 4) + (curchar - '0');
            else
                addr = (addr << 4) + (curchar - '0');
            break;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            if (adflag)
                data = (data << 4) | (curchar - '7');
            else
                addr = (addr << 4) | (curchar - '7');
            break;
        /* "broken hex" */
        case ':': case ';': case '<': case '=': case '>': case '?':
            if (adflag)
                data = (data << 4) | (curchar - '0');
            else
                addr = (addr << 4) | (curchar - '0');
            break;
        case 'i':
            addr++;
            break;
        case 'I':
            addr += 2;
            break;
        case 'w':
            *(char*)addr = data;
            break;
        case 'W':
            *(unsigned short*)addr = data;
            break;
        case 'L':
            *(int*)addr = data;
            addr += sizeof(int);
            break;
        case 'q':
            printf("\ta0 = #%8x\td0 = #%8x\n", addr, data);
            break;
        case 'r':
            data = (unsigned int)*(char*)addr;
            break;
        case 'R':
            data = *(unsigned int*)addr;
            printf("\ta0    = #%8x\td0 = #%8x\n", addr, data);
            addr += sizeof(int);
            break;
        case 'j':
            jumpaddr = (funcptr)(adflag ? data : addr);
            echo = 1;                   /* enable echo */
            (*jumpaddr)();              /* call code at jumpaddr */
            /*FALLTHRU*/
        case 'z':
            addr = data = 0;
            break;
        case 'Q':
            return;
        case '\n':
        case '\r':
            putchar('.');
            break;
        default:
            break;
        }
    }
}

/* parses a register name in monreg, return 1 if okay, 0 if error */
int parsereg()
{
    register int i;
    for (i=0; i < regtable_sz; i++) {   /* find register in monreg in table */
        if (!strcmp(regtable[i].name, monreg))
            break;
    }
    if (i==regtable_sz) {               /* unknown */
        printf("%s not known\n", monreg);
        return 0;
    }

    /* found one */
    monaddr = regtable[i]. offset;
    monregflg++;
    if (monaddr < 100)                  /* is a A or D register */
        monaddr += (int)&regframe;
    else if (monaddr < 1100) {          /* is a special CPU register */
        monaddr -= 1000;
        monaddr = (int)(((int*)specregs)+monaddr);
        getspecregs(&specregs);
    } else                              /* is a special location like PTBR or ESR */
        monregflg = 0;

    monaddrsz = regtable[i].size;
    if (monaddrsz==0)                   /* if memory location, save last addr */
        monlast = monaddr;

    return 1;
}

/* phase =0: normal autoboot, =1: forced, =2: load ICC first */
autoboot(phase)
{
    char bootdev[40];
    char bootbuf[40];
    char *bootf;
    register int i, delay, ncons;

    if (phase == 1) {
        printf("Autoboot - Hit any key to break\n");
        for (i=0, ncons=0; i < 16; i++)         /* count # of consoles */
            if (con_type & (1 << i)) ncons++;

        if (con_type & 0x80)                    /* is ICC console active? */
            ncons += 2;

        delay = 100000 / ncons;                 /* make delay depend on #consoles */
        for (i=0; i < delay; i++) {             /* poll consoles */
            if ((ncons = haschar()) != 0) {     /* got key */
                echo = 0;                       /* disable echo */
                printf("Console is on %s\n", cons_devices[ncons]);
                con_type = 1 << ncons;          /* disable other consoles */
                write_virtlong(con_current, con_type); /* store console in highest RAM word */
                getchar();                      /* eat key press */
                echo++;                         /* reenable echo */
                return;
            }
        }

        /* no key was hit, so go into autoboot */
        name_console();                         /* print name of active console */
        write_virtlong(con_current, con_type); /* store consoles in highest RAM word */
        bootf = "/unix";                        /* use default */
    } else {
        /* obtain boot file from input line */
        i = 0;
        while (curchar != ' ' && curchar != '\0') {
            if (i == 0 && curchar != '/')       /* ensure file starts with '/' */
                bootbuf[i++] = '/';
            bootbuf[i++] = curchar;
            nextchar();
        }
        if (i != 0) {
            bootbuf[i++] = '\0';
            bootf = bootbuf;                    /* user entered a filename */
        } else
            bootf = "/unix";                    /* use default */
    }
    
    /* now trying to boot the named file */
    noopenerr = 1;                              /* suppress errors for nonexisting devs */

    if (find_bootdev(bootdev)==0) {     
        printf("can't find boot device\n");     /* don't have one */
        noopenerr = 0;                          /* reenable error reporting */
        return;
    }
    
    /* found a valid boot dev */
    if (con_type == 0x80 ||                     /* console on ICC0? */
            (bootdev[i]=='i' && bootdev[1]=='w') || 
            (bootdev[i]=='i' && bootdev[1]=='l') ||
            (phase != 2 && hasicc)) {           /* need to load ICC kernel */
        strcpy(bootdev+2, "/icckernel");
        printf("I %s\n", bootdev);
        chptr = bootdev;
        nextchar();
        mon_iccinit();
        iccloaded++;
    }
    
    if (bootdev[0] == 'i' && bootdev[1] == 'l') /* get bootfile for network boot */
        bootf = il_bootfile;
    strcpy(bootdev+2, bootf);                   /* append bootfile after dev */

    putchar(iccloaded ? 'L' : 'l');             /* report L/l filename */
    printf(" %s\n", bootdev);                   /* L=loadonly, l=load&run */
    
    chptr = bootdev;                            /* engage loadgo of monitor */
    nextchar();
    minitor_init();
    mon_loadgo(1);
}

/* locate a valid boot device, and write ists name into buf
 * return 0 if none found */
int find_bootdev(buf)
char* buf;
{
    /* attempting to boot in this order:
     * HK (RK06/07)
     * IW (ICC winchester)
     * IL (ICC lance network boot
     */
    
    int fd;
    register struct devtbl *dp;

    for (dp = dev_table; dp->d_name; dp++) {    /* loop through devices */
        if (dp->d_iobase) {             /* only bootable devices, hk, il, iw */
            if (is_icc(dp->d_iobase)==0 &&
                mem_readbyte(dp->d_iobase) == -1 &&
                mem_readbyte(dp->d_iobase) != -1)
                    continue;           /* not ICC and no device at I/O address */
        }
        /* found device at address */
        buf[0] = dp->d_name[0];
        buf[1] = dp->d_name[1];
        buf[2] = '\0';
        if ((fd = open(buf)) > 0) {     /* can raw device be opened */
            close(fd);
            return 1;                   /* yes */
        }
    }
    return 0;                           /* no valid device found */
}

/* is 1 if ioaddr is ICC0 addr */
int is_icc(ioaddr)
int ioaddr;
{
    if (ioaddr != ICCADDR)
        return 0;

    if (hasicc)
        return 1;
    return 0;
}

/* process monitor I command */
mon_iccinit()
{
    register int i = 0;
    char *savechptr = chptr;
    int iccdebug = 0;
    funcptr initcode;
    
    if (curchar=='-') {
        nextchar();
        if (curchar == 's') {
            icc_start();
            initcode = (funcptr)SYSVA;
            (*initcode)();
        } else if (curchar == 'd')
            iccdebug++;
        else
            return;
        nextchar();
    }
    
    if (mon_loadgo(0) == 0) {
        icc_load();
        if (iccdebug)
            icc_debug();
    }
}

/* clear memory flag=1: check only, =0: clear memory */
clear_mem(flag)
int flag;
{
    register int *ptr;
    register int *ptend;

    if (flag) {
        ptr = (int*)SYSVA;              /* start of SYSVA */
        ptend = (int*)(SYSVA+8192);     /* +8K */
        for (; ptr < ptend; ptr += 2) {
            if (mem_readword((int)ptr) == -1) {
                printf("\n\nClearing memory..\n");
                memsize();              /* find memory size */
                break;
            }
        }
    }

    ptr = (int*)SYSVA;
    ptend = (int*)(SYSVA + 0xF7C00);    /* from SYSVA up to SYSVA-(1MB-1K) */
    for (; ptr < ptend; ptr++)          /* clear memory */
        *ptr = 0;                       /* i.e. don't touch pte_cxxxxx area */
}


mon_cmdk()
{
    printf("not yet implemented\n");
}

mon_cmdm()
{
    printf("not yet implemented\n");
}
