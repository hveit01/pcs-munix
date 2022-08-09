/* @(#)page.h   1.0 */
/*
 * VAX page table entry
 */

/* ccr bits are readable */
/* now in mch.s
/*#define clratb()        _ccr &= ~2
/*#define clrptbr()       _ccr &= ~1
/*#define clrcache()      _ccr &= ~4
*/
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
#define PHYSSTART 0                     /* start of physical memory */

#define svtop(X)        (((int)(X) - SYSVA) >> 12)
#define ptosv(X)        (((int)(X) << 12) + SYSVA)
#define svtoc(X)        (((int)(X) - SYSVA) >> 10)
#define ctosv(X)        (((int)(X) << 10) + SYSVA)
#define svtopte(X)      (sbrpte + svtop(X))
extern int sbrpte[];

typedef union pte {

/*
 *      -------------------------------------------------------
 *      |1| prot |of|res|cw|l|  reserved |        pfn         |
 *      -------------------------------------------------------
 *       1    3    1  1  1  1    12               12
 */
	struct {
	ulong           pg_v : 1,       /* valid page                   */
			pg_prot : 3,    /* protection field             */
			pg_offb : 1,    /* offboard bit                 */
				: 1,    /* unused, reserved             */
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

typedef union pte	spte_t;
typedef union pte       pt_t[256];

#define PTE_KR          1
#define PTE_KW          2
#define PTE_UW          7
#define PTE_UR          5
#define PTE_EX          1

#define PT_VAL		1		/* page valid			*/
#define PT_LOCK		1		/* page locked			*/

#define NPGPT           256             /* PTE per page */

extern pte_t *mapa();

#define NOSLEEP 01
