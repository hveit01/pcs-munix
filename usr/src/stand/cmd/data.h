#ifndef __data_h__
#define __data_h__

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/inode.h>
#include <sys/ino.h>
#include <sys/dir.h>
#include <sys/reg.h>
#include <sys/page.h>
#include <setjmp.h>

/* some absolute addresses */
/*define SYSVA  0x3f800000 - defined in page.h */
#define SYSRAM  0x3f8f0000
#define ICCMBX  0x3f8f7000
#define con_current ((int*)0x3f8ffffc)

#define HIGHMEM 0x3f900000
#define HIGHEND 0x3fe00000

#define MINITOR 0x3ff80000
#define PTBR    0x3ffc0000
#define _ptbr   ((short*)PTBR)
#define PCR     0x3ffe8000
#define ESR     0x3ffe8002
#define PTEC    0x3ffc8000
#define SPT     0x3ffca000
#define _sbrpte ((spte_t*)SPT)

#define CCR     0x3ffe8006
#define _ccr    *(short*)CCR
#define clrcache() { _ccr &= ~4; }
#define MFP     0x3ffe0000
#define PBC     0x3ffe8004
#define _pbcr   (short*)PBC
#define DPT     0x3ffd0000
#define _dmapt  ((pte_t*)DPT)
#define LRU     0x3ffd8000

/* devices on QBUS */
#define DH11ADDR    0x3fffe010
#define ICCADDR     0x3fffe300
#define TSADDR      0x3ffff550
#define DLV11ADDR   0x3ffffd48
/*#define SWADDR      0x3ffffdc0 - defined in sw.h */
#define HKADDR      0x3fffff20
#define STADDR      0x3fffff80
#define STDATA      0x3fffffa0
#define BIPADDR     0x3fe00000
#define BIPGO       (BIPADDR + 0x3ffe0) /* offset of ctrl reg */
#define COLORRAM    0xb0000000
#define COLORCTL    0xf0000000


typedef int (*funcptr)();

struct devtbl {
    char* d_name;
    int (*d_strat)();
    int (*d_open)();
    int (*d_close)();
    int d_iobase;       /* IO address for bootable devices, 0 else */
};

struct iobuf {
#define B_READ      001 /* is open for read */
#define B_WRITE     002 /* is open for write */
#define B_USED      004 /* buffer is in use */
#define B_DISK      010 /* is a disk file */
    short b_flags;
    struct inode b_inode;
    int unknown1;
    int b_unit;
    int b_diskoff;
#define b_fnum b_diskoff
    int unknown2;
    int b_foffset;
    int b_sector;
    char *b_secbuf;
    char *b_dma;
    int b_count;
    char b_blk[1024];
};

struct specregs {
    int r_usp;
    int r_vbr;
    int r_sfc;
    int r_dfc;
    int r_cacr;
    int r_caar;
    int reserved;
};

struct bkpt {
    int addr;
    unsigned short instr;
};

/* these buffers are shared between FD buffers and IND blk buffers */
union bufdata {
    struct {
        char _pad[4096+4*sizeof(int)-3*sizeof(struct iobuf)];
        struct iobuf buf[3];
        struct iobuf fbuf[4];
    } fd;   /* stdio and FD buffers */
    struct {    /* this will overlap fd[0], fd[1], fd[2] */
        char buf[4096];
        int blk[4];
    } ib;   /* IND blk buffers */
};
#define filbuf(i) buffers.fd.fbuf[i]
#define fdbuf(i) buffers.fd.buf[i]
#define ibbuf(i) buffers.ib.buf[i<<10]
#define ibblk(i) buffers.ib.blk[i]

/* locore */
extern int tracevec, trap1vec, physbase;
extern enetaddr macaddress;
extern warmstart(), memsize(), invalid_trap(), retexcept();
extern clrcache020(), setsr(), setvbr();
extern setspecregs(), getspecregs();
extern int getvbr();


/* monitor data */
extern char mon_date[]; /* version data */
extern int con_type, coldinit, echo;

extern union bufdata buffers;

extern int monaddr, monlast, moncur, monrep, coloncmd;
extern char monline[];
extern char *chptr;
extern jmp_buf monreturn;
extern char monreg[];
extern int monaddrsz;
extern struct specregs specregs;
extern int monregflg, noopenerr;

extern int iccloaded, hasicc, icc_noprint;
extern char bootfile[];

extern int textsize, datasize;
extern struct exvec regframe;

extern char il_bootfile[];
extern struct bkpt breakpoints[];
extern int monrepcnt, montraceflg, monrunflg, vbrsave;

/* console data */
extern char dh_char[2];
extern short dh_inited, dh11_char, col_column, col_line;

/* disassembler data */
extern unsigned short ioffset;

/* util data */
extern int fmt_width;

/* other externals */
extern int atoi();
extern unsigned int mem_readbyte(), mem_readword(), mem_readlong();
extern int open(), lseek(), read(), write(), close();
extern int readb(), readw(), readl(), writeb(), writew(), writel();
extern int write_virtword(), write_virtlong();
extern memcpy(), printf(), putchar(), name_console();
extern int ser_read(), strcmp(), haschar(), getchar();
extern int find_bootdev();
extern char* strcpy();
extern icc_load(), icc_debug(), icc_start();
extern int beginnum();
extern mem_test();
extern int phys_offset();
extern exit(), fatalerror();

extern struct devtbl dev_table[];
extern char* cons_devices[];

#endif
