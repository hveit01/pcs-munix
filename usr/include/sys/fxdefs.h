/*
 * Description of C pre-processor definitions.
 *
 *      This file contains a list of all conditional compile
 *      flags used by the cgi C library and the device driver
 *
 *      Some of the flags are defined by the compiler, the other
 *      can be defined to generate different flavours of the software
 *      that runs of the CGS-4600/CGS-4700
 */

/* Operating System Defines */
/*
 *      FLAG            USAGE
 *      ----            -----
 *
 *      VMS             used for all vms specific code
 *                      EG. sys$translate, sys$qiow
 *
 *      BSD             Berkeley unix specific code
 *                      EG. include file.h vs fcntl.h
 *
 *      SYSV            System V unix specific code
 *                      EG. include file.h vs fcntl.h
 *
 *	ULTRIX		used for all ultrix specific general code
 *
 *      ULTRIX_1        used to differenciate between release 1 & release 2
 *                      of ultrix, relative to start address of 4600 in qbus mem.
 *
 *      ULTRIX_2        used to differenciate between release 1 & release 2
 *                      of ultrix, relative to start address of 4600 in qbus mem.
 */

/*      Note: compiler will define VMS */
/* #define      VMS             /* VMS operating system */
/* #define      ULTRIX          /* Ultrix operating system */
/* #define      ULTRIX_1        /* Ultrix operating system rel 1 */
/* #define      ULTRIX_2        /* Ultrix operating system > rel 1 */
#define      SYSV            /* system V unix operating system */
/* #define      BSD             /* berkeley unix operating system */


/*      sed -e /#define ULTRIX_2/s/...//        /* */

/* Hardware Defines */
/*
 *      FLAG            USAGE
 *      ----            -----
 *
 *      vax             used for vax byte ordering
 *      mips            used for mips byte ordering
 *      unos            used for unos byte ordering
 *      m68k            used for motorola 68020 byte ordering
 *
 *      NOTE: vax, mips, m68k are all defined by the compilers
 */

/* #define      vax     /* vax hardware */
/* #define      mips    /* mips hardware */
/* #define      unos    /* unos hardware */
/* #define      m68k    /* motorola 68000 hardware */

#ifdef m68000   /* for CADMUS, this is defined */
#define m68k    1
#endif

/* Floating-point Defines */
/*
 *      FLAG            USAGE
 *      ----            -----
 *
 *      HOST_VAX_FLOAT  vax floating point format
 *      HOST_IEEE_FLOAT IEEE floating point format
 */

#ifdef	vax
#  define HOST_VAX_FLOAT  /* vax floating point format */
#else   /* vax */
#ifdef	mips 
#  define HOST_IEEE_FLOAT /* IEEE floating point format */
#else   /* mips */
#ifdef	m68k 
#  define HOST_IEEE_FLOAT /* IEEE floating point format */
#else   /* m68k */
#ifdef	unos 
#  define HOST_IEEE_FLOAT /* IEEE floating point format */
#else   /* unos */
        You should define what kind of floating point your machine has
#endif  /* unos */
#endif  /* m68k */
#endif  /* mips */
#endif  /* vax */

/* Word ordering Defines */
/*
 *      FLAG            USAGE
 *      ----            -----
 *
 *      LITTLE_ENDIAN	vax 
 *      BIG_ENDIAN	motorola processors
 */
#ifdef	vax
#  define LITTLE_ENDIAN /* VAX addresses and byte ordering */
			/* longword byte 0 contains bit 0 */
#else	/* vax */
#ifdef	mips 
#  define BIG_ENDIAN    /* uses 680X0 style addresses and byte ordering */
			/* longword byte 0 contains bit 31 */
#else   /* mips */
#ifdef	m68k 
#  define BIG_ENDIAN    /* 680X0 addresses and byte ordering */
			/* longword byte 0 contains bit 31 */
#else   /* m68k */
#ifdef	unos 
#  define BIG_ENDIAN    /* 680X0 addresses and byte ordering */
			/* longword byte 0 contains bit 31 */
#else   /* unos */
        You should define what kind of word ordering your machine has
#endif  /* unos */
#endif  /* m68k */
#endif  /* mips */
#endif	/* vax */

/* Optional Defines */
/*
 *      FLAG            USAGE
 *      ----            -----
 *
 *	CAL_MEMCPY	routine for fast memory to memory copy
 *
 *      NO_MEM_MAP      no memory mapping is used, we use ioctl to move
 *                      the data.
 *
 *      MEM_MAP         memory mapping is used, we use ioctl to receive
 *                      interrupts.
 *
 *      USE_MMAP        we use the system call mmap() to map in the
 *                      CGS 4600/CGS 4700 memory to user processes
 *
 *      USE_SHM         we will use sys V shared memory to map in the
 *                      CGS 4600/CGS 4700 memory to user processes
 */

#ifndef	VMS
#if defined(mips) && !defined(PCS)
#  define CAL_MEMCPY    bcopy   /*UH: CAL_MEMCPY not used in fx.c! */
#else	/* mips */
#  define CAL_MEMCPY	memcpy
#endif	/* mips */
#endif	/* VMS */

/* #define NO_MEM_MAP   /* no memory mapping */

#ifndef NO_MEM_MAP
/*UH: NO_MEM_MAP, MEM_MAP and USE_SHM not used in fx.c */

#define	MEM_MAP		/* use memory mapping */
#ifdef  mips
#define USE_MMAP        /* we always use mmap on a mips machine */
#endif  /* mips */

#ifdef  sysV68
#define USE_SHM         /* we might use shared memory on a sys V machine */
#endif  /* sysV68 */

#endif  /* NO_MEM_MAP */

#if defined(mips) && defined(PCS)
#define USE_SHM         /* use shmat on MIPS */
#endif MIPS && PCS
