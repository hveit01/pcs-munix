#include "data.h"

#define AM_AX_XWind8 0
#define AM_AX_XLind8 1
#define AM_AX_WBI2 2
#define AM_AX_LBI2 3
#define AM_AX_WBI1 4
#define AM_AX_LBI1 5
#define AM_unused2 6
#define AM_unused3 7
#define AM_AX_WBI6 8
#define AM_AX_LBI6 9
#define AM_AX_WBI5 10
#define AM_AX_LBI5 11
#define AM_AX_WI5 12
#define AM_AX_LI5 13
#define AM_AX_WI2 14
#define AM_AX_LI2 15
#define AM_AX_WI1 16
#define AM_AX_LI1 17
#define AM_AX_WI6 18
#define AM_AX_LI6 19
#define AM_AX_WI0 20
#define AM_AX_LI0 21
#define AM_AX_noBI 22
#define AM_AX_B 23
#define AM_unused24 24
#define AM_DX 25
#define AM_AX 26
#define AM_AXind 27
#define AM_AXinc 28
#define AM_AXdec 29
#define AM_AXdisp16 30
#define AM_ABS 31
#define AM_IMMWL 33
#define AM_PCIND 34
#define AM_PCind8 35
#define AM_PCext 36
#define AM_unused38 38
#define AM_IMMX 39

#define DM_MOVEP    1
#define DM_CHK2     2
#define DM_CMP2     3
#define DM_ORI      4
#define DM_ANDI     5
#define DM_SUBI     6
#define DM_RTM      7
#define DM_CALLM    8
#define DM_ADDI     9
#define DM_EORI     0xA
#define DM_CMPI     0xB
#define DM_MOVES    0xC
#define DM_MOVEB    0xD
#define DM_MOVEW    0xE
#define DM_MOVEL    0xF
#define DM_LEA      0x10
#define DM_CHK      0x11
#define DM_MOVE     0x12
#define DM_NEGX     0x13
#define DM_MOVE_0   0x14
#define DM_CLR      0x15
#define DM_MOVE_1   0x16
#define DM_NEG      0x17
#define DM_MOVE_2   0x18
#define DM_NOT      0x19
#define DM_LINK     0x1A
#define DM_SWAP     0x1B
#define DM_BKPT     0x1C
#define DM_EXTW     0x1D
#define DM_EXTL     0x1E
#define DM_NBCD     0x1F
#define DM_PEA      0x20
#define DM_MOVEM    0x21
#define DM_EXTBL    0x22
#define DM_TAS      0x23
#define DM_TST      0x24
#define DM_MULS     0x25
#define DM_MULU     0x26
#define DM_DIVS     0x27
#define DM_DIVU     0x28
#define DM_MOVEM_0  0x29
#define DM_TRAP     0x2A
#define DM_UNLK     0x2B
#define DM_LINKW    0x2C
#define DM_MOVE_3   0x2D
#define DM_MOVE_4   0x2E
#define DM_RESET    0x2F
#define DM_NOP      0x30
#define DM_STOP     0x31
#define DM_RTE      0x32
#define DM_RTD      0x33
#define DM_RTS      0x34
#define DM_TRAPV    0x35
#define DM_RTR      0x36
#define DM_MOVEC    0x37
#define DM_JMP      0x38
#define DM_JSR      0x39
#define DM_TRAP_0   0x3A
#define DM_DBT      0x3B
#define DM_DBF      0x3C
#define DM_DBCC     0x3D
#define DM_DBCS     0x3E
#define DM_DBEQ     0x3F
#define DM_DBGE     0x40
#define DM_DBGT     0x41
#define DM_DBHI     0x42
#define DM_DBLE     0x43
#define DM_DBLS     0x44
#define DM_DBLT     0x45
#define DM_DBMI     0x46
#define DM_DBNE     0x47
#define DM_DBPL     0x48
#define DM_DBVC     0x49
#define DM_DBVS     0x4A
#define DM_S        0x4B
#define DM_SUBQ     0x4C
#define DM_ADDQ     0x4D
#define DM_BCLR     0x4E
#define DM_BSET     0x4F
#define DM_BTST     0x50
#define DM_BRA      0x51
#define DM_BSR      0x52
#define DM_BCC      0x53
#define DM_BCS      0x54
#define DM_BEQ      0x55
#define DM_BGE      0x56
#define DM_BGT      0x57
#define DM_BHI      0x58
#define DM_BLE      0x59
#define DM_BLS      0x5A
#define DM_BLT      0x5B
#define DM_BMI      0x5C
#define DM_BNE      0x5D
#define DM_BPL      0x5E
#define DM_BVC      0x5F
#define DM_BVS      0x60
#define DM_MOVEQ    0x61
#define DM_SBCD     0x62
#define DM_PACK     0x63
#define DM_UNPK     0x64
#define DM_DIVSW    0x65
#define DM_DIVUW    0x66
#define DM_OR       0x67
#define DM_SUBX     0x68
#define DM_SUB      0x69
#define DM_UNASSIGNED   0x6A
#define DM_CMPM     0x6B
#define DM_EOR      0x6C
#define DM_CMP      0x6D
#define DM_EXGM     0x6E
#define DM_EXGA     0x6F
#define DM_EXGD     0x70
#define DM_ABCD     0x71
#define DM_ABCD_0   0x72
#define DM_MULSW    0x73
#define DM_MULUW    0x74
#define DM_AND      0x75
#define DM_ADDX     0x76
#define DM_ADD      0x77
#define DM_BFTST    0x78
#define DM_BFEXTU   0x79
#define DM_BFCHG    0x7A
#define DM_BFEXTS   0x7B
#define DM_BFCLR    0x7C
#define DM_BFFFO    0x7D
#define DM_BFSET    0x7E
#define DM_BFINS    0x7F
#define DM_ASR      0x80
#define DM_ASL      0x81
#define DM_LSR      0x82
#define DM_LSL      0x83
#define DM_ROXR     0x84
#define DM_ROXL     0x85
#define DM_ROR      0x86
#define DM_ROL      0x87
#define DM_ASR_0    0x88
#define DM_ASL_0    0x89
#define DM_LSR_0    0x8A
#define DM_LSL_0    0x8B
#define DM_ROXR_0   0x8C
#define DM_ROXL_0   0x8D
#define DM_ROR_0    0x8E
#define DM_ROL_0    0x8F
#define DM_FMOVE    0x90
#define DM_FADD     0x91
#define DM_FSUB     0x92
#define DM_FMUL     0x93
#define DM_FDIV     0x94
#define DM_FREM     0x95
#define DM_FCMP     0x96
#define DM_FTST     0x97
#define DM_FBEQ     0x98
#define DM_FBNE     0x99
#define DM_FBGE     0x9A
#define DM_FBLT     0x9B
#define DM_FBLE     0x9C
#define DM_FBGT     0x9D
#define DM_FSEQ     0x9E
#define DM_FSNE     0x9F
#define DM_FSGE     0xA0
#define DM_FSLT     0xA1
#define DM_FSLE     0xA2
#define DM_FSGT     0xA3
#define DM_FNEG     0xA4
#define DM_FSINH    0xA5
#define DM_FSQRT    0xA6
#define DM_FTANH    0xA7
#define DM_FATAN    0xA8
#define DM_FASIN    0xA9
#define DM_FATANH   0xAA
#define DM_FSIN     0xAB
#define DM_FTAN     0xAC
#define DM_FETOX    0xAD
#define DM_FTWOTOX  0xAE
#define DM_FTENTOX  0xAF
#define DM_FLOGN    0xB0
#define DM_FLOG10   0xB1
#define DM_FLOG2    0xB2
#define DM_FABS     0xB3
#define DM_FCOSH    0xB4
#define DM_FACOS    0xB5
#define DM_FCOS     0xB6
#define DM_FINT     0xB7
#define DM_FINTRZ   0xB8
#define DM_FGETEXP  0xB9
#define DM_FGETMAN  0xBA
#define DM_FSGLDIV  0xBB
#define DM_FSGLMUL  0xBC
#define DM_FMOVEM   0xBD
#define DM_FMOVECR  0xBE


/* to handle 16/32 bit diplacements */
typedef union {
    unsigned int i;
    unsigned char b[4];
    unsigned short w[2];
} DISPL;

/* description of an instruction argument */
struct disarg {
    short type;
    DISPL disp1;
    DISPL disp2;
    short reg;
    short xreg;
    short scale;
    DISPL immlong[2];
};

/* workspace for disass.c */
static unsigned short instbuf[12];   /* instruction words */
static short instbufcnt;             /* # of inst words in buffer */
static unsigned short *curw;         /* point to current instbuf word */
static struct disarg argbuf;         /* contains description of an argument */
static short fbfsflg;                /* distinguish between fs* and fb* float instrs */
static short prarg;                  /* flag in printarg() */

char *mnemotbl[] = {
    "???",    "movep",  "chk2",   "cmp2",   "ori",    "andi",   "subi",   "rtm",    /* 0-7 */
    "callm",  "addi",   "eori",   "cmpi",   "moves",  "move.b", "move.l", "move.w", /* 8-15 */
    "lea",    "chk",    "move",   "negx",   "move",   "clr",    "move",   "neg",    /* 16-23 */
    "move",   "not",    "link",   "swap",   "bkpt",   "ext.w",  "ext.l",  "nbcd",   /* 24-31 */
    "pea",    "movem",  "extb.l", "tas",    "tst",    "muls",   "mulu",   "divs",   /* 32-39 */
    "divu",   "movem",  "trap",   "unlk",   "link.w", "move",   "move",   "reset",  /* 40-47 */
    "nop",    "stop",   "rte",    "rtd",    "rts",    "trapv",  "rtr",    "movec",  /* 48-55 */
    "jmp",    "jsr",    "trap",   "dbt",    "dbf",    "dbcc",   "dbcs",   "dbeq",   /* 56-63 */
    "dbge",   "dbgt",   "dbhi",   "dble",   "dbls",   "dblt",   "dbmi",   "dbne",   /* 64-71 */
    "dbpl",   "dbvc",   "dbvs",   "s",      "subq",   "addq",   "bclr",   "bset",   /* 72-29 */
    "btst",   "bra",    "bsr",    "bcc",    "bcs",    "beq",    "bge",    "bgt",    /* 80-87 */
    "bhi",    "ble",    "bls",    "blt",    "bmi",    "bne",    "bpl",    "bvc",    /* 88-95 */
    "bvs",    "moveq",  "sbcd",   "pack",   "unpk",   "divs.w", "divu.w", "or",     /* 96-103 */
    "subx",   "sub", "unassigned","cmpm",   "eor",    "cmp",    "exgm",   "exga",   /* 104-111 */
    "exgd",   "abcd",   "abcd",   "muls.w", "mulu.w", "and",    "addx",   "add",    /* 112-119 */
    "bftst",  "bfextu", "bfchg",  "bfexts", "bfclr",  "bfffo",  "bfset",  "bfins",  /* 120-127 */
    "asr",    "asl",    "lsr",    "lsl",    "roxr",   "roxl",   "ror",    "rol",    /* 128-135 */
    "asr",    "asl",    "lsr",    "lsl",    "roxr",   "roxl",   "ror",    "rol",    /* 136-143 */
    "fmove",  "fadd",   "fsub",   "fmul",   "fdiv",   "frem",   "fcmp",   "ftst",   /* 144-151 */
    "fbeq",   "fbne",   "fbge",   "fblt",   "fble",   "fbgt",   "fseq",   "fsne",   /* 152-159 */
    "fsge",   "fslt",   "fsle",   "fsgt",   "fneg",   "fsinh",  "fsqrt",  "ftanh",  /* 160-167 */
    "fatan",  "fasin",  "fatanh", "fsin",   "ftan",   "fetox",  "ftwotox","ftentox",/* 168-175 */
    "flogn",  "flog10", "flog2",  "fabs",   "fcosh",  "facos",  "fcos",   "fint",   /* 176-183 */
    "fintrz", "fgetexp","fgetman","fsgldiv","fsglmul","fmovem", "fmovecr"           /* 184-190 */
};

short dis_scale[] = { 1, 2, 4, 8 };
char *dis_regs[] = { "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", 
                    "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", 
                    "FP0","FP1","FP2","FP3","FP4","FP5","FP6","FP7"};
char *dis_ascale[] = { "1", "2", "?", "4", "?", "?", "?", "8" };
char *dis_scond[] = { "t",  "f",  "hi", "ls", "cc", "cs", "ne", "eq",
                      "vc", "vs", "pl", "mi", "ge", "lt", "gt", "ge"};
                    
dis_getmore()
{
    if (instbufcnt==0) {
        instbuf[1] = dis_nextword();
        curw = &instbuf[1];
        instbufcnt = 1;
        return;
    }
    if (&instbuf[instbufcnt] >= curw) {
        curw++;
        return;
    }
    if (++instbufcnt > 12) {
        printf("Internal error, buffer overflow\n");
        instbufcnt--;
    }

    instbuf[instbufcnt] = dis_nextword();
    curw++;
}

do_disassemble()
{
    register int i;

    dis_makeargs();
    printf("\t\t\t");
    instbuf[0] = *curw;
    instbufcnt--;
    for (i=0; i < instbufcnt; i++)
        printf("%04x ", instbuf[i]);
    
    instbuf[1] = instbuf[0];
    curw = &instbuf[1];
    instbufcnt = 1;
}

dis_makearg(ap, mode, reg, sz)
struct disarg *ap;
short mode;
int reg, sz;
{
    unsigned short extw;
    union extwf {
        unsigned short i;
        struct {
            unsigned short da:1, reg:3, wl:1, scale:2, :1,
                           bs:1, is:1, bd:2, :1, iis:3 
        } f;
        struct {
            unsigned short :9, bs:1, is:1, bd:2, :1, i:1, is2:2
        } s;
    } extwfull;

    ap->reg = reg;
    ap->scale = 0;
    ap->disp1.i = ap->disp2.i = 0;

    switch (mode) {
    case 0:
        ap->type = AM_DX;
        break;
    case 1:
    case 2:
        ap->type = mode==1 ? AM_AX : AM_AXind;
        ap->reg += 8;
        break;
    case 3:
    case 4:
        ap->type = mode==3 ? AM_AXinc : AM_AXdec;
        ap->reg += 8;
        break;
    case 5:
        ap->type = AM_AXdisp16;
        ap->reg += 8;
        ap->disp1.w[0] = *curw;
        dis_getmore();
        break;
    case 6:
        extw = *curw;
        dis_getmore();
        if ((extw & 0x100)==0) { /* old 68000 mode? */
            ap->type = (extw & 0x800)==0 ? AM_AX_XWind8 : AM_AX_XLind8; /*WL*/
            ap->xreg = (extw & 0x7000) >> 12;
            if (extw & 0x8000)
                ap->xreg += 8;
            ap->reg += 8;
            ap->scale = dis_scale[(extw & 0x600) >> 9];
            ap->disp1.i = extw & 0xff;
        } else {
            extwfull.i = extw;
            ap->xreg = extwfull.f.reg;
            if (extwfull.f.da== 1)
                ap->xreg += 8;
            ap->scale = extwfull.f.scale;
            ap->reg += 8;
            if (extwfull.f.bd == 3) {
                ap->disp1.w[0] = *curw;
                dis_getmore();
                ap->disp1.w[1] = *curw;
                dis_getmore();
            } else if (extwfull.f.bd == 2) {
                ap->disp1.i = *curw;
                dis_getmore();
            }
            switch (extwfull.f.iis) {
            case 2:
                ap->disp2.i = *curw;
                dis_getmore();
                break;
            case 3:
                ap->disp2.w[0] = *curw;
                dis_getmore();
                ap->disp2.w[1] = *curw;
                dis_getmore();
                break;
            case 6:
                if (extwfull.f.bs)
                    ap->disp2.w[0] = *curw;
                dis_getmore();
                break;
            case 7:
                if (extwfull.f.bs) {
                    ap->disp2.w[0] = *curw;
                    dis_getmore();
                    ap->disp2.w[1] = *curw;
                    dis_getmore();
                }
                break;
            }
            if ((extwfull.s.bs | extwfull.s.i)==0) { /* both 0 */
                switch (extwfull.f.iis) {
                case 0:
                    ap->type = extwfull.f.wl ? AM_AX_XLind8 : AM_AX_XWind8;
                    break;
                case 1:
                    ap->type = extwfull.f.wl ? AM_AX_LBI1 : AM_AX_WBI1;
                    break;
                case 2:
                case 3:
                    ap->type = extwfull.f.wl ? AM_AX_LBI2 : AM_AX_WBI2;
                    break;
                case 5:
                    ap->type = extwfull.f.wl ? AM_AX_LBI5 : AM_AX_WBI5;
                    break;
                case 6:
                    ap->type = extwfull.f.wl ? AM_AX_LBI6 : AM_AX_WBI6;
                    break;
                }
            } else if ((extwfull.s.bs & extwfull.s.i)==1) { /* both 1 */
                ap->type = AM_AX_noBI;
            } else if (extwfull.s.bs) { /* only BS=1 */
                ap->type = AM_AX_B;
            } else {
                switch(extwfull.f.iis) {
                case 0:
                    ap->type = extwfull.f.wl ? AM_AX_LI0 : AM_AX_WI0;
                    break;
                case 1:
                    ap->type = extwfull.f.wl ? AM_AX_LI1 : AM_AX_WI1;
                    break;
                case 2:
                case 3:
                    ap->type = extwfull.f.wl ? AM_AX_LI2 : AM_AX_WI2;
                    break;
                case 5:
                    ap->type = extwfull.f.wl ? AM_AX_LI5 : AM_AX_WI5;
                    break;
                case 6:
                case 7:
                    ap->type = extwfull.f.wl ? AM_AX_LI6 : AM_AX_WI6;
                    break;
                }
            }
        }
        break;
    case 7:
        switch (reg) {
        case 0:
        case 1:
            ap->type = AM_ABS;
            if (reg == 1) {
                ap->disp1.w[0] = *curw;
                dis_getmore();
            }
            ap->disp1.w[1] = *curw;
            dis_getmore();
            break;
        case 2:
            ap->type = AM_PCIND;
            ap->disp1.i = *curw;
            break;
        case 3:
            ap->type = (*curw & 0x800) ? AM_PCext : AM_PCind8;
            ap->xreg = (extw & 0x7000) >> 12;
            if (*curw & 0x8000)
                ap->xreg += 8;
            ap->scale = dis_scale[(extw & 0x600) >> 9];
            ap->disp1.w[1] = extw & 0xff;
            dis_getmore();
            break;
        case 4:
            ap->type = AM_IMMWL;
            if (sz >= 3) {
                ap->disp1.w[0] = *curw;
                dis_getmore();
            }
            ap->disp1.w[1] = *curw;
            dis_getmore();
            if (sz == 4) {
                ap->type = AM_IMMX;
                ap->immlong[0].i = ap->disp1.i;
                ap->immlong[1].w[0] = *curw;
                dis_getmore();
                ap->immlong[1].w[1] = *curw;
                dis_getmore();
            }
            break;
        }
    }
}

dis_prt_movem_ad(bits)
int bits;
{
    char *comma = "";
    register int i;
    for (i=7; i >= 0; i--) {
        if (bits & 0x8000) {
            printf("%sA%d", comma, i);
            comma = ",";
        }
        bits <<= 1;
    }
    for (i=7; i >= 0; i--) {
        if (bits & 0x8000) {
            printf("%sD%d", comma, i);
            comma = ",";
        }
        bits <<= 1;
    }
}

dis_prt_movem_da(bits)
int bits;
{
    char *comma = "";
    register int i;
    for (i=0; i <= 7; i++) {
        if (bits & 0x8000) {
            printf("%sD%d", comma, i);
            comma = ",";
        }
        bits <<= 1;
    }
    for (i=0; i <= 7; i++) {
        if (bits & 0x8000) {
            printf("%sA%d", comma, i);
            comma = ",";
        }
        bits <<= 1;
    }
}

dis_prt_fmovem(extw)
unsigned short extw;
{
    int i;
    char *comma;
    unsigned char regs;
    union {
        unsigned short i;
        struct {
            unsigned short :3, mode:2, :3, regs:8;
        } f;
    } bits;
    
    bits.i = extw;
    regs = bits.f.regs;
    comma = "";
    if (bits.f.mode >= 2) {
        for (i=0; i <= 7; i++) {
            if (regs & 1) {
                printf("%sFP%d", comma, i);
                comma = ",";
            }
            regs >>= 1;
        }
    } else {
        for (i=0; i <= 7; i++) {
            if (regs & 0x80) {
                printf("%sFP%d", comma, i);
                comma = ",";
            }
            regs <<= 1;
        }
    }
}

/* bitfields */
typedef union {
    unsigned short i;
    unsigned char b[2];
    struct {
        unsigned short :2, sz:2, reg1:3, mod1:3, mod2:3, reg2:3;
    } m;    /* arith reg,modrm or modrm,reg */
    struct {
        unsigned short :4, reg1:3, dr:1, sz:2, ir:1, :2, reg2:3;
    } s;    /* shift ops */
    struct {
        unsigned short :8, sz:2, mod2, :3, reg2:3;
    } xi;   /* immediate ops */
    struct {
        unsigned short :5, dr:1, :3, sz,:1, mod2:3, reg2:3;
    } mm;   /* movem */
    struct {
        unsigned short :4, reg1:3, :1, data:8;
    } q; /* quick */
    struct {
        unsigned short :4, reg1:3, :1, sz:2, :2, rm:1, reg2:3;
    } bx; /* bcd, addx, subx, cmpm */
    struct {
        unsigned short :4, reg1:3, :1, mod1:5, reg2:3;
    } e; /* exg */
    struct {
        unsigned short rm:3, src:3, dst:3, con:7;
    } f; /* float */
    struct {
        unsigned short :1, reg1:3, dof:1, off:5, dw:1, w:5;
    } bf; /* bf ops */
} BITFIELD;

#define MAKEEA1(sz)        dis_makearg(&argbuf, opcode.m.mod1, opcode.m.reg1, sz)
#define MAKEEA2(sz)        dis_makearg(&argbuf, opcode.m.mod2, opcode.m.reg2, sz)
#define MAKEEAM1(mod, sz)  dis_makearg(&argbuf, mod, opcode.m.reg1, sz)
#define MAKEEAM2(mod, sz)  dis_makearg(&argbuf, mod, opcode.m.reg2, sz)
#define PRINTARG()         dis_printarg(&argbuf)
#define PRINTEA1(sz)       MAKEEAM1(opcode.m.mod1,sz); PRINTARG()
#define PRINTEA2(sz)       MAKEEAM2(opcode.m.mod2,sz); PRINTARG()
#define PRINTEAM1(mod, sz) MAKEEAM1(mod, sz); PRINTARG()
#define PRINTEAM2(mod, sz) MAKEEAM2(mod, sz); PRINTARG()

dis_makeargs()
{
    BITFIELD opcode;
    BITFIELD fext;  /* float extw */
    BITFIELD bf;    /* bfops extw */
    int sz;
    int extw;       /* standard extw */
    DISPL disp;     /* word/long displacement */
    char *spec;

    register int idx = dis_makemnemo();
    printf(mnemotbl[idx]);
    opcode.i = *curw;
    dis_getmore();
    
    switch (idx) {
    /* db* ops */
    case DM_DBT:    case DM_DBF:    case DM_DBCC:   case DM_DBCS:
    case DM_DBEQ:   case DM_DBGE:   case DM_DBGT:   case DM_DBHI:
    case DM_DBLE:   case DM_DBLS:   case DM_DBLT:   case DM_DBMI:
    case DM_DBNE:   case DM_DBPL:   case DM_DBVC:   case DM_DBVS:
        printf(" D%d,", opcode.i & 7);
        printf("#%lx", monaddr + *curw + 2);
        dis_getmore();
        break;

    /* branch ops */
    case DM_BRA:    case DM_BSR:    case DM_BCC:    case DM_BCS:
    case DM_BEQ:    case DM_BGE:    case DM_BGT:    case DM_BHI:
    case DM_BLE:    case DM_BLS:    case DM_BLT:    case DM_BMI:
    case DM_BNE:    case DM_BPL:    case DM_BVC:    case DM_BVS:
        if (opcode.b[1] == 0) {
            printf(" ");
            printf("#%lx", monaddr + *curw + 2);
            dis_getmore();
        } else if (opcode.b[1] == 0xff) {
            disp.w[0] = *curw;
            dis_getmore();
            disp.w[1] = *curw;
            dis_getmore();
            printf(" ");
            printf("#%lx", monaddr + disp.i + 2);
        } else {
            printf(" ");
            printf("#%lx", monaddr + (int)opcode.b[0] + 2);
        }
        break;

    case DM_MOVEB: /* move.b */
    case DM_MOVEW: /* move.w */
    case DM_MOVEL: /* move.l */
        printf(" ");
        sz = opcode.m.sz==1 ? 1 : (opcode.m.sz==2 ? 2 : 3);
        PRINTEA2(sz); printf(","); PRINTEA1(0);
        break;

    case DM_EXTW:    /* ext */
    case DM_EXTL:    /* ext */
    case DM_EXTBL:   /* ext */
        printf(" ");
        PRINTEAM2(0, 0);
        break;

    case DM_UNLK:    /* unlk */
        printf(" ");
        PRINTEAM2(1, 0);
        break;

    case DM_LINK:    /* link */
    case DM_LINKW:   /* link */
        printf(" ");
        PRINTEAM2(1, 0);
        if (idx == DM_LINK) {
            argbuf.disp1.w[0] = *curw;
            dis_getmore();
            argbuf.disp1.w[1] = *curw;
            dis_getmore();
            printf(",%ld", argbuf.disp1);
        } else {
            printf(",%d", *curw);
            dis_getmore();
        }
        break;

    case DM_BCLR:    /* bclr */
    case DM_BSET:    /* bset */
    case DM_BTST:    /* btst */
        if (opcode.i & 0x100)
            printf(" D%d,", opcode.m.reg1);
        else
            printf(" @%d,", *curw); /* BUG? sz uninitialized? */
        PRINTEA2(sz);
        break;

    case DM_OR:   /* or */
    case DM_SUB:  /* sub */
    case DM_EOR:  /* eor */
    case DM_CMP:  /* cmp */
    case DM_AND:  /* and */
    case DM_ADD:  /* add */
        switch(opcode.m.mod1) { /*op mode */
        case 0:
        case 4:
            sz = 1;
            printf(".b");
            break;
        case 1:
        case 3:
        case 5:
            sz = 2;
            printf(".w");
            break;
        case 2:
        case 6:
        case 7:
            sz = 3;
            printf(".l");
            break;
        }
        if (opcode.m.mod1 < 4 || opcode.m.mod1==7) {
            PRINTEA2(sz);
            printf(",");
            if (opcode.m.mod1==3 || opcode.m.mod1==7)
                MAKEEAM2(1, sz);
            else
                MAKEEAM2(0, sz);
        } else {
            PRINTEAM1(0, sz);
            printf(",");
            MAKEEA2(sz);
        }
        PRINTARG();
        break;

    case DM_ASR:   /* asr */
    case DM_ASL:   /* asl */
    case DM_LSR:   /* lsr */
    case DM_LSL:   /* lsl */
    case DM_ROXR:  /* roxr*/
    case DM_ROXL:  /* roxl */
    case DM_ROR:   /* ror */
    case DM_ROL:   /* rol */
        switch(opcode.s.sz) {
        case 0:
            sz = 1;
            printf(".b ");
            break;
        case 1:
            sz = 2;
            printf(".w ");
            break;
        case 2:
            sz = 3;
            printf(".l ");
            break;
        }
        if (opcode.s.ir==0) {
            argbuf.disp1.i = opcode.s.reg1==0 ? 8 : opcode.s.reg1; /* number 1..8 */
            argbuf.reg = 0;
            argbuf.xreg = 0;
            argbuf.scale = 0;
            argbuf.type = AM_IMMWL;
        } else {
            argbuf.disp1.i = 0;
            argbuf.reg = opcode.s.reg1;
            argbuf.xreg = 0;
            argbuf.scale = 0;
            argbuf.type = AM_DX;
        }
        PRINTARG();
        printf(",");
        PRINTEAM2(0, sz);
        break;

    case DM_NBCD:   /* nbcd */
    case DM_PEA:    /* pea */
    case DM_TAS:    /* tas */
    case DM_JMP:    /* jmp */ 
    case DM_JSR:    /* jsr */
    case DM_ASR_0:  /* asr */
    case DM_ASL_0:  /* asl */
    case DM_LSR_0:  /* lsr */
    case DM_LSL_0:  /* lsl */
    case DM_ROXR_0: /* roxr */
    case DM_ROXL_0: /* roxl */
    case DM_ROR_0:  /* ror */
    case DM_ROL_0:  /* rol */
        printf(" ");
        PRINTEA2(2);
        break;

    case DM_ORI:     /* ori */
    case DM_ANDI:    /* andi */
    case DM_SUBI:    /* subi */
    case DM_ADDI:    /* addi */
    case DM_EORI:    /* eori */
    case DM_CMPI:    /* )cmpi */
        sz = opcode.xi.sz==0 ? 1 : (opcode.xi.sz==1 ? 2 : 3);
        if (sz == 3) {
            disp.w[0] = *curw;
            dis_getmore();
            disp.w[1] = *curw;
            dis_getmore();
            printf(".l @%ld,", disp.i);
        } else {
            disp.w[1] = *curw;
            dis_getmore();
            if (sz == 1)
                printf(".b @%d,", disp.w[1]);
            else
                printf(".w @%d,", disp.w[1]);
        }
        PRINTEA2(sz);
        break;

    case DM_SUBQ:    /* subq */
    case DM_ADDQ:    /* addq */
        sz = opcode.xi.sz==0 ? 1 : (opcode.xi.sz==1 ? 2 : 3);
        if (sz == 1)
            printf(".b");
        else if (sz == 2)
            printf(".w");
        else 
            printf(".l");
        if (opcode.s.reg1==0)
            printf(" @8,");
        else
            printf(" @%d,", opcode.s.reg1);
        PRINTEA2(sz);
        break;

    case DM_NEGX:   /* nexg */
    case DM_CLR:    /* clr */
    case DM_NEG:    /* neg */
    case DM_NOT:    /* not */
    case DM_TST:    /* tst */
        sz = opcode.xi.sz==0 ? 1 : (opcode.xi.sz==1 ? 2 : 3);
        if (sz == 1)
            printf(".b");
        else if (sz == 2)
            printf(".w");
        else 
            printf(".l");
        PRINTEA2(sz);
        break;

    case DM_MOVEM:    /* movem */
    case DM_MOVEM_0:  /* movem */
        sz = opcode.mm.sz==0 ? 2 : 3;
        if (sz == 2)
            printf(".w ");
        else
            printf(".l ");
        if (idx == DM_MOVEM) {
            extw = *curw;
            dis_getmore();
            printf("<");
            if (opcode.m.mod2==4)
                dis_prt_movem_da(extw);
            else
                dis_prt_movem_ad(extw);
            printf(">,");
            PRINTEA2(sz);
        } else {
            extw = *curw;
            dis_getmore();
            PRINTEA2(sz);
            printf(",<");
            dis_prt_movem_ad(extw);
            printf(">");
        }
        break;

    case DM_MOVES:    /*moves */
        sz = opcode.mm.sz==0 ? 2 : 3;
        if (sz == 1)
            printf(".b");
        else if (sz == 2)
            printf(".w");
        else 
            printf(".l");
        extw = *curw;
        dis_getmore();
        if ((extw & 0x800)==0) { /* dr bit */
            PRINTEA2(sz);
            printf(",");
            extw >>= 12; /* move reg1 in extw into bits 0-3 */
            if ((extw & 8)==0)
                printf("D%d", extw & 7);
            else
                printf("A%d", extw & 7);
        } else {
            extw &= 12;
            if ((extw & 8)==0)
                printf("D%d,", extw & 7);
            else
                printf("A%d,", extw & 7);
            PRINTEA2(sz);
        }
        break;

    case DM_LEA:    /* lea */
    case DM_CHK:    /* chk */
    case DM_DIVSW:  /* divs.w */
    case DM_DIVUW:  /* divu.w */
    case DM_MULSW:  /* muls.w */
    case DM_MULUW:  /* mulu.w */
        printf(" ");
        PRINTEA2(0);
        if (idx == DM_LEA)
            printf(",A%d", opcode.s.reg1);
        else
            printf(",A%d", opcode.s.reg1);
        break;

    case DM_MOVEQ:    /* moveq */
        printf(" @%d,D%d", opcode.q.reg1, opcode.q.data);
        break;

    case DM_STOP:   /* stop */
    case DM_RTD:    /* rtd */
        printf(" %d", *curw);
        dis_getmore();
        break;

    case DM_SBCD:    /* sbcd */
    case DM_ABCD:    /* abcd */
    case DM_ABCD_0:  /* abcd */
        if (opcode.bx.rm==0)
            printf(" D%d,D%d", opcode.bx.reg2, opcode.bx.reg1);
        else
            printf(" -(A%d),-(A%d)", opcode.bx.reg2, opcode.bx.reg1);
        break;

    case DM_SUBX:   /* subx */
    case DM_ADDX:   /* addx */
        if (opcode.bx.sz == 1)
            printf(".b");
        else if (opcode.bx.sz == 2)
            printf(".w");
        else 
            printf(".l");
        if (opcode.bx.rm==0)
            printf(" D%d,D%d", opcode.bx.reg2, opcode.bx.reg1);
        else
            printf(" -(A%d),-(A%d)", opcode.bx.reg2, opcode.bx.reg1);
        break;

    case DM_S:    /* sCC */
        printf("%s ", dis_scond[(opcode.i & 0xf00) >> 8]);
        PRINTEA2(1);
        break;

    case DM_MOVEP: /* movep */
        switch (opcode.m.mod1) {
        case 4:
            print(".w %d(A%d),D%d", *curw, opcode.m.reg2, opcode.m.reg1);
            break;
        case 5:
            print(".l %d(A%d),D%d", *curw, opcode.m.reg2, opcode.m.reg1);
            break;
        case 6:
            /* BUG! should be opcode.x.reg2, *curw, opcode.x.reg1 */
            print(".w %D%d,%d(A%d)", *curw, opcode.m.reg2, opcode.m.reg1);
            break;
        case 7:
            /* BUG! should be opcode.x.reg2, *curw, opcode.x.reg1 */
            print(".w %D%d,%d(A%d)", *curw, opcode.m.reg2, opcode.m.reg1);
            break;
        }
        dis_getmore();
        break;

    case DM_SWAP:    /* swap */
        printf(" D%d", opcode.m.reg2);
        break;

    case DM_TRAP:    /* trap */
        printf(" %d", opcode.i & 0x0f);
        break;
    
    case DM_BKPT:    /* bkpt */
        printf(" %d", opcode.i & 0x0f);
        break;

    case DM_MOVEC:    /* movec */
        switch (*curw & 0xfff) {
        case 0x000:
            spec = "SFC"; break;
        case 0x001:
            spec = "DFC"; break;
        case 0x002:
            spec = "CACR"; break;
        case 0x800:
            spec = "USP"; break;
        case 0x801:
            spec = "VBR"; break;
        case 0x802:
            spec = "CAAR"; break;
        case 0x803:
            spec = "MSP"; break;
        case 0x804:
            spec = "ISP"; break;
        default:
            spec = "???"; break;
        }
        extw = (*curw & 0xf000) >> 12;
        dis_getmore();
        if ((opcode.i & 1)==0) {
            printf(" %s", spec);
            if ((extw & 8)==0)
                printf("D%d", extw & 7);
            else
                printf("A%d", extw & 7);
        } else {
            if ((extw & 8)==0)
                printf("D%d", extw & 7);
            else
                printf("A%d", extw & 7);
            printf(",%s", spec);
        }
        break;

    case DM_CMPM:   /* cmpm */
        if (opcode.bx.sz==1)
            printf(".b ");
        else if (opcode.bx.sz==2)
            printf(".w ");
        else
            printf(".l ");
        printf(" (A%d)+,(A%d)+", opcode.bx.reg2, opcode.bx.reg1);
        break;

    case DM_MOVE:    /* move SR, ea */
    case DM_MOVE_0:  /* move CCR, ea */
        MAKEEA2(2);
        if (idx == DM_MOVE)
            spec = "SR";
        else
            spec = "CCR";
        printf(" %s,", spec);
        PRINTEA();
        break;

    case DM_MOVE_1:    /* move ea, CCR */
    case DM_MOVE_2:    /* move ea, SR */
        printf(" ");
        PRINTEA2(2);
        if (idx == DM_MOVE_2)
            spec = "SR";
        else
            spec = "CCR";
        printf(",%s", spec);
        break;

    case DM_MOVE_3:    /* move USP,An */
        printf(" USP,A%d", opcode.m.reg2);
        break;

    case DM_MOVE_4:    /* move An,USP */
        printf(" A%d,USP", opcode.m.reg2);
        break;

    case DM_EXGM:   /* exg */
    case DM_EXGA:   /* exg */
    case DM_EXGD:   /* exg */
        printf(" ");
        if (opcode.e.mod1 == 0x01001)
            printf("A%d,", opcode.e.reg1);
        else
            printf("D%d,", opcode.e.reg1);
        if (opcode.e.mod1 == 0x01000)
            printf("D%d", opcode.e.reg2);
        else
            printf("A%d", opcode.e.reg2);
        break;

    case DM_MULS:    /* muls */
    case DM_MULU:    /* mulu */
    case DM_DIVS:    /* divs */
    case DM_DIVU:    /* divu */
        extw = *curw;
        dis_getmore();
        if ((extw & 0x400)==0)
            printf(".l ");
        else
            printf(".q ");
        PRINTEA2(3);
        if (instbuf[2] & 0x400)
            printf(",D%d:D%d", (extw & 0x7000)>>12, extw & 7);
        else if (idx == DM_MULU || idx == DM_MULS)
            printf(",D%d", (extw & 0x7000)>>12);
        else if ((instbuf[2] >> 12) != (instbuf[2] & 7))
            printf(",D%d:D%d", (extw & 0x7000)>>12, extw & 7);
        else
            printf(",D%d", (extw & 0x7000)>>12);
        break;

    case DM_BFTST:   /* bftst */
    case DM_BFCHG:   /* bfchg */
    case DM_BFCLR:   /* bfclr */
    case DM_BFSET:   /* bfset */
        printf(" ");
        bf.i = *curw;
        dis_getmore();
        PRINTEA2(0);
        if (bf.bf.dof)
            printf(",D%d,", bf.bf.off & 7);
        else
            printf(",@%d,", bf.bf.off);
        if (bf.bf.dw)
            printf("D%d", bf.bf.w & 7);
        else if (bf.bf.w==0)
            printf("@32");
        else
            printf("@%d", bf.bf.w);
        break;

    case DM_BFEXTU:   /* bfextu */
    case DM_BFEXTS:   /* bfexts */
    case DM_BFFFO:    /* bfffo */
    case DM_BFINS:    /* bfins */
        printf(" ");
        bf.i = *curw;
        dis_getmore();
        PRINTEA2(0);
        if (idx == DM_BFINS)
            printf(",D%d", bf.bf.reg1);
        if (bf.bf.dof)
            printf("D%d,", bf.bf.off & 7);
        else
            printf(",@%d:", bf.bf.off);
        if (bf.bf.dw)
            printf("D%d", bf.bf.w & 7);
        else if (bf.bf.w==0)
            printf("@32");
        else if (idx == DM_BFINS)
            printf("@%d", bf.bf.w);
        else
            printf("@%d,D%d", bf.bf.w, bf.bf.reg1);
        break;

    case DM_PACK:   /* pack */
    case DM_UNPK:   /* unpk */
        if (opcode.bx.rm==0)
            printf("D%d,D%d", opcode.bx.reg2, opcode.bx.reg1);
        else
            printf("A%d@-,A%d@-", opcode.bx.reg2, opcode.bx.reg1);
        printf("@%d", *curw);
        dis_getmore();
        break;

    case DM_FBEQ:   /* fbeq */
    case DM_FBNE:   /* fbne */
    case DM_FBGE:   /* fbge */
    case DM_FBLT:   /* fblt */
    case DM_FBLE:   /* fble */
    case DM_FBGT:   /* fbgt */
        extw = *curw;
        dis_getmore();
        printf(" ");
        printf("#%lx", monaddr + (int)instbuf[2]);
        break;

    case DM_FSEQ:   /* fseq */
    case DM_FSNE:   /* fsne */
    case DM_FSGE:   /* fsge */
    case DM_FSLT:   /* fslt */
    case DM_FSLE:   /* fsle */
    case DM_FSGT:   /* fsgt */
        printf("D%d", instbuf[1] & 7);
        break;

    case DM_FMOVE:    /* fmove */
    case DM_FADD:     /* fadd */
    case DM_FSUB:     /* fsub */
    case DM_FMUL:     /* fmul */
    case DM_FDIV:     /* fdiv */
    case DM_FREM:     /* frem */
    case DM_FCMP:     /* fcmp */
    case DM_FTST:     /* ftst */
    case DM_FNEG:     /* fneg */
    case DM_FSINH:    /* fsinh */
    case DM_FSQRT:    /* fsqrt */
    case DM_FTANH:    /* ftanh */
    case DM_FATAN:    /* fatan */
    case DM_FASIN:    /* fasin */
    case DM_FATANH:   /* fatanh */
    case DM_FSIN:     /* fsin */
    case DM_FTAN:     /* ftan */
    case DM_FETOX:    /* fetox */
    case DM_FTWOTOX:  /* ftwotox */
    case DM_FTENTOX:  /* ftentox */
    case DM_FLOGN:    /* flogn */
    case DM_FLOG10:   /* flog10 */
    case DM_FLOG2:    /* flog2 */
    case DM_FABS:     /* fabs */
    case DM_FCOSH:    /* fcosh */
    case DM_FACOS:    /* facos */
    case DM_FCOS:     /* fcos */
    case DM_FINT:     /* fint */
    case DM_FINTRZ:   /* fintrz */
    case DM_FGETEXP:  /* fgetexp */
    case DM_FGETMAN:   /* fgetman */
    case DM_FSGLDIV:  /* fsgldiv */
    case DM_FSGLMUL:  /* fsglmul */
        fext.i = *curw;
        dis_getmore();
        if (fext.f.rm) {
            switch(fext.f.src) {
            case 0:
                sz = 3;
                printf(".l");
                break;
            case 1:
                sz = 3;
                printf(".s");
                break;
            case 2:
                sz = 5;
                printf(".x");
                break;
            case 4:
                sz = 2;
                printf(".w");
                break;
            case 5:
                sz = 4;
                printf(".d");
                break;
            case 6:
                sz = 1;
                printf(".b");
                break;
            default:
                break;
            }
        } else {
            printf(".x");
        }
        switch (fext.f.rm) {
        case 0:
            if (idx == DM_FTST)
                printf("FP%d", fext.f.src);
            else
                printf("FP%d,FP%d", fext.f.src, fext.f.dst);
            break;
        case 2:
            PRINTEA2(sz);
            if (idx != DM_FTST)
                printf(",FP%d", fext.f.dst);
            break;
        case 3:
            MAKEEA2(sz);
            printf("FP%d,", fext.f.dst);
            PRINTARG();
        }
        break;

    case DM_FMOVECR:   /* fmovecr */
        fext.i = *--curw;
        dis_getmore();
        printf(".x @%d,FP%d", fext.f.con, fext.f.dst);
        break;

    case DM_FMOVEM:   /* fmovem */
        printf(".x");
        sz = 5;
        fext.i = *curw;
        dis_getmore();
        if (fext.f.rm==6) {
            PRINTEA2(sz);
            printf(",<");
            dis_prt_fmovem(fext.i);
            printf(">");
        } else {
            printf("<");
            dis_prt_fmovem(fext.i);
            printf(">,");
            PRINTEA2(sz);
        }
        /*FALLTHRU*/
    default:
        break;
    }
}

int dis_makemnemo()
{
    switch (*curw & 0xf000) {
    case 0x0000:
        if ((*curw & 0x38)==8)
            return DM_MOVEP;
        else if ((*curw & 0x09c0)==0x00c0) {
            dis_getmore();
            return (*curw-- & 0x0800)==0x0800 ? DM_CHK2 : DM_CMP2;
        } else {
            switch (*curw & 0xffc0) {
            case 0x0880:
                return DM_BCLR;
            case 0x08c0:
                return DM_BSET;
            case 0x0800:
                return DM_BTST;
            default:
                switch (*curw & 0x1c0) {
                case 0x0180:
                    return DM_BCLR;
                case 0x01c0:
                    return DM_BSET;
                case 0x0100:
                    return DM_BTST;
                default:
                    switch (*curw & 0xf00) {
                    case 0x0000:
                        return DM_ORI;
                    case 0x0200:
                        return DM_ANDI;
                    case 0x0400:
                        return DM_SUBI;
                    case 0x0600:
                        if ((*curw & 0xf0)==0xc0)
                            return DM_RTM;
                        else if ((*curw & 0xc0)==0xc0)
                            return DM_CALLM;
                        else
                            return DM_ADDI;
                    case 0x0a00:
                        return DM_EORI;
                    case 0x0c00:
                        return DM_CMPI;
                    case 0x0e00:
                        return DM_MOVES;
                    }
                }
            }
        }
        return 0;
    case 0x1000:
        return DM_MOVEB;
    case 0x2000:
        return DM_MOVEW;
    case 0x3000:
        return DM_MOVEL;
    case 0x4000:
        if ((*curw & 0x100)==0x0100) {
            if ((*curw & 0xc0)==0xc0) {
                if ((*curw & 0x38)==0) 
                    return DM_EXTBL;
                else
                    return DM_LEA;
            } else
                return DM_CHK;
            
        } else {
            switch(*curw & 0xf00) {
            case 0x000:
                return (*curw & 0xc0)==0xc0 ? DM_MOVE : DM_NEGX;
            case 0x200:
                return (*curw & 0xc0)==0xc0 ? DM_MOVE_0 : DM_CLR;
            case 0x400:
                return (*curw & 0xc0)==0xc0 ? DM_MOVE_1 : DM_NEG;
            case 0x600:
                return (*curw & 0xc0)==0xc0 ? DM_MOVE_2 : DM_NOT;
            case 0x800:
                switch (*curw & 0xf8) {
                case 0x08:
                    return DM_LINK;
                case 0x40:
                    return DM_SWAP;
                case 0x48:
                    return DM_BKPT;
                case 0x80:
                    return DM_EXTW;
                case 0xc0:
                    return DM_EXTL;
                default:
                    if ((*curw & 0xc0)==0)
                        return DM_NBCD;
                    else if ((*curw & 0xc0) == 0x40)
                        return DM_PEA;
                    else
                        return DM_MOVEM;
                }
            case 0xa00:
                return ((*curw & 0xc0)==0xc0) ? DM_TAS : DM_TST;
            case 0xc00:
                if ((*curw & 0xc0)==0) {
                    dis_getmore();
                    return (*curw-- & 0x800)==0x800 ? DM_MULS : DM_MULU;
                } else if ((*curw & 0xc0)==0x40) {
                    dis_getmore();
                    return (*curw-- & 0x800)==0x800 ? DM_DIVS : DM_DIVU;
                } else
                    return DM_MOVEM_0;
            case 0xe00:
                switch(*curw & 0xf0) {
                case 0x40:
                    return DM_TRAP;
                case 0x50:
                    return (*curw & 8)==8 ? DM_UNLK : DM_LINKW;
                case 0x60:
                    return (*curw & 8)==8 ? DM_MOVE_3 : DM_MOVE_4;
                case 0x70:
                    switch (*curw & 0x0f) {
                    case 0:
                        return DM_RESET;
                    case 1:
                        return DM_NOP;
                    case 2:
                        return DM_STOP;
                    case 3:
                        return DM_RTE;
                    case 4:
                        return DM_RTD;
                    case 5:
                        return DM_RTS;
                    case 6:
                        return DM_TRAPV;
                    case 7:
                        return DM_RTR;
                    default:
                        return DM_MOVEC;
                    }
                default:
                    return (*curw & 0xc0)==0xc0 ? DM_JMP : DM_JSR;
                }
            }
            return 0;
        }

    case 0x5000:
        if ((*curw & 0xf8)==0xf8)
            return DM_TRAP_0;
        else if ((*curw & 0xf8)==0xc8) {
            switch (*curw & 0xf00) {
            case 0x000:
                return DM_DBT;
            case 0x100:
                return DM_DBF;
            case 0x400:
                return DM_DBCC;
            case 0x500:
                return DM_DBCS;
            case 0x700:
                return DM_DBEQ;
            case 0xc00:
                return DM_DBGE;
            case 0xe00:
                return DM_DBGT;
            case 0x200:
                return DM_DBHI;
            case 0xf00:
                return DM_DBLE;
            case 0x300:
                return DM_DBLS;
            case 0xd00:
                return DM_DBLT;
            case 0x0b00:
                return DM_DBMI;
            case 0x600:
                return DM_DBNE;
            case 0xa00:
                return DM_DBPL;
            case 0x800:
                return DM_DBVC;
            case 0x900:
                return DM_DBVS;
            default:
                return 0;
            }
        } else if ((*curw & 0xc0)==0xc0)
            return DM_S;
        else if ((*curw & 0x100)==0x100)
            return DM_SUBQ;
        else
            return DM_ADDQ;
    case 0x6000:
        switch (*curw & 0xf00) {
        case 0x000:
            return DM_BRA;
        case 0x100:
            return DM_BSR;
        case 0x400:
            return DM_BCC;
        case 0x500:
            return DM_BCS;
        case 0x700:
            return DM_BEQ;
        case 0xc00:
            return DM_BGE;
        case 0xe00:
            return DM_BGT;
        case 0x200:
            return DM_BHI;
        case 0xf00:
            return DM_BLE;
        case 0x300:
            return DM_BLS;
        case 0xd00:
            return DM_BLT;
        case 0xb00:
            return DM_BMI;
        case 0x600:
            return DM_BNE;
        case 0xa00:
            return DM_BPL;
        case 0x800:
            return DM_BVC;
        case 0x900:
            return DM_BVS;
        default:
            return 0;
        }
    case 0x7000:
        return DM_MOVEQ;
    case 0x8000:
        if ((*curw & 0x1f0)==0x100)
            return DM_SBCD;
        else if ((*curw & 0x1f0)==0x140)
            return DM_PACK;
        else if ((*curw & 0x1f0)==0x180)
            return DM_UNPK;
        else if ((*curw & 0x1c0)==0x1c0)
            return DM_DIVSW;
        else if ((*curw & 0x1c0)==0x0c0)
            return DM_DIVUW;
        else
            return DM_OR;
    case 0x9000:
        if ((*curw & 0x1f0)!=0x100 && (*curw &0x1f0)!=0x140 && (*curw & 0x1f0)!=0x180)
            return DM_SUBX;
        else
            return DM_SUB;
    case 0xa000:
        return DM_UNASSIGNED;
    case 0xb000:
        if ((*curw & 0x1f8)!=0x108 && (*curw &0x1f8)!=0x148 && (*curw & 0x1f8)!=0x188)
            return DM_CMPM;
        else if ((*curw & 0x1c0)==0x1c0 || (*curw & 0x1c0)==0x0c0)
            return DM_CMP;
        else if ((*curw & 0x100)==0x100)
            return DM_EOR;
        else DM_CMP;
    case 0xc000:
        switch (*curw & 0x1f8) {
        case 0x188:
            return DM_EXGM;
        case 0x148:
            return DM_EXGA;
        case 0x140:
            return DM_EXGD;
        case 0x108:
            return DM_ABCD;
        case 0x100:
            return DM_ABCD_0;
        default:
            if ((*curw & 0x1c0)==0x1c0)
                return DM_MULSW;
            else if ((*curw & 0x1c0)==0x0c0)
                return DM_MULUW;
            else
                return DM_AND;
        }
    case 0xd000:
        if ((*curw & 0x130)==0x100)
            return DM_ADDX;
        else
            return DM_ADD;
    case 0xe000:
        switch (*curw & 0xfc0) {
        case 0x8c0:
            return DM_BFTST;
        case 0x9c0:
            return DM_BFEXTU;
        case 0xac0:
            return DM_BFCHG;
        case 0xbc0:
            return DM_BFEXTS;
        case 0xcc0:
            return DM_BFCLR;
        case 0xdc0:
            return DM_BFFFO;
        case 0xec0:
            return DM_BFSET;
        case 0xfc0:
            return DM_BFINS;
        case 0x0c0:
            return DM_ASR;
        case 0x1c0:
            return DM_ASL;
        case 0x2c0:
            return DM_LSR;
        case 0x3c0:
            return DM_LSL;
        case 0x4c0:
            return DM_ROXR;
        case 0x5c0:
            return DM_ROXL;
        case 0x6c0:
            return DM_ROR;
        case 0x7c0:
            return DM_ROL;
        default:
            switch (*curw & 0x118) {
            case 0x000:
                return DM_ASR_0;
            case 0x100:
                return DM_ASL_0;
            case 0x008:
                return DM_LSR_0;
            case 0x108:
                return DM_LSL_0;
            case 0x010:
                return DM_ROXR_0;
            case 0x110:
                return DM_ROXL_0;
            case 0x018:
                return DM_ROR_0;
            case 0x118:
                return DM_ROL_0;
            default:
                return 0;
            }
        }
    case 0xf000:
        switch (*curw & 0xf3c0) {
        case 0xf200:
            fbfsflg = 0;
            if ((*curw & 0xfc00)==0x5c00)
                return DM_FMOVECR;
            else {
                switch(*curw-- & 0xe000) {
                case 0x0000:
                case 0x4000:
                    dis_getmore();
                    switch (*curw-- & 0x7f) {
                    case 0:     return DM_FMOVE;
                    case 1:     return DM_FINT;
                    case 2:     return DM_FSINH;
                    case 3:     return DM_FINTRZ;
                    case 4:     return DM_FSQRT;
                    case 9:     return DM_FTANH;
                    case 10:    return DM_FATAN;
                    case 12:    return DM_FASIN;
                    case 13:    return DM_FATANH;
                    case 14:    return DM_FSIN;
                    case 15:    return DM_FTAN;
                    case 16:    return DM_FETOX;
                    case 17:    return DM_FTWOTOX;
                    case 18:    return DM_FTENTOX;
                    case 20:    return DM_FLOGN;
                    case 21:    return DM_FLOG10;
                    case 22:    return DM_FLOG2;
                    case 24:    return DM_FABS;
                    case 25:    return DM_FCOSH;
                    case 26:    return DM_FNEG;
                    case 28:    return DM_FACOS;
                    case 29:    return DM_FCOS;
                    case 30:    return DM_FGETEXP;
                    case 31:    return DM_FGETMAN;
                    case 32:    return DM_FDIV;
                    case 33:    return DM_FREM;
                    case 34:    return DM_FADD;
                    case 35:    return DM_FMUL;
                    case 36:    return DM_FSGLDIV;
                    case 39:    return DM_FSGLMUL;
                    case 40:    return DM_FSUB;
                    case 56:    return DM_FCMP;
                    case 58:    return DM_FTST;
                    default:    return 0;
                    }
                case 0x6000:
                    return DM_FMOVE;
                case 0x8000:
                case 0xa000:
                    return 0;
                case 0xc000:
                case 0xe000:
                    return DM_FMOVEM;
                default:
                    return 0;
                }
            }
        case 0xf280:
            fbfsflg = 1;
            /*FALLTHRU*/
        case 0xf240:
            switch (*curw & 0x3f) {
            case 0x01:
                return fbfsflg ? DM_FBEQ : DM_FSEQ;
            case 0x0e:
                return fbfsflg ? DM_FBNE : DM_FSNE;
            case 0x12:
                return fbfsflg ? DM_FBGT : DM_FSGT;
            case 0x13:
                return fbfsflg ? DM_FBGE : DM_FSGE;
            case 0x14:
                return fbfsflg ? DM_FBLT : DM_FSLT;
            case 0x15:
                return fbfsflg ? DM_FBLE : DM_FSLE;
            default:
                return 0;
            }
        case 0xf300:
            return 0;
        default:
            return 0;
        }
    default:
        return 0;
    }
}

dis_printarg(ap)
register struct disarg *ap;
{
    prarg = 0;
    switch (ap->type) {
    case AM_unused38:
        printf("%s", dis_regs[ap->reg]);
        break;
    case AM_ABS:
        printf("#%lx", ap->disp1.i);
        break;
    case AM_AX_WI0:
    case AM_AX_LI0:
        printf("#%lx", ap->disp1.i);
        printf("(,%s%s*%s)", dis_regs[ap->reg],
            ap->type== AM_AX_WI0 ? ".W" : ".L", dis_scale[ap->scale]);
        break;
    case AM_DX:
        printf("%s", dis_regs[ap->reg]);
        break;
    case AM_AXind:
        printf("(%s)", dis_regs[ap->reg]);
        break;
    case AM_AXinc:
        printf("(%s)+", dis_regs[ap->reg]);
        break;
    case AM_AXdec:
        printf("-(%s)", dis_regs[ap->reg]);
        break;
    case AM_AXdisp16:
        printf("(%d,%s)", ap->disp1.w, dis_regs[ap->reg]);
        break;
    case AM_AX_WBI6:
    case AM_AX_LBI6:
    case AM_AX_WBI5:
    case AM_AX_LBI5:
    case AM_AX_B:
    case AM_unused24:
        printf("([%ld,", ap->disp1.i);
        printf("%s]", dis_regs[ap->reg]);
        switch (ap->type) {
        case AM_AX_WBI6:
        case AM_AX_LBI6:
        case AM_AX_WBI5:
        case AM_AX_LBI5:
            prarg = 1;
            break;
        default:
            prarg = 0;
            break;
        }
        if (prarg) {
            printf(",%s%s*%s", dis_regs[ap->xreg],
                (ap->type==AM_AX_WBI6 || ap->type==AM_AX_WBI5) ? ".W" : ".L",
                dis_scale[ap->scale]);
        } else if (ap->disp2.i)
            printf(",%ld", ap->disp2.i);
        printf(")");
        break;
    case AM_AX_WBI2:
    case AM_AX_LBI2:
    case AM_AX_WBI1:
    case AM_AX_LBI1:
    case AM_unused2:
    case AM_unused3:
        printf("([");
        printf("%ld,", ap->disp1.i);
        printf("%s,%s%s*%s]", 
            dis_regs[ap->reg], dis_regs[ap->xreg],
            (ap->type==AM_AX_WBI2 || ap->type==AM_AX_WBI1 || ap->type==AM_unused2)
                ? ".W" : ".L", dis_scale[ap->scale]);
        if (ap->disp2.i)
            printf(",%ld", ap->disp2.i);
        printf(")");
        break;
    case AM_AX_WI5:
    case AM_AX_LI5:
    case AM_AX_WI6:
    case AM_AX_LI6:
    case AM_AX_noBI:
        printf("([");
        printf("#%lx", ap->disp1);
        printf(",,]");
        if (ap->type != AM_AX_noBI) {
            printf(",%s%s*%s", dis_regs[ap->xreg],
                ap->type==AM_AX_WI5 || ap->type==AM_AX_WI6 ? ".W" : ".L",
            dis_scale[ap->scale]);
        }
        if (ap->disp2.i)
            printf(",%ld", ap->disp2.i);
        printf(")");
        break;
    case AM_AX_WI2:
    case AM_AX_LI2:
    case AM_AX_WI1:
    case AM_AX_LI1:
        printf("([");
        printf("#%lx", ap->disp1.i);
        printf(",%s%s*%s]", dis_regs[ap->xreg],
            ap->type==AM_AX_WI2 || ap->type== AM_AX_WI1 ? ".W" : ".L",
            dis_scale[ap->scale]);
        if (ap->type==AM_AX_LI2 || ap->type==AM_AX_WI2)
            printf(",%ld", ap->disp2.i);
        printf(")");
        break;
    case AM_AX_XWind8:
    case AM_AX_XLind8:
        if (ap->disp1.i) {
            if (ap->disp1.w[1] > 0xff80 && ap->disp1.w[1] < 0x7f)
                printf("(%d", (int)ap->disp1.b);
            else
                printf("(%d", (int)ap->disp1.w);
        } else
            printf("(0");
        printf(",%s,%s%s*%s)", dis_regs[ap->reg], dis_regs[ap->xreg],
            ap->type==AM_AX_XWind8 ? ".W" : ".L",
            dis_scale[ap->scale]);
        break;

    case AM_PCind8:
    case AM_PCext:
        printf("(%d,PC,%s%s*%s)",
            ap->disp1.w[1], dis_regs[ap->xreg], 
            ap->type == AM_PCind8 ? ".W" : ".L",
            dis_scale[ap->scale]);
        break;
    case AM_PCIND:
        printf("(%d,PC)", ap->disp1.w[1]);
        break;
    case AM_IMMWL:
        if (ap->disp1.i >= 0xffffff00 && ap->disp1.i < 256)
            printf("@%ld", ap->disp1.i);
        else if (ap->disp1.w[0]==0)
            printf("@%d", ap->disp1.i & 0xffff);
        else
            printf("@#%lx", ap->disp1.i);
        break;
    case AM_IMMX:
        printf("@#%08lx%08lx", ap->immlong[0], ap->immlong[1]);
        break;
    default:
        printf(" addrmode=%d\n", ap->type);
        break;
    }
}

int dis_nextword()
{
    int ret = mem_readword(monaddr+ioffset);
    ioffset++;
    return ret;
}

mon_disassemble(cnt)
register int cnt;
{
    while(cnt-- >= 0) {
        ioffset = 0;
        instbufcnt = 0;
        dis_getmore();
        printf("#%8lx:  ", monaddr);
        do_disassemble();
        putchar('\n');
        ioffset -= 2;
        monaddr + = ioffset;
    }
}
