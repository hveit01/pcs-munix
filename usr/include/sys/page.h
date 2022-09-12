/* @(#)page.h   1.0 */
/*
 * CADMUS page table entry
 */

#ifndef C20
#define invsatb(X)      {_ptec[(X>>12)&0x3ff]=0; _ptec[((X>>12)&0x3ff)+1024]=0;}

#define PG_PFNUM        0x00000fff
#define PG_V            0x80000000
#define PG_PROT         0x70000000
#define PG_LOCK         0x01000000

#define PG_NOACC         0
#define PG_KR   0x10000000
#define PG_KW   0x20000000
#define PG_UW   0x70000000
#define PG_URKR 0x50000000

/* LRU Hardware bits */
#define PL_MOD		0x04	/* Page was modified */
#define PL_REF          0x83    /* 2 bit counter value mask */
#define PL_ROR          0x80    /* OR of 2 counter bits */

extern int  _ptec[];
extern char _lru[];
extern char _puc[];

/* LRU Hardware get and set */
#define setpl(pt,val)   _lru[pt->pgm.pg_pfn]=(val)
#define andpl(pt,val)   _lru[pt->pgm.pg_pfn]&=(val)
#define orpl(pt,val)    _lru[pt->pgm.pg_pfn]|=(val)
#define getpl(pt)       _lru[pt->pgm.pg_pfn]
#define decpl(pt)       _puc[pt->pgm.pg_pfn]=0

#define SYSVA   0x3f800000      /* 1 GB - 8 MB */
#define SYSPROM 0x3ff80000      /* address of EPROMs */

typedef union pte {

/*
 *      -------------------------------------------------------
 *      |v| prot | res  |cw|l|  reserved |        pfn         |
 *      -------------------------------------------------------
 *       1    3     2    1  1    12               12
 */
	struct {
	ulong           pg_v : 1,       /* valid page                   */
			pg_prot : 3,    /* protection field             */
				: 2,    /* unused, reserved             */
			pg_cw : 1,      /* copy on write                */
			pg_lock : 1,    /* page locked for IO           */
				:12,    /* unused, reserved             */
			pg_pfn : 12;    /* real page number             */
	} pgm;

	/* 
	 * For fast mask operations when needed
	 * (a single mask operation beats setting
	 * several individual fields to constant values).
	 */
	struct {
		long    pg_pte;
	} pgi;
} pte_t;

#define PTE_KR          1
#define PTE_KW          2
#define PTE_UW          7
#define PTE_UR          5

extern short _ptbr[];           /* HW address of PTBR */
#else /* now C20 part */

#define PG_PFNUM        0xfffff000
#define PG_V            0x00000001
#define PG_PROT         0x00000004
#define PG_LOCK         0x00000100
#define PL_MOD          0x00000010      /* Page was modified */
#define PL_REF          0x00000008      /* Page was referenced */
#define PL_LRU          0x00000c00

#define andpl(pt,val)   pt->pgi.pg_pte &= (val)
#define orpl(pt,val)    pt->pgi.pg_pte |= (val)
#define setpl(pt,val)   ( andpl(pt,~(PL_MOD|PL_REF)), orpl(pt,(val)) )
#define getpl(pt)       pt->pgi.pg_pte
#define decpl(pt)       pt->pgi.pg_pte &= ~PL_REF

/* protection consists of write protect bit only */
/* distinction between user and system space via cpu and system root pointer */

#define PG_KR   0x00000004
#define PG_KW   0x00000400      /* set 1 bit in lru field so pte is not all 0 */
#define PG_UW   0x00000400
#define PG_URKR 0x00000004

#define SYSVA   0xf8000000      /* 4 GB - 256 MB */
#define SYSPROM 0xff000000      /* address of EPROMs */

typedef union pte {

/*       31                       12  10  9 8 7  6   4 3  2  0
 *      -------------------------------------------------------
 *      |             pfn           |lru|cw|l|0|ci|0|m|u|wp|dt|
 *      -------------------------------------------------------
 *                   20               2   1 1 1  1 1 1 1  1  2
 */
	struct {
	ulong           pg_pfn  : 20,   /* physical page number */
			pg_lru  : 2,    /* 2 bit lru counter */
			pg_cw   : 1,    /* copy on write */
			pg_lock : 1,    /* page locked for IO */
				: 1,    /* not used */
			pg_ci   : 1,    /* cache inhibit */
				: 1,    /* not used */
			pg_mod  : 1,    /* page modified */
			pg_use  : 1,    /* page used */
			pg_prot : 1,    /* page write protected */
				: 1,    /* must always be 0 */
			pg_v    : 1;    /* page valid if = 1 */
			/* DT must be 00 or 01 */
	} pgm;

	/* 
	 * For fast mask operations when needed
	 * (a single mask operation beats setting
	 * several individual fields to constant values).
	 */
	struct {
		long    pg_pte;
	} pgi;
} pte_t;

#define PTE_KR          1
#define PTE_KW          0
#define PTE_UW          0
#define PTE_UR          1

extern long _ptbr[];
#endif C20


/* common definitions */

#define PT_VAL		1		/* page valid			*/
#define PT_LOCK		1		/* page locked			*/

#define NPGPT           256             /* PTE per page */

#define NOSLEEP 01

extern pte_t *mapa();
extern int sbrpte[];
#ifdef C20
#define SBRCNT 2048     /* number of entries in supervisor page table */
#else
#define SBRCNT 1536     /* number of entries in supervisor page table */
#endif

typedef union pte	spte_t;
typedef union pte       pt_t[NPGPT];

#define svtop(X)        (((int)(X) - SYSVA) >> 12)
#define ptosv(X)        (((int)(X) << 12) + SYSVA)
#define svtoc(X)        (((int)(X) - SYSVA) >> 10)
#define ctosv(X)        (((int)(X) << 10) + SYSVA)
#define svtopte(X)      (sbrpte + svtop(X))
