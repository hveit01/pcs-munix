#ifndef FPVECSIZE
/* @(#)reg.h	6.1 */

/* layout of variables on the stack on entry to trap-procedure,
   a) for M68000, b) for M68010
   1) after normal exceptions, 2) after bus or address error
*/


struct  exvec
{       long exa7,exd0,exd1,exd2,exd3,exd4,exd5,exd6,
	exd7,exa0,exa1,exa2,exa3,exa4,exa5,exa6;/* Saved registers */
	long exret;                             /* Return address */
	long exno;                              /* Exception number */
	union {
		/* M68020 normal */
		struct { ushort exstatreg;      /* Status register */
			 long  expc;            /* Program counter */
			 ushort exfmtvec;       /* Format / Vector offset */
			 long  exinstaddr;      /* Instruction address */
		} ex1;
		/* M68020 bus or address error */
		struct { ushort exstatreg;      /* Status register */
			 long  expc;            /* Program counter */
			 ushort exfmtvec;       /* Format / Vector offset */
			 ushort dummy0;
			 ushort exssw;          /* Special status word */
			 ushort pipec;
			 ushort pipeb;
			 long  exaccessaddr;    /* Access address */
			 ushort dummy1[2];
			 long  dob;
			 ushort dummy2[4];
			 long  stageb;
			 ushort dummy3[2];
			 long  dib;
			 ushort dummy4[22];
		} ex2;
	} exu;
};

/* for ptrace, all stackframes are converted
   to the m68000 bus error format. Length: 16*4+4+2+sizeof(ex2o)
   see sig.c veccopy()
   Do NOT change this, else adb and other debuggers (e.g. Oregon Pascal)
   get into trouble!
*/
struct  oexvec
{       long exa7,exd0,exd1,exd2,exd3,exd4,exd5,exd6,
	     exd7,exa0,exa1,exa2,exa3,exa4,exa5,exa6;
	long exret;
	ushort exno;
	ushort excpustate;
	long exaccessaddr;
	ushort exinstrreg;
	ushort exstatreg;
	long expc;
};
#define VECSIZE sizeof(struct oexvec)

struct  fp_vec
{
	long fpcr, fpsr, fpiar;
	char fp0[12];           /* these are all extended precision values */
	char fp1[12];
	char fp2[12];
	char fp3[12];
	char fp4[12];
	char fp5[12];
	char fp6[12];
	char fp7[12];
};
#define FPVECSIZE sizeof(struct fp_vec)

struct  fpe_vec
{       long exd0,exd1,exd2,exd3,exd4,exd5,exd6,exd7,
	     exa0,exa1,exa2,exa3,exa4,exa5,exa6,exa7;
	ushort exstatreg;
	long  expc;
};

struct corefil {
	struct  oexvec d_exvec;
	struct  fp_vec d_fpvec;
	ulong   d_tsize;
	ulong   d_dsize;
	ulong   d_ssize;
	ulong   d_stacktop;
	ulong   d_stackbas;
	struct dfhd {
		short   dx_mag;
		ulong   dx_tsize;
		ulong   dx_dsize;
		ulong   dx_bsize;
	} d_exdata;
	char    d_comm[14 /*DIRSIZ*/];
	char    d_psargs[40 /*PSARGSZ*/];
	time_t  d_start;
	time_t  d_ticks;
	short   d_signal;
};

extern struct corefil corefil;
#endif
